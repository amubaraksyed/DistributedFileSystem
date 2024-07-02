#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sstream>
#include <iostream>
#include <map>
#include <string>
#include <algorithm>
#include <cstring>

#include "DistributedFileSystemService.h"
#include "ClientError.h"
#include "ufs.h"
#include "WwwFormEncodedDict.h"

using namespace std;

// Helper Function
bool compareEntries(const pair<string, bool>& a, const pair<string, bool>& b) {
    return a.first < b.first;
}

// Constructor for DistributedFileSystemService
DistributedFileSystemService::DistributedFileSystemService(string diskFile) : HttpService("/ds3/") {
    this->fileSystem = new LocalFileSystem(new Disk(diskFile, UFS_BLOCK_SIZE));
}

void DistributedFileSystemService::get(HTTPRequest *request, HTTPResponse *response) {

    /*
     To read a file, you use the HTTP GET method, specifying the file location as the path of your URL
     and your server will return the contents of the file as the body in the response. To read a directory,
     you also use the HTTP GET method, but directories list the entries in a directory. To encode directory
     entries, you put each directory entry on a new line and regular files are listed directly, and
     directories are listed with a trailing "/". For GET on a directory omit the entries for . and ...

     For example, GET on /ds3/a/b will return: c.txt

     And GET on /ds3/a/ will return: b/

     The listed entries should be sorted using standard string comparison sorting functions.
     */

    // Split the URL into components of the file system
    vector<string> components = request->getPathComponents();
    
    // Check if the URL is valid (check number of components)
    if (components.empty() || components[0] != "ds3") { throw ClientError::notFound(); }
    
    // Remove the first element of the components (ds3)
    components.erase(components.begin());

    // Initialize the inode to the root directory inode number
    int inode = UFS_ROOT_DIRECTORY_INODE_NUMBER;

    // Recursively search for the inode number of the desired file/entry
    for (size_t i = 0; i < components.size(); ++i) {
        inode = fileSystem->lookup(inode, components[i]);
        if (inode < 0) { throw ClientError::notFound(); }
    }

    // Retrieve the inode data using the inode number
    inode_t entryInode; fileSystem->stat(inode, &entryInode);

    // Allocate buffer to read the inode data
    char* buffer = new char[entryInode.size];
    int bytesRead = fileSystem->read(inode, buffer, entryInode.size);
    if (bytesRead < 0) { throw ClientError::notFound(); }

    switch (entryInode.type) {
            
        // If the entry is a file, then print out its contents
        case UFS_REGULAR_FILE: { response->setBody(string(buffer, bytesRead)); break; }

        // If the entry is a directory, list its contents
        case UFS_DIRECTORY: {

            // Parse the buffer content into entries
            vector<pair<string, bool>> entries;
            for (int i = 0; i < (entryInode.size / sizeof(dir_ent_t)); ++i) {
                
                // Copy the entry from the buffer into a local variable
                dir_ent_t entry; memcpy(&entry, buffer + i * sizeof(dir_ent_t), sizeof(dir_ent_t));
                
                // Get the entry inode using the entry inode number
                inode_t entryInode; fileSystem->stat(entry.inum, &entryInode);
                
                // Push the entry as long as they are not the self or parent entries
                if (strcmp(entry.name, ".") != 0 && strcmp(entry.name, "..") != 0) {
                    entries.push_back(make_pair((string)entry.name, entryInode.type == UFS_DIRECTORY));
                }
            }

            // Sort the entries alphabetically
            sort(entries.begin(), entries.end(), compareEntries);

            // Print out the entries in the directory
            stringstream ss; for (const auto& entry : entries) { ss << entry.first << (entry.second ? "/" : "") << "\n"; }

            // Set the response body to the directory listing
            response->setBody(ss.str()); break;
        }

        default: { throw ClientError::badRequest(); }
            
    }

    // Clean up allocated buffer, and send the response to the client
    delete[] buffer; response->setStatus(200); return;
}

void DistributedFileSystemService::put(HTTPRequest *request, HTTPResponse *response) {

    /*
     To create or update a file, use HTTP PUT method where the URL defines the file name and path and
     the body of your PUT request contains the entire contents of the file. If the file already exists,
     the PUT call overwrites the contents with the new data sent via the body.
     
     In the system, directories are created implicitly. If a client PUTs a file located at /ds3/a/b/c.txt
     and directories a and b do not already exist, you will create them as a part of handling the request.
     If one of the directories on the path exists as a file already, like /a/b, then it is an error.
     */

    // Split the URL into components of the file system
    vector<string> components = request->getPathComponents();
    
    // Check if the URL is valid (check number of components)
    if (components.empty() || components[0] != "ds3") { throw ClientError::notFound(); }
    
    // Remove the first element of the components (ds3)
    components.erase(components.begin());

    // Initialize the inode to the root directory inode number
    int inodeNumber = UFS_ROOT_DIRECTORY_INODE_NUMBER;

    // Begin a transaction on the disk before making any changes to the file system
    this->fileSystem->disk->beginTransaction();

    try {
        // Recursively search for the inode number of the desired directory/file
        for (size_t i = 0; i < components.size() - 1; ++i) {
            
            // Get the Inode Number of the Component
            int entryInodeNumber = this->fileSystem->lookup(inodeNumber, components[i]);
            
            // If the Inode Does Not Exist, then Create it
            if (entryInodeNumber < 0) {
                entryInodeNumber = this->fileSystem->create(inodeNumber, UFS_DIRECTORY, components[i]);
                if (entryInodeNumber < 0) { throw ClientError::badRequest(); }
            }
            
            // Otherwise Check if the Existing Inode is Valid
            else {
                inode_t entryInode;
                int statResult = this->fileSystem->stat(entryInodeNumber, &entryInode);
                if (statResult < 0 || entryInode.type != UFS_DIRECTORY) { throw ClientError::conflict(); }
            }
            
            // Set the current Inode Number to the Entry Inode Number to Continue Recursion
            inodeNumber = entryInodeNumber;
        }

        // Create the file in the specified directory
        int entryInodeNumber = this->fileSystem->create(inodeNumber, UFS_REGULAR_FILE, components.back());
        if (entryInodeNumber < 0 && entryInodeNumber != -EINVALIDTYPE) { throw ClientError::badRequest(); }

        // Write the contents to the file
        entryInodeNumber = this->fileSystem->write(entryInodeNumber, request->getBody().data(), request->getBody().size());
        if (entryInodeNumber < 0) { throw ClientError::insufficientStorage(); }

        // Commit the transaction and set the response status to success
        this->fileSystem->disk->commit(); response->setStatus(200); return;

    }
    
    // If any error occurs, rollback and throw error to signify request failure
    catch (const ClientError& e) { this->fileSystem->disk->rollback(); throw e; }
}

void DistributedFileSystemService::del(HTTPRequest *request, HTTPResponse *response) {

    /*
     To delete a file, you use the HTTP DELETE method, specifying the file location as the path of your URL.
     To delete a directory, you also use DELETE but deleting a directory that is not empty it is an error.
     */

    // Split the URL into components of the file system
    vector<string> components = request->getPathComponents();
    
    // Check if the URL is valid (check number of components)
    if (components.empty() || components[0] != "ds3") { throw ClientError::notFound(); }
    
    // Remove the first element of the components (ds3)
    components.erase(components.begin());

    // Initialize the inode to the root directory inode number
    int inode = UFS_ROOT_DIRECTORY_INODE_NUMBER;

    // Begin a transaction on the disk before making any changes to the file system
    this->fileSystem->disk->beginTransaction();

    try {
        // Recursively search for the inode number of the desired directory/file
        for (size_t i = 0; i < components.size() - 1; ++i) {
            int entryInodeNumber = this->fileSystem->lookup(inode, components[i]);
            if (entryInodeNumber < 0) { throw ClientError::notFound(); }
            inode = entryInodeNumber;
        }

        // Unlink (delete) the file or directory
        int result = this->fileSystem->unlink(inode, components.back());
        if (result < 0) { throw (result == -EDIRNOTEMPTY ? ClientError::conflict() : ClientError::badRequest()); }

        // Commit the transaction and set the response status to success
        this->fileSystem->disk->commit(); response->setStatus(200); return;

    }
    
    // If any error occurs, rollback and throw error to signify request failure
    catch (const ClientError& e) { this->fileSystem->disk->rollback(); throw e; }
}

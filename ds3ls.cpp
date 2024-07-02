#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>
#include <vector>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

/*
 The `ds3ls` utility prints the names for all of the files and directories in
 a disk image. This utility takes a single command line argument: the name of
 the disk image file to use. This program will start at the root of the file system, print
 the contents of that directory in full, and then traverse to each directory
 contained within. This process repeats in a depth-first fasion until all
 file and directory names have been printed.

 When printing the contents of a directory, first, print the word `Directory`
 followed by a space and then the full path for the directory you're printing,
 and ending it with a newline. Second, print each of the entries in that
 directory. Sorted each entry using `std::strcmp` and print them in that order.
 Each entry will include the inode number, a tab, the name of the entry, and
 finishing it off with a newline. Third, print a empty line consisting of only
 a newline to finish printing the contents of the directory.

 Make sure that your solution does _not_ print the contents of `.` and `..`
 subdirectories as this action would lead to an infinitely loop.

 After printing a directory, traverse into each subdirectory and repeat the process
 recursively in a depth-first fashion.
 */

void printDirectoryContents(LocalFileSystem&, int, const string&);

int main(int argc, char *argv[]) {
    
    // Check Command Line Arguments
    if (argc != 2) { cout << argv[0] << ": diskImageFile" << endl; return 1; }

    // Create a File System and Disk using the disk file from the arguments
    string diskFile = argv[1];
    Disk disk(diskFile, UFS_BLOCK_SIZE);
    LocalFileSystem fs(&disk);

    // Call a function to recursively print out all the directories and files
    printDirectoryContents(fs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "/");

    return 0;   /* Terminate Successfully */
}

bool compareEntryNames(const dir_ent_t &a, const dir_ent_t &b) { return strcmp(a.name, b.name) < 0; }

void printDirectoryContents(LocalFileSystem &fs, int inodeNumber, const string &path) {
    
    // Get the inode using the inode number
    inode_t inode; int ret = fs.stat(inodeNumber, &inode);
    if (ret == -EINVALIDINODE) { cout << "stat failed with " << ret << endl; return; }

    // Read the contents of the block into a buffer
    char buffer[inode.size]; fs.read(inodeNumber, buffer, inode.size);

    // Separate the contents of the block into entries
    vector<dir_ent_t> entries;
    for (int i = 0; (long unsigned int)i < (inode.size / sizeof(dir_ent_t)); i++) {
        dir_ent_t entry;
        memcpy(&entry, buffer + i * sizeof(dir_ent_t), sizeof(dir_ent_t));
        entries.push_back(entry);
    }

    // Sort the Entries
    sort(entries.begin(), entries.end(), compareEntryNames);

    // Output the sorted entries in the directory
    cout << "Directory " << path << endl;
    for (const dir_ent_t &entry : entries) { cout << entry.inum << "\t" << entry.name << endl; }
    cout << endl;

    // Recursively print any directories within the directory
    for (const dir_ent_t &entry : entries) {
        fs.stat(entry.inum, &inode);
        if (inode.type == UFS_DIRECTORY && strcmp(entry.name, ".") && strcmp(entry.name, "..")) {
            printDirectoryContents(fs, entry.inum, path + entry.name + "/");
        }
    }
}

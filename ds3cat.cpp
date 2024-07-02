#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

/*
 The `ds3cat` utility prints the contents of a file to standard output. It takes
 the name of the disk image file and an inode number as the only arguments. It prints
 the contents of the file that is specified by the inode number.

 For this utility, first print the string `File blocks` with a newline at the end
 and then print each of the disk block numbers for the file to standard out, one
 disk block number per line. After printing the file blocks, print an empty line
 with only a newline.

 Next, print the string `File data` with a newline at the end and then print
 the full contents of the file to standard out. You do not need to differentiate
 between files and directories, for this utility everything is considered to be
 data and should be printed to standard out.
 */

int main(int argc, char *argv[]) {
    
    // Check Command Line Arguments
    if (argc != 3) { cout << argv[0] << ": diskImageFile inodeNumber" << endl; return 1; }
    
    // Parse the command Line Arguments
    string diskFile = argv[1]; int inodeNumber = stoi(argv[2]);
    
    // Create a File System and Disk using the disk file from the arguments
    Disk disk(diskFile, UFS_BLOCK_SIZE); LocalFileSystem fs(&disk);
    
    // Get the file inode using the inode number
    inode_t inode; fs.stat(inodeNumber, &inode);
    
    // Print the file blocks
    cout << "File blocks" << endl;
    int numBlocks = (inode.size / UFS_BLOCK_SIZE) + ((inode.size % UFS_BLOCK_SIZE) ? 1 : 0);
    for (int i = 0; i < numBlocks; i++) { if (inode.direct[i] != 0) { cout << inode.direct[i] << endl; }}
    cout << endl;
    
    // Read all the file data into a buffer
    char buffer[inode.size + 1];
    int ret = fs.read(inodeNumber, buffer, inode.size); if (ret < 0) { return ret; }
    buffer[inode.size] = '\0';
    
    // Print File data
    cout << "File data" << endl; if (inode.size > 0) { cout << buffer; }
    
    return 0; /* Terminate Successfully */
}

#include <iostream>
#include <string>
#include <algorithm>
#include <cstring>

#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

/*
 The `ds3bits` utility prints metadata for the file system on a disk image. It takes
 a single command line argument: the name of a disk image file.

 First, it prints metadata about the file system starting with the string `Super` on a
 line, then `inode_region_addr` followed by a space and then the inode region address from
 the super block. Next, it should print the string `data_region_addr` followed by a space and the
 data region address from the super block, followed by a newline. Finally, print an empty line
 with just a newline character.

 Next it prints the inode and data bitmaps. Each bitmap starts with a string on its own line,
 `Inode bitmap` and `Data bitmap` respectively. For each bitmap, print the byte value,
 formatted as an `unsigned int` followed by a space. For each byte in your bitmap your print
 statement might look something like this:

 ```
 cout << (unsigned int) bitmap[idx] << " ";
 ```

 Where each byte is followed by a space, including the last byte and after you're done printing
 all of the bytes print a single newline character.

 Print the indoe bitmap first, followed by blank line consisting of only a single
 newline character, then print the data bitmap.
 */

int main(int argc, char *argv[]) {
    
    // Check Valid Command Line Arguments
    if (argc != 2) { cout << argv[0] << ": diskImageFile" << endl; return 1; }

    // Create a File System and Disk using the disk file from the arguments
    string diskFile = argv[1];
    Disk disk(diskFile, UFS_BLOCK_SIZE);
    LocalFileSystem fs(&disk);

    // Read the super block to get the file system metadata
    super_t super; fs.readSuperBlock(&super);

    // Print the metadata about the file system
    cout << "Super" << endl
         << "inode_region_addr " << super.inode_region_addr << endl
         << "data_region_addr " << super.data_region_addr << endl
         << endl;

    // Copy the inode Bitmap into a local variable
    unsigned char inodeBitmap[UFS_BLOCK_SIZE * super.inode_bitmap_len];
    for (int i = 0; i < super.inode_bitmap_len; i++) {
        disk.readBlock((super.inode_bitmap_addr + i), (inodeBitmap + (i * UFS_BLOCK_SIZE)));
    }

    // Copy the data bitmap into a local variable
    unsigned char dataBitmap[UFS_BLOCK_SIZE * super.data_bitmap_len];
    for (int i = 0; i < super.data_bitmap_len; i++) {
        disk.readBlock((super.data_bitmap_addr + i), (dataBitmap + (i * UFS_BLOCK_SIZE)));
    }

    // Print the inode bitmap
    cout << "Inode bitmap" << endl;
    for (int i = 0; i < (UFS_BLOCK_SIZE * super.inode_bitmap_len); i++) {
        cout << (unsigned int)inodeBitmap[i] << " ";
    }   cout << endl << endl;

    // Print the data bitmap
    cout << "Data bitmap" << endl;
    for (int i = 0; i < (UFS_BLOCK_SIZE * super.data_bitmap_len); i++) {
        cout << (unsigned int)dataBitmap[i] << " ";
    }   cout << endl;
    
    return 0;   /* Terminate Successfully */
}

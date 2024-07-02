#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <assert.h>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

//// the operation failed because there wasn't enough space on the disk
//#define ENOTENOUGHSPACE    (1)
//// Unlinking a directory that is _not_ empty
//#define EDIRNOTEMPTY       (2)
//// The inode number is invalid
//#define EINVALIDINODE      (3)
//// The inode is valid but not allocated
//#define ENOTALLOCATED      (4)
//// The `size` for a read or write is invalid
//#define EINVALIDSIZE       (5)
//// Attempting to write to a directory
//#define EWRITETODIR        (6)
//// Lookup an entity that does not exist
//#define ENOTFOUND          (7)
//// Invalid name
//#define EINVALIDNAME       (8)
//// Creating an entity that exists with a different type or writing to a directory
//#define EINVALIDTYPE       (9)
//// Unlinking '.' or '..'
//#define EUNLINKNOTALLOWED  (10)

LocalFileSystem::LocalFileSystem(Disk *disk) { this->disk = disk; }

void LocalFileSystem::readSuperBlock(super_t *super) {
    
    // Copy the contents of the super block into a buffer
    char buffer[UFS_BLOCK_SIZE]; disk->readBlock(0, buffer);
    
    // Paste the superblock from the buffer into the super variable
    memcpy(super, buffer, sizeof(super_t));
    
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
    
    /**
     * Lookup an inode.
     *
     * Takes the parent inode number (which should be the inode number
     * of a directory) and looks up the entry name in it. The inode
     * number of name is returned.
     *
     * Success: return inode number of name
     * Failure: return -ENOTFOUND, -EINVALIDINODE.
     * Failure modes: invalid parentInodeNumber, name does not exist.
     */
    
    // Get the parent inode and check if it is valid
    inode_t parent; int EVALUE;
    if ((EVALUE = stat(parentInodeNumber, &parent)) < 0) { return EVALUE; }
    
    // If the parent inode is not a directory, then return an error
    if (parent.type != UFS_DIRECTORY) { return -EINVALIDINODE; }
    
    // If the file name is invalid, return an error
    if (name.length() <= 0|| name.length() >= DIR_ENT_NAME_SIZE) { return -EINVALIDNAME; }
    
    // Read the contents of the parent directory into a buffer
    char buffer[parent.size]; read(parentInodeNumber, buffer, parent.size);
    
    // Check every entry in the contents to find a match
    for (int i = 0; i < (parent.size / sizeof(dir_ent_t)); i++) {
        
        // Copy the entry from the buffer into an entry object
        dir_ent_t entry; memcpy(&entry, buffer + i * sizeof(dir_ent_t), sizeof(dir_ent_t));
        
        // Return the inode number if the entry name matches the target
        if(strcmp(entry.name, name.c_str()) == 0) { return entry.inum; }
        
    }
    
    return -ENOTFOUND; /* File Could Not Be Found in the Directory */
    
}


int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
    
    /**
     * Read an inode.
     *
     * Given an inodeNumber this function will fill in the `inode` struct with
     * the type of the entry and the size of the data, in bytes, and direct blocks.
     *
     * Success: return 0
     * Failure: return -EINVALIDINODE
     * Failure modes: invalid inodeNumber
     */
    
    // Copy the superblock into our local variable
    super_t super; readSuperBlock(&super);
    
    // Check if the inode number is within bounds
    if (inodeNumber < 0 || inodeNumber >= super.num_inodes) { return -EINVALIDINODE; }

    // Read the inode bitmap into a buffer
    unsigned char inodeBitmap[UFS_BLOCK_SIZE * super.inode_bitmap_len]; readInodeBitmap(&super, inodeBitmap);
    
    // Check if the inode is marked valid in the inode bitmap
    if (((inodeBitmap[inodeNumber / 8] >> (inodeNumber % 8)) & 1) == 0) { return -EINVALIDINODE; }

    // Read in the inode region and copy the inode corresponding to the inode number
    inode_t inodes[super.num_inodes]; readInodeRegion(&super, inodes);
    memcpy(inode, &inodes[inodeNumber], sizeof(inode_t));

    return 0;   /* Successful Termination */
    
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {

    /**
     * Read the contents of a file or directory.
     *
     * Reads up to `size` bytes of data into the buffer from file specified by
     * inodeNumber. The routine should work for either a file or directory;
     * directories should return data in the format specified by dir_ent_t.
     *
     * Success: number of bytes read
     * Failure: -EINVALIDINODE, -EINVALIDSIZE.
     * Failure modes: invalid inodeNumber, invalid size.
     */
    
    // Bounds Check: If size exceeds the inode size, only read to the end of the inode
    if (size < 0 || size > MAX_FILE_SIZE) { return -EINVALIDSIZE; }
    
    // Get the inode using the inodeNumber and return if it is valid
    inode_t inode; int EVALUE; if ((EVALUE = stat(inodeNumber, &inode)) < 0) { return EVALUE; }

    // Adjust the size to stay within the bounds of the contents; Define variables for reading through data
    size = min(size, inode.size); int bytesToRead = size; int bytesRead = 0; char readBuffer[UFS_BLOCK_SIZE];
    
    // Read blocks from the inode contents as long as there are still bytes to read
    for (int i = 0; bytesToRead > 0; i++) {
        
        // If there are more bytes to read than the block size, read only up to the block size
        int readBytes = min(bytesToRead, UFS_BLOCK_SIZE);
        
        // Copy the block from the disk into the buffer for reading
        disk->readBlock(inode.direct[i], readBuffer);
        
        // Copy the bytes from the reading buffer to the return buffer
        memcpy((char*)buffer + bytesRead, readBuffer, readBytes);
        
        // Increment and decrement variables for reading through data
        bytesRead += readBytes; bytesToRead -= readBytes;
    }

    return size; /* Return number of bytes read */
    
}

int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
  
    /**
     * Makes a file or directory.
     *
     * Makes a file (type == UFS_REGULAR_FILE) or directory (type == UFS_DIRECTORY)
     * in the parent directory specified by parentInodeNumber of name name.
     *
     * Success: return the inode number of the new file or directory
     * Failure: -EINVALIDINODE, -EINVALIDNAME, -EINVALIDTYPE, -ENOTENOUGHSPACE.
     * Failure modes: parentInodeNumber does not exist, or name is too long.
     * If name already exists and is of the correct type, return success, but
     * if the name already exists and is of the wrong type, return an error.
     */
    
    // Read the Super Block
    super_t super; readSuperBlock(&super); char blockBuffer[UFS_BLOCK_SIZE];
    
    // If the name of the entry exceeds the maximum length, return an error
    if (name.size() > DIR_ENT_NAME_SIZE || name.size() == 0) { return -EINVALIDNAME; }
    
    // Check if the parent inode exists and is a valid directory, otherwise return an error
    inode_t parent; if (stat(parentInodeNumber, &parent) < 0) { return -EINVALIDINODE; }
    if (parent.type != UFS_DIRECTORY) { return -EINVALIDINODE; }
    
    // Check if the file already exists
    int inodeNumber = -1; inode_t inode;
    if ((inodeNumber = lookup(parentInodeNumber, name)) != -ENOTFOUND) {
        if(stat(inodeNumber, &inode) < 0) { return -EINVALIDINODE; }
        return (inode.type == type ? inodeNumber : -EINVALIDTYPE);
    }   inodeNumber = -1;
    
    // Load in the inode Bitmap
    unsigned char inodeBitmap[(UFS_BLOCK_SIZE * super.inode_bitmap_len)]; readInodeBitmap(&super, inodeBitmap);
    
    // Find an available inode in the inode bitmap
    for (int i = 0; i < super.num_inodes; ++i) {
        if (((inodeBitmap[i / 8] >> (i % 8)) & 1) == 0) { inodeNumber = i; break;
    }}  if (inodeNumber == -1) { return -ENOTENOUGHSPACE; }
    
    // Determine the number of new data blocks needed
    int blocksNeeded = static_cast<int>(type == UFS_DIRECTORY);
    
    // Determine the block number and offset of the parent contents
    int parentBlockNumber = (parent.size / UFS_BLOCK_SIZE);
    int parentBlockOffset = (parent.size % UFS_BLOCK_SIZE);
    
    // Add an extra block if the offset is 0 (offset starts in new block)
    blocksNeeded += static_cast<int>(parentBlockOffset == 0);
    
    // If there's not enough for a new block, then return an error
    if (parentBlockOffset == 0 && parent.size + UFS_BLOCK_SIZE > MAX_FILE_SIZE) { return -ENOTENOUGHSPACE; }
    
    // Load in the data bitmap
    unsigned char dataBitmap[(UFS_BLOCK_SIZE * super.data_bitmap_len)]; readDataBitmap(&super, dataBitmap);
    
    // Find available data blocks in the data bitmap
    vector<int> availableBlocks;
    for (int i = 0; i < super.num_data && (int)availableBlocks.size() < blocksNeeded; ++i) {
        if ((dataBitmap[i / 8] & (1 << (i % 8))) == 0) { availableBlocks.push_back(i); }
    }   if ((int)(availableBlocks.size()) < blocksNeeded) { return -ENOTENOUGHSPACE; }
    
    // If a new data block is needed for the parent directory, then assign a block to the parent directory
    if (parentBlockOffset == 0) { parent.direct[parentBlockNumber] = availableBlocks[0] + super.data_region_addr; }
    

    // Create a new inode structure
    inode.type = type; inode.size = (type == UFS_DIRECTORY) ? 2 * sizeof(dir_ent_t) : 0;
    for (int i = 0; i < DIRECT_PTRS; ++i) { inode.direct[i] = 0; }

    // If the entry is a directory, then allocate the self and parent directory entries
    if (type == UFS_DIRECTORY) {
        inode.direct[0] = availableBlocks.back() + super.data_region_addr;
        dir_ent_t entries[2] = {{".", inodeNumber}, {"..", parentInodeNumber}};
        memset(blockBuffer, 0, UFS_BLOCK_SIZE); memcpy(blockBuffer, entries, inode.size);
        disk->writeBlock(inode.direct[0], blockBuffer);
    }

    // Update the data and inode bitmaps
    for (int block : availableBlocks) { dataBitmap[block / 8] |= 1 << (block % 8); }
    inodeBitmap[inodeNumber / 8] |= 1 << (inodeNumber % 8);

    // Create a new entry to store the new file / directory
    dir_ent_t newEntry; newEntry.inum = inodeNumber; strcpy(newEntry.name, name.c_str());

    // Write the new entry to the parent direct table
    disk->readBlock(parent.direct[parentBlockNumber], blockBuffer);
    memcpy(blockBuffer + parentBlockOffset, &newEntry, sizeof(dir_ent_t));
    disk->writeBlock(parent.direct[parentBlockNumber], blockBuffer);
    
    // Write the inode and bitmaps back to the disk
    writeInodeBitmap(&super, inodeBitmap); writeDataBitmap(&super, dataBitmap);

    // Update the size of the parent inode
    parent.size += sizeof(dir_ent_t);

    // Assign the parent and new inode to the inode region, then write it back to the disk
    inode_t inodes[super.num_inodes]; readInodeRegion(&super, inodes);
    memcpy(&inodes[parentInodeNumber], &parent, sizeof(inode_t));
    memcpy(&inodes[inodeNumber], &inode, sizeof(inode_t));
    writeInodeRegion(&super, inodes);

    return inodeNumber;   /* Terminate Successfully */
    
}


int LocalFileSystem::write(int inodeNumber, const void *buffer, int size) {

    /**
     * Write the contents of a file.
     *
     * Writes a buffer of size to the file, replacing any content that
     * already exists.
     *
     * Success: number of bytes written
     * Failure: -EINVALIDINODE, -EINVALIDSIZE, -EINVALIDTYPE, -ENOTENOUGHSPACE.
     * Failure modes: invalid inodeNumber, invalid size, not a regular file
     * (because you can't write to directories).
     */

    // Read in the Super block
    super_t super; readSuperBlock(&super);

    // Check if the inodeNumber is Valid
    inode_t inode; int EVALUE; if ((EVALUE = stat(inodeNumber, &inode)) < 0) { return EVALUE; }

    // Check if the Size is Valid
    if (size < 0 || size > MAX_FILE_SIZE) { return -EINVALIDSIZE; }

    // Check if the entry type is valid (is a directory)
    if (inode.type != UFS_REGULAR_FILE) { return -EWRITETODIR; }

    // Calculate the number of blocks needed
    int blocksNeeded = (size / UFS_BLOCK_SIZE) + (size % UFS_BLOCK_SIZE ? 1 : 0);
    
    // Load in the data bitmap
    unsigned char dataBitmap[(UFS_BLOCK_SIZE * super.data_bitmap_len)]; readDataBitmap(&super, dataBitmap);

    // Get the available data blocks
    vector<int> availableBlocks;
    for (int i = 0; i < super.num_data && blocksNeeded > 0; i++) {
        if (!((dataBitmap[i / 8] >> (i % 8)) & 1)) { availableBlocks.push_back(i); blocksNeeded--; }
    }   if (blocksNeeded > 0) { return -ENOTENOUGHSPACE; }
    
    // Define Variables for writing data to the buffer
    int bytesToWrite = size, bytesWritten = 0; char blockBuffer[UFS_BLOCK_SIZE];
    
    // Delete the blocks in the direct table from the data bitmap
    for (int i = 0; i < DIRECT_PTRS; i++) { if (inode.direct[i] != 0){
        int dataBlockNumber = inode.direct[i] - super.data_region_addr;
        dataBitmap[dataBlockNumber / 8] &= ~(1 << (dataBlockNumber % 8)); inode.direct[i] = 0;
    }}
    
    // Write data from the buffer into direct table
    for (int i = 0; i < (int)availableBlocks.size(); i++) {
        
        // Get the number of bytes to write at the current iteration
        int writeBytes = min(bytesToWrite, UFS_BLOCK_SIZE);
        
        // Copy the data from the data buffer into a local block buffer
        memcpy(blockBuffer, ((char*)buffer + (i * UFS_BLOCK_SIZE)), writeBytes);
        
        // Write the block buffer back to the disk
        inode.direct[i] = availableBlocks[i] + super.data_region_addr;
        disk->writeBlock(inode.direct[i], blockBuffer);
        
        // Update the variables used for writing data
        bytesToWrite -= writeBytes; bytesWritten += writeBytes;
        
        // Update the data bitmap
        dataBitmap[availableBlocks[i] / 8] |= 1 << (availableBlocks[i] % 8);
      
    }   inode.size = size; /* Update the size of the inode */
    
    // Write the data bitmap back to the disk
    writeDataBitmap(&super, dataBitmap);
    
    // Calculate the Location of the Inode in the Inode Region
    int numInodes = UFS_BLOCK_SIZE / sizeof(inode_t);
    int inodeBlockNumber = inodeNumber / numInodes, inodeBlockOffset = inodeNumber % numInodes;
    
    // Read the Inode Block, Update it, then Write it Back to Disk
    disk->readBlock((super.inode_region_addr + inodeBlockNumber), blockBuffer);
    memcpy((blockBuffer + (inodeBlockOffset * sizeof(inode_t))), &inode, sizeof(inode_t));
    disk->writeBlock((super.inode_region_addr + inodeBlockNumber), blockBuffer);

    return bytesWritten; /* Terminate Successfully */

}

int LocalFileSystem::unlink(int parentInodeNumber, string name) {
    
    /**
     * Remove a file or directory.
     *
     * Removes the file or directory name from the directory specified by
     * parentInodeNumber.
     *
     * Success: 0
     * Failure: -EINVALIDINODE, -EDIRNOTEMPTY, -EINVALIDNAME, -ENOTENOUGHSPACE,
     *          -EUNLINKNOTALLOWED
     * Failure modes: parentInodeNumber does not exist, directory is NOT
     * empty, or the name is invalid. Note that the name not existing is NOT
     * a failure by our definition. You can't unlink '.' or '..'
     */
    
    // Check if the name length is valid (0 < name < DIR_ENT_NAME_SIZE)
    if (name.length() <= 0 || name.length() >= (DIR_ENT_NAME_SIZE - 1)) { return -EINVALIDNAME; }
    
    // Check if user is attempting to unlink an unlinkable file
    if (name == "." || name == "..") { return -EUNLINKNOTALLOWED; }
    
    // Load the super block
    super_t super; readSuperBlock(&super); unsigned char blockBuffer[UFS_BLOCK_SIZE];
    
    // Check if the parent inode number is valid
    inode_t parent, inode;
    if (stat(parentInodeNumber, &parent) < 0) { return -EINVALIDINODE; }
    
    // If the parent inode is not a directory, return an error
    if (parent.type != UFS_DIRECTORY) { return -EINVALIDTYPE; }
    
    // Get the inode number of the entry by looking up the name
    int inodeNumber = lookup(parentInodeNumber, name);
    
    // If the entry does not exist, terminate successfully
    if (inodeNumber == -ENOTFOUND) { return 0; }
    
    // Otherwise get the inode of the file using the inode number
    if (stat(inodeNumber, &inode) < 0) { return -EINVALIDINODE; }
    
    // If the directory is not empty, return an error
    if (inode.type == UFS_DIRECTORY && (long unsigned int)inode.size > (2 * sizeof(dir_ent_t))) { return -EDIRNOTEMPTY; }
    
    // Load in the inode bitmap
    unsigned char inodeBitmap[UFS_BLOCK_SIZE * super.inode_bitmap_len];
    readInodeBitmap(&super, inodeBitmap);
    
    // Load in the data bitmap
    unsigned char dataBitmap[UFS_BLOCK_SIZE * super.data_bitmap_len];
    readDataBitmap(&super, dataBitmap);
    
    // Remove the data blocks from the data bitmap
    for (int i = 0; i < DIRECT_PTRS; i++) { if (inode.direct[i] != 0) {
        int j = inode.direct[i] - super.data_region_addr; int byteIndex = (j / 8), bitIndex = (j % 8);
        dataBitmap[byteIndex] &= ~(1 << bitIndex); inode.direct[i] = 0;
    }}  inode.size = 0;
    
    // Remove the inode from the inode bitmap
    inodeBitmap[(inodeNumber / 8)] &= ~(1 << (inodeNumber % 8));
    
    // Find and remove the entry in the parent's directory entries
    int position = -1;
    for (int i = 0; i < DIRECT_PTRS; i++) { if (parent.direct[i] != 0) {
        
        // Read the data block into the buffer
        disk->readBlock(parent.direct[i], blockBuffer);
        
        // Convert the data block buffer into entries
        dir_ent_t* entries = reinterpret_cast<dir_ent_t*>(blockBuffer);
        
        // Calculate the number of entries in the buffer
        int numEntries = UFS_BLOCK_SIZE / sizeof(dir_ent_t);
        
        // Check each entry to find a match
        for (int j = 0; j < numEntries; j++) { if (entries[j].inum >= 0 && entries[j].name == name) {
            
            // Track the position of the entry and set its inode number to invalid
            position = (i * numEntries) + j; entries[j].inum = -1;
            
            // Clear out the entry name
            memset(entries[j].name, 0, DIR_ENT_NAME_SIZE); break;
            
        }}
        
        // If the entry was found, then break fron the function after writing back the block
        if (position != -1) { disk->writeBlock(parent.direct[i], blockBuffer); break; }
        
    }}  if (position == -1) { return -ENOTFOUND; } /* If the entry was not found, return an error */
    
    // Shift entries to fill the gap left by the removed entry
    int numEntries = UFS_BLOCK_SIZE / sizeof(dir_ent_t);
    for (int i = position; (long unsigned int)i < (parent.size / sizeof(dir_ent_t)) - 1; i++) {
        
        // Get the block number and offset for the current and next entry
        int currBlockNumber = i / numEntries;       int currBlockOffset = i % numEntries;
        int nextBlockNumber = (i + 1) / numEntries; int nextBlockOffset = (i + 1) % numEntries;
        
        // If they are not the same, read in the next block buffer
        if (currBlockNumber != nextBlockNumber) { disk->readBlock(parent.direct[nextBlockNumber], blockBuffer); }
        
        // Read in the Block from the disk
        disk->readBlock(parent.direct[currBlockNumber], blockBuffer);
        
        // Convert the data block into entries
        dir_ent_t* entries = reinterpret_cast<dir_ent_t*>(blockBuffer);
        
        // Shift the entries
        entries[currBlockOffset] = entries[nextBlockOffset];
        
        // Write the block back to disk
        disk->writeBlock(parent.direct[currBlockNumber], blockBuffer);
        
    } parent.size -= sizeof(dir_ent_t); /* Adjust the parent size */
    
    // Remove the inode from the inode region
    inode_t inodes[super.num_inodes]; readInodeRegion(&super, inodes);
    memset(&inodes[inodeNumber], 0, sizeof(inode_t)); writeInodeRegion(&super, inodes);
    
    // Write the inode and data bitmap back to disk
    writeInodeBitmap(&super, inodeBitmap); writeDataBitmap(&super, dataBitmap);
    
    // Update the parent inode
    inodes[parentInodeNumber] = parent; writeInodeRegion(&super, inodes);
    
    return 0; /* Successful Termination */
    
}



bool LocalFileSystem::diskHasSpace(super_t *super, int numInodesNeeded, int numDataBytesNeeded, int numDataBlocksNeeded) {

    /**
     * numDataBytesNeeded is converted to blocks and added to numDataBlocksNeeded
     * Having two separate arguments for data helps for operations that write
     * new data to two separate entities. If you don't need a value
     * you can set the number needed to 0.
     */
    
    // Convert bytes needed to data blocks needed
    numDataBlocksNeeded += (numDataBytesNeeded + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;

    // Check for space in the inode bitmap
    if (numInodesNeeded > 0) {
        
        // Load the inode bitmap
        unsigned char inodeBitmap[super->inode_bitmap_len * UFS_BLOCK_SIZE]; readInodeBitmap(super, inodeBitmap);
        
        // Initialize variable to track number of available inodes
        int availableInodes = 0;
        
        // Count how many inodes are available
        for (int i = 0; i < (super->num_inodes); i++) { if (!(inodeBitmap[(i / 8)] & (1 << (i % 8)))) {
            availableInodes++;
        }}
        
        // Return if the number of inodes will suffice
        return availableInodes >= numInodesNeeded;
    }
    
    // Check for space in the data bitmap
    if (numDataBlocksNeeded > 0) {
        
        // Load the inode bitmap
        unsigned char dataBitmap[super->data_bitmap_len * UFS_BLOCK_SIZE]; readDataBitmap(super, dataBitmap);
        
        // Initialize variable to track number of available inodes
        int availableDataBlocks = 0;
        
        // Count how many inodes are available
        for (int i = 0; i < (super->num_data); i++) { if (!(dataBitmap[(i / 8)] & (1 << (i % 8)))) {
            availableDataBlocks++;
        }}
        
        // Return if the number of inodes will suffice
        return availableDataBlocks >= numInodesNeeded;
    }
    
    return true; /* Default Case */

}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->inode_bitmap_len; i++) {
        disk->readBlock((super->inode_bitmap_addr + i), buffer);
        memcpy((inodeBitmap + (i * UFS_BLOCK_SIZE)), buffer, UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->inode_bitmap_len; i++) {
        memcpy(buffer, (inodeBitmap + (i * UFS_BLOCK_SIZE)), UFS_BLOCK_SIZE);
        disk->writeBlock((super->inode_bitmap_addr + i), buffer);
    }
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->data_bitmap_len; i++) {
        disk->readBlock((super->data_bitmap_addr + i), buffer);
        memcpy((dataBitmap + (i * UFS_BLOCK_SIZE)), buffer, UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->data_bitmap_len; i++) {
        memcpy(buffer, (dataBitmap + (i * UFS_BLOCK_SIZE)), UFS_BLOCK_SIZE);
        disk->writeBlock((super->data_bitmap_addr + i), buffer);
    }
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
    
    // Calculate the total size of the inode region in bytes
    int inodeRegionSize = super->num_inodes * sizeof(inode_t);
    char* buffer = new char[inodeRegionSize];
    
    // Read the inode region block by block
    int readSize = 0;
    for (int i = 0; i < super->inode_region_len; i++) {
        int blockSize = std::min(UFS_BLOCK_SIZE, inodeRegionSize - readSize);
        disk->readBlock(super->inode_region_addr + i, buffer + readSize);
        readSize += blockSize;
    }
    
    // Copy the data into the provided inode array
    memcpy(inodes, buffer, inodeRegionSize);
    delete[] buffer;
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
    
    // Calculate the total size of the inode region in bytes
    int inodeRegionSize = super->num_inodes * sizeof(inode_t);
    char* buffer = new char[inodeRegionSize];

    // Copy the inode data into a buffer
    memcpy(buffer, inodes, inodeRegionSize);
    
    // Write the inode region block by block
    int writtenSize = 0;
    for (int i = 0; i < super->inode_region_len; i++) {
        int blockSize = std::min(UFS_BLOCK_SIZE, inodeRegionSize - writtenSize);
        disk->writeBlock(super->inode_region_addr + i, buffer + writtenSize);
        writtenSize += blockSize;
    }   delete[] buffer;
    
}

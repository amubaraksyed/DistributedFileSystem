#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <cassert>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "LocalFileSystem.h"
#include "Disk.h"
#include "ufs.h"

using namespace std;

// Function prototypes
void testCreateDir(LocalFileSystem &lfs, int parentInode, const string &name);
void testCreateFile(LocalFileSystem &lfs, int parentInode, const string &name);
void testWriteFile(LocalFileSystem &lfs, int parentInode, const string &name, const char *data);
void testReadFile(LocalFileSystem &lfs, int parentInode, const string &name);
void testUnlinkFile(LocalFileSystem &lfs, int parentInode, const string &name);
void testUnlinkDir(LocalFileSystem &lfs, int parentInode, const string &name);
void runUtility(const char *utility, const char *arg1 = nullptr, const char *arg2 = nullptr);

int main() {
    cout << "Creating a blank image using mkfs..." << endl;
    system("./mkfs -f disk.img -i 64 -d 64");

    // Initialize disk and filesystem
    cout << "Initializing disk and filesystem..." << endl;
    Disk disk("disk.img", UFS_BLOCK_SIZE);
    LocalFileSystem lfs(&disk);
    runUtility("./ds3bits", "disk.img");

    // Step 1: Create a directory
//    cout << "Step 1: Creating a directory 'testdir' in root directory..." << endl;
//    testCreateDir(lfs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "testdir");
    
    cout << "Step 1: Creating a file 'c.txt' in root directory..." << endl;
    testCreateFile(lfs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "c.txt");
    runUtility("./ds3ls", "disk.img");
    runUtility("./ds3bits", "disk.img");

    // Uncomment the following steps to run them
//    // Step 2: Confirm bitmap state
//    cout << "Step 2: Confirming bitmap state..." << endl;
//    runUtility("./ds3bits", "disk.img");
//
//    // Step 3: Create a file in root
//    cout << "Step 3: Creating a file 'testfile' in root directory..." << endl;
//    testCreateFile(lfs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "testfile");
//    runUtility("./ds3ls", "disk.img");
//
//    // Step 4: Confirm bitmap state
//    cout << "Step 4: Confirming bitmap state..." << endl;
//    runUtility("./ds3bits", "disk.img");
//
//    // Step 5: Write to the file in root
//    cout << "Step 5: Writing to the file 'testfile' in root directory..." << endl;
//    testWriteFile(lfs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "testfile", "Hello, root file!");
//    runUtility("./ds3bits", "disk.img");
//
//    // Step 6: Read the file in root
//    cout << "Step 6: Reading the file 'testfile' in root directory..." << endl;
//    testReadFile(lfs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "testfile");
//    runUtility("./ds3cat", "disk.img", "1"); // Assuming "testfile" inode is 1
//
//    // Step 7: Create a nested directory
//    cout << "Step 7: Creating a nested directory 'nesteddir' inside 'testdir'..." << endl;
//    int testdirInode = lfs.lookup(UFS_ROOT_DIRECTORY_INODE_NUMBER, "testdir");
//    testCreateDir(lfs, testdirInode, "nesteddir");
//    runUtility("./ds3ls", "disk.img");
//
//    // Step 8: Create a file in the nested directory
//    cout << "Step 8: Creating a file 'nestedfile' inside 'nesteddir'..." << endl;
//    int nestedDirInode = lfs.lookup(testdirInode, "nesteddir");
//    testCreateFile(lfs, nestedDirInode, "nestedfile");
//    runUtility("./ds3ls", "disk.img");
//
//    // Step 9: Write to the file in the nested directory
//    cout << "Step 9: Writing to the file 'nestedfile' inside 'nesteddir'..." << endl;
//    testWriteFile(lfs, nestedDirInode, "nestedfile", "Hello, nested file!");
//    runUtility("./ds3bits", "disk.img");
//
//    // Step 10: Read the file in the nested directory
//    cout << "Step 10: Reading the file 'nestedfile' inside 'nesteddir'..." << endl;
//    testReadFile(lfs, nestedDirInode, "nestedfile");
//    runUtility("./ds3cat", "disk.img", "2"); // Assuming "nestedfile" inode is 2
//
//    // Step 11: Unlink files and directories
//    cout << "Step 11: Unlinking files and directories..." << endl;
//    testUnlinkFile(lfs, nestedDirInode, "nestedfile");
//    testUnlinkDir(lfs, testdirInode, "nesteddir");
//    testUnlinkDir(lfs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "testdir");
//    testUnlinkFile(lfs, UFS_ROOT_DIRECTORY_INODE_NUMBER, "testfile");
//    runUtility("./ds3ls", "disk.img");
//    runUtility("./ds3bits", "disk.img");

    cout << "Finished running tests." << endl;
    return 0;
}

void testCreateDir(LocalFileSystem &lfs, int parentInode, const string &name) {
    cout << "Creating directory '" << name << "' with parent inode " << parentInode << "..." << endl;
    int result = lfs.create(parentInode, UFS_DIRECTORY, name);
    assert(result >= 0);
    cout << "Directory '" << name << "' created with inode number: " << result << endl;
}

void testCreateFile(LocalFileSystem &lfs, int parentInode, const string &name) {
    cout << "Creating file '" << name << "' with parent inode " << parentInode << "..." << endl;
    int result = lfs.create(parentInode, UFS_REGULAR_FILE, name);
    assert(result >= 0);
    cout << "File '" << name << "' created with inode number: " << result << endl;
}

void testWriteFile(LocalFileSystem &lfs, int parentInode, const string &name, const char *data) {
    cout << "Writing to file '" << name << "' with parent inode " << parentInode << "..." << endl;
    int inodeNumber = lfs.lookup(parentInode, name);
    int result = lfs.write(inodeNumber, data, strlen(data));
    assert(result == (int)strlen(data));
    cout << "Data written to '" << name << "': " << data << endl;
}

void testReadFile(LocalFileSystem &lfs, int parentInode, const string &name) {
    cout << "Reading file '" << name << "' with parent inode " << parentInode << "..." << endl;
    char buffer[256] = {0};
    int inodeNumber = lfs.lookup(parentInode, name);
    int result = lfs.read(inodeNumber, buffer, 256);
    assert(result > 0);
    cout << "Data read from '" << name << "': " << buffer << endl;
}

void testUnlinkFile(LocalFileSystem &lfs, int parentInode, const string &name) {
    cout << "Unlinking file '" << name << "' with parent inode " << parentInode << "..." << endl;
    int result = lfs.unlink(parentInode, name);
    assert(result == 0);
    cout << "File '" << name << "' unlinked successfully" << endl;
}

void testUnlinkDir(LocalFileSystem &lfs, int parentInode, const string &name) {
    cout << "Unlinking directory '" << name << "' with parent inode " << parentInode << "..." << endl;
    int result = lfs.unlink(parentInode, name);
    assert(result == 0);
    cout << "Directory '" << name << "' unlinked successfully" << endl;
}

void runUtility(const char *utility, const char *arg1, const char *arg2) {
    cout << "Running utility: " << utility;
    if (arg1) cout << " " << arg1;
    if (arg2) cout << " " << arg2;
    cout << "..." << endl;
    
    pid_t pid = fork();
    if (pid == 0) {
        if (arg2) {
            execl(utility, utility, arg1, arg2, nullptr);
        } else if (arg1) {
            execl(utility, utility, arg1, nullptr);
        } else {
            execl(utility, utility, nullptr);
        }
        perror("execl failed");
        exit(1);
    } else {
        wait(nullptr);
    }
    cout << "Utility " << utility << " finished." << endl;
}

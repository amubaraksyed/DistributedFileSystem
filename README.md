# Distributed File System

## Project Overview

This project involves developing a functional distributed file server designed to manage persistent storage with an emphasis on understanding on-disk structures, file system internals, and distributed storage systems. The project includes a series of utilities to interact with and manage the file system effectively.

## Project Goals

- Gain a deep understanding of on-disk structures for file systems.
- Explore the internal workings of file systems.
- Learn about the principles and implementation of distributed storage systems.

## Project Structure

The project is structured into three main components:

1. **File System Utilities**: Command-line tools to interact with and manage the file system.
2. **Local File System**: A local file system implementation managing disk blocks and inodes.
3. **Distributed File System**: A distributed file system using the local file system to provide a networked storage solution.

## Distributed File System

### Background

The distributed file system (DFS) allows multiple clients to access the same file system concurrently, similar to Amazon's S3 storage system. The DFS provides high-level file system operations like `read()`, `write()`, and `delete()` through a simple REST/HTTP API.

### HTTP/REST API

The DFS manages two main entities: `files` and `directories`. These entities are accessed using standard HTTP/REST calls, with URL paths beginning with `/ds3/`.

#### File Operations

- **Create/Update File**: The HTTP `PUT` method creates or updates a file. The URL defines the file path, and the request body contains the file's contents.
- **Read File**: The HTTP `GET` method retrieves the contents of a file specified by the URL.
- **Read Directory**: The HTTP `GET` method lists directory entries, sorted using standard string comparison.
- **Delete File/Directory**: The HTTP `DELETE` method removes a file or directory. Deleting a non-empty directory results in an error.

### API Handlers

The API handlers are implemented in `DistributedFileSystemService.cpp`. The DFS can be tested using command-line utilities like cURL:

```bash
curl -X PUT -d "file contents" http://localhost:8080/ds3/a/b/c.txt 
curl http://localhost:8080/ds3/a/b/c.txt                          
curl http://localhost:8080/ds3/a/b/     
curl http://localhost:8080/ds3/a  
curl -X DELETE http://localhost:8080/ds3/a/b/c.txt
```

### Error Handling

The DFS ensures that errors do not alter the underlying disk or file system. Transactions managed through the `Disk` interface (`beginTransaction`, `commit`, `rollback`) maintain consistency.

#### Error Types

- **Resource Not Found**: Returned when accessing a non-existent resource.
- **Insufficient Storage**: Triggered when there is not enough storage to complete an operation.
- **Conflict**: Occurs when creating a directory in a location where a file already exists.
- **Bad Request**: Used for all other errors.

## Local File System

### On-Disk File System Structure

The local file system mimics a simple Unix file system:
- **Super Block**: A single 4KB block.
- **Inode Bitmap**: One or more 4KB blocks.
- **Data Bitmap**: One or more 4KB blocks.
- **Inode Table**: Multiple 4KB blocks.
- **Data Region**: A number of 4KB blocks.

### Directories

Directories contain 32-byte entries with a name and inode number pair, including `.` and `..` entries for the root directory.

### Consistency

The file system maintains consistency by carefully ordering disk writes. Transactions (`beginTransaction`, `commit`, `rollback`) ensure atomic operations.

### Bitmaps for Block Allocation

Bitmaps track allocated inodes and data blocks, with appropriate use of LSB and MSB.

### Read and Write Semantics

- **Write**: Overwrites the entire file.
- **Read**: Reads from the beginning of the file, returning the specified number of bytes.

### Out of Storage Errors

The file system detects and handles out of storage errors before making any disk writes, ensuring no changes occur if storage is insufficient.

## File System Utilities

### `ds3ls` Utility

Lists all files and directories in a disk image, starting at the root and traversing depth-first. Entries are sorted using `std::strcmp`.

### `ds3cat` Utility

Prints the contents of a file specified by the inode number, displaying disk block numbers and file data.

### `ds3bits` Utility

Prints metadata about the file system, including the super block, inode bitmap, and data bitmap.

## Conclusion

This Distributed File System project showcases a comprehensive implementation of a local and distributed file system, highlighting key aspects of persistent storage, error handling, and efficient file management. The project serves as a valuable addition to any software engineering portfolio, demonstrating expertise in system-level programming and distributed systems.

## Disclaimer

This project was developed as part of ECS 150: Operating Systems at UC Davis under the guidance of Professor Sam King, aimed at applying theoretical knowledge to the practical implementation of distributed file systems. I do not claim credit for the development of the Gunrock web server. My contributions include the implementation of `DistributedFileSystemService.cpp`, `LocalFileSystem.cpp`, `ds3bits.cpp`, `ds3cat.cpp`, and `ds3ls.cpp`, demonstrating my proficiency in distributed file systems.

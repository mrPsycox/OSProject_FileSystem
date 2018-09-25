a simple file system interface using binary files mapped with Bitmap. Use for the disk a txt file where store the files or directories.
There are two blocks of type: 
- Directory blocks
- Data blocks
The file system stores these blocks reserving the first part to store the linked lists blocks (free blocks, files blocks) and
also a single global directory (root).
The final project was an user interface by shell, the FS can create files or directory and make operations on them.

This project was assigned by the course of Sistemi Operativi (Operating Systems) hosted by Sapienza University of Rome.

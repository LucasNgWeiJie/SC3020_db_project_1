# SC3020 Database Project 1 – Task 1: Block-Based Storage Manager

## Overview

This project implements a simple block-based storage manager in C++.  
It simulates disk storage using a binary file (`disk.img`), storing data from `games.txt` in fixed-size blocks (4096 bytes each).

## Features

- Reads records from `../data/games.txt`
- Stores data into a virtual disk file (`disk.img`) in 4096-byte blocks
- Provides functions to create, destroy, read, and write blocks
- Reports statistics: record size, number of records, records per block, and number of blocks used

## How to Run

1. Place `games.txt` in the data directory (one level above src).
2. Build the project:
   ```
   g++ -o main main.cpp storage.cpp
   ```
3. Run the program from the src directory:
   ```
   ./main
   ```
4. The program will:
   - Copy the contents of `games.txt` into `disk.img`
   - Print storage statistics

## File Structure

- storage.h – Storage manager API
- storage.cpp – Storage manager implementation
- main.cpp – Main program for Task 1
- games.txt – Input data file
- disk.img – Virtual disk file (created by the program)

## Notes

- All storage operations are performed on `disk.img` to avoid modifying the real file system.
- Block size is set to 4096 bytes for efficient storage and retrieval.

---


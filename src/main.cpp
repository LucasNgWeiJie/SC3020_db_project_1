#include "storage.h"
#include <iostream>
#include <fstream>
#include <cstring>

int main() {
    std::string gamesPath = "../data/games.txt";
    std::string diskFilename = "disk.img";

    // 1. Open games.txt and determine its size
    std::ifstream gamesFile(gamesPath, std::ios::binary | std::ios::ate);
    if (!gamesFile) {
        std::cout << "Failed to open " << gamesPath << "\n";
        return 1;
    }
    std::streamsize gamesSize = gamesFile.tellg();
    gamesFile.seekg(0);

    // 2. Calculate number of blocks needed
    int numBlocks = static_cast<int>((gamesSize + BLOCK_SIZE - 1) / BLOCK_SIZE);

    // 3. Create virtual disk file
    if (!createFile(diskFilename, numBlocks)) {
        std::cout << "Failed to create virtual disk file.\n";
        return 1;
    }

    // 4. Read from games.txt and write to disk.img block by block
    char buffer[BLOCK_SIZE];
    for (int blockNum = 0; blockNum < numBlocks; ++blockNum) {
        std::memset(buffer, 0, BLOCK_SIZE);
        gamesFile.read(buffer, BLOCK_SIZE);
        if (!writeBlock(diskFilename, blockNum, buffer)) {
            std::cout << "Failed to write block " << blockNum << " to disk.\n";
            return 1;
        }
    }

    // --- Statistics ---
    // 1. Size of a record (assuming all records are the same size)
    std::ifstream gamesFile2(gamesPath);
    std::string line;
    size_t recordSize = 0;
    int numRecords = 0;
    if (std::getline(gamesFile2, line)) {
        recordSize = line.size() + 1; // +1 for newline
        numRecords = 1;
        while (std::getline(gamesFile2, line)) {
            ++numRecords;
        }
    }

    // 2. Number of records per block
    int recordsPerBlock = recordSize ? (BLOCK_SIZE / recordSize) : 0;

    // 3. Number of blocks (already computed: numBlocks)

    std::cout << "\n--- Statistics ---\n";
    std::cout << "Size of a record: " << recordSize << " bytes\n";
    std::cout << "Number of records: " << numRecords << "\n";
    std::cout << "Records per block: " << recordsPerBlock << "\n";
    std::cout << "Number of blocks: " << numBlocks << "\n";
}
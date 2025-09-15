#include "storage.h"
#include <fstream>
#include <iostream>
#include <cstring>
#include <vector>
#include <sys/stat.h>

// Helper to get file size in bytes
static std::streamoff getFileSize(const std::string &filename) {
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    if (!file) return -1;
    return file.tellg();
}

bool createFile(const std::string &filename, int numBlocks) {
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file) return false;
    std::vector<char> emptyBlock(BLOCK_SIZE, 0); // vector of 
    for (int i = 0; i < numBlocks; ++i) {
        file.write(emptyBlock.data(), BLOCK_SIZE);
        if (!file) return false;
    }
    return true;
}

bool destroyFile(const std::string &filename) {
    return std::remove(filename.c_str()) == 0;
}

bool readBlock(const std::string &filename, int blockNum, char *buffer) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return false;
    file.seekg(blockNum * BLOCK_SIZE);
    if (!file) return false;
    file.read(buffer, BLOCK_SIZE);
    return file.gcount() == BLOCK_SIZE;
}

bool writeBlock(const std::string &filename, int blockNum, const char *buffer) {
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);
    if (!file) return false;
    file.seekp(blockNum * BLOCK_SIZE);
    if (!file) return false;
    file.write(buffer, BLOCK_SIZE);
    return file.good();
}

int getNumBlocks(const std::string &filename) {
    std::streamoff size = getFileSize(filename);
    if (size < 0) return -1;
    return static_cast<int>(size / BLOCK_SIZE);
}
#pragma once
#include <string>

const int BLOCK_SIZE = 4096;

bool createFile(const std::string &filename, int numBlocks);
bool destroyFile(const std::string &filename);
bool readBlock(const std::string &filename, int blockNum, char *buffer);
bool writeBlock(const std::string &filename, int blockNum, const char *buffer);
int getNumBlocks(const std::string &filename);
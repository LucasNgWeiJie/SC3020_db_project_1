#ifndef GAME_RECORD_H
#define GAME_RECORD_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

// Structure to represent a single game record
struct GameRecord
{
    char game_date[11]; // YYYY-MM-DD format (10 chars + null terminator)
    int team_id_home;   // Team ID for home team
    int pts_home;       // Points scored by home team
    float fg_pct_home;  // Field goal percentage
    float ft_pct_home;  // Free throw percentage
    float fg3_pct_home; // 3-point field goal percentage
    int ast_home;       // Assists by home team
    int reb_home;       // Rebounds by home team
    int home_team_wins; // 1 if home team wins, 0 otherwise

    // Default constructor
    GameRecord();

    // Constructor with parameters
    GameRecord(const std::string &date, int team_id, int pts, float fg_pct,
               float ft_pct, float fg3_pct, int ast, int reb, int wins);

    // Display record information
    void display() const;

    // Get the size of a single record in bytes
    static size_t getRecordSize();
};

// Structure to represent a disk block
struct Block
{
    static const size_t BLOCK_SIZE = 4096; // 4KB block size
    char data[BLOCK_SIZE];
    size_t used_space;
    int record_count;

    Block();

    // Add a record to the block if there's space
    bool addRecord(const GameRecord &record);

    // Get a record from the block at specified index
    GameRecord getRecord(int index) const;

    // Check if block can fit another record
    bool canFitRecord() const;

    // Get maximum records per block
    static int getMaxRecordsPerBlock();
};

// Database file manager class
class DatabaseFile
{
private:
    std::string filename;
    std::fstream file;
    std::vector<Block> blocks;
    size_t total_records;
    size_t total_blocks;

public:
    DatabaseFile(const std::string &db_filename);
    ~DatabaseFile();

    // Read data from games.txt and store in database format
    bool loadFromTextFile(const std::string &text_filename);

    // Write blocks to disk file
    bool writeBlocksToDisk();

    // Read blocks from disk file
    bool readBlocksFromDisk();

    // Add a single record to the database
    bool addRecord(const GameRecord &record);

    // Get statistics
    size_t getTotalRecords() const { return total_records; }
    size_t getTotalBlocks() const { return total_blocks; }
    size_t getRecordSize() const { return GameRecord::getRecordSize(); }
    int getRecordsPerBlock() const { return Block::getMaxRecordsPerBlock(); }

    // Display all records
    void displayAllRecords() const;

    // Display statistics
    void displayStatistics() const;

    // Parse a line from games.txt
    bool parseGameLine(const std::string &line, GameRecord &record);
};

// Utility functions
namespace Utils
{
    // Trim whitespace from string
    std::string trim(const std::string &str);

    // Convert string to float, return 0.0 if invalid
    float safeStringToFloat(const std::string &str);

    // Convert string to int, return 0 if invalid
    int safeStringToInt(const std::string &str);

    // Split string by delimiter
    std::vector<std::string> split(const std::string &str, char delimiter);
}

#endif // GAME_RECORD_H
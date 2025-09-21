#ifndef GAME_RECORD_H
#define GAME_RECORD_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

// Forward declaration
class DatabaseFile;

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

// B+ Tree Node for indexing
template<typename KeyType>
struct BPlusTreeNode
{
    static const int MAX_KEYS = 20;  // Maximum 20 keys per node
    static const int MIN_KEYS = MAX_KEYS / 2;  // Minimum keys for balanced tree
    
    bool is_leaf;
    int key_count;
    KeyType keys[MAX_KEYS];
    
    // For leaf nodes: pointers to data records. Last pointer to next leaf node.
    // For internal nodes: pointers to child nodes
    // a union allows the same memory space to serve different purposes depending on the node type
    union {
        BPlusTreeNode<KeyType>* children[MAX_KEYS + 1];  // Internal nodes
        struct {
            int block_ids[MAX_KEYS];    // Block containing the record
            int record_ids[MAX_KEYS];   // Position within the block
            BPlusTreeNode<KeyType>* next_leaf; // Link to next leaf
        } leaf_data;
    };
    
    // constructor and destructor
    BPlusTreeNode(bool leaf = true);
    ~BPlusTreeNode();

    // scenarios
    bool isFull() const { return key_count >= MAX_KEYS; }
    bool isUnderflow() const { return key_count < MIN_KEYS; }
};


// Index manager for different attributes
class IndexManager
{
private:
    BPlusTreeNode<int>* team_id_index;      // Index on team_id_home
    BPlusTreeNode<int>* points_index;       // Index on pts_home  
    BPlusTreeNode<float>* fg_pct_index;     // Index on fg_pct_home
    BPlusTreeNode<std::string>* date_index; // Index on game_date
    BPlusTreeNode<float>* ft_pct_index;     // Index on ft_pct_home

public:
    IndexManager();
    ~IndexManager();
    
    // Build indexes from database
    bool buildIndexes(const DatabaseFile& db);
    
    // Search operations
    std::vector<std::pair<int, int>> searchByTeamId(int team_id);
    std::vector<std::pair<int, int>> searchByPointsRange(int min_pts, int max_pts);
    std::vector<std::pair<int, int>> searchByFGPercentage(float min_pct, float max_pct);
    std::vector<std::pair<int, int>> searchByDate(const std::string& date);
    std::vector<std::pair<int, int>> searchByFTPercentage(float min_pct, float max_pct);
    
    // Insert/Update operations
    bool insertIndex(const GameRecord& record, int block_id, int record_id);
    bool deleteIndex(const GameRecord& record);
    
    // Index statistics
    void displayIndexStatistics() const;
    
private:
    // B+ Tree operations
    template<typename KeyType>
    bool insert(BPlusTreeNode<KeyType>*& root, KeyType key, int block_id, int record_id);
    
    template<typename KeyType>
    std::vector<std::pair<int, int>> search(BPlusTreeNode<KeyType>* root, KeyType key);
    
    template<typename KeyType>
    std::vector<std::pair<int, int>> rangeSearch(BPlusTreeNode<KeyType>* root, KeyType min_key, KeyType max_key);
    
    template<typename KeyType>
    std::pair<KeyType, BPlusTreeNode<KeyType>*> splitLeaf(BPlusTreeNode<KeyType>* leaf);
    
    template<typename KeyType>
    bool insertIntoLeaf(BPlusTreeNode<KeyType>* leaf, KeyType key, int block_id, int record_id);
    
    template<typename KeyType>
    void displaySingleIndexStats(const std::string& index_name, BPlusTreeNode<KeyType>* root) const;
    
    template<typename KeyType>
    int countNodes(BPlusTreeNode<KeyType>* root) const;
    
    template<typename KeyType>
    int countLeafNodes(BPlusTreeNode<KeyType>* root) const;
    
    template<typename KeyType>
    int getTreeHeight(BPlusTreeNode<KeyType>* root) const;
    
    template<typename KeyType>
    int getTotalKeys(BPlusTreeNode<KeyType>* root) const;
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
    IndexManager* index_manager;

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

    //Index Related method
    bool buildIndexes();
    std::vector<GameRecord>searchByTeamId(int team_id);
    std::vector<GameRecord>searchByPointsRange(int min_pts, int max_pts);
    std::vector<GameRecord>searchByFGPercentage(float min_pct, float max_pct);
    std::vector<GameRecord>searchByFTPercentage(float min_pct, float max_pct);

    // Utility method to access blocks
    const Block& getBlock(size_t index) const { return blocks[index]; }

    //Display index stats
    void displayIndexStatistics() const;
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

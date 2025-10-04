#ifndef GAME_RECORD_H
#define GAME_RECORD_H

#include <string>
#include <vector>
#include <fstream>
#include <iostream>
#include <cstdint>

// Forward declaration
class DatabaseFile;

// =============================
// Record & Block (Task 1 base)
// =============================
struct GameRecord
{
    // 11 bytes for date + 1 byte for boolean = 12 bytes (divisible by 4)
    char game_date[11];  // 11 bytes - "YYYY-MM-DD"
    bool home_team_wins; // 1 byte - true/false instead of int
    int team_id_home;    // 4 bytes
    int pts_home;        // 4 bytes
    float fg_pct_home;   // 4 bytes
    float ft_pct_home;   // 4 bytes
    float fg3_pct_home;  // 4 bytes
    int ast_home;        // 4 bytes
    int reb_home;        // 4 bytes

    GameRecord();
    GameRecord(const std::string &date, int team_id, int pts, float fg_pct,
               float ft_pct, float fg3_pct, int ast, int reb, bool wins);

    void display() const;
    static size_t getRecordSize();
};

struct Block
{
    static const size_t BLOCK_SIZE = 4096; // 4KB page
    char data[BLOCK_SIZE];
    size_t used_space;
    int record_count;

    Block();
    bool addRecord(const GameRecord &record);
    GameRecord getRecord(int index) const;
    bool canFitRecord() const;
    static int getMaxRecordsPerBlock();
};

// =============================
// B+ Tree node (fixed arrays)
// =============================
template <typename KeyType>
struct BPlusTreeNode
{
    static const int MAX_KEYS = 20; // matches your stats print
    static const int MIN_KEYS = MAX_KEYS / 2;

    bool is_leaf;
    int key_count;
    KeyType keys[MAX_KEYS];

    union
    {
        // Internal nodes
        BPlusTreeNode<KeyType> *children[MAX_KEYS + 1];
        // Leaf nodes
        struct
        {
            int block_ids[MAX_KEYS];
            int record_ids[MAX_KEYS];
            BPlusTreeNode<KeyType> *next_leaf;
        } leaf_data;
    };

    BPlusTreeNode(bool leaf = true);
    ~BPlusTreeNode();

    bool isFull() const { return key_count >= MAX_KEYS; }
    bool isUnderflow() const { return key_count < MIN_KEYS; }
};

// ======================================================
// Task 3 – Deletion experiment (add-only, non-disruptive)
// ======================================================
struct DeletionStats
{
    uint32_t nInternal = 0; // internal B+ nodes visited (indexed path)
    uint32_t nLeaf = 0;     // leaf B+ nodes visited (indexed path)
    uint32_t nOverflow = 0; // kept for symmetry; not used here
    uint32_t nData = 0;     // distinct data blocks touched
    uint32_t nDeleted = 0;  // records deleted
    double sumFT = 0.0;     // sum of FT% for deleted set (to average)
    long long timeUs = 0;   // wallclock microseconds
};

// =============================
// IndexManager (Task 2 + Task 3)
// =============================
class IndexManager
{
private:
    BPlusTreeNode<int> *team_id_index;      // TEAM_ID_home
    BPlusTreeNode<int> *points_index;       // PTS_home
    BPlusTreeNode<float> *fg_pct_index;     // FG_PCT_home
    BPlusTreeNode<std::string> *date_index; // GAME_DATE
    BPlusTreeNode<float> *ft_pct_index;     // FT_PCT_home

public:
    IndexManager();
    ~IndexManager();

    // Build full indexes (existing Task 2)
    bool buildIndexes(const DatabaseFile &db);

    // Search (existing Task 2)
    std::vector<std::pair<int, int>> searchByTeamId(int team_id);
    std::vector<std::pair<int, int>> searchByPointsRange(int min_pts, int max_pts);
    std::vector<std::pair<int, int>> searchByFGPercentage(float min_pct, float max_pct);
    std::vector<std::pair<int, int>> searchByDate(const std::string &date);
    std::vector<std::pair<int, int>> searchByFTPercentage(float min_pct, float max_pct);

    // Task 3: counts-aware FT% range scan
    std::vector<std::pair<int, int>>
    searchByFTPercentageWithCounts(float min_pct, float max_pct,
                                   uint32_t &outInternal, uint32_t &outLeaf);

    // Task 3: rebuild indexes skipping tombstoned rows
    bool buildIndexesSkippingDeleted(const DatabaseFile &db);

    // Stats (existing)
    void displayIndexStatistics() const;

private:
    // Core B+ ops (existing)
    template <typename KeyType>
    bool insert(BPlusTreeNode<KeyType> *&root, KeyType key, int block_id, int record_id);

    template <typename KeyType>
    std::vector<std::pair<int, int>> search(BPlusTreeNode<KeyType> *root, KeyType key);

    template <typename KeyType>
    std::vector<std::pair<int, int>> rangeSearch(BPlusTreeNode<KeyType> *root, KeyType min_key, KeyType max_key);

    template <typename KeyType>
    std::pair<KeyType, BPlusTreeNode<KeyType> *> splitLeaf(BPlusTreeNode<KeyType> *leaf);

    template <typename KeyType>
    bool insertIntoLeaf(BPlusTreeNode<KeyType> *leaf, KeyType key, int block_id, int record_id);

    template <typename KeyType>
    void displaySingleIndexStats(const std::string &index_name, BPlusTreeNode<KeyType> *root) const;

    template <typename KeyType>
    int countNodes(BPlusTreeNode<KeyType> *root) const;

    template <typename KeyType>
    int countLeafNodes(BPlusTreeNode<KeyType> *root) const;

    template <typename KeyType>
    int getTreeHeight(BPlusTreeNode<KeyType> *root) const;

    template <typename KeyType>
    int getTotalKeys(BPlusTreeNode<KeyType> *root) const;
};

// =============================
// DatabaseFile (Task 1/2 + 3)
// =============================
class DatabaseFile
{
private:
    std::string filename;
    std::fstream file;
    std::vector<Block> blocks;
    size_t total_records;
    size_t total_blocks;
    IndexManager *index_manager;

    // Task 3: in-memory tombstones (does NOT change on-disk layout)
    std::vector<std::vector<uint8_t>> deleted_; // deleted_[block][slot] = 1
    void ensureDeletedBitmapInitialized_();

public:
    DatabaseFile(const std::string &db_filename);
    ~DatabaseFile();

    // Task 1: storage
    bool loadFromTextFile(const std::string &text_filename);
    bool writeBlocksToDisk();
    bool readBlocksFromDisk();
    bool addRecord(const GameRecord &record);

    // Stats / access
    size_t getTotalRecords() const { return total_records; }
    size_t getTotalBlocks() const { return total_blocks; }
    size_t getRecordSize() const { return GameRecord::getRecordSize(); }
    int getRecordsPerBlock() const { return Block::getMaxRecordsPerBlock(); }
    const Block &getBlock(size_t index) const { return blocks[index]; }

    void displayAllRecords() const;
    void displayStatistics() const;

    // Parsing and validation
    bool parseGameLine(const std::string &line, GameRecord &record);
    bool isRecordValid(const GameRecord &record) const;

    // Task 2: indexes
    bool buildIndexes();
    std::vector<GameRecord> searchByTeamId(int team_id);
    std::vector<GameRecord> searchByPointsRange(int min_pts, int max_pts);
    std::vector<GameRecord> searchByFGPercentage(float min_pct, float max_pct);
    std::vector<GameRecord> searchByFTPercentage(float min_pct, float max_pct);
    void displayIndexStatistics() const;

    // Task 3: tombstone helpers + deletion paths
    bool isDeleted(size_t block_id, int record_id) const;
    void markDeleted(size_t block_id, int record_id);

    DeletionStats deleteByFTAboveIndexed(float thresh); // via FT% index
    DeletionStats deleteByFTAboveLinear(float thresh);  // full scan

    // Task 3: rebuild FT index skipping deleted rows
    void rebuildFTIndexSkippingDeleted();
};

// =============================
// Utils (existing helpers)
// =============================
namespace Utils
{
    std::string trim(const std::string &str);
    float safeStringToFloat(const std::string &str);
    int safeStringToInt(const std::string &str);
    std::vector<std::string> split(const std::string &str, char delimiter);
    bool isEmptyOrWhitespace(const std::string &str); // NEW
}

#endif // GAME_RECORD_H
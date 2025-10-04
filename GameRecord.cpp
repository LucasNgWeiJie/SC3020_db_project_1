#include "GameRecord.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>
#include <unordered_set>
#include <chrono>
#include <limits>
#include <cmath>

// =========================
// Utils (existing helpers)
// =========================
namespace Utils
{
    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    float safeStringToFloat(const std::string &str)
    {
        try { return str.empty() ? 0.0f : std::stof(str); }
        catch (...) { return 0.0f; }
    }

    int safeStringToInt(const std::string &str)
    {
        try { return str.empty() ? 0 : std::stoi(str); }
        catch (...) { return 0; }
    }

    std::vector<std::string> split(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        while (std::getline(ss, token, delimiter)) tokens.push_back(token);
        return tokens;
    }

    // NEW: Check if a string is effectively empty (whitespace only or truly empty)
    bool isEmptyOrWhitespace(const std::string &str)
    {
        return str.empty() || str.find_first_not_of(" \t\r\n") == std::string::npos;
    }
}

// =========================
// GameRecord (existing)
// =========================
GameRecord::GameRecord()
{
    std::memset(game_date, 0, sizeof(game_date));
    team_id_home = 0;
    pts_home = 0;
    fg_pct_home = 0.0f;
    ft_pct_home = 0.0f;
    fg3_pct_home = 0.0f;
    ast_home = 0;
    reb_home = 0;
    home_team_wins = 0;
}

GameRecord::GameRecord(const std::string &date, int team_id, int pts, float fg_pct,
                       float ft_pct, float fg3_pct, int ast, int reb, int wins)
{
    std::strncpy(game_date, date.c_str(), sizeof(game_date) - 1);
    game_date[sizeof(game_date) - 1] = '\0';
    team_id_home = team_id;
    pts_home = pts;
    fg_pct_home = fg_pct;
    ft_pct_home = ft_pct;
    fg3_pct_home = fg3_pct;
    ast_home = ast;
    reb_home = reb;
    home_team_wins = wins;
}

void GameRecord::display() const
{
    std::cout << "Date: " << game_date
              << ", Team ID: " << team_id_home
              << ", Points: " << pts_home
              << ", FG%: " << std::fixed << std::setprecision(3) << fg_pct_home
              << ", FT%: " << ft_pct_home
              << ", 3P%: " << fg3_pct_home
              << ", AST: " << ast_home
              << ", REB: " << reb_home
              << ", Win: " << home_team_wins << std::endl;
}

size_t GameRecord::getRecordSize()
{
    return sizeof(GameRecord);
}

// =========================
// Block (existing)
// =========================
Block::Block()
{
    std::memset(data, 0, BLOCK_SIZE);
    used_space = 0;
    record_count = 0;
}

bool Block::addRecord(const GameRecord &record)
{
    size_t record_size = GameRecord::getRecordSize();
    if (used_space + record_size > BLOCK_SIZE) return false;

    std::memcpy(data + used_space, &record, record_size);
    used_space += record_size;
    record_count++;
    return true;
}

GameRecord Block::getRecord(int index) const
{
    GameRecord record;
    if (index >= 0 && index < record_count)
    {
        size_t record_size = GameRecord::getRecordSize();
        std::memcpy(&record, data + (index * record_size), record_size);
    }
    return record;
}

bool Block::canFitRecord() const
{
    return (used_space + GameRecord::getRecordSize()) <= BLOCK_SIZE;
}

int Block::getMaxRecordsPerBlock()
{
    return BLOCK_SIZE / GameRecord::getRecordSize();
}

// =========================
// DatabaseFile (Task 1/2)
// =========================
DatabaseFile::DatabaseFile(const std::string &db_filename)
    : filename(db_filename), total_records(0), total_blocks(0)
{
    index_manager = new IndexManager();
}

DatabaseFile::~DatabaseFile()
{
    if (file.is_open()) file.close();
    delete index_manager;
}

bool DatabaseFile::buildIndexes()
{
    if (index_manager) return index_manager->buildIndexes(*this);
    return false;
}

std::vector<GameRecord> DatabaseFile::searchByTeamId(int team_id)
{
    std::vector<GameRecord> results;
    if (!index_manager) return results;

    auto locations = index_manager->searchByTeamId(team_id);
    for (const auto& loc : locations) {
        if (static_cast<size_t>(loc.first) < blocks.size()) {
            results.push_back(blocks[loc.first].getRecord(loc.second));
        }
    }
    return results;
}

std::vector<GameRecord> DatabaseFile::searchByPointsRange(int min_pts, int max_pts)
{
    std::vector<GameRecord> results;
    if (!index_manager) return results;

    auto locations = index_manager->searchByPointsRange(min_pts, max_pts);
    for (const auto& loc : locations) {
        if (static_cast<size_t>(loc.first) < blocks.size()) {
            results.push_back(blocks[loc.first].getRecord(loc.second));
        }
    }
    return results;
}

std::vector<GameRecord> DatabaseFile::searchByFGPercentage(float min_pct, float max_pct)
{
    std::vector<GameRecord> results;
    if (!index_manager) return results;

    auto locations = index_manager->searchByFGPercentage(min_pct, max_pct);
    for (const auto& loc : locations) {
        if (static_cast<size_t>(loc.first) < blocks.size()) {
            results.push_back(blocks[loc.first].getRecord(loc.second));
        }
    }
    return results;
}

std::vector<GameRecord> DatabaseFile::searchByFTPercentage(float min_pct, float max_pct)
{
    std::vector<GameRecord> results;
    if (!index_manager) return results;

    auto locations = index_manager->searchByFTPercentage(min_pct, max_pct);
    for (const auto& loc : locations) {
        if (static_cast<size_t>(loc.first) < blocks.size()) {
            results.push_back(blocks[loc.first].getRecord(loc.second));
        }
    }
    return results;
}

void DatabaseFile::displayIndexStatistics() const
{
    if (index_manager) index_manager->displayIndexStatistics();
}

bool DatabaseFile::loadFromTextFile(const std::string &text_filename)
{
    std::ifstream input_file(text_filename);
    if (!input_file.is_open())
    {
        std::cerr << "Error: Cannot open file " << text_filename << std::endl;
        return false;
    }

    std::string line;
    bool first_line = true;
    int skipped_records = 0;  // Track skipped records

    // Start first block
    blocks.push_back(Block());
    total_blocks = 1;

    while (std::getline(input_file, line))
    {
        if (first_line) { first_line = false; continue; } // skip header

        GameRecord record;
        if (parseGameLine(line, record))
        {
            // NEW: Validate record before adding
            if (isRecordValid(record))
            {
                if (!blocks.back().canFitRecord())
                {
                    blocks.push_back(Block());
                    total_blocks++;
                }
                if (blocks.back().addRecord(record)) 
                    total_records++;
                else 
                    std::cerr << "Error: Could not add record to block\n";
            }
            else
            {
                skipped_records++;
            }
        }
        else
        {
            skipped_records++;
        }
    }

    input_file.close();
    std::cout << "Successfully loaded " << total_records << " records into "
              << total_blocks << " blocks." << std::endl;
    if (skipped_records > 0)
    {
        std::cout << "Skipped " << skipped_records << " records with empty or invalid values." << std::endl;
    }

    // Lazy-init tombstones for Task 3 (keeps Task 1/2 behavior unchanged)
    ensureDeletedBitmapInitialized_();
    return true;
}

bool DatabaseFile::writeBlocksToDisk()
{
    file.open(filename, std::ios::binary | std::ios::out);
    if (!file.is_open())
    {
        std::cerr << "Error: Cannot create database file " << filename << std::endl;
        return false;
    }

    // Write header
    file.write(reinterpret_cast<const char *>(&total_records), sizeof(total_records));
    file.write(reinterpret_cast<const char *>(&total_blocks), sizeof(total_blocks));

    // Write raw blocks
    for (const auto &block : blocks)
    {
        file.write(reinterpret_cast<const char *>(&block), sizeof(Block));
    }

    file.close();
    std::cout << "Database written to disk: " << filename << std::endl;
    return true;
}

bool DatabaseFile::readBlocksFromDisk()
{
    file.open(filename, std::ios::binary | std::ios::in);
    if (!file.is_open())
    {
        std::cerr << "Error: Cannot open database file " << filename << std::endl;
        return false;
    }

    // Read header
    file.read(reinterpret_cast<char *>(&total_records), sizeof(total_records));
    file.read(reinterpret_cast<char *>(&total_blocks), sizeof(total_blocks));

    // Read blocks
    blocks.clear();
    blocks.resize(total_blocks);
    for (auto &block : blocks)
    {
        file.read(reinterpret_cast<char *>(&block), sizeof(Block));
    }

    file.close();
    std::cout << "Database read from disk: " << filename << std::endl;

    // Refresh tombstones to match current in-memory blocks
    ensureDeletedBitmapInitialized_();
    return true;
}

bool DatabaseFile::addRecord(const GameRecord &record)
{
    // NEW: Validate before adding
    if (!isRecordValid(record))
    {
        std::cerr << "Warning: Attempted to add invalid record with empty values\n";
        return false;
    }

    if (blocks.empty())
    {
        blocks.push_back(Block());
        total_blocks = 1;
    }

    if (!blocks.back().canFitRecord())
    {
        blocks.push_back(Block());
        total_blocks++;
    }

    if (blocks.back().addRecord(record))
    {
        total_records++;
        // Maintain tombstone bitmap shape
        ensureDeletedBitmapInitialized_();
        return true;
    }
    return false;
}

void DatabaseFile::displayAllRecords() const
{
    std::cout << "\n=== All Game Records ===" << std::endl;
    int record_num = 0;
    for (const auto &block : blocks)
        for (int i = 0; i < block.record_count; i++)
        {
            std::cout << "Record #" << (++record_num) << ": ";
            block.getRecord(i).display();
        }
}

void DatabaseFile::displayStatistics() const
{
    std::cout << "\n=== Database Statistics ===" << std::endl;
    std::cout << "Size of a record: " << GameRecord::getRecordSize() << " bytes" << std::endl;
    std::cout << "Number of records: " << total_records << std::endl;
    std::cout << "Number of records per block: " << Block::getMaxRecordsPerBlock() << std::endl;
    std::cout << "Number of blocks: " << total_blocks << std::endl;
    std::cout << "Block size: " << Block::BLOCK_SIZE << " bytes" << std::endl;
    std::cout << "Total database size: "
              << (total_blocks * Block::BLOCK_SIZE + sizeof(size_t) * 2) << " bytes" << std::endl;
}

bool DatabaseFile::parseGameLine(const std::string &line, GameRecord &record)
{
    std::vector<std::string> fields = Utils::split(line, '\t');
    if (fields.size() < 9)
    {
        std::cerr << "Warning: Line has insufficient fields: " << line << std::endl;
        return false;
    }

    try
    {
        std::string date = Utils::trim(fields[0]);
        std::string team_id_str = Utils::trim(fields[1]);
        std::string pts_str = Utils::trim(fields[2]);
        std::string fg_pct_str = Utils::trim(fields[3]);
        std::string ft_pct_str = Utils::trim(fields[4]);
        std::string fg3_pct_str = Utils::trim(fields[5]);
        std::string ast_str = Utils::trim(fields[6]);
        std::string reb_str = Utils::trim(fields[7]);
        std::string wins_str = Utils::trim(fields[8]);

        // NEW: Check for empty fields before conversion
        if (Utils::isEmptyOrWhitespace(date) ||
            Utils::isEmptyOrWhitespace(team_id_str) ||
            Utils::isEmptyOrWhitespace(pts_str) ||
            Utils::isEmptyOrWhitespace(fg_pct_str) ||
            Utils::isEmptyOrWhitespace(ft_pct_str) ||
            Utils::isEmptyOrWhitespace(fg3_pct_str) ||
            Utils::isEmptyOrWhitespace(ast_str) ||
            Utils::isEmptyOrWhitespace(reb_str) ||
            Utils::isEmptyOrWhitespace(wins_str))
        {
            return false;  // Skip this record
        }

        int   team_id    = Utils::safeStringToInt(team_id_str);
        int   pts        = Utils::safeStringToInt(pts_str);
        float fg_pct     = Utils::safeStringToFloat(fg_pct_str);
        float ft_pct     = Utils::safeStringToFloat(ft_pct_str);
        float fg3_pct    = Utils::safeStringToFloat(fg3_pct_str);
        int   ast        = Utils::safeStringToInt(ast_str);
        int   reb        = Utils::safeStringToInt(reb_str);
        int   wins       = Utils::safeStringToInt(wins_str);

        record = GameRecord(date, team_id, pts, fg_pct, ft_pct, fg3_pct, ast, reb, wins);
        return true;
    }
    catch (...)
    {
        std::cerr << "Error parsing line: " << line << std::endl;
        return false;
    }
}

// NEW: Validate that a record has no empty/zero critical values
bool DatabaseFile::isRecordValid(const GameRecord &record) const
{
    // Check date is not empty
    if (std::strlen(record.game_date) == 0)
        return false;

    // Check team_id is valid (not 0)
    if (record.team_id_home == 0)
        return false;

    // You can add more validation rules as needed
    // For example, check if percentages are in valid range [0, 1]
    if (record.fg_pct_home < 0.0f || record.fg_pct_home > 1.0f)
        return false;
    if (record.ft_pct_home < 0.0f || record.ft_pct_home > 1.0f)
        return false;
    if (record.fg3_pct_home < 0.0f || record.fg3_pct_home > 1.0f)
        return false;

    // Check for negative values that shouldn't be negative
    if (record.pts_home < 0 || record.ast_home < 0 || record.reb_home < 0)
        return false;

    return true;
}

// ============================================
// Task 3 – Tombstones & Deletion implementations
// ============================================
void DatabaseFile::ensureDeletedBitmapInitialized_() {
    if (deleted_.size() == blocks.size()) return;
    deleted_.assign(blocks.size(), {});
    for (size_t b = 0; b < blocks.size(); ++b) {
        deleted_[b].assign(blocks[b].record_count, 0);
    }
}

bool DatabaseFile::isDeleted(size_t block_id, int record_id) const {
    if (block_id >= deleted_.size()) return false;
    if (record_id < 0 || record_id >= (int)deleted_[block_id].size()) return false;
    return deleted_[block_id][record_id] != 0;
}

void DatabaseFile::markDeleted(size_t block_id, int record_id) {
    if (block_id >= deleted_.size()) return;
    if (record_id < 0 || record_id >= (int)deleted_[block_id].size()) return;
    deleted_[block_id][record_id] = 1;
}

// Linear baseline: visit all blocks and tombstone FT% > thresh
DeletionStats DatabaseFile::deleteByFTAboveLinear(float thresh) {
    using clk = std::chrono::steady_clock;
    DeletionStats st{};
    auto t1 = clk::now();

    ensureDeletedBitmapInitialized_();
    st.nData = (uint32_t)blocks.size();

    for (size_t b = 0; b < blocks.size(); ++b) {
        const Block& blk = blocks[b];
        for (int r = 0; r < blk.record_count; ++r) {
            if (isDeleted(b, r)) continue;
            GameRecord rec = blk.getRecord(r);
            if (rec.ft_pct_home > thresh) {  // strict '>'
                markDeleted(b, r);
                st.nDeleted++;
                st.sumFT += (double)rec.ft_pct_home;
            }
        }
    }

    auto t2 = clk::now();
    st.timeUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    return st;
}

// Indexed deletion: FT% range scan (min_key, 1.0] → tombstone
DeletionStats DatabaseFile::deleteByFTAboveIndexed(float thresh) {
    using clk = std::chrono::steady_clock;
    DeletionStats st{};
    auto t1 = clk::now();

    ensureDeletedBitmapInitialized_();
    if (!index_manager) buildIndexes();

    const float min_k = std::nextafter(thresh, std::numeric_limits<float>::infinity());
    const float max_k = 1.0f;

    uint32_t vInt = 0, vLeaf = 0;
    auto locs = index_manager->searchByFTPercentageWithCounts(min_k, max_k, vInt, vLeaf);
    st.nInternal = vInt; st.nLeaf = vLeaf; st.nOverflow = 0;

    std::sort(locs.begin(), locs.end());
    locs.erase(std::unique(locs.begin(), locs.end()), locs.end());

    std::unordered_set<size_t> blocksTouched;
    for (auto &pr : locs) {
        const size_t b = (size_t)pr.first;
        const int    r = pr.second;
        if (b >= blocks.size()) continue;
        if (isDeleted(b, r)) continue;

        GameRecord rec = blocks[b].getRecord(r);
        if (rec.ft_pct_home > thresh) {  // belt-and-braces
            markDeleted(b, r);
            blocksTouched.insert(b);
            st.nDeleted++;
            st.sumFT += (double)rec.ft_pct_home;
        }
    }
    st.nData = (uint32_t)blocksTouched.size();

    auto t2 = clk::now();
    st.timeUs = std::chrono::duration_cast<std::chrono::microseconds>(t2 - t1).count();
    return st;
}

// Rebuild FT index skipping tombstoned rows
void DatabaseFile::rebuildFTIndexSkippingDeleted() {
    ensureDeletedBitmapInitialized_();
    if (!index_manager) index_manager = new IndexManager();
    index_manager->buildIndexesSkippingDeleted(*this);
}
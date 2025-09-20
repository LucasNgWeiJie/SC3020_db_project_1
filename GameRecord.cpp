#include "GameRecord.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <algorithm>

// GameRecord Implementation
GameRecord::GameRecord()
{
    memset(game_date, 0, sizeof(game_date));
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
    strncpy(game_date, date.c_str(), sizeof(game_date) - 1);
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

// Block Implementation
Block::Block()
{
    memset(data, 0, BLOCK_SIZE);
    used_space = 0;
    record_count = 0;
}

bool Block::addRecord(const GameRecord &record)
{
    size_t record_size = GameRecord::getRecordSize();
    if (used_space + record_size > BLOCK_SIZE)
    {
        return false; // Not enough space
    }

    // Copy record to block data
    memcpy(data + used_space, &record, record_size);
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
        memcpy(&record, data + (index * record_size), record_size);
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

// DatabaseFile Implementation
DatabaseFile::DatabaseFile(const std::string &db_filename)
    : filename(db_filename), total_records(0), total_blocks(0)
{
    index_manager = new IndexManager();
}

DatabaseFile::~DatabaseFile()
{
    if (file.is_open())
    {
        file.close();
    }
    delete index_manager;
}

// Build indexes for the database
bool DatabaseFile::buildIndexes()
{
    if (index_manager) {
        return index_manager->buildIndexes(*this);
    }
    return false;
}

// Search methods using team id
std::vector<GameRecord> DatabaseFile::searchByTeamId(int team_id)
{
    std::vector<GameRecord> results;
    if (!index_manager) return results;
    
    auto locations = index_manager->searchByTeamId(team_id);
    
    for (const auto& loc : locations) {
        if (static_cast<size_t>(loc.first) < blocks.size()) {  // Cast int to size_t
            GameRecord record = blocks[loc.first].getRecord(loc.second);
            results.push_back(record);
        }
    }
    
    return results;
}

// search methods using points range
std::vector<GameRecord> DatabaseFile::searchByPointsRange(int min_pts, int max_pts)
{
    std::vector<GameRecord> results;
    if (!index_manager) return results;
    
    auto locations = index_manager->searchByPointsRange(min_pts, max_pts);
    
    for (const auto& loc : locations) {
        if (static_cast<size_t>(loc.first) < blocks.size()) {  // Cast int to size_t
            GameRecord record = blocks[loc.first].getRecord(loc.second);
            results.push_back(record);
        }
    }
    
    return results;
}

// search methods using FG percentage range
std::vector<GameRecord> DatabaseFile::searchByFGPercentage(float min_pct, float max_pct)
{
    std::vector<GameRecord> results;
    if (!index_manager) return results;
    
    auto locations = index_manager->searchByFGPercentage(min_pct, max_pct);
    
    for (const auto& loc : locations) {
        if (static_cast<size_t>(loc.first) < blocks.size()) {  // Cast int to size_t
            GameRecord record = blocks[loc.first].getRecord(loc.second);
            results.push_back(record);
        }
    }
    
    return results;
}

// Display index statistics
void DatabaseFile::displayIndexStatistics() const
{
    if (index_manager) {
        index_manager->displayIndexStatistics();
    }
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

    // Initialize first block
    blocks.push_back(Block());
    total_blocks = 1;

    while (std::getline(input_file, line))
    {
        // Skip header line
        if (first_line)
        {
            first_line = false;
            continue;
        }

        GameRecord record;
        if (parseGameLine(line, record))
        {
            // Try to add record to current block
            if (!blocks.back().canFitRecord())
            {
                // Create new block if current is full
                blocks.push_back(Block());
                total_blocks++;
            }

            if (blocks.back().addRecord(record))
            {
                total_records++;
            }
            else
            {
                std::cerr << "Error: Could not add record to block" << std::endl;
            }
        }
    }

    input_file.close();
    std::cout << "Successfully loaded " << total_records << " records into "
              << total_blocks << " blocks." << std::endl;
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

    // Write header information
    file.write(reinterpret_cast<const char *>(&total_records), sizeof(total_records));
    file.write(reinterpret_cast<const char *>(&total_blocks), sizeof(total_blocks));

    // Write blocks
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

    // Read header information
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
    return true;
}

bool DatabaseFile::addRecord(const GameRecord &record)
{
    if (blocks.empty())
    {
        blocks.push_back(Block());
        total_blocks = 1;
    }

    // Try to add to last block
    if (!blocks.back().canFitRecord())
    {
        blocks.push_back(Block());
        total_blocks++;
    }

    if (blocks.back().addRecord(record))
    {
        total_records++;
        return true;
    }
    return false;
}

void DatabaseFile::displayAllRecords() const
{
    std::cout << "\n=== All Game Records ===" << std::endl;
    int record_num = 0;

    for (const auto &block : blocks)
    {
        for (int i = 0; i < block.record_count; i++)
        {
            GameRecord record = block.getRecord(i);
            std::cout << "Record #" << (++record_num) << ": ";
            record.display();
        }
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
    std::cout << "Total database size: " << (total_blocks * Block::BLOCK_SIZE + sizeof(size_t) * 2)
              << " bytes" << std::endl;
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
        // Parse each field
        std::string date = Utils::trim(fields[0]);
        int team_id = Utils::safeStringToInt(Utils::trim(fields[1]));
        int pts = Utils::safeStringToInt(Utils::trim(fields[2]));
        float fg_pct = Utils::safeStringToFloat(Utils::trim(fields[3]));
        float ft_pct = Utils::safeStringToFloat(Utils::trim(fields[4]));
        float fg3_pct = Utils::safeStringToFloat(Utils::trim(fields[5]));
        int ast = Utils::safeStringToInt(Utils::trim(fields[6]));
        int reb = Utils::safeStringToInt(Utils::trim(fields[7]));
        int wins = Utils::safeStringToInt(Utils::trim(fields[8]));

        record = GameRecord(date, team_id, pts, fg_pct, ft_pct, fg3_pct, ast, reb, wins);
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
        return false;
    }
}


// Utility Functions Implementation
namespace Utils
{
    std::string trim(const std::string &str)
    {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos)
            return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }

    float safeStringToFloat(const std::string &str)
    {
        try
        {
            return str.empty() ? 0.0f : std::stof(str);
        }
        catch (const std::exception &)
        {
            return 0.0f;
        }
    }

    int safeStringToInt(const std::string &str)
    {
        try
        {
            return str.empty() ? 0 : std::stoi(str);
        }
        catch (const std::exception &)
        {
            return 0;
        }
    }

    std::vector<std::string> split(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;

        while (std::getline(ss, token, delimiter))
        {
            tokens.push_back(token);
        }

        return tokens;
    }
}
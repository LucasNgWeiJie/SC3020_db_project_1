#include "GameRecord.h"
#include <iostream>

int main()
{
    std::cout << "NBA Games Database Management System" << std::endl;
    std::cout << "====================================" << std::endl;

    // Create database file manager
    DatabaseFile db("nba_games.db");

    // Load data from games.txt
    std::cout << "\n1. Loading data from games.txt..." << std::endl;
    if (!db.loadFromTextFile("games.txt"))
    {
        std::cerr << "Failed to load data from games.txt" << std::endl;
        return 1;
    }

    // Display statistics
    db.displayStatistics();

    // Write data to disk in binary format
    std::cout << "\n2. Writing database to disk..." << std::endl;
    if (!db.writeBlocksToDisk())
    {
        std::cerr << "Failed to write database to disk" << std::endl;
        return 1;
    }

    //Build indexes
    std::cout << "\n3. Building indexes..." << std::endl;
    if (!db.buildIndexes())
    {
        std::cerr << "Failed to build indexes" << std::endl;
        return 1;
    }
    // Display index statistics
    db.displayIndexStatistics();

    // Demonstrate index searches
    std::cout << "\n4. Index-based searches:" << std::endl;
    
    // Search by team ID
    std::cout << "\nSearching for team ID 1610612744:" << std::endl;
    auto team_results = db.searchByTeamId(1610612744);
    std::cout << "Found " << team_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), team_results.size()); i++) {
        team_results[i].display();
    }

    // Search by points range
    std::cout << "\nSearching for games with 110-120 points:" << std::endl;
    auto points_results = db.searchByPointsRange(110, 120);
    std::cout << "Found " << points_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), points_results.size()); i++) {
        points_results[i].display();
    }

    // Search by FG percentage range
    std::cout << "\nSearching for games with FG% between 0.5 and 0.6:" << std::endl;
    auto fg_results = db.searchByFGPercentage(0.5f, 0.6f);
    std::cout << "Found " << fg_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), fg_results.size()); i++) {
        fg_results[i].display();
    }

    // Search by FT percentage range
    std::cout << "\nSearching for games with FT% between 0.9 and 1.0:" << std::endl;
    auto ft_results = db.searchByFTPercentage(0.9f, 1.0f);
    std::cout << "Found " << ft_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min(size_t(5), ft_results.size()); i++) {
        ft_results[i].display();
    }

    std::cout << "\nDatabase operations completed successfully!" << std::endl;
    return 0;

    // Demonstrate reading from disk
    std::cout << "\n3. Testing disk read functionality..." << std::endl;
    DatabaseFile db2("nba_games.db");
    if (db2.readBlocksFromDisk())
    {
        std::cout << "Successfully read database from disk!" << std::endl;
        db2.displayStatistics();
    }

    std::cout << "\nDatabase operations completed successfully!" << std::endl;
    return 0;
}

// Alternative main function for testing individual components
void testComponents()
{
    std::cout << "=== Component Testing ===" << std::endl;

    // Test GameRecord
    std::cout << "\nTesting GameRecord:" << std::endl;
    GameRecord game("2023-01-15", 1610612747, 112, 0.456f, 0.789f, 0.367f, 25, 45, 1);
    game.display();
    std::cout << "Record size: " << GameRecord::getRecordSize() << " bytes" << std::endl;

    // Test Block
    std::cout << "\nTesting Block:" << std::endl;
    Block block;
    std::cout << "Max records per block: " << Block::getMaxRecordsPerBlock() << std::endl;
    std::cout << "Block size: " << Block::BLOCK_SIZE << " bytes" << std::endl;

    // Add some records to block
    for (int i = 0; i < 5; i++)
    {
        GameRecord test_game("2023-01-" + std::to_string(15 + i),
                             1610612747 + i, 100 + i, 0.45f + i * 0.01f,
                             0.78f + i * 0.01f, 0.36f + i * 0.01f,
                             20 + i, 40 + i, i % 2);
        if (block.addRecord(test_game))
        {
            std::cout << "Added record " << i + 1 << " to block" << std::endl;
        }
    }

    // Retrieve and display records
    std::cout << "\nRecords in block:" << std::endl;
    for (int i = 0; i < 5; i++)
    {
        GameRecord retrieved = block.getRecord(i);
        retrieved.display();
    }
}

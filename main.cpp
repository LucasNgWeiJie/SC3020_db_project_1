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

    // Demonstrate reading from disk
    std::cout << "\n3. Testing disk read functionality..." << std::endl;
    DatabaseFile db2("nba_games.db");
    if (db2.readBlocksFromDisk())
    {
        std::cout << "Successfully read database from disk!" << std::endl;
        db2.displayStatistics();
    }

    // Display first 10 records as sample
    std::cout << "\n4. Sample records (first 10):" << std::endl;
    int count = 0;
    for (size_t block_idx = 0; block_idx < db.getTotalBlocks() && count < 10; block_idx++)
    {
        // This would require additional methods to access blocks,
        // but demonstrates the concept
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
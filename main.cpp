#include "GameRecord.h"
#include <iostream>
#include <iomanip>

int main()
{
    std::cout << "NBA Games Database Management System" << std::endl;
    std::cout << "====================================" << std::endl;

    // Create database file manager
    DatabaseFile db("nba_games.db");

    // 1) Load data
    std::cout << "\n1. Loading data from games.txt..." << std::endl;
    if (!db.loadFromTextFile("games.txt"))
    {
        std::cerr << "Failed to load data from games.txt" << std::endl;
        return 1;
    }

    // Show base stats
    db.displayStatistics();

    // 2) Write to disk
    std::cout << "\n2. Writing database to disk..." << std::endl;
    if (!db.writeBlocksToDisk())
    {
        std::cerr << "Failed to write database to disk" << std::endl;
        return 1;
    }

    // 3) Build indexes
    std::cout << "\n3. Building indexes..." << std::endl;
    if (!db.buildIndexes())
    {
        std::cerr << "Failed to build indexes" << std::endl;
        return 1;
    }

    // Show index stats
    db.displayIndexStatistics();

    // 4) Demo index searches (existing behavior)
    std::cout << "\n4. Index-based searches:" << std::endl;

    std::cout << "\nSearching for team ID 1610612744:" << std::endl;
    auto team_results = db.searchByTeamId(1610612744);
    std::cout << "Found " << team_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(5, team_results.size()); i++) {
        team_results[i].display();
    }

    std::cout << "\nSearching for games with 110-120 points:" << std::endl;
    auto points_results = db.searchByPointsRange(110, 120);
    std::cout << "Found " << points_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(5, points_results.size()); i++) {
        points_results[i].display();
    }

    std::cout << "\nSearching for games with FG% between 0.5 and 0.6:" << std::endl;
    auto fg_results = db.searchByFGPercentage(0.5f, 0.6f);
    std::cout << "Found " << fg_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(5, fg_results.size()); i++) {
        fg_results[i].display();
    }

    std::cout << "\nSearching for games with FT% between 0.9 and 1.0:" << std::endl;
    auto ft_results = db.searchByFTPercentage(0.9f, 1.0f);
    std::cout << "Found " << ft_results.size() << " records" << std::endl;
    for (size_t i = 0; i < std::min<size_t>(5, ft_results.size()); i++) {
        ft_results[i].display();
    }

    // ==================== Task 3: Delete FT_PCT_home > 0.9 ====================
    // Run on fresh DB objects so Tasks 1/2 results remain unchanged.
    {
        // ---- Linear baseline ----
        DatabaseFile db_linear("nba_games_linear.db");
        db_linear.loadFromTextFile("games.txt");
        DeletionStats sLin = db_linear.deleteByFTAboveLinear(0.9f);
        const double avgLin = sLin.nDeleted ? (sLin.sumFT / sLin.nDeleted) : 0.0;

        // ---- Indexed path ----
        DatabaseFile db_indexed("nba_games_indexed.db");
        db_indexed.loadFromTextFile("games.txt");
        db_indexed.buildIndexes();
        DeletionStats sIdx = db_indexed.deleteByFTAboveIndexed(0.9f);
        const double avgIdx = sIdx.nDeleted ? (sIdx.sumFT / sIdx.nDeleted) : 0.0;

        std::cout << "\n=== Task 3 — Delete FT_PCT_home > 0.9 ===\n";

        std::cout << "\n> Linear Deletion\n";
        std::cout << "Data blocks accessed: " << sLin.nData << "\n";
        std::cout << "Records deleted: "     << sLin.nDeleted << "\n";
        std::cout << "Average FT%: "         << std::fixed << std::setprecision(3) << avgLin << "\n";
        std::cout << "Time: "                << (sLin.timeUs / 1000.0) << " ms\n";

        std::cout << "\n> Indexed Deletion\n";
        std::cout << "Index blocks accessed: "
                  << sIdx.nInternal << " internal, "
                  << sIdx.nLeaf     << " leaf, "
                  << sIdx.nOverflow << " overflow (total "
                  << (sIdx.nInternal + sIdx.nLeaf + sIdx.nOverflow) << ")\n";
        std::cout << "Data blocks accessed: " << sIdx.nData << "\n";
        std::cout << "Records deleted: "     << sIdx.nDeleted << "\n";
        std::cout << "Average FT%: "         << std::fixed << std::setprecision(3) << avgIdx << "\n";
        std::cout << "Time: "                << (sIdx.timeUs / 1000.0) << " ms\n";

        // Rebuild FT index on the indexed copy (skip tombstoned) and show structure
        db_indexed.rebuildFTIndexSkippingDeleted();
        std::cout << "\n> FT Index structure after deletion\n";
        db_indexed.displayIndexStatistics();
    }
    // ================== end Task 3 additions ====================

    std::cout << "\nDatabase operations completed successfully!" << std::endl;
    return 0;
}

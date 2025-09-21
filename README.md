# NBA Games Database Management System

This project implements a database management system with storage and indexing components specifically designed for NBA game data.

## Project Structure

- `GameRecord.h` - Header file containing all class and structure definitions
- `GameRecord.cpp` - Implementation file with all functionality
- `IndexManager.cpp` - B+ Tree indexing implementation
- `main.cpp` - Main program demonstrating the system
- `games.txt` - Input data file (tab-separated values)
- `nba_games.db` - Binary database file (generated after running)
- `.gitignore` - Git ignore file (excludes compiled binaries and generated files)
- `README.md` - This documentation file

## Generated files (not tracked):
- `nba_games.db` - Binary database file (generated after running)
- `nbadb` / `nbadb.exe` - Compiled executable

## Features

### Storage Component

- **Disk-based storage**: Data is stored in a binary file format
- **Block organization**: Data is organized into 4KB blocks
- **Record structure**: Fixed-size records for NBA game data
- **File simulation**: Uses binary files to simulate disk storage

### Data Structure

Each record contains:

- `GAME_DATE_EST` - Game date (YYYY-MM-DD format)
- `TEAM_ID_home` - Home team ID (integer)
- `PTS_home` - Home team points (integer)
- `FG_PCT_home` - Field goal percentage (float)
- `FT_PCT_home` - Free throw percentage (float)
- `FG3_PCT_home` - 3-point field goal percentage (float)
- `AST_home` - Home team assists (integer)
- `REB_home` - Home team rebounds (integer)
- `HOME_TEAM_WINS` - 1 if home team wins, 0 otherwise (integer)

## Additional Indexes / B+ Trees

In addition to the required FT_PCT_home B+ tree, we also built indexes for:
- FG_PCT_home
- TEAM_ID_home
- PTS_home

These indexes were implemented to demonstrate that our B+ tree component works across different attribute types.


## Compilation and Usage

### Prerequisites (Windows)

- C++ compiler with C++11 support (MinGW-w64 recommended)
- Windows PowerShell or Command Prompt

### Installing C++ Compiler (if needed for windows)

1. Download MinGW-w64 from: https://www.mingw-w64.org/downloads/
2. Install and add to your system PATH
3. Verify installation: `g++ --version`

### Building the Project

```powershell
# Compile all files together (Windows)
g++ -std=c++14 -Wall -Wextra -g -O2 main.cpp GameRecord.cpp IndexManager.cpp -o nba_db.exe

# Compile all files together (MacOs)
g++ -std=c++14 -Wall -Wextra -g -O2 main.cpp GameRecord.cpp IndexManager.cpp -o nba_db
```

### Running the Program

```powershell
# Run the compiled executable (Windows)
.\nba_db.exe

# Run the compiled executable (MacOs)
./nba_db
```



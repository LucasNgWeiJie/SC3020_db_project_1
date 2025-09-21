#include "GameRecord.h"
#include <algorithm>
#include <queue>
#include <cstring>

// B+ Tree Node Implementation
template<typename KeyType>
BPlusTreeNode<KeyType>::BPlusTreeNode(bool leaf) : is_leaf(leaf), key_count(0)
{
    // Initialize keys array
    for (int i = 0; i < MAX_KEYS; i++) {
        keys[i] = KeyType{};
    }
    
    if (is_leaf) {
        leaf_data.next_leaf = nullptr;
        for (int i = 0; i < MAX_KEYS; i++) {
            leaf_data.block_ids[i] = -1;
            leaf_data.record_ids[i] = -1;
        }
    } else {
        for (int i = 0; i <= MAX_KEYS; i++) {
            children[i] = nullptr;
        }
    }
}

template<typename KeyType>
BPlusTreeNode<KeyType>::~BPlusTreeNode()
{
    if (!is_leaf) {
        for (int i = 0; i <= key_count; i++) {
            delete children[i];
        }
    }
}

// IndexManager Implementation
IndexManager::IndexManager() 
    : team_id_index(nullptr), points_index(nullptr), 
      fg_pct_index(nullptr), date_index(nullptr), ft_pct_index(nullptr)
{
}

IndexManager::~IndexManager()
{
    delete team_id_index;
    delete points_index;
    delete fg_pct_index;
    delete date_index;
    delete ft_pct_index;
}

bool IndexManager::buildIndexes(const DatabaseFile& db)
{
    std::cout << "Building B+ tree indexes with max 20 keys per node..." << std::endl;
    
    // Initialize index roots
    team_id_index = new BPlusTreeNode<int>(true);
    points_index = new BPlusTreeNode<int>(true);
    fg_pct_index = new BPlusTreeNode<float>(true);
    date_index = new BPlusTreeNode<std::string>(true);
    ft_pct_index = new BPlusTreeNode<float>(true);
    
    // Insert all records into indexes
    for (size_t block_idx = 0; block_idx < db.getTotalBlocks(); block_idx++) {
        const Block& block = db.getBlock(block_idx);
        
        for (int record_idx = 0; record_idx < block.record_count; record_idx++) {
            GameRecord record = block.getRecord(record_idx);
            
            // Insert into all indexes with proper splitting
            insert(team_id_index, record.team_id_home, block_idx, record_idx);
            insert(points_index, record.pts_home, block_idx, record_idx);
            insert(fg_pct_index, record.fg_pct_home, block_idx, record_idx);
            insert(date_index, std::string(record.game_date), block_idx, record_idx);
            insert(ft_pct_index, record.ft_pct_home, block_idx, record_idx);
        }
    }
    
    std::cout << "B+ tree indexes built successfully with node splitting!" << std::endl;
    return true;
}

// Main insert function with automatic node splitting
template<typename KeyType>
bool IndexManager::insert(BPlusTreeNode<KeyType>*& root, KeyType key, int block_id, int record_id)
{
    using NodeType = BPlusTreeNode<KeyType>;
    static const int MAX_KEYS = NodeType::MAX_KEYS;

    if (!root) {
        root = new BPlusTreeNode<KeyType>(true);
    }
    
    if (root->is_leaf) {
        // Try to insert directly into leaf
        if (root->key_count < MAX_KEYS) {
            return insertIntoLeaf(root, key, block_id, record_id);
        } else {
            // Leaf is full, need to split

            // For debugging:
            // std::cout << "Splitting leaf node (key count: " << root->key_count << ")" << std::endl;
            
            // Split first, then insert
            auto split_result = splitLeaf(root);
            KeyType promoted_key = split_result.first;
            BPlusTreeNode<KeyType>* new_leaf = split_result.second;
            
            if (!new_leaf) {
                return false;  // Split failed
            }
            
            // Decide which leaf to insert into
            if (key < promoted_key) {
                insertIntoLeaf(root, key, block_id, record_id);
            } else {
                insertIntoLeaf(new_leaf, key, block_id, record_id);
            }
            
            // Create new root
            BPlusTreeNode<KeyType>* new_root = new BPlusTreeNode<KeyType>(false);
            new_root->keys[0] = promoted_key;
            new_root->children[0] = root;
            new_root->children[1] = new_leaf;
            new_root->key_count = 1;
            
            root = new_root;

            // For debugging:
            // std::cout << "Created new root after leaf split" << std::endl;
            
            return true;
        }
    } else {
        // Internal node - find appropriate child
        int pos = 0;
        while (pos < root->key_count && pos < MAX_KEYS && key > root->keys[pos]) {
            pos++;
        }
        
        // Ensure we don't go out of bounds
        if (pos > MAX_KEYS) pos = MAX_KEYS;
        
        // Recursively insert into child
        if (root->children[pos]) {
            return insert(root->children[pos], key, block_id, record_id);
        } else {
            // Create new child if it doesn't exist
            root->children[pos] = new BPlusTreeNode<KeyType>(true);
            return insert(root->children[pos], key, block_id, record_id);
        }
    }
}

// Insert into a leaf node (assumes node has space)
template<typename KeyType>
bool IndexManager::insertIntoLeaf(BPlusTreeNode<KeyType>* leaf, KeyType key, int block_id, int record_id)
{
    static const int MAX_KEYS = BPlusTreeNode<KeyType>::MAX_KEYS;

    if (!leaf || leaf->key_count >= MAX_KEYS) {
        return false;  // Safety check
    }
    
    // Find insertion position
    int pos = 0;
    while (pos < leaf->key_count && leaf->keys[pos] < key) {
        pos++;
    }
    
    // Shift elements to make room
    for (int i = leaf->key_count; i > pos; i--) {
        leaf->keys[i] = leaf->keys[i-1];
        leaf->leaf_data.block_ids[i] = leaf->leaf_data.block_ids[i-1];
        leaf->leaf_data.record_ids[i] = leaf->leaf_data.record_ids[i-1];
    }
    
    // Insert new key-value pair
    leaf->keys[pos] = key;
    leaf->leaf_data.block_ids[pos] = block_id;
    leaf->leaf_data.record_ids[pos] = record_id;
    leaf->key_count++;
    
    return true;
}

// Split a full leaf node
template<typename KeyType>
std::pair<KeyType, BPlusTreeNode<KeyType>*> IndexManager::splitLeaf(BPlusTreeNode<KeyType>* leaf)
{
    static const int MAX_KEYS = BPlusTreeNode<KeyType>::MAX_KEYS;

    if (!leaf) {
        return std::make_pair(KeyType{}, nullptr);
    }
    
    // Create new leaf node
    BPlusTreeNode<KeyType>* new_leaf = new BPlusTreeNode<KeyType>(true);
    
    // Calculate split point (middle)
    int split_point = leaf->key_count / 2;
    
    // Copy second half to new leaf
    int j = 0;
    for (int i = split_point; i < leaf->key_count && i < MAX_KEYS && j < MAX_KEYS; i++, j++) {
        new_leaf->keys[j] = leaf->keys[i];
        new_leaf->leaf_data.block_ids[j] = leaf->leaf_data.block_ids[i];
        new_leaf->leaf_data.record_ids[j] = leaf->leaf_data.record_ids[i];
        
        // Clear from original leaf
        leaf->keys[i] = KeyType{};
        leaf->leaf_data.block_ids[i] = -1;
        leaf->leaf_data.record_ids[i] = -1;
    }
    new_leaf->key_count = j;
    
    // Update original leaf
    leaf->key_count = split_point;
    
    // Link leaves together
    new_leaf->leaf_data.next_leaf = leaf->leaf_data.next_leaf;
    leaf->leaf_data.next_leaf = new_leaf;
    
    // Return the first key of the new leaf for promotion
    KeyType promoted_key = (new_leaf->key_count > 0) ? new_leaf->keys[0] : KeyType{};
    
    return std::make_pair(promoted_key, new_leaf);
}

// Search functions remain the same
template<typename KeyType>
std::vector<std::pair<int, int>> IndexManager::search(BPlusTreeNode<KeyType>* root, KeyType key)
{
    std::vector<std::pair<int, int>> results;
    
    if (!root) return results;
    
    if (root->is_leaf) {
        // Search in leaf node
        for (int i = 0; i < root->key_count; i++) {
            if (root->keys[i] == key) {
                results.push_back({root->leaf_data.block_ids[i], root->leaf_data.record_ids[i]});
            }
        }
    } else {
        // Navigate to appropriate child
        int pos = 0;
        while (pos < root->key_count && key > root->keys[pos]) {
            pos++;
        }
        return search(root->children[pos], key);
    }
    
    return results;
}

template<typename KeyType>
std::vector<std::pair<int, int>> IndexManager::rangeSearch(BPlusTreeNode<KeyType>* root, KeyType min_key, KeyType max_key)
{
    std::vector<std::pair<int, int>> results;
    
    if (!root) return results;
    
    if (root->is_leaf) {
        // Search in leaf node for range
        for (int i = 0; i < root->key_count; i++) {
            if (root->keys[i] >= min_key && root->keys[i] <= max_key) {
                results.push_back({root->leaf_data.block_ids[i], root->leaf_data.record_ids[i]});
            }
        }
        
        // Continue with next leaf if exists
        if (root->leaf_data.next_leaf) {
            auto next_results = rangeSearch(root->leaf_data.next_leaf, min_key, max_key);
            results.insert(results.end(), next_results.begin(), next_results.end());
        }
    } else {
        // Search all relevant children
        for (int i = 0; i <= root->key_count; i++) {
            if (root->children[i]) {
                if (i == 0 || min_key <= root->keys[i-1]) {
                    if (i == root->key_count || max_key >= root->keys[i]) {
                        auto child_results = rangeSearch(root->children[i], min_key, max_key);
                        results.insert(results.end(), child_results.begin(), child_results.end());
                    }
                }
            }
        }
    }
    
    return results;
}

// Statistics functions
template<typename KeyType>
int IndexManager::countNodes(BPlusTreeNode<KeyType>* root) const
{
    if (!root) return 0;
    
    int count = 1;
    if (!root->is_leaf) {
        for (int i = 0; i <= root->key_count; i++) {
            if (root->children[i]) {
                count += countNodes(root->children[i]);
            }
        }
    }
    return count;
}

template<typename KeyType>
int IndexManager::countLeafNodes(BPlusTreeNode<KeyType>* root) const
{
    if (!root) return 0;
    if (root->is_leaf) return 1;
    
    int count = 0;
    for (int i = 0; i <= root->key_count; i++) {
        if (root->children[i]) {
            count += countLeafNodes(root->children[i]);
        }
    }
    return count;
}

template<typename KeyType>
int IndexManager::getTreeHeight(BPlusTreeNode<KeyType>* root) const
{
    if (!root) return 0;
    if (root->is_leaf) return 1;
    
    int max_height = 0;
    for (int i = 0; i <= root->key_count; i++) {
        if (root->children[i]) {
            max_height = std::max(max_height, getTreeHeight(root->children[i]));
        }
    }
    return max_height + 1;
}

template<typename KeyType>
int IndexManager::getTotalKeys(BPlusTreeNode<KeyType>* root) const
{
    if (!root) return 0;
    
    int count = root->key_count;
    if (!root->is_leaf) {
        for (int i = 0; i <= root->key_count; i++) {
            if (root->children[i]) {
                count += getTotalKeys(root->children[i]);
            }
        }
    }
    return count;
}

// Public interface methods
std::vector<std::pair<int, int>> IndexManager::searchByTeamId(int team_id)
{
    return search(team_id_index, team_id);
}

std::vector<std::pair<int, int>> IndexManager::searchByPointsRange(int min_pts, int max_pts)
{
    return rangeSearch(points_index, min_pts, max_pts);
}

std::vector<std::pair<int, int>> IndexManager::searchByFGPercentage(float min_pct, float max_pct)
{
    return rangeSearch(fg_pct_index, min_pct, max_pct);
}

std::vector<std::pair<int, int>> IndexManager::searchByDate(const std::string& date)
{
    return search(date_index, date);
}

std::vector<std::pair<int, int>> IndexManager::searchByFTPercentage(float min_pct, float max_pct)
{
    return rangeSearch(ft_pct_index, min_pct, max_pct);
}

void IndexManager::displayIndexStatistics() const
{
    std::cout << "\n=== B+ Tree Index Statistics (Max 20 keys per node) ===" << std::endl;
    
    displaySingleIndexStats("Team ID", team_id_index);
    displaySingleIndexStats("Points", points_index);
    displaySingleIndexStats("FG Percentage", fg_pct_index);
    displaySingleIndexStats("Date", date_index);
    displaySingleIndexStats("FT Percentage", ft_pct_index);
    
    int total_nodes = countNodes(team_id_index) + countNodes(points_index) + countNodes(fg_pct_index) + countNodes(date_index) + countNodes(ft_pct_index);
    
    std::cout << "\nOverall Index Statistics:" << std::endl;
    std::cout << "Total index nodes: " << total_nodes << std::endl;
    std::cout << "Memory usage estimate: " << (total_nodes * sizeof(BPlusTreeNode<int>)) << " bytes" << std::endl;
}

template<typename KeyType>
void IndexManager::displaySingleIndexStats(const std::string& index_name, BPlusTreeNode<KeyType>* root) const
{
    if (!root) {
        std::cout << index_name << " Index: Not initialized" << std::endl;
        return;
    }
    
    int total_nodes = countNodes(root);
    int leaf_nodes = countLeafNodes(root);
    int internal_nodes = total_nodes - leaf_nodes;
    int tree_height = getTreeHeight(root);
    int total_keys = getTotalKeys(root);
    
    std::cout << "\n" << index_name << " Index:" << std::endl;
    std::cout << "  - Total nodes: " << total_nodes << std::endl;
    std::cout << "  - Leaf nodes: " << leaf_nodes << std::endl;
    std::cout << "  - Internal nodes: " << internal_nodes << std::endl;
    std::cout << "  - Tree height: " << tree_height << std::endl;
    std::cout << "  - Total keys: " << total_keys << std::endl;
    std::cout << "  - Max keys per node: " << BPlusTreeNode<KeyType>::MAX_KEYS << std::endl;
    
    if (leaf_nodes > 0) {
        std::cout << "  - Avg keys per leaf: " << (total_keys / leaf_nodes) << std::endl;
    }
}

// Template instantiations
template struct BPlusTreeNode<int>;
template struct BPlusTreeNode<float>;
template struct BPlusTreeNode<std::string>;


// Below is the implementation without node splitting for reference :)

// #include "GameRecord.h"
// #include <algorithm>
// #include <queue>

// // B+ Tree Node Implementation
// template<typename KeyType>
// BPlusTreeNode<KeyType>::BPlusTreeNode(bool leaf) : is_leaf(leaf), key_count(0)
// {
//     if (is_leaf) {
//         // Initialize leaf data - vectors start empty
//         new (&leaf_data.block_ids) std::vector<int>();
//         new (&leaf_data.record_ids) std::vector<int>();
//         leaf_data.next_leaf = nullptr;
//     } else {
//         // Initialize children vector
//         new (&children) std::vector<BPlusTreeNode<KeyType>*>();
//     }
// }

// template<typename KeyType>
// BPlusTreeNode<KeyType>::~BPlusTreeNode()
// {
//     if (!is_leaf) {
//         // Clean up children
//         for (auto child : children) {
//             delete child;
//         }
//         children.~vector();  // Explicitly destroy vector
//     } else {
//         // Clean up leaf data vectors
//         leaf_data.block_ids.~vector();
//         leaf_data.record_ids.~vector();
//     }
// }

// // IndexManager Implementation
// IndexManager::IndexManager() 
//     : team_id_index(nullptr), points_index(nullptr), 
//       fg_pct_index(nullptr), date_index(nullptr)
// {
// }

// IndexManager::~IndexManager()
// {
//     delete team_id_index;
//     delete points_index;
//     delete fg_pct_index;
//     delete date_index;
// }

// // Build indexes from the entire database
// bool IndexManager::buildIndexes(const DatabaseFile& db)
// {
//     std::cout << "Building indexes..." << std::endl;
    
//     // Initialize index roots
//     team_id_index = new BPlusTreeNode<int>(true);
//     points_index = new BPlusTreeNode<int>(true);
//     fg_pct_index = new BPlusTreeNode<float>(true);
//     date_index = new BPlusTreeNode<std::string>(true);
    
//     // Iterate through all blocks and records to build indexes
//     for (size_t block_idx = 0; block_idx < db.getTotalBlocks(); block_idx++) {
//         const Block& block = db.getBlock(block_idx); // You'll need to add this method
        
//         for (int record_idx = 0; record_idx < block.record_count; record_idx++) {
//             GameRecord record = block.getRecord(record_idx);
            
//             // Insert into all indexes
//             insert(team_id_index, record.team_id_home, block_idx, record_idx);
//             insert(points_index, record.pts_home, block_idx, record_idx);
//             insert(fg_pct_index, record.fg_pct_home, block_idx, record_idx);
//             insert(date_index, std::string(record.game_date), block_idx, record_idx);
//         }
//     }
    
//     std::cout << "Indexes built successfully!" << std::endl;
//     return true;
// }

// std::vector<std::pair<int, int>> IndexManager::searchByTeamId(int team_id)
// {
//     return search(team_id_index, team_id);
// }

// std::vector<std::pair<int, int>> IndexManager::searchByPointsRange(int min_pts, int max_pts)
// {
//     return rangeSearch(points_index, min_pts, max_pts);
// }

// std::vector<std::pair<int, int>> IndexManager::searchByFGPercentage(float min_pct, float max_pct)
// {
//     return rangeSearch(fg_pct_index, min_pct, max_pct);
// }

// std::vector<std::pair<int, int>> IndexManager::searchByDate(const std::string& date)
// {
//     return search(date_index, date);
// }

// // Insert a record into all indexes
// template<typename KeyType>
// bool IndexManager::insert(BPlusTreeNode<KeyType>*& root, KeyType key, int block_id, int record_id)
// {
//     if (root->is_leaf) {
//         // Find insertion position
//         int pos = 0;
//         while (pos < root->key_count && root->keys[pos] < key) {
//             pos++;
//         }
        
//         // Insert into vectors (they automatically resize)
//         root->keys.insert(root->keys.begin() + pos, key);
//         root->leaf_data.block_ids.insert(root->leaf_data.block_ids.begin() + pos, block_id);
//         root->leaf_data.record_ids.insert(root->leaf_data.record_ids.begin() + pos, record_id);
        
//         root->key_count++;
//         return true;
        
//     } else {
//         // Find child to insert into
//         int pos = 0;
//         while (pos < root->key_count && key > root->keys[pos]) {
//             pos++;
//         }
        
//         // Ensure children vector is large enough
//         if (pos >= static_cast<int>(root->children.size())) {
//             root->children.resize(pos + 1, nullptr);
//             if (!root->children[pos]) {
//                 root->children[pos] = new BPlusTreeNode<KeyType>(true);
//             }
//         }
        
//         return insert(root->children[pos], key, block_id, record_id);
//     }
// }

// // Exact search for a specific key
// template<typename KeyType>
// std::vector<std::pair<int, int>> IndexManager::search(BPlusTreeNode<KeyType>* root, KeyType key)
// {
//     std::vector<std::pair<int, int>> results;
    
//     if (!root) return results;
    
//     if (root->is_leaf) {
//         // Search in leaf node
//         for (int i = 0; i < root->key_count; i++) {
//             if (i < static_cast<int>(root->keys.size()) && root->keys[i] == key) {
//                 results.push_back({root->leaf_data.block_ids[i], root->leaf_data.record_ids[i]});
//             }
//         }
//     } else {
//         // Navigate to appropriate child
//         int pos = 0;
//         while (pos < root->key_count && pos < static_cast<int>(root->keys.size()) && key > root->keys[pos]) {
//             pos++;
//         }
        
//         if (pos < static_cast<int>(root->children.size()) && root->children[pos]) {
//             return search(root->children[pos], key);
//         }
//     }
    
//     return results;
// }

// // Range search for keys between min_key and max_key
// template<typename KeyType>
// std::vector<std::pair<int, int>> IndexManager::rangeSearch(BPlusTreeNode<KeyType>* root, KeyType min_key, KeyType max_key)
// {
//     std::vector<std::pair<int, int>> results;
    
//     if (!root) return results;
    
//     if (root->is_leaf) {
//         // Search in leaf node for range
//         for (int i = 0; i < root->key_count; i++) {
//             if (root->keys[i] >= min_key && root->keys[i] <= max_key) {
//                 results.push_back({root->leaf_data.block_ids[i], root->leaf_data.record_ids[i]});
//             }
//         }
        
//         // Continue with next leaf if exists
//         if (root->leaf_data.next_leaf) {
//             auto next_results = rangeSearch(root->leaf_data.next_leaf, min_key, max_key);
//             results.insert(results.end(), next_results.begin(), next_results.end());
//         }
//     } else {
//         // Search all relevant children
//         for (int i = 0; i <= root->key_count; i++) {
//             if (i == 0 || min_key <= root->keys[i-1]) {
//                 if (i == root->key_count || max_key >= root->keys[i]) {
//                     auto child_results = rangeSearch(root->children[i], min_key, max_key);
//                     results.insert(results.end(), child_results.begin(), child_results.end());
//                 }
//             }
//         }
//     }
    
//     return results;
// }

// // Count the total number of nodes in the B+ tree
// template<typename KeyType>
// int IndexManager::countNodes(BPlusTreeNode<KeyType>* root) const
// {
//     if (!root) return 0;
    
//     int count = 1;
//     if (!root->is_leaf) {
//         for (size_t i = 0; i < root->children.size(); i++) {
//             if (root->children[i]) {
//                 count += countNodes(root->children[i]);
//             }
//         }
//     }
//     return count;
// }

// // Display index statistics
// void IndexManager::displayIndexStatistics() const
// {
//     std::cout << "\n=== Index Statistics ===" << std::endl;
    
//     // Enhanced statistics for each index
//     displaySingleIndexStats("Team ID", team_id_index);
//     displaySingleIndexStats("Points", points_index);
//     displaySingleIndexStats("FG Percentage", fg_pct_index);
//     displaySingleIndexStats("Date", date_index);
    
//     // Overall statistics
//     int total_nodes = countNodes(team_id_index) + countNodes(points_index) + 
//                       countNodes(fg_pct_index) + countNodes(date_index);
    
//     std::cout << "\nOverall Index Statistics:" << std::endl;
//     std::cout << "Total index nodes: " << total_nodes << std::endl;
//     std::cout << "Memory usage estimate: " << (total_nodes * sizeof(BPlusTreeNode<int>)) << " bytes" << std::endl;
// }

// template<typename KeyType>
// void IndexManager::displaySingleIndexStats(const std::string& index_name, 
//                                           BPlusTreeNode<KeyType>* root) const
// {
//     if (!root) {
//         std::cout << index_name << " Index: Not initialized" << std::endl;
//         return;
//     }
    
//     int total_nodes = countNodes(root);
//     int leaf_nodes = countLeafNodes(root);
//     int internal_nodes = total_nodes - leaf_nodes;
//     int tree_height = getTreeHeight(root);
//     int total_keys = getTotalKeys(root);
    
//     std::cout << "\n" << index_name << " Index:" << std::endl;
//     std::cout << "  - Total nodes: " << total_nodes << std::endl;
//     std::cout << "  - Leaf nodes: " << leaf_nodes << std::endl;
//     std::cout << "  - Internal nodes: " << internal_nodes << std::endl;
//     std::cout << "  - Tree height: " << tree_height << std::endl;
//     std::cout << "  - Total keys: " << total_keys << std::endl;
    
//     if (leaf_nodes > 0) {
//         std::cout << "  - Avg keys per leaf: " << (total_keys / leaf_nodes) << std::endl;
//     }
// }

// // Helper functions you'd need to implement:
// template<typename KeyType>
// int IndexManager::countLeafNodes(BPlusTreeNode<KeyType>* root) const
// {
//     if (!root) return 0;
//     if (root->is_leaf) return 1;
    
//     int count = 0;
//     for (size_t i = 0; i < root->children.size(); i++) {
//         count += countLeafNodes(root->children[i]);
//     }
//     return count;
// }

// template<typename KeyType>
// int IndexManager::getTreeHeight(BPlusTreeNode<KeyType>* root) const
// {
//     if (!root) return 0;
//     if (root->is_leaf) return 1;
    
//     int max_height = 0;
//     for (size_t i = 0; i < root->children.size(); i++) {
//         if (root->children[i]) {
//             max_height = std::max(max_height, getTreeHeight(root->children[i]));
//         }
//     }
//     return max_height + 1;
// }

// template<typename KeyType>
// int IndexManager::getTotalKeys(BPlusTreeNode<KeyType>* root) const
// {
//     if (!root) return 0;
    
//     int count = root->key_count;
//     if (!root->is_leaf) {
//         for (size_t i = 0; i < root->children.size(); i++) {
//             if (root->children[i]) {
//                 count += getTotalKeys(root->children[i]);
//             }
//         }
//     }
//     return count;
// }

#include "GameRecord.h"
#include <algorithm>
#include <queue>
#include <cstring>
#include <type_traits>
#include <iomanip>

// =============================
// B+ Tree Node (existing base)
// =============================
template<typename KeyType>
BPlusTreeNode<KeyType>::BPlusTreeNode(bool leaf) : is_leaf(leaf), key_count(0)
{
    for (int i = 0; i < MAX_KEYS; i++) keys[i] = KeyType{};
    if (is_leaf) {
        leaf_data.next_leaf = nullptr;
        for (int i = 0; i < MAX_KEYS; i++) {
            leaf_data.block_ids[i] = -1;
            leaf_data.record_ids[i] = -1;
        }
    } else {
        for (int i = 0; i <= MAX_KEYS; i++) children[i] = nullptr;
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

// =============================
// IndexManager (Task 2 base)
// =============================
IndexManager::IndexManager()
    : team_id_index(nullptr), points_index(nullptr),
      fg_pct_index(nullptr), date_index(nullptr), ft_pct_index(nullptr) {}

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

    // Fresh roots
    team_id_index = new BPlusTreeNode<int>(true);
    points_index  = new BPlusTreeNode<int>(true);
    fg_pct_index  = new BPlusTreeNode<float>(true);
    date_index    = new BPlusTreeNode<std::string>(true);
    ft_pct_index  = new BPlusTreeNode<float>(true);

    // Insert all records
    for (size_t block_idx = 0; block_idx < db.getTotalBlocks(); block_idx++) {
        const Block& block = db.getBlock(block_idx);
        for (int record_idx = 0; record_idx < block.record_count; record_idx++) {
            GameRecord record = block.getRecord(record_idx);
            insert(team_id_index, record.team_id_home,           (int)block_idx, record_idx);
            insert(points_index,  record.pts_home,               (int)block_idx, record_idx);
            insert(fg_pct_index,  record.fg_pct_home,            (int)block_idx, record_idx);
            insert(date_index,    std::string(record.game_date), (int)block_idx, record_idx);
            insert(ft_pct_index,  record.ft_pct_home,            (int)block_idx, record_idx);
        }
    }

    std::cout << "B+ tree indexes built successfully with node splitting!" << std::endl;
    return true;
}

// =============================
// Core B+ ops (existing)
// =============================
template<typename KeyType>
bool IndexManager::insert(BPlusTreeNode<KeyType>*& root, KeyType key, int block_id, int record_id)
{
    using Node = BPlusTreeNode<KeyType>;
    const int MAX_KEYS = Node::MAX_KEYS;

    if (!root) root = new Node(true);

    // If root is a leaf and has space
    if (root->is_leaf && root->key_count < MAX_KEYS) {
        return insertIntoLeaf(root, key, block_id, record_id);
    }

    // Split internal helper
    auto splitInternalHere = [&](Node* left, KeyType& promoted_key_out) -> Node* {
        Node* right = new Node(false);
        int mid = left->key_count / 2;
        promoted_key_out = left->keys[mid];

        int rkeys = left->key_count - (mid + 1);
        right->key_count = rkeys;
        for (int i = 0; i < rkeys; ++i) {
            right->keys[i] = left->keys[mid + 1 + i];
        }
        for (int i = 0; i <= rkeys; ++i) {
            right->children[i] = left->children[mid + 1 + i];
        }

        left->key_count = mid;
        return right;
    };

    // Descend while remembering path
    Node* path_nodes[128];
    int   path_pos[128];
    int depth = 0;

    Node* cur = root;
    while (!cur->is_leaf) {
        int pos = 0;
        while (pos < cur->key_count && key > cur->keys[pos]) pos++;
        path_nodes[depth] = cur;
        path_pos[depth] = pos;
        depth++;

        if (!cur->children[pos]) cur->children[pos] = new Node(true);
        cur = cur->children[pos];
    }

    // Leaf insert or split
    if (cur->key_count < MAX_KEYS) {
        return insertIntoLeaf(cur, key, block_id, record_id);
    }

    auto split_res = splitLeaf(cur);
    KeyType promoted_key = split_res.first;
    Node* new_right = split_res.second;
    if (!new_right) return false;

    if (key < promoted_key) insertIntoLeaf(cur, key, block_id, record_id);
    else                    insertIntoLeaf(new_right, key, block_id, record_id);

    // Bubble up
    for (int i = depth - 1; i >= 0; --i) {
        Node* parent = path_nodes[i];
        int insert_pos = path_pos[i];

        for (int k = parent->key_count; k > insert_pos; --k) {
            parent->keys[k] = parent->keys[k - 1];
            parent->children[k + 1] = parent->children[k];
        }
        parent->keys[insert_pos] = promoted_key;
        parent->children[insert_pos + 1] = new_right;
        parent->key_count++;

        if (parent->key_count < MAX_KEYS) return true;

        KeyType parent_promoted;
        Node* parent_right = splitInternalHere(parent, parent_promoted);

        promoted_key = parent_promoted;
        new_right = parent_right;
    }

    Node* new_root = new Node(false);
    new_root->keys[0] = promoted_key;
    new_root->children[0] = root;
    new_root->children[1] = new_right;
    new_root->key_count = 1;
    root = new_root;
    return true;
}

template<typename KeyType>
bool IndexManager::insertIntoLeaf(BPlusTreeNode<KeyType>* leaf, KeyType key, int block_id, int record_id)
{
    static const int MAX_KEYS = BPlusTreeNode<KeyType>::MAX_KEYS;
    if (!leaf || leaf->key_count >= MAX_KEYS) return false;

    int pos = 0;
    while (pos < leaf->key_count && leaf->keys[pos] < key) pos++;

    for (int i = leaf->key_count; i > pos; i--) {
        leaf->keys[i] = leaf->keys[i-1];
        leaf->leaf_data.block_ids[i]  = leaf->leaf_data.block_ids[i-1];
        leaf->leaf_data.record_ids[i] = leaf->leaf_data.record_ids[i-1];
    }

    leaf->keys[pos] = key;
    leaf->leaf_data.block_ids[pos]  = block_id;
    leaf->leaf_data.record_ids[pos] = record_id;
    leaf->key_count++;
    return true;
}

template<typename KeyType>
std::pair<KeyType, BPlusTreeNode<KeyType>*> IndexManager::splitLeaf(BPlusTreeNode<KeyType>* leaf)
{
    static const int MAX_KEYS = BPlusTreeNode<KeyType>::MAX_KEYS;
    if (!leaf) return std::make_pair(KeyType{}, nullptr);

    auto* new_leaf = new BPlusTreeNode<KeyType>(true);
    int split_point = leaf->key_count / 2;

    int j = 0;
    for (int i = split_point; i < leaf->key_count && i < MAX_KEYS && j < MAX_KEYS; i++, j++) {
        new_leaf->keys[j] = leaf->keys[i];
        new_leaf->leaf_data.block_ids[j]  = leaf->leaf_data.block_ids[i];
        new_leaf->leaf_data.record_ids[j] = leaf->leaf_data.record_ids[i];

        leaf->keys[i] = KeyType{};
        leaf->leaf_data.block_ids[i]  = -1;
        leaf->leaf_data.record_ids[i] = -1;
    }
    new_leaf->key_count = j;

    leaf->key_count = split_point;

    new_leaf->leaf_data.next_leaf = leaf->leaf_data.next_leaf;
    leaf->leaf_data.next_leaf = new_leaf;

    KeyType promoted_key = (new_leaf->key_count > 0) ? new_leaf->keys[0] : KeyType{};
    return std::make_pair(promoted_key, new_leaf);
}

template<typename KeyType>
std::vector<std::pair<int, int>> IndexManager::search(BPlusTreeNode<KeyType>* root, KeyType key)
{
    std::vector<std::pair<int, int>> results;
    if (!root) return results;

    if (root->is_leaf) {
        for (int i = 0; i < root->key_count; i++) {
            if (root->keys[i] == key) {
                results.emplace_back(root->leaf_data.block_ids[i], root->leaf_data.record_ids[i]);
            }
        }
    } else {
        int pos = 0;
        while (pos < root->key_count && key > root->keys[pos]) pos++;
        return search(root->children[pos], key);
    }
    return results;
}

// Note: Simple rangeSearch (fine for Task 2 demo).
template<typename KeyType>
std::vector<std::pair<int,int>> IndexManager::rangeSearch(BPlusTreeNode<KeyType>* root,
                          KeyType min_key, KeyType max_key)
{
    std::vector<std::pair<int,int>> results;
    if (!root) return results;

    // 1) Descend to the first leaf that may contain min_key.
    BPlusTreeNode<KeyType>* node = root;
    while (node && !node->is_leaf) {
        int i = 0;
        // find first separator > min_key (i.e., lower_bound)
        while (i < node->key_count && min_key > node->keys[i]) ++i;
        node = node->children[i]; // 0..key_count
    }
    if (!node) return results;

    // 2) Scan forward across leaves, stopping when keys exceed max_key.
    for (auto leaf = node; leaf != nullptr; leaf = leaf->leaf_data.next_leaf) {
        for (int i = 0; i < leaf->key_count; ++i) {
            const KeyType k = leaf->keys[i];
            if (k < min_key) continue;
            if (k > max_key) return results; // we can stop entirely
            results.push_back({ leaf->leaf_data.block_ids[i],
                            leaf->leaf_data.record_ids[i] });
        }
    }
    return results;
}

// =============================
// Public wrapper methods (MISSING BEFORE) ✅
// These fix your linker errors.
// =============================
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

// =============================
// Stats printing (existing)
// =============================
template<typename KeyType>
int IndexManager::countNodes(BPlusTreeNode<KeyType>* root) const
{
    if (!root) return 0;
    int count = 1;
    if (!root->is_leaf) {
        for (int i = 0; i <= root->key_count; i++) {
            if (root->children[i]) count += countNodes(root->children[i]);
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
        if (root->children[i]) count += countLeafNodes(root->children[i]);
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
            if (root->children[i]) count += getTotalKeys(root->children[i]);
        }
    }
    return count;
}

template<typename KeyType>
static void printRootKeysLine(const std::string& index_name, BPlusTreeNode<KeyType>* root) {
    (void)index_name; // silence unused-parameter warning

    std::cout << "  - Root keys (" << (root ? root->key_count : 0) << "): ";
    if (!root || root->key_count == 0) { std::cout << "(empty)\n"; return; }

    // Save/restore stream state
    std::ios::fmtflags f = std::cout.flags();
    std::streamsize p = std::cout.precision();

    if (std::is_floating_point<KeyType>::value) {
        std::cout.setf(std::ios::fixed);
        std::cout.precision(3);
    }

    for (int i = 0; i < root->key_count; ++i) {
        if (i) std::cout << " | ";
        std::cout << root->keys[i];
    }
    std::cout << "\n";

    std::cout.flags(f);
    std::cout.precision(p);
}

void IndexManager::displayIndexStatistics() const
{
    std::cout << "\n=== B+ Tree Index Statistics (Max 20 keys per node) ===" << std::endl;

    displaySingleIndexStats("Team ID",        team_id_index);
    displaySingleIndexStats("Points",         points_index);
    displaySingleIndexStats("FG Percentage",  fg_pct_index);
    displaySingleIndexStats("Date",           date_index);
    displaySingleIndexStats("FT Percentage",  ft_pct_index);

    int total_nodes =
        countNodes(team_id_index) + countNodes(points_index) +
        countNodes(fg_pct_index)  + countNodes(date_index) +
        countNodes(ft_pct_index);

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

    int total_nodes    = countNodes(root);
    int leaf_nodes     = countLeafNodes(root);
    int internal_nodes = total_nodes - leaf_nodes;
    int tree_height    = getTreeHeight(root);
    int total_keys     = getTotalKeys(root);

    std::cout << "\n" << index_name << " Index:" << std::endl;
    std::cout << "  - Total nodes: "    << total_nodes    << std::endl;
    std::cout << "  - Leaf nodes: "     << leaf_nodes     << std::endl;
    std::cout << "  - Internal nodes: " << internal_nodes << std::endl;
    std::cout << "  - Tree height: "    << tree_height    << std::endl;
    std::cout << "  - Total keys: "     << total_keys     << std::endl;
    std::cout << "  - Max keys per node: " << BPlusTreeNode<KeyType>::MAX_KEYS << std::endl;
    if (leaf_nodes > 0) {
        std::cout << "  - Avg keys per leaf: " << (total_keys / leaf_nodes) << std::endl;
    }
    printRootKeysLine(index_name, root);
}

// ==========================================
// Task 3 — counts-aware FT% leaf sweep + rebuild
// ==========================================
namespace {
    struct NodeVisitCounters { uint32_t internal = 0, leaf = 0; };

    template<typename T>
    BPlusTreeNode<T>* descendToFirstLeafWithCounts(BPlusTreeNode<T>* root,
                                                   const T& min_key,
                                                   NodeVisitCounters& c) {
        auto* cur = root;
        while (cur && !cur->is_leaf) {
            c.internal++;
            int pos = 0;
            while (pos < cur->key_count && min_key > cur->keys[pos]) ++pos;
            cur = cur->children[pos];
        }
        if (cur) c.leaf++;
        return cur;
    }

    template<typename T>
    void sweepRightLeavesWithin(BPlusTreeNode<T>* leaf,
                                const T& min_key, const T& max_key,
                                std::vector<std::pair<int,int>>& out,
                                NodeVisitCounters& c) {
        for (auto* p = leaf; p; p = p->leaf_data.next_leaf) {
            if (p != leaf) c.leaf++;
            for (int i = 0; i < p->key_count; i++) {
                const T& k = p->keys[i];
                if (k < min_key) continue;
                if (k > max_key) return; // leaves are globally ordered
                out.emplace_back(p->leaf_data.block_ids[i], p->leaf_data.record_ids[i]);
            }
        }
    }
}

std::vector<std::pair<int,int>>
IndexManager::searchByFTPercentageWithCounts(float min_pct, float max_pct,
                                             uint32_t& outInternal, uint32_t& outLeaf)
{
    std::vector<std::pair<int,int>> results;
    outInternal = outLeaf = 0;
    if (!ft_pct_index) return results;

    NodeVisitCounters c{};
    if (ft_pct_index->is_leaf) {
        c.leaf++;
        sweepRightLeavesWithin<float>(ft_pct_index, min_pct, max_pct, results, c);
    } else {
        auto* firstLeaf = descendToFirstLeafWithCounts<float>(ft_pct_index, min_pct, c);
        if (firstLeaf) sweepRightLeavesWithin<float>(firstLeaf, min_pct, max_pct, results, c);
    }
    outInternal = c.internal;
    outLeaf = c.leaf;
    return results;
}

bool IndexManager::buildIndexesSkippingDeleted(const DatabaseFile& db)
{
    delete team_id_index; delete points_index; delete fg_pct_index;
    delete date_index;    delete ft_pct_index;
    team_id_index = new BPlusTreeNode<int>(true);
    points_index  = new BPlusTreeNode<int>(true);
    fg_pct_index  = new BPlusTreeNode<float>(true);
    date_index    = new BPlusTreeNode<std::string>(true);
    ft_pct_index  = new BPlusTreeNode<float>(true);

    for (size_t block_idx = 0; block_idx < db.getTotalBlocks(); ++block_idx) {
        const Block& block = db.getBlock(block_idx);
        for (int record_idx = 0; record_idx < block.record_count; ++record_idx) {
            if (db.isDeleted(block_idx, record_idx)) continue; // skip deleted
            GameRecord r = block.getRecord(record_idx);
            insert(team_id_index, r.team_id_home,           (int)block_idx, record_idx);
            insert(points_index,  r.pts_home,               (int)block_idx, record_idx);
            insert(fg_pct_index,  r.fg_pct_home,            (int)block_idx, record_idx);
            insert(date_index,    std::string(r.game_date), (int)block_idx, record_idx);
            insert(ft_pct_index,  r.ft_pct_home,            (int)block_idx, record_idx);
        }
    }
    return true;
}

// Explicit instantiation so templates link in this TU
template struct BPlusTreeNode<int>;
template struct BPlusTreeNode<float>;
template struct BPlusTreeNode<std::string>;

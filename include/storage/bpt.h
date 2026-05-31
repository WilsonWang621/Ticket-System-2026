#pragma once

#include <fstream>
#include <iostream>
#include <string>

using std::string;
using std::fstream;

constexpr int Node_size = 128;
constexpr int MIN_NODE = Node_size / 2;
constexpr int base = 131;
constexpr int CACHE_SIZE = 2560;
constexpr int HASH_SIZE = 521;

struct Data {
    unsigned long long key = 0;
    int value = 0;

    Data() = default;
    Data(const std::string &s, const int _value):value(_value){
        for (auto &ch : s) {
            key = (key * base + ch);
        }
    }

    Data &operator=(const Data &other) {
        if (this == &other) {
            return *this;
        }
        key = other.key;
        value = other.value;
        return *this;
    }

    friend bool operator==(const Data &lhs, const Data &rhs) {
        return lhs.key == rhs.key && rhs.value == lhs.value;
    }

    friend bool operator<(const Data &lhs, const Data &rhs) {
        if (lhs.key != rhs.key) {
            return lhs.key < rhs.key;
        }
        return lhs.value < rhs.value;
    }

    friend bool operator>(const Data &lhs, const Data &rhs) {
        if (lhs.key != rhs.key) {
            return lhs.key > rhs.key;
        }
        return lhs.value > rhs.value;
    }

    friend bool operator<=(const Data &lhs, const Data &rhs) {
        if (lhs.key != rhs.key) {
            return lhs.key < rhs.key;
        }
        return lhs.value <= rhs.value;
    }

    friend bool operator>=(const Data &lhs, const Data &rhs) {
        if (lhs.key != rhs.key) {
            return lhs.key > rhs.key;
        }
        return lhs.value >= rhs.value;
    }
};

template<typename T>
struct Node {
    int idx = -1;
    int size = 0;
    T boundary[Node_size + 1];
    int son[Node_size + 2]{};

    bool is_leaf = true;
    int parent = -1;
    int prev = -1;
    int next = -1;
};

template <typename T>
class BPT {
private:
    struct CacheEntry {
        bool used = false;
        bool dirty = false;
        int node_idx = -1;
        int prev_lru = -1;
        int next_lru = -1;
        int hash_next = -1;
        Node<T> node{};
    };

    int root = -1;
    int head = -1;
    int next_available = 0;
    int free_head = -1;
    Node<T> cur{};

    std::fstream init_file;
    std::fstream data_file;
    std::string init_file_name;
    std::string data_file_name;

    CacheEntry cache[CACHE_SIZE];
    int hash_head[HASH_SIZE];
    int lru_head = -1;
    int lru_tail = -1;
    int cache_count = 0;

    static int hashIndex(const int node_idx) {
        const int x = node_idx % HASH_SIZE;
        return x < 0 ? x + HASH_SIZE : x;
    }

    void initCache() {
        for (int & i : hash_head) i = -1;
        lru_head = lru_tail = -1;
        cache_count = 0;
    }

    void detachLRU(int slot) {
        int p = cache[slot].prev_lru;
        int n = cache[slot].next_lru;
        if (p != -1) cache[p].next_lru = n;
        else lru_head = n;
        if (n != -1) cache[n].prev_lru = p;
        else lru_tail = p;
        cache[slot].prev_lru = cache[slot].next_lru = -1;
    }

    void attachFrontLRU(int slot) {
        cache[slot].prev_lru = -1;
        cache[slot].next_lru = lru_head;
        if (lru_head != -1) cache[lru_head].prev_lru = slot;
        else lru_tail = slot;
        lru_head = slot;
    }

    void touchSlot(int slot) {
        if (lru_head == slot) return;
        if (cache[slot].prev_lru != -1 || cache[slot].next_lru != -1 || lru_tail == slot) {
            detachLRU(slot);
        }
        attachFrontLRU(slot);
    }

    void addToHash(int slot) {
        int h = hashIndex(cache[slot].node_idx);
        cache[slot].hash_next = hash_head[h];
        hash_head[h] = slot;
    }

    void removeFromHash(int slot) {
        int h = hashIndex(cache[slot].node_idx);
        int p = -1;
        int u = hash_head[h];
        while (u != -1) {
            if (u == slot) {
                if (p == -1) hash_head[h] = cache[u].hash_next;
                else cache[p].hash_next = cache[u].hash_next;
                cache[u].hash_next = -1;
                return;
            }
            p = u;
            u = cache[u].hash_next;
        }
    }

    int findCacheSlot(int node_idx) {
        int h = hashIndex(node_idx);
        int u = hash_head[h];
        while (u != -1) {
            if (cache[u].used && cache[u].node_idx == node_idx) return u;
            u = cache[u].hash_next;
        }
        return -1;
    }

    void flushSlot(int slot) {
        if (!cache[slot].used || !cache[slot].dirty) return;
        data_file.seekp(static_cast<std::streamoff>(cache[slot].node_idx) * sizeof(Node<T>));
        data_file.write(reinterpret_cast<char*>(&cache[slot].node), sizeof(Node<T>));
        cache[slot].dirty = false;
    }

    void flushCache() {
        for (int i = 0; i < CACHE_SIZE; ++i) {
            if (cache[i].used && cache[i].dirty) flushSlot(i);
        }
        data_file.flush();
    }

    int acquireSlot() {
        for (int i = 0; i < CACHE_SIZE; ++i) {
            if (!cache[i].used) {
                cache[i].used = true;
                cache[i].dirty = false;
                cache[i].node_idx = -1;
                cache[i].prev_lru = cache[i].next_lru = -1;
                cache[i].hash_next = -1;
                ++cache_count;
                return i;
            }
        }

        int victim = lru_tail;
        if (victim == -1) return -1;
        detachLRU(victim);
        removeFromHash(victim);
        flushSlot(victim);
        cache[victim].used = true;
        cache[victim].dirty = false;
        cache[victim].node_idx = -1;
        cache[victim].prev_lru = cache[victim].next_lru = -1;
        cache[victim].hash_next = -1;
        return victim;
    }

    int loadNodeToCache(int index) {
        int slot = findCacheSlot(index);
        if (slot != -1) {
            touchSlot(slot);
            return slot;
        }

        slot = acquireSlot();
        cache[slot].node_idx = index;
        data_file.seekg(static_cast<std::streamoff>(index) * sizeof(Node<T>));
        data_file.read(reinterpret_cast<char*>(&cache[slot].node), sizeof(Node<T>));
        cache[slot].node.idx = index;
        cache[slot].dirty = false;
        addToHash(slot);
        attachFrontLRU(slot);
        return slot;
    }

    int putNodeToCache(int index, const Node<T> &node) {
        int slot = findCacheSlot(index);
        if (slot == -1) {
            slot = acquireSlot();
            cache[slot].node_idx = index;
            addToHash(slot);
            attachFrontLRU(slot);
        } else {
            touchSlot(slot);
        }
        cache[slot].node = node;
        cache[slot].node.idx = index;
        cache[slot].dirty = true;
        return slot;
    }

    int allocateNodeIndex() {
        if (free_head == -1) {
            return next_available++;
        }

        const int index = free_head;
        readNode(index);
        free_head = cur.parent;
        return index;
    }

    void recycleNode(const int index) {
        if (index == -1) return;

        Node<T> node;
        node.idx = index;
        node.size = 0;
        node.is_leaf = true;
        node.parent = free_head;
        node.prev = -1;
        node.next = -1;
        for (int i = 0; i < Node_size + 2; ++i) node.son[i] = -1;

        writeNode(index, node);
        free_head = index;
    }

    static int findChildIndex(const T &key, const Node<T> &node) {
        int l = 0, r = node.size - 1;
        while (l <= r) {
            int mid = (l + r) / 2;
            if (key < node.boundary[mid]) r = mid - 1;
            else l = mid + 1;
        }
        return l;
    }

    static int lowerBoundInNode(const T &key, const Node<T> &node) {
        int l = 0, r = node.size;
        while (l < r) {
            const int mid = (l + r) / 2;
            if (node.boundary[mid] < key) l = mid + 1;
            else r = mid;
        }
        return l;
    }

    int findSonIndex(const Node<T> &node, int child_idx) {
        for (int i = 0; i <= node.size; ++i) {
            if (node.son[i] == child_idx) {
                return i;
            }
        }
        return -1;
    }

    T getFirstData(int node_idx) {
        readNode(node_idx);
        Node<T> node = cur;
        while (!node.is_leaf) {
            readNode(node.son[0]);
            node = cur;
        }
        return node.boundary[0];
    }

    void updateParentSeparator(int parent_idx, int child_idx) {
        if (parent_idx == -1) return;

        readNode(parent_idx);
        Node<T> parent = cur;
        int pos = findSonIndex(parent, child_idx);
        if (pos <= 0) return;

        readNode(child_idx);
        if (cur.size == 0) return;
        T new_min = getFirstData(child_idx);

        if (parent.boundary[pos - 1] == new_min) return;

        parent.boundary[pos - 1] = new_min;
        writeNode(parent_idx, parent);
        updateParentSeparator(parent.parent, parent_idx);
    }

    void handleUnderflow(const int node_idx) {
        readNode(node_idx);
        if (cur.size >= MIN_NODE || cur.idx == root) return;

        if (borrowFromRight(node_idx)) {
            return;
        }
        if (borrowFromLeft(node_idx)) return;

        merge(node_idx);
    }

    bool borrowFromRight(int node_idx) {
        readNode(node_idx);
        Node<T> node = cur;
        int parent_idx = node.parent;
        if (parent_idx == -1) return false;

        readNode(parent_idx);
        Node<T> parent = cur;
        int pos = findSonIndex(parent, node_idx);
        if (pos == -1 || pos == parent.size) return false;

        int right_idx = parent.son[pos + 1];
        readNode(right_idx);
        Node<T> right_node = cur;
        if (right_node.size <= MIN_NODE) return false;

        if (node.is_leaf) {
            node.boundary[node.size] = right_node.boundary[0];
            ++node.size;

            for (int i = 0; i < right_node.size - 1; ++i) {
                right_node.boundary[i] = right_node.boundary[i + 1];
            }
            --right_node.size;

            writeNode(node.idx, node);
            writeNode(right_node.idx, right_node);

            parent.boundary[pos] = right_node.boundary[0];
            writeNode(parent.idx, parent);
        } else {
            node.boundary[node.size] = parent.boundary[pos];
            node.son[node.size + 1] = right_node.son[0];
            ++node.size;

            if (node.son[node.size] != -1) {
                readNode(node.son[node.size]);
                Node<T> child = cur;
                child.parent = node.idx;
                writeNode(child.idx, child);
            }

            parent.boundary[pos] = right_node.boundary[0];
            for (int i = 0; i < right_node.size - 1; ++i) {
                right_node.boundary[i] = right_node.boundary[i + 1];
            }
            for (int i = 0; i < right_node.size; ++i) {
                right_node.son[i] = right_node.son[i + 1];
            }
            --right_node.size;

            writeNode(node.idx, node);
            writeNode(right_node.idx, right_node);
            writeNode(parent.idx, parent);
        }
        return true;
    }

    bool borrowFromLeft(int node_idx) {
        readNode(node_idx);
        Node<T> node = cur;
        int parent_idx = node.parent;
        if (parent_idx == -1) return false;

        readNode(parent_idx);
        Node<T> parent = cur;
        int pos = findSonIndex(parent, node_idx);
        if (pos <= 0) return false;

        int left_idx = parent.son[pos - 1];
        readNode(left_idx);
        Node<T> left_node = cur;
        if (left_node.size <= MIN_NODE) return false;

        if (node.is_leaf) {
            for (int i = node.size; i > 0; --i) {
                node.boundary[i] = node.boundary[i - 1];
            }
            node.boundary[0] = left_node.boundary[left_node.size - 1];
            ++node.size;
            --left_node.size;

            writeNode(left_node.idx, left_node);
            writeNode(node.idx, node);

            parent.boundary[pos - 1] = node.boundary[0];
            writeNode(parent.idx, parent);
        } else {
            for (int i = node.size; i > 0; --i) {
                node.boundary[i] = node.boundary[i - 1];
            }
            for (int i = node.size + 1; i > 0; --i) {
                node.son[i] = node.son[i - 1];
            }

            node.boundary[0] = parent.boundary[pos - 1];
            node.son[0] = left_node.son[left_node.size];
            ++node.size;

            if (node.son[0] != -1) {
                readNode(node.son[0]);
                Node<T> child = cur;
                child.parent = node.idx;
                writeNode(child.idx, child);
            }

            parent.boundary[pos - 1] = left_node.boundary[left_node.size - 1];
            --left_node.size;

            writeNode(left_node.idx, left_node);
            writeNode(node.idx, node);
            writeNode(parent.idx, parent);
        }
        return true;
    }

    void merge(const int idx) {
        readNode(idx);
        Node<T> node = cur;
        int parent_idx = node.parent;
        if (parent_idx == -1) {
            if (!node.is_leaf && node.size == 0) {
                const int old_root = node.idx;
                root = node.son[0];
                readNode(root);
                cur.parent = -1;
                writeNode(root, cur);
                recycleNode(old_root);
            }
            return;
        }

        readNode(parent_idx);
        Node<T> parent = cur;
        int pos = findSonIndex(parent, idx);
        if (pos == -1) return;

        int left_idx = (pos > 0) ? parent.son[pos - 1] : -1;
        int right_idx = (pos < parent.size) ? parent.son[pos + 1] : -1;

        if (left_idx != -1) {
            readNode(left_idx);
            Node<T> left_node = cur;

            if (node.is_leaf) {
                for (int i = 0; i < node.size; ++i) {
                    left_node.boundary[left_node.size + i] = node.boundary[i];
                }
                left_node.size += node.size;
                left_node.next = node.next;
                if (node.next != -1) {
                    readNode(node.next);
                    cur.prev = left_node.idx;
                    writeNode(cur.idx, cur);
                }
                writeNode(left_node.idx, left_node);
            } else {
                int old_size = left_node.size;
                left_node.boundary[old_size] = parent.boundary[pos - 1];
                for (int i = 0; i < node.size; ++i) {
                    left_node.boundary[old_size + 1 + i] = node.boundary[i];
                }
                for (int i = 0; i <= node.size; ++i) {
                    left_node.son[old_size + 1 + i] = node.son[i];
                }
                left_node.size = old_size + 1 + node.size;
                writeNode(left_node.idx, left_node);

                for (int i = old_size + 1; i <= left_node.size; ++i) {
                    if (left_node.son[i] != -1) {
                        readNode(left_node.son[i]);
                        cur.parent = left_node.idx;
                        writeNode(cur.idx, cur);
                    }
                }
            }

            for (int i = pos - 1; i < parent.size - 1; ++i) {
                parent.boundary[i] = parent.boundary[i + 1];
            }
            for (int i = pos; i < parent.size; ++i) {
                parent.son[i] = parent.son[i + 1];
            }
            --parent.size;
            writeNode(parent.idx, parent);

            if (parent.idx == root && parent.size == 0) {
                const int old_root = parent.idx;
                root = left_idx;
                readNode(root);
                cur.parent = -1;
                writeNode(root, cur);
                recycleNode(old_root);
            } else if (parent.size < MIN_NODE && parent.idx != root) {
                handleUnderflow(parent.idx);
            }
            recycleNode(node.idx);
            return;
        }

        if (right_idx != -1) {
            readNode(right_idx);
            Node<T> right_node = cur;

            if (node.is_leaf) {
                for (int i = 0; i < right_node.size; ++i) {
                    node.boundary[node.size + i] = right_node.boundary[i];
                }
                node.size += right_node.size;
                node.next = right_node.next;
                if (right_node.next != -1) {
                    readNode(right_node.next);
                    cur.prev = node.idx;
                    writeNode(cur.idx, cur);
                }
                writeNode(node.idx, node);
            } else {
                int old_size = node.size;
                node.boundary[old_size] = parent.boundary[pos];
                for (int i = 0; i < right_node.size; ++i) {
                    node.boundary[old_size + 1 + i] = right_node.boundary[i];
                }
                for (int i = 0; i <= right_node.size; ++i) {
                    node.son[old_size + 1 + i] = right_node.son[i];
                }
                node.size = old_size + 1 + right_node.size;
                writeNode(node.idx, node);

                for (int i = old_size + 1; i <= node.size; ++i) {
                    if (node.son[i] != -1) {
                        readNode(node.son[i]);
                        cur.parent = node.idx;
                        writeNode(cur.idx, cur);
                    }
                }
            }

            for (int i = pos; i < parent.size - 1; ++i) {
                parent.boundary[i] = parent.boundary[i + 1];
            }
            for (int i = pos + 1; i < parent.size; ++i) {
                parent.son[i] = parent.son[i + 1];
            }
            --parent.size;
            writeNode(parent.idx, parent);

            if (parent.idx == root && parent.size == 0) {
                const int old_root = parent.idx;
                root = node.idx;
                readNode(root);
                cur.parent = -1;
                writeNode(root, cur);
                recycleNode(old_root);
            } else if (parent.size < MIN_NODE && parent.idx != root) {
                handleUnderflow(parent.idx);
            }
            recycleNode(right_idx);
        }
    }

    void split() {
        int old_size = cur.size;
        int mid = old_size / 2;

        Node<T> node = cur;
        Node<T> new_node;
        new_node.idx = allocateNodeIndex();
        new_node.is_leaf = node.is_leaf;
        new_node.parent = node.parent;
        new_node.prev = -1;
        new_node.next = -1;
        for (int i = 0; i < Node_size + 2; ++i) new_node.son[i] = -1;

        T up_key;
        if (node.is_leaf) {
            for (int i = mid; i < old_size; ++i) {
                new_node.boundary[i - mid] = node.boundary[i];
            }
            new_node.size = old_size - mid;
            node.size = mid;

            new_node.next = node.next;
            new_node.prev = node.idx;
            if (node.next != -1) {
                readNode(node.next);
                cur.prev = new_node.idx;
                writeNode(cur.idx, cur);
            }
            node.next = new_node.idx;
            up_key = new_node.boundary[0];
        } else {
            up_key = node.boundary[mid];
            for (int i = mid + 1; i < old_size; ++i) {
                new_node.boundary[i - mid - 1] = node.boundary[i];
            }
            for (int i = mid + 1; i <= old_size; ++i) {
                new_node.son[i - mid - 1] = node.son[i];
            }
            new_node.size = old_size - mid - 1;
            node.size = mid;

            for (int i = 0; i <= new_node.size; ++i) {
                if (new_node.son[i] != -1) {
                    readNode(new_node.son[i]);
                    cur.parent = new_node.idx;
                    writeNode(cur.idx, cur);
                }
            }
        }

        writeNode(node.idx, node);
        writeNode(new_node.idx, new_node);

        if (node.parent == -1) {
            Node<T> new_root;
            new_root.idx = allocateNodeIndex();
            new_root.is_leaf = false;
            new_root.size = 1;
            new_root.parent = -1;
            new_root.prev = -1;
            new_root.next = -1;
            for (int i = 0; i < Node_size + 2; ++i) new_root.son[i] = -1;
            new_root.boundary[0] = up_key;
            new_root.son[0] = node.idx;
            new_root.son[1] = new_node.idx;

            node.parent = new_node.parent = new_root.idx;
            writeNode(node.idx, node);
            writeNode(new_node.idx, new_node);
            writeNode(new_root.idx, new_root);
            root = new_root.idx;
        } else {
            int parent_idx = node.parent;
            readNode(parent_idx);
            Node<T> parent = cur;
            int pos = findSonIndex(parent, node.idx);

            for (int i = parent.size; i > pos; --i) {
                parent.boundary[i] = parent.boundary[i - 1];
            }
            for (int i = parent.size + 1; i > pos + 1; --i) {
                parent.son[i] = parent.son[i - 1];
            }
            parent.boundary[pos] = up_key;
            parent.son[pos + 1] = new_node.idx;
            ++parent.size;
            writeNode(parent_idx, parent);

            if (parent.size > Node_size) {
                cur = parent;
                split();
            }
        }
    }

public:
    explicit BPT(const std::string & name) {
        init_file_name = "init_" + name;
        data_file_name = "data_" + name;
        initCache();

        data_file.open(data_file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (!data_file.is_open()) {
            data_file.open(data_file_name, std::ios::out | std::ios::binary);
            data_file.close();
            data_file.open(data_file_name, std::ios::in | std::ios::out | std::ios::binary);
        }

        init_file.open(init_file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (!init_file.is_open()) {
            init_file.open(init_file_name, std::ios::out | std::ios::binary);
            init_file.close();
            init_file.open(init_file_name, std::ios::in | std::ios::out | std::ios::binary);
            root = head = 0;
            next_available = 1;
            Node<T> root_node;
            root_node.idx = 0;
            root_node.size = 0;
            root_node.is_leaf = true;
            root_node.parent = -1;
            root_node.prev = -1;
            root_node.next = -1;
            for (int i = 0; i < Node_size + 2; ++i) root_node.son[i] = -1;
            writeNode(root_node.idx, root_node);
        } else {
            init_file.read(reinterpret_cast<char*>(&root), sizeof(int));
            init_file.read(reinterpret_cast<char*>(&head), sizeof(int));
            init_file.read(reinterpret_cast<char*>(&next_available), sizeof(int));
            if (!init_file.read(reinterpret_cast<char*>(&free_head), sizeof(int))) {
                free_head = -1;
                init_file.clear();
            }
        }
        init_file.close();
    }

    ~BPT() {
        flushCache();
        init_file.open(init_file_name, std::ios::in | std::ios::out | std::ios::binary);
        if (init_file.is_open()) {
            init_file.seekp(0);
            init_file.write(reinterpret_cast<char*>(&root), sizeof(int));
            init_file.write(reinterpret_cast<char*>(&head), sizeof(int));
            init_file.write(reinterpret_cast<char*>(&next_available), sizeof(int));
            init_file.write(reinterpret_cast<char*>(&free_head), sizeof(int));
            init_file.close();
        }
        data_file.close();
    }

    void readNode(const int index) {
        int slot = loadNodeToCache(index);
        cur = cache[slot].node;
    }

    void writeNode(const int index, const Node<T> &node) {
        putNodeToCache(index, node);
    }

    void add(const T& data) {
        if (root == -1) {
            root = head = 0;
            next_available = 1;
            Node<T> new_root;
            new_root.idx = 0;
            new_root.is_leaf = true;
            new_root.size = 0;
            new_root.parent = -1;
            new_root.prev = -1;
            new_root.next = -1;
            for (int i = 0; i < Node_size + 2; ++i) new_root.son[i] = -1;
            writeNode(new_root.idx, new_root);
        }
        int p = root;
        while (true) {
            readNode(p);
            if (cur.is_leaf) break;
            int pos = findChildIndex(data, cur);
            p = cur.son[pos];
        }

        int pos = lowerBoundInNode(data, cur);
        for (int i = cur.size; i > pos; --i) {
            cur.boundary[i] = cur.boundary[i - 1];
        }
        cur.boundary[pos] = data;
        ++cur.size;
        Node<T> node = cur;
        writeNode(node.idx, node);

        if (pos == 0) {
            updateParentSeparator(node.parent, node.idx);
        }
        if (node.size > Node_size) {
            cur = node;
            split();
        }
    }

    void remove(const T& data) {
        if (root == -1) return;
        int p = root;
        while (true) {
            readNode(p);
            if (cur.is_leaf) break;
            int pos = findChildIndex(data, cur);
            p = cur.son[pos];
        }

        int pos = lowerBoundInNode(data, cur);
        if (pos == cur.size || !(cur.boundary[pos] == data)) return;

        bool first_changed = (pos == 0);
        for (int i = pos; i < cur.size - 1; i++) {
            cur.boundary[i] = cur.boundary[i + 1];
        }
        --cur.size;
        Node<T> node = cur;
        writeNode(node.idx, node);

        if (node.idx == root) {
            if (node.size == 0 && !node.is_leaf) {
                const int old_root = node.idx;
                root = node.son[0];
                readNode(root);
                cur.parent = -1;
                writeNode(root, cur);
                recycleNode(old_root);
            }
            return;
        }

        if (first_changed && node.size > 0) {
            updateParentSeparator(node.parent, node.idx);
        }
        if (node.size < Node_size / 2 && node.idx != root) {
            handleUnderflow(node.idx);
        }
        else if (node.idx == root && node.size == 0 && !node.is_leaf) {
            const int old_root = node.idx;
            root = node.son[0];
            readNode(root);
            cur.parent = -1;
            writeNode(root, cur);
            recycleNode(old_root);
        }
    }

    bool lower_bound(const T &key, T &result) {
        if (root == -1) return false;

        int p = root;
        while (true) {
            readNode(p);
            if (cur.is_leaf) break;
            int pos = findChildIndex(key, cur);
            p = cur.son[pos];
        }

        while (true) {
            int pos = lowerBoundInNode(key, cur);
            if (pos < cur.size) {
                result = cur.boundary[pos];
                return true;
            }
            if (cur.next == -1) {
                return false;
            }
            readNode(cur.next);
        }
    }

    template <typename ResultContainer>
    void range_query(const T &left, const T &right, ResultContainer &results) {
        results.clear();
        if (root == -1) return;

        int p = root;
        while (true) {
            readNode(p);
            if (cur.is_leaf) break;
            int pos = findChildIndex(left, cur);
            p = cur.son[pos];
        }

        while (true) {
            int pos = lowerBoundInNode(left, cur);
            for (; pos < cur.size; ++pos) {
                if (right < cur.boundary[pos]) {
                    return;
                }
                results.push_back(cur.boundary[pos]);
            }
            if (cur.next == -1) {
                return;
            }
            readNode(cur.next);
        }
    }

    void find(const string &s) {
        unsigned long long key_ = 0;
        for (char ch : s) key_ = key_ * base + ch;
        if (root == -1) {
            std::cout << "null\n";
            return;
        }
        Data probe;
        probe.key = key_;
        probe.value = -2147483647 - 1;

        int p = root;
        while (true) {
            readNode(p);
            if (cur.is_leaf) break;
            int pos = findChildIndex(probe, cur);
            p = cur.son[pos];
        }
        bool found = false;
        while (true) {
            for (int i = 0; i < cur.size; ++i) {
                if (cur.boundary[i].key == key_) {
                    std::cout << cur.boundary[i].value << " ";
                    found = true;
                } else if (cur.boundary[i].key > key_) {
                    if (!found) std::cout << "null";
                    std::cout << "\n";
                    return;
                }
            }
            if (cur.next == -1) break;
            readNode(cur.next);
        }
        if (!found) std::cout << "null";
        std::cout << "\n";
    }
};

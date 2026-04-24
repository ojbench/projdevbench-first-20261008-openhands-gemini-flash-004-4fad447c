


#ifndef DATABASE_HPP
#define DATABASE_HPP

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <algorithm>

template<typename T>
class MemoryRiver {
private:
    std::fstream file;
    std::string filename;
    int sizeofT = sizeof(T);

public:
    MemoryRiver() = default;
    MemoryRiver(const std::string& fname) : filename(fname) {
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) {
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        }
    }
    ~MemoryRiver() {
        if (file.is_open()) file.close();
    }

    void initialise(const std::string& fname) {
        filename = fname;
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) {
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        }
    }

    void write(T& t, const int index) {
        file.seekp(index);
        file.write(reinterpret_cast<char*>(&t), sizeofT);
    }

    void read(T& t, const int index) {
        file.seekg(index);
        file.read(reinterpret_cast<char*>(&t), sizeofT);
    }

    void get_info(int& tmp, int index) {
        file.seekg(index);
        file.read(reinterpret_cast<char*>(&tmp), sizeof(int));
    }

    void write_info(int tmp, int index) {
        file.seekp(index);
        file.write(reinterpret_cast<char*>(&tmp), sizeof(int));
    }
};

template<typename Key, typename Value, int M = 100>
class BPlusTree {
    struct Node {
        long offset;
        long parent;
        long next;
        int size;
        bool is_leaf;
        Key keys[M];
        Value values[M];
        long children[M + 1];

        Node() : offset(-1), parent(-1), next(-1), size(0), is_leaf(false) {
            for (int i = 0; i < M + 1; ++i) children[i] = -1;
        }
    };

    std::string filename;
    std::fstream file;
    long root_offset;
    long end_offset;

    void read_node(long offset, Node& node) {
        if (offset == -1) return;
        file.seekg(offset);
        file.read(reinterpret_cast<char*>(&node), sizeof(Node));
    }

    void write_node(long offset, Node& node) {
        file.seekp(offset);
        file.write(reinterpret_cast<char*>(&node), sizeof(Node));
    }

    long allocate_node() {
        long offset = end_offset;
        end_offset += sizeof(Node);
        return offset;
    }

public:
    BPlusTree(const std::string& fname) : filename(fname) {
        file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
        if (!file) {
            file.open(filename, std::ios::out | std::ios::binary);
            file.close();
            file.open(filename, std::ios::in | std::ios::out | std::ios::binary);
            root_offset = -1;
            end_offset = sizeof(long);
            file.seekp(0);
            file.write(reinterpret_cast<char*>(&root_offset), sizeof(long));
        } else {
            file.seekg(0);
            file.read(reinterpret_cast<char*>(&root_offset), sizeof(long));
            file.seekg(0, std::ios::end);
            end_offset = file.tellg();
        }
    }

    ~BPlusTree() {
        file.seekp(0);
        file.write(reinterpret_cast<char*>(&root_offset), sizeof(long));
        file.close();
    }

    void insert(const Key& key, const Value& value) {
        if (root_offset == -1) {
            Node root;
            root.is_leaf = true;
            root.size = 1;
            root.keys[0] = key;
            root.values[0] = value;
            root_offset = allocate_node();
            root.offset = root_offset;
            write_node(root_offset, root);
            return;
        }

        long curr_offset = root_offset;
        Node curr;
        while (true) {
            read_node(curr_offset, curr);
            if (curr.is_leaf) break;
            int i = 0;
            while (i < curr.size && key >= curr.keys[i]) i++;
            curr_offset = curr.children[i];
        }

        // Check for duplicate key if necessary, but for indices we might want duplicates
        // For simplicity, this B+ tree will handle unique keys. 
        // For non-unique, we can use (Key, ISBN) as the key.

        int i = curr.size - 1;
        while (i >= 0 && curr.keys[i] > key) {
            curr.keys[i + 1] = curr.keys[i];
            curr.values[i + 1] = curr.values[i];
            i--;
        }
        curr.keys[i + 1] = key;
        curr.values[i + 1] = value;
        curr.size++;
        write_node(curr_offset, curr);

        if (curr.size == M) {
            split_leaf(curr_offset, curr);
        }
    }

    void split_leaf(long offset, Node& node) {
        Node newNode;
        newNode.is_leaf = true;
        newNode.size = M - M / 2;
        for (int i = 0; i < newNode.size; ++i) {
            newNode.keys[i] = node.keys[i + M / 2];
            newNode.values[i] = node.values[i + M / 2];
        }
        node.size = M / 2;
        newNode.next = node.next;
        long newNodeOffset = allocate_node();
        newNode.offset = newNodeOffset;
        node.next = newNodeOffset;
        newNode.parent = node.parent;

        write_node(offset, node);
        write_node(newNodeOffset, newNode);

        insert_into_parent(offset, newNode.keys[0], newNodeOffset, node.parent);
    }

    void insert_into_parent(long left_offset, const Key& key, long right_offset, long parent_offset) {
        if (parent_offset == -1) {
            Node root;
            root.is_leaf = false;
            root.size = 1;
            root.keys[0] = key;
            root.children[0] = left_offset;
            root.children[1] = right_offset;
            root_offset = allocate_node();
            root.offset = root_offset;
            write_node(root_offset, root);

            Node left, right;
            read_node(left_offset, left);
            left.parent = root_offset;
            write_node(left_offset, left);
            read_node(right_offset, right);
            right.parent = root_offset;
            write_node(right_offset, right);
            return;
        }

        Node parent;
        read_node(parent_offset, parent);
        int i = parent.size - 1;
        while (i >= 0 && parent.keys[i] > key) {
            parent.keys[i + 1] = parent.keys[i];
            parent.children[i + 2] = parent.children[i + 1];
            i--;
        }
        parent.keys[i + 1] = key;
        parent.children[i + 2] = right_offset;
        parent.size++;
        write_node(parent_offset, parent);

        Node right;
        read_node(right_offset, right);
        right.parent = parent_offset;
        write_node(right_offset, right);

        if (parent.size == M) {
            split_internal(parent_offset, parent);
        }
    }

    void split_internal(long offset, Node& node) {
        Node newNode;
        newNode.is_leaf = false;
        newNode.size = M - M / 2 - 1;
        Key midKey = node.keys[M / 2];
        for (int i = 0; i < newNode.size; ++i) {
            newNode.keys[i] = node.keys[i + M / 2 + 1];
            newNode.children[i] = node.children[i + M / 2 + 1];
        }
        newNode.children[newNode.size] = node.children[M];
        node.size = M / 2;
        long newNodeOffset = allocate_node();
        newNode.offset = newNodeOffset;
        newNode.parent = node.parent;

        for (int i = 0; i <= newNode.size; ++i) {
            Node child;
            read_node(newNode.children[i], child);
            child.parent = newNodeOffset;
            write_node(newNode.children[i], child);
        }

        write_node(offset, node);
        write_node(newNodeOffset, newNode);

        insert_into_parent(offset, midKey, newNodeOffset, node.parent);
    }

    bool find(const Key& key, Value& value) {
        if (root_offset == -1) return false;
        long curr_offset = root_offset;
        Node curr;
        while (true) {
            read_node(curr_offset, curr);
            if (curr.is_leaf) break;
            int i = 0;
            while (i < curr.size && key >= curr.keys[i]) i++;
            curr_offset = curr.children[i];
        }
        for (int i = 0; i < curr.size; ++i) {
            if (curr.keys[i] == key) {
                value = curr.values[i];
                return true;
            }
        }
        return false;
    }

    void update(const Key& key, const Value& value) {
        if (root_offset == -1) return;
        long curr_offset = root_offset;
        Node curr;
        while (true) {
            read_node(curr_offset, curr);
            if (curr.is_leaf) break;
            int i = 0;
            while (i < curr.size && key >= curr.keys[i]) i++;
            curr_offset = curr.children[i];
        }
        for (int i = 0; i < curr.size; ++i) {
            if (curr.keys[i] == key) {
                curr.values[i] = value;
                write_node(curr_offset, curr);
                return;
            }
        }
    }

    void erase(const Key& key) {
        // Simplified erase: just mark as deleted or remove from leaf
        // For this problem, we might not need full B+ tree deletion with merging
        // But let's at least remove it from the leaf.
        if (root_offset == -1) return;
        long curr_offset = root_offset;
        Node curr;
        while (true) {
            read_node(curr_offset, curr);
            if (curr.is_leaf) break;
            int i = 0;
            while (i < curr.size && key >= curr.keys[i]) i++;
            curr_offset = curr.children[i];
        }
        for (int i = 0; i < curr.size; ++i) {
            if (curr.keys[i] == key) {
                for (int j = i; j < curr.size - 1; ++j) {
                    curr.keys[j] = curr.keys[j + 1];
                    curr.values[j] = curr.values[j + 1];
                }
                curr.size--;
                write_node(curr_offset, curr);
                return;
            }
        }
    }

    // For range queries
    void find_all(std::vector<Value>& results) {
        if (root_offset == -1) return;
        long curr_offset = root_offset;
        Node curr;
        while (true) {
            read_node(curr_offset, curr);
            if (curr.is_leaf) break;
            curr_offset = curr.children[0];
        }
        while (curr_offset != -1) {
            read_node(curr_offset, curr);
            for (int i = 0; i < curr.size; ++i) {
                results.push_back(curr.values[i]);
            }
            curr_offset = curr.next;
        }
    }

    void find_range(const Key& start_key, const Key& end_key, std::vector<Value>& results) {
        if (root_offset == -1) return;
        long curr_offset = root_offset;
        Node curr;
        while (true) {
            read_node(curr_offset, curr);
            if (curr.is_leaf) break;
            int i = 0;
            while (i < curr.size && start_key >= curr.keys[i]) i++;
            curr_offset = curr.children[i];
        }
        bool finished = false;
        while (curr_offset != -1 && !finished) {
            read_node(curr_offset, curr);
            for (int i = 0; i < curr.size; ++i) {
                if (curr.keys[i] > end_key) {
                    finished = true;
                    break;
                }
                if (curr.keys[i] >= start_key) {
                    results.push_back(curr.values[i]);
                }
            }
            curr_offset = curr.next;
        }
    }
    
    // For prefix search (e.g. name="...")
    void find_prefix(const Key& prefix, std::vector<Value>& results) {
        if (root_offset == -1) return;
        long curr_offset = root_offset;
        Node curr;
        while (true) {
            read_node(curr_offset, curr);
            if (curr.is_leaf) break;
            int i = 0;
            while (i < curr.size && prefix >= curr.keys[i]) i++;
            curr_offset = curr.children[i];
        }
        bool finished = false;
        while (curr_offset != -1 && !finished) {
            read_node(curr_offset, curr);
            for (int i = 0; i < curr.size; ++i) {
                if (!is_prefix(prefix, curr.keys[i])) {
                    if (curr.keys[i] > prefix) {
                        finished = true;
                        break;
                    }
                    continue;
                }
                results.push_back(curr.values[i]);
            }
            curr_offset = curr.next;
        }
    }

    bool is_prefix(const Key& prefix, const Key& key) {
        return strncmp(prefix.key, key.key, strlen(prefix.key)) == 0;
    }
};

#endif



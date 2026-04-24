


#ifndef MODELS_HPP
#define MODELS_HPP

#include <cstring>
#include <string>

struct Account {
    char user_id[31];
    char password[31];
    char username[31];
    int privilege;

    Account() {
        memset(user_id, 0, sizeof(user_id));
        memset(password, 0, sizeof(password));
        memset(username, 0, sizeof(username));
        privilege = 0;
    }

    Account(const char* id, const char* pwd, const char* name, int priv) {
        memset(user_id, 0, sizeof(user_id));
        memset(password, 0, sizeof(password));
        memset(username, 0, sizeof(username));
        strncpy(user_id, id, 30);
        strncpy(password, pwd, 30);
        strncpy(username, name, 30);
        privilege = priv;
    }
};

struct Book {
    char isbn[21];
    char name[61];
    char author[61];
    char keyword[61];
    double price;
    long long stock;

    Book() {
        memset(isbn, 0, sizeof(isbn));
        memset(name, 0, sizeof(name));
        memset(author, 0, sizeof(author));
        memset(keyword, 0, sizeof(keyword));
        price = 0.0;
        stock = 0;
    }

    Book(const char* i) {
        memset(isbn, 0, sizeof(isbn));
        memset(name, 0, sizeof(name));
        memset(author, 0, sizeof(author));
        memset(keyword, 0, sizeof(keyword));
        strncpy(isbn, i, 20);
        price = 0.0;
        stock = 0;
    }
};

struct StringKey {
    char key[65];
    StringKey() { memset(key, 0, sizeof(key)); }
    StringKey(const char* s) {
        memset(key, 0, sizeof(key));
        strncpy(key, s, 64);
    }
    StringKey(const std::string& s) {
        memset(key, 0, sizeof(key));
        strncpy(key, s.c_str(), 64);
    }
    bool operator<(const StringKey& other) const { return strcmp(key, other.key) < 0; }
    bool operator>(const StringKey& other) const { return strcmp(key, other.key) > 0; }
    bool operator<=(const StringKey& other) const { return strcmp(key, other.key) <= 0; }
    bool operator>=(const StringKey& other) const { return strcmp(key, other.key) >= 0; }
    bool operator==(const StringKey& other) const { return strcmp(key, other.key) == 0; }
};

// For non-unique indices, we use a pair of (Key, ISBN)
struct IndexKey {
    char key[65];
    char isbn[21];
    IndexKey() {
        memset(key, 0, sizeof(key));
        memset(isbn, 0, sizeof(isbn));
    }
    IndexKey(const char* k, const char* i) {
        memset(key, 0, sizeof(key));
        memset(isbn, 0, sizeof(isbn));
        strncpy(key, k, 64);
        strncpy(isbn, i, 20);
    }
    bool operator<(const IndexKey& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp != 0) return cmp < 0;
        return strcmp(isbn, other.isbn) < 0;
    }
    bool operator>(const IndexKey& other) const {
        int cmp = strcmp(key, other.key);
        if (cmp != 0) return cmp > 0;
        return strcmp(isbn, other.isbn) > 0;
    }
    bool operator<=(const IndexKey& other) const { return !(*this > other); }
    bool operator>=(const IndexKey& other) const { return !(*this < other); }
    bool operator==(const IndexKey& other) const {
        return strcmp(key, other.key) == 0 && strcmp(isbn, other.isbn) == 0;
    }
};

#endif







#ifndef SYSTEMS_HPP
#define SYSTEMS_HPP

#include "database.hpp"
#include "models.hpp"
#include <stack>
#include <vector>
#include <string>
#include <iostream>
#include <iomanip>

class AccountSystem {
private:
    BPlusTree<StringKey, Account> accounts;
    struct LoginInfo {
        Account account;
        char selected_isbn[21];
    };
    std::vector<LoginInfo> login_stack;

public:
    AccountSystem() : accounts("accounts.dat") {
        Account root;
        if (!accounts.find(StringKey("root"), root)) {
            accounts.insert(StringKey("root"), Account("root", "sjtu", "root", 7));
        }
    }

    int get_current_privilege() {
        if (login_stack.empty()) return 0;
        return login_stack.back().account.privilege;
    }

    bool su(const std::string& user_id, const std::string& password = "") {
        Account acc;
        if (!accounts.find(StringKey(user_id), acc)) return false;
        if (password.empty()) {
            if (get_current_privilege() > acc.privilege) {
                LoginInfo info;
                info.account = acc;
                memset(info.selected_isbn, 0, sizeof(info.selected_isbn));
                login_stack.push_back(info);
                return true;
            }
            return false;
        } else {
            if (password == acc.password) {
                LoginInfo info;
                info.account = acc;
                memset(info.selected_isbn, 0, sizeof(info.selected_isbn));
                login_stack.push_back(info);
                return true;
            }
            return false;
        }
    }

    bool logout() {
        if (login_stack.empty()) return false;
        login_stack.pop_back();
        return true;
    }

    bool register_user(const std::string& user_id, const std::string& password, const std::string& username) {
        Account acc;
        if (accounts.find(StringKey(user_id), acc)) return false;
        accounts.insert(StringKey(user_id), Account(user_id.c_str(), password.c_str(), username.c_str(), 1));
        return true;
    }

    bool passwd(const std::string& user_id, const std::string& new_password, const std::string& current_password = "") {
        Account acc;
        if (!accounts.find(StringKey(user_id), acc)) return false;
        if (get_current_privilege() == 7) {
            strncpy(acc.password, new_password.c_str(), 30);
            accounts.update(StringKey(user_id), acc);
            return true;
        } else {
            if (current_password == acc.password) {
                strncpy(acc.password, new_password.c_str(), 30);
                accounts.update(StringKey(user_id), acc);
                return true;
            }
            return false;
        }
    }

    bool useradd(const std::string& user_id, const std::string& password, int privilege, const std::string& username) {
        if (privilege >= get_current_privilege()) return false;
        if (privilege != 1 && privilege != 3 && privilege != 7) return false;
        Account acc;
        if (accounts.find(StringKey(user_id), acc)) return false;
        accounts.insert(StringKey(user_id), Account(user_id.c_str(), password.c_str(), username.c_str(), privilege));
        return true;
    }

    bool delete_user(const std::string& user_id) {
        Account acc;
        if (!accounts.find(StringKey(user_id), acc)) return false;
        for (const auto& info : login_stack) {
            if (strcmp(info.account.user_id, user_id.c_str()) == 0) return false;
        }
        accounts.erase(StringKey(user_id));
        return true;
    }

    void select_book(const char* isbn) {
        if (login_stack.empty()) return;
        strncpy(login_stack.back().selected_isbn, isbn, 20);
    }

    char* get_selected_isbn() {
        if (login_stack.empty()) return nullptr;
        if (login_stack.back().selected_isbn[0] == 0) return nullptr;
        return login_stack.back().selected_isbn;
    }
    
    void update_selected_isbn(const char* new_isbn) {
        if (login_stack.empty()) return;
        // When a book's ISBN is modified, all login sessions selecting this book should be updated?
        // The requirement says: "no need to save selected book information after account logout"
        // And "select [ISBN] ... The current account selects the specified book"
        // It doesn't explicitly say what happens if the ISBN changes.
        // Usually, it's the ISBN that is selected. If the ISBN of the selected book changes, 
        // the selection might still point to the old ISBN which might not exist anymore.
        // Wait, "modify -ISBN=[ISBN]" changes the ISBN of the selected book.
        // Let's re-read: "Operation fails if no book is selected... Cannot change ISBN to another ISBN already existing"
        // If we change the ISBN of the book we currently have selected, our selection should probably update to the new ISBN.
        // But only for the CURRENT account.
        strncpy(login_stack.back().selected_isbn, new_isbn, 20);
    }
};

class BookSystem {
private:
    BPlusTree<StringKey, Book> books;
    BPlusTree<IndexKey, StringKey> name_index;
    BPlusTree<IndexKey, StringKey> author_index;
    BPlusTree<IndexKey, StringKey> keyword_index;

    struct FinanceRecord {
        double income;
        double expenditure;
    };
    std::vector<FinanceRecord> finance_log;
    // For persistence of finance log, we should use a file.
    MemoryRiver<FinanceRecord> finance_file;
    int finance_count;

public:
    BookSystem() : books("books.dat"), name_index("name_index.dat"), 
                   author_index("author_index.dat"), keyword_index("keyword_index.dat"),
                   finance_file("finance.dat") {
        finance_file.get_info(finance_count, 0);
        if (finance_count < 0) finance_count = 0; // Handle first run
    }
    
    ~BookSystem() {
        finance_file.write_info(finance_count, 0);
    }

    void add_finance(double income, double expenditure) {
        FinanceRecord rec = {income, expenditure};
        finance_file.write(rec, sizeof(int) + finance_count * sizeof(FinanceRecord));
        finance_count++;
    }

    void show_finance(int count = -1) {
        if (count == 0) {
            std::cout << "\n";
            return;
        }
        if (count == -1 || count > finance_count) count = finance_count;
        if (count < 0) {
            std::cout << "Invalid\n";
            return;
        }

        double total_income = 0, total_expenditure = 0;
        for (int i = finance_count - count; i < finance_count; ++i) {
            FinanceRecord rec;
            finance_file.read(rec, sizeof(int) + i * sizeof(FinanceRecord));
            total_income += rec.income;
            total_expenditure += rec.expenditure;
        }
        std::cout << std::fixed << std::setprecision(2) << "+ " << total_income << " - " << total_expenditure << "\n";
    }

    bool buy(const std::string& isbn, int quantity) {
        Book book;
        if (!books.find(StringKey(isbn), book)) return false;
        if (quantity <= 0 || book.stock < quantity) return false;
        book.stock -= quantity;
        books.update(StringKey(isbn), book);
        double total_price = book.price * quantity;
        add_finance(total_price, 0);
        std::cout << std::fixed << std::setprecision(2) << total_price << "\n";
        return true;
    }

    void select(const std::string& isbn, AccountSystem& accounts) {
        Book book;
        if (!books.find(StringKey(isbn), book)) {
            book = Book(isbn.c_str());
            books.insert(StringKey(isbn), book);
        }
        accounts.select_book(isbn.c_str());
    }

    bool modify(AccountSystem& accounts, const std::vector<std::pair<std::string, std::string>>& mods) {
        char* selected_isbn = accounts.get_selected_isbn();
        if (!selected_isbn) return false;
        Book book;
        books.find(StringKey(selected_isbn), book);

        std::string new_isbn = book.isbn;
        std::string new_name = book.name;
        std::string new_author = book.author;
        std::string new_keyword = book.keyword;
        double new_price = book.price;

        bool isbn_changed = false, name_changed = false, author_changed = false, keyword_changed = false, price_changed = false;

        for (const auto& mod : mods) {
            if (mod.first == "ISBN") {
                if (isbn_changed || mod.second == book.isbn) return false;
                Book tmp;
                if (books.find(StringKey(mod.second), tmp)) return false;
                new_isbn = mod.second;
                isbn_changed = true;
            } else if (mod.first == "name") {
                if (name_changed) return false;
                new_name = mod.second;
                name_changed = true;
            } else if (mod.first == "author") {
                if (author_changed) return false;
                new_author = mod.second;
                author_changed = true;
            } else if (mod.first == "keyword") {
                if (keyword_changed) return false;
                // Check for duplicate keywords
                std::vector<std::string> kws;
                std::stringstream ss(mod.second);
                std::string kw;
                while (std::getline(ss, kw, '|')) {
                    if (kw.empty()) return false;
                    for (const auto& k : kws) if (k == kw) return false;
                    kws.push_back(kw);
                }
                new_keyword = mod.second;
                keyword_changed = true;
            } else if (mod.first == "price") {
                if (price_changed) return false;
                new_price = std::stod(mod.second);
                price_changed = true;
            }
        }

        // Remove old indices
        if (book.name[0]) name_index.erase(IndexKey(book.name, book.isbn));
        if (book.author[0]) author_index.erase(IndexKey(book.author, book.isbn));
        if (book.keyword[0]) {
            std::stringstream ss(book.keyword);
            std::string kw;
            while (std::getline(ss, kw, '|')) {
                keyword_index.erase(IndexKey(kw.c_str(), book.isbn));
            }
        }
        books.erase(StringKey(book.isbn));

        // Update book
        strncpy(book.isbn, new_isbn.c_str(), 20);
        strncpy(book.name, new_name.c_str(), 60);
        strncpy(book.author, new_author.c_str(), 60);
        strncpy(book.keyword, new_keyword.c_str(), 60);
        book.price = new_price;

        // Add new indices
        books.insert(StringKey(book.isbn), book);
        if (book.name[0]) name_index.insert(IndexKey(book.name, book.isbn), StringKey(book.isbn));
        if (book.author[0]) author_index.insert(IndexKey(book.author, book.isbn), StringKey(book.isbn));
        if (book.keyword[0]) {
            std::stringstream ss(book.keyword);
            std::string kw;
            while (std::getline(ss, kw, '|')) {
                keyword_index.insert(IndexKey(kw.c_str(), book.isbn), StringKey(book.isbn));
            }
        }

        if (isbn_changed) accounts.update_selected_isbn(book.isbn);
        return true;
    }

    bool import(AccountSystem& accounts, int quantity, double total_cost) {
        char* selected_isbn = accounts.get_selected_isbn();
        if (!selected_isbn) return false;
        if (quantity <= 0 || total_cost <= 0) return false;
        Book book;
        books.find(StringKey(selected_isbn), book);
        book.stock += quantity;
        books.update(StringKey(selected_isbn), book);
        add_finance(0, total_cost);
        return true;
    }

    void show(const std::string& type = "", const std::string& value = "") {
        std::vector<Book> results;
        if (type == "") {
            std::vector<Book> all_books;
            books.find_all(all_books);
            results = all_books;
        } else if (type == "ISBN") {
            Book book;
            if (books.find(StringKey(value), book)) results.push_back(book);
        } else if (type == "name") {
            std::vector<StringKey> isbns;
            name_index.find_range(IndexKey(value.c_str(), ""), IndexKey(value.c_str(), "\xff"), isbns);
            for (const auto& isbn : isbns) {
                Book book;
                books.find(isbn, book);
                results.push_back(book);
            }
        } else if (type == "author") {
            std::vector<StringKey> isbns;
            author_index.find_range(IndexKey(value.c_str(), ""), IndexKey(value.c_str(), "\xff"), isbns);
            for (const auto& isbn : isbns) {
                Book book;
                books.find(isbn, book);
                results.push_back(book);
            }
        } else if (type == "keyword") {
            if (value.find('|') != std::string::npos) {
                std::cout << "Invalid\n";
                return;
            }
            std::vector<StringKey> isbns;
            keyword_index.find_range(IndexKey(value.c_str(), ""), IndexKey(value.c_str(), "\xff"), isbns);
            for (const auto& isbn : isbns) {
                Book book;
                books.find(isbn, book);
                results.push_back(book);
            }
        }

        if (results.empty()) {
            std::cout << "\n";
        } else {
            std::sort(results.begin(), results.end(), [](const Book& a, const Book& b) {
                return strcmp(a.isbn, b.isbn) < 0;
            });
            for (const auto& book : results) {
                std::cout << book.isbn << "\t" << book.name << "\t" << book.author << "\t" << book.keyword << "\t" 
                          << std::fixed << std::setprecision(2) << book.price << "\t" << book.stock << "\n";
            }
        }
    }
};

#endif




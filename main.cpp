

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include "src/systems.hpp"

std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (tokenStream >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

bool is_valid_id_pwd(const std::string& s) {
    if (s.length() > 30) return false;
    for (char c : s) {
        if (!isalnum(c) && c != '_') return false;
    }
    return true;
}

bool is_valid_username(const std::string& s) {
    if (s.length() > 30) return false;
    for (char c : s) {
        if (c < 33 || c > 126) return false;
    }
    return true;
}

int main() {
    AccountSystem account_system;
    BookSystem book_system;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        std::vector<std::string> tokens = split(line);
        if (tokens.empty()) continue;

        std::string cmd = tokens[0];
        if (cmd == "quit" || cmd == "exit") {
            if (tokens.size() != 1) std::cout << "Invalid\n";
            else break;
        } else if (cmd == "su") {
            if (tokens.size() == 2) {
                if (!is_valid_id_pwd(tokens[1]) || !account_system.su(tokens[1])) std::cout << "Invalid\n";
            } else if (tokens.size() == 3) {
                if (!is_valid_id_pwd(tokens[1]) || !is_valid_id_pwd(tokens[2]) || !account_system.su(tokens[1], tokens[2])) std::cout << "Invalid\n";
            } else std::cout << "Invalid\n";
        } else if (cmd == "logout") {
            if (tokens.size() != 1 || account_system.get_current_privilege() < 1 || !account_system.logout()) std::cout << "Invalid\n";
        } else if (cmd == "register") {
            if (tokens.size() == 4) {
                if (!is_valid_id_pwd(tokens[1]) || !is_valid_id_pwd(tokens[2]) || !is_valid_username(tokens[3]) || !account_system.register_user(tokens[1], tokens[2], tokens[3])) std::cout << "Invalid\n";
            } else std::cout << "Invalid\n";
        } else if (cmd == "passwd") {
            if (tokens.size() == 3) {
                if (account_system.get_current_privilege() < 1 || !is_valid_id_pwd(tokens[1]) || !is_valid_id_pwd(tokens[2]) || !account_system.passwd(tokens[1], tokens[2])) std::cout << "Invalid\n";
            } else if (tokens.size() == 4) {
                if (account_system.get_current_privilege() < 1 || !is_valid_id_pwd(tokens[1]) || !is_valid_id_pwd(tokens[2]) || !is_valid_id_pwd(tokens[3]) || !account_system.passwd(tokens[1], tokens[3], tokens[2])) std::cout << "Invalid\n";
            } else std::cout << "Invalid\n";
        } else if (cmd == "useradd") {
            if (tokens.size() == 5) {
                int priv = 0;
                try { priv = std::stoi(tokens[3]); } catch (...) { std::cout << "Invalid\n"; continue; }
                if (account_system.get_current_privilege() < 3 || !is_valid_id_pwd(tokens[1]) || !is_valid_id_pwd(tokens[2]) || !is_valid_username(tokens[4]) || !account_system.useradd(tokens[1], tokens[2], priv, tokens[4])) std::cout << "Invalid\n";
            } else std::cout << "Invalid\n";
        } else if (cmd == "delete") {
            if (tokens.size() == 2) {
                if (account_system.get_current_privilege() < 7 || !is_valid_id_pwd(tokens[1]) || !account_system.delete_user(tokens[1])) std::cout << "Invalid\n";
            } else std::cout << "Invalid\n";
        } else if (cmd == "show") {
            if (tokens.size() >= 2 && tokens[1] == "finance") {
                if (account_system.get_current_privilege() < 7) std::cout << "Invalid\n";
                else if (tokens.size() == 2) book_system.show_finance();
                else if (tokens.size() == 3) {
                    int count = 0;
                    try { count = std::stoi(tokens[2]); } catch (...) { std::cout << "Invalid\n"; continue; }
                    book_system.show_finance(count);
                } else std::cout << "Invalid\n";
            } else {
                if (account_system.get_current_privilege() < 1) std::cout << "Invalid\n";
                else if (tokens.size() == 1) book_system.show();
                else if (tokens.size() == 2) {
                    if (tokens[1].substr(0, 6) == "-ISBN=") book_system.show("ISBN", tokens[1].substr(6));
                    else if (tokens[1].substr(0, 7) == "-name=\"") {
                        if (tokens[1].back() != '"') std::cout << "Invalid\n";
                        else book_system.show("name", tokens[1].substr(7, tokens[1].length() - 8));
                    } else if (tokens[1].substr(0, 9) == "-author=\"") {
                        if (tokens[1].back() != '"') std::cout << "Invalid\n";
                        else book_system.show("author", tokens[1].substr(9, tokens[1].length() - 10));
                    } else if (tokens[1].substr(0, 10) == "-keyword=\"") {
                        if (tokens[1].back() != '"') std::cout << "Invalid\n";
                        else book_system.show("keyword", tokens[1].substr(10, tokens[1].length() - 11));
                    } else std::cout << "Invalid\n";
                } else std::cout << "Invalid\n";
            }
        } else if (cmd == "buy") {
            if (tokens.size() == 3) {
                int qty = 0;
                try { qty = std::stoi(tokens[2]); } catch (...) { std::cout << "Invalid\n"; continue; }
                if (account_system.get_current_privilege() < 1 || !book_system.buy(tokens[1], qty)) std::cout << "Invalid\n";
            } else std::cout << "Invalid\n";
        } else if (cmd == "select") {
            if (tokens.size() == 2) {
                if (account_system.get_current_privilege() < 3) std::cout << "Invalid\n";
                else book_system.select(tokens[1], account_system);
            } else std::cout << "Invalid\n";
        } else if (cmd == "modify") {
            if (account_system.get_current_privilege() < 3 || tokens.size() < 2) std::cout << "Invalid\n";
            else {
                std::vector<std::pair<std::string, std::string>> mods;
                bool ok = true;
                for (size_t i = 1; i < tokens.size(); ++i) {
                    if (tokens[i].substr(0, 6) == "-ISBN=") mods.push_back({"ISBN", tokens[i].substr(6)});
                    else if (tokens[i].substr(0, 7) == "-name=\"") {
                        if (tokens[i].back() != '"') { ok = false; break; }
                        mods.push_back({"name", tokens[i].substr(7, tokens[i].length() - 8)});
                    } else if (tokens[i].substr(0, 9) == "-author=\"") {
                        if (tokens[i].back() != '"') { ok = false; break; }
                        mods.push_back({"author", tokens[i].substr(9, tokens[i].length() - 10)});
                    } else if (tokens[i].substr(0, 10) == "-keyword=\"") {
                        if (tokens[i].back() != '"') { ok = false; break; }
                        mods.push_back({"keyword", tokens[i].substr(10, tokens[i].length() - 11)});
                    } else if (tokens[i].substr(0, 7) == "-price=") {
                        mods.push_back({"price", tokens[i].substr(7)});
                    } else { ok = false; break; }
                }
                if (!ok || !book_system.modify(account_system, mods)) std::cout << "Invalid\n";
            }
        } else if (cmd == "import") {
            if (tokens.size() == 3) {
                int qty = 0;
                double cost = 0;
                try { qty = std::stoi(tokens[1]); cost = std::stod(tokens[2]); } catch (...) { std::cout << "Invalid\n"; continue; }
                if (account_system.get_current_privilege() < 3 || !book_system.import(account_system, qty, cost)) std::cout << "Invalid\n";
            } else std::cout << "Invalid\n";
        } else if (cmd == "log" || cmd == "report") {
            // Optional commands, but let's at least handle the privilege check
            if (account_system.get_current_privilege() < 7) std::cout << "Invalid\n";
            // For now, we don't implement these fully as they are not strictly required for basic functionality
        } else {
            std::cout << "Invalid\n";
        }
    }
    return 0;
}


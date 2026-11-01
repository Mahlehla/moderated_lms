#pragma once

#include <sqlite3.h>
#include <string>

class Database {
private:
    sqlite3* db_{};

public:
    explicit Database(const std::string& path);

    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    sqlite3* handle() const {
        return db_;
    }

    void initialise();
};

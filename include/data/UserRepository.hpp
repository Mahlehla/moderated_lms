#pragma once

#include "data/Database.hpp"
#include "domain/User.hpp"

#include <memory>
#include <string>

class UserRepository {
private:
    Database& database_;

public:
    explicit UserRepository(Database& database)
        : database_(database) {}

    void ensureDemoUser();

    std::unique_ptr<User> findByUsername(const std::string& username);

    void updatePasswordHash(
        const std::string& username,
        const std::string& passwordHash
    );
};

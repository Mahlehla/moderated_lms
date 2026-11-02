#pragma once

#include "data/Database.hpp"
#include "domain/User.hpp"

#include <memory>
#include <string>
#include <vector>

class UserRepository {
private:
    Database& database_;

public:
    explicit UserRepository(Database& database)
        : database_(database) {}

    // Development/test helper retained for existing tests.
    void ensureDemoUser();

    // Account management.
    int countUsers();

    int createUser(
        const std::string& username,
        const std::string& password,
        const std::string& fullName,
        Role role
    );

    std::unique_ptr<User> findByUsername(
        const std::string& username
    );

    std::vector<std::unique_ptr<User>> findAll();

    void updatePasswordHash(
        const std::string& username,
        const std::string& passwordHash
    );
};

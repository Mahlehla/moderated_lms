#pragma once

#include "data/UserRepository.hpp"

class AuthenticationService {
private:
    UserRepository& users_;

public:
    explicit AuthenticationService(
        UserRepository& users
    )
        : users_(users) {}

    std::unique_ptr<User>
    login(
        const std::string& username,
        const std::string& password
    );
};

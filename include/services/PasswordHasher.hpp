#pragma once

#include <string>

class PasswordHasher {
public:
    static std::string hash(const std::string& password);
    static bool verify(
        const std::string& password,
        const std::string& storedHash
    );
    static bool isHashed(const std::string& storedHash);
};

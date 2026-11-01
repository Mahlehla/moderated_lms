#include "services/PasswordHasher.hpp"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    constexpr int ITERATIONS = 310000;
    constexpr int SALT_BYTES = 16;
    constexpr int HASH_BYTES = 32;

    std::string toHex(const unsigned char* data, std::size_t length) {
        std::ostringstream output;
        output << std::hex << std::setfill('0');

        for (std::size_t i = 0; i < length; ++i) {
            output << std::setw(2) << static_cast<unsigned int>(data[i]);
        }

        return output.str();
    }

    std::vector<unsigned char> fromHex(const std::string& value) {
        if (value.size() % 2 != 0) {
            throw std::invalid_argument("Invalid hexadecimal value.");
        }

        std::vector<unsigned char> result;
        result.reserve(value.size() / 2);

        for (std::size_t i = 0; i < value.size(); i += 2) {
            const std::string byteString = value.substr(i, 2);
            std::size_t consumed = 0;
            const unsigned long byte = std::stoul(byteString, &consumed, 16);

            if (consumed != 2 || byte > 255) {
                throw std::invalid_argument("Invalid hexadecimal value.");
            }

            result.push_back(static_cast<unsigned char>(byte));
        }

        return result;
    }

    std::vector<unsigned char> derive(
        const std::string& password,
        const std::vector<unsigned char>& salt,
        int iterations,
        int outputBytes
    ) {
        std::vector<unsigned char> result(
            static_cast<std::size_t>(outputBytes)
        );

        if (PKCS5_PBKDF2_HMAC(
                password.c_str(),
                static_cast<int>(password.size()),
                salt.data(),
                static_cast<int>(salt.size()),
                iterations,
                EVP_sha256(),
                outputBytes,
                result.data()
            ) != 1) {
            throw std::runtime_error("PBKDF2 password derivation failed.");
        }

        return result;
    }
}

bool PasswordHasher::isHashed(const std::string& storedHash) {
    return storedHash.rfind("pbkdf2_sha256$", 0) == 0;
}

std::string PasswordHasher::hash(const std::string& password) {
    if (password.empty()) {
        throw std::invalid_argument("Password cannot be empty.");
    }

    std::array<unsigned char, SALT_BYTES> salt{};

    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) {
        throw std::runtime_error("Secure random salt generation failed.");
    }

    const std::vector<unsigned char> saltVector(
        salt.begin(), salt.end()
    );

    const auto derived = derive(
        password, saltVector, ITERATIONS, HASH_BYTES
    );

    return "pbkdf2_sha256$" +
           std::to_string(ITERATIONS) + "$" +
           toHex(salt.data(), salt.size()) + "$" +
           toHex(derived.data(), derived.size());
}

bool PasswordHasher::verify(
    const std::string& password,
    const std::string& storedHash
) {
    if (!isHashed(storedHash)) {
        // Legacy plaintext records are accepted only for migration
        // after a successful exact password comparison.
        return storedHash == password;
    }

    try {
        const std::string prefix = "pbkdf2_sha256$";
        const std::size_t first = storedHash.find('$', prefix.size());
        const std::size_t second =
            first == std::string::npos
                ? std::string::npos
                : storedHash.find('$', first + 1);

        if (first == std::string::npos || second == std::string::npos) {
            return false;
        }

        const int iterations = std::stoi(
            storedHash.substr(prefix.size(), first - prefix.size())
        );

        if (iterations < 100000 || iterations > 2000000) {
            return false;
        }

        const auto salt = fromHex(
            storedHash.substr(first + 1, second - first - 1)
        );
        const auto expected = fromHex(storedHash.substr(second + 1));

        if (salt.size() != SALT_BYTES || expected.size() != HASH_BYTES) {
            return false;
        }

        const auto actual = derive(
            password, salt, iterations, static_cast<int>(expected.size())
        );

        return CRYPTO_memcmp(
            actual.data(), expected.data(), expected.size()
        ) == 0;
    } catch (...) {
        return false;
    }
}

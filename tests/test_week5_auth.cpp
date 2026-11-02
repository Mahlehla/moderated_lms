#include "data/Database.hpp"
#include "data/UserRepository.hpp"
#include "services/AuthenticationService.hpp"
#include "services/PasswordHasher.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <sqlite3.h>
#include <stdexcept>
#include <string>

int main() {
    const char* path = "/tmp/moderated_lms_auth_test.db";
    std::remove(path);

    {
        Database database(path);
        database.initialise();

        UserRepository users(database);
        AuthenticationService authentication(users);

        // New demo account must be stored as a hash.
        users.ensureDemoUser();

        auto admin = users.findByUsername("admin");
        assert(admin != nullptr);
        assert(PasswordHasher::isHashed(admin->passwordHash()));
                // Account management tests.
        assert(users.countUsers() == 1);

        const int lecturerId = users.createUser(
            "lecturer01",
            "LecturerPassword2026!",
            "Test Lecturer",
            Role::Lecturer
        );

        assert(lecturerId > 0);
        assert(users.countUsers() == 2);

        auto lecturer = users.findByUsername("lecturer01");
        assert(lecturer != nullptr);
        assert(lecturer->role() == Role::Lecturer);
        assert(lecturer->fullName() == "Test Lecturer");
        assert(PasswordHasher::isHashed(lecturer->passwordHash()));
        assert(PasswordHasher::verify(
            "LecturerPassword2026!",
            lecturer->passwordHash()
        ));

        auto allUsers = users.findAll();
        assert(allUsers.size() == 2);

        // Invalid account details must be rejected.
        bool shortPasswordRejected = false;

        try {
            users.createUser(
                "student01",
                "short",
                "Test Student",
                Role::Student
            );
        } catch (const std::invalid_argument&) {
            shortPasswordRejected = true;
        }

        assert(shortPasswordRejected);
        assert(users.countUsers() == 2);

        bool invalidUsernameRejected = false;

        try {
            users.createUser(
                "bad username",
                "ValidPassword2026!",
                "Invalid User",
                Role::Student
            );
        } catch (const std::invalid_argument&) {
            invalidUsernameRejected = true;
        }

        assert(invalidUsernameRejected);
        assert(PasswordHasher::verify("admin123", admin->passwordHash()));
        assert(!PasswordHasher::verify("wrong-password", admin->passwordHash()));

        assert(authentication.login("admin", "admin123") != nullptr);
        assert(authentication.login("admin", "wrong-password") == nullptr);
        assert(authentication.login("missing-user", "anything") == nullptr);

        // Create a legacy plaintext account to test automatic migration.
        const char* insert =
            "INSERT INTO USER (username, password_hash, full_name, role) "
            "VALUES ('legacy_user', 'legacy123', 'Legacy User', 'STUDENT');";

        char* error = nullptr;
        if (sqlite3_exec(database.handle(), insert, nullptr, nullptr, &error)
            != SQLITE_OK) {
            std::string message = error ? error : "Unknown SQLite error";
            sqlite3_free(error);
            throw std::runtime_error(message);
        }

        // Wrong password must not upgrade the legacy record.
        assert(authentication.login("legacy_user", "incorrect") == nullptr);

        auto legacyBefore = users.findByUsername("legacy_user");
        assert(legacyBefore != nullptr);
        assert(legacyBefore->passwordHash() == "legacy123");

        // Correct password upgrades the record to PBKDF2.
        assert(authentication.login("legacy_user", "legacy123") != nullptr);

        auto legacyAfter = users.findByUsername("legacy_user");
        assert(legacyAfter != nullptr);
        assert(PasswordHasher::isHashed(legacyAfter->passwordHash()));
        assert(PasswordHasher::verify(
            "legacy123", legacyAfter->passwordHash()
        ));
    }

    std::remove(path);

    std::cout << "All Week 5 authentication tests passed.\n";
    return 0;
}

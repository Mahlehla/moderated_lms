#include "data/UserRepository.hpp"
#include "services/PasswordHasher.hpp"

#include <sqlite3.h>

#include <cctype>
#include <stdexcept>
#include <string>

namespace {

std::string columnText(
    sqlite3_stmt* statement,
    int column
)
{
    const auto* value = sqlite3_column_text(statement, column);

    return value
        ? reinterpret_cast<const char*>(value)
        : "";
}

Role parseRole(const std::string& role)
{
    if (role == "ADMINISTRATOR")
        return Role::Administrator;

    if (role == "LECTURER")
        return Role::Lecturer;

    if (role == "HOD")
        return Role::HeadOfDepartment;

    if (role == "STUDENT")
        return Role::Student;

    throw std::runtime_error(
        "Unknown user role in database: " + role
    );
}

void validateUsername(const std::string& username)
{
    if (username.size() < 3 || username.size() > 32) {
        throw std::invalid_argument(
            "Username must contain between 3 and 32 characters."
        );
    }

    for (unsigned char character : username) {
        if (!std::isalnum(character)
            && character != '_'
            && character != '-'
            && character != '.') {
            throw std::invalid_argument(
                "Username may contain only letters, numbers, "
                "underscores, hyphens and periods."
            );
        }
    }
}

void validateFullName(const std::string& fullName)
{
    if (fullName.empty()
        || fullName.find_first_not_of(" \t\r\n") == std::string::npos) {
        throw std::invalid_argument(
            "Full name cannot be empty."
        );
    }

    if (fullName.size() > 120) {
        throw std::invalid_argument(
            "Full name cannot exceed 120 characters."
        );
    }
}

void validatePassword(const std::string& password)
{
    if (password.size() < 12) {
        throw std::invalid_argument(
            "Password must contain at least 12 characters."
        );
    }

    if (password.size() > 1024) {
        throw std::invalid_argument(
            "Password is too long."
        );
    }
}

void validateRole(Role role)
{
    switch (role) {
        case Role::Administrator:
        case Role::Lecturer:
        case Role::HeadOfDepartment:
        case Role::Student:
            return;
    }

    throw std::invalid_argument("Invalid user role.");
}

std::unique_ptr<User> readUser(sqlite3_stmt* statement)
{
    return makeUser(
        sqlite3_column_int(statement, 0),
        columnText(statement, 1),
        columnText(statement, 2),
        columnText(statement, 3),
        parseRole(columnText(statement, 4))
    );
}

} // namespace

std::string roleToString(Role role)
{
    switch (role) {
        case Role::Administrator:
            return "ADMINISTRATOR";

        case Role::Lecturer:
            return "LECTURER";

        case Role::HeadOfDepartment:
            return "HOD";

        case Role::Student:
            return "STUDENT";
    }

    throw std::invalid_argument("Invalid user role.");
}

std::unique_ptr<User> makeUser(
    int id,
    const std::string& username,
    const std::string& passwordHash,
    const std::string& fullName,
    Role role
)
{
    switch (role) {
        case Role::Administrator:
            return std::make_unique<Administrator>(
                id, username, passwordHash, fullName, role
            );

        case Role::Lecturer:
            return std::make_unique<Lecturer>(
                id, username, passwordHash, fullName, role
            );

        case Role::HeadOfDepartment:
            return std::make_unique<HeadOfDepartment>(
                id, username, passwordHash, fullName, role
            );

        case Role::Student:
            return std::make_unique<Student>(
                id, username, passwordHash, fullName, role
            );
    }

    throw std::invalid_argument("Invalid user role.");
}

void UserRepository::ensureDemoUser()
{
    const char* query =
        "INSERT OR IGNORE INTO USER "
        "(username, password_hash, full_name, role) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(), query, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    const std::string passwordHash =
        PasswordHasher::hash("admin123");

    sqlite3_bind_text(
        statement, 1, "admin", -1, SQLITE_STATIC
    );
    sqlite3_bind_text(
        statement, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 3, "System Administrator", -1, SQLITE_STATIC
    );
    sqlite3_bind_text(
        statement, 4, "ADMINISTRATOR", -1, SQLITE_STATIC
    );

    if (sqlite3_step(statement) != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(database_.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
}

int UserRepository::countUsers()
{
    const char* query = "SELECT COUNT(*) FROM USER";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(), query, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    if (sqlite3_step(statement) != SQLITE_ROW) {
        const std::string error = sqlite3_errmsg(database_.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    const int count = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);

    return count;
}

int UserRepository::createUser(
    const std::string& username,
    const std::string& password,
    const std::string& fullName,
    Role role
)
{
    validateUsername(username);
    validatePassword(password);
    validateFullName(fullName);
    validateRole(role);

    const std::string passwordHash = PasswordHasher::hash(password);
    const std::string databaseRole = roleToString(role);

    const char* query =
        "INSERT INTO USER "
        "(username, password_hash, full_name, role) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(), query, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_text(
        statement, 1, username.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 2, passwordHash.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 3, fullName.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 4, databaseRole.c_str(), -1, SQLITE_TRANSIENT
    );

    if (sqlite3_step(statement) != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(database_.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    const int userId = static_cast<int>(sqlite3_last_insert_rowid(
        database_.handle()
    ));

    sqlite3_finalize(statement);

    return userId;
}

std::unique_ptr<User> UserRepository::findByUsername(
    const std::string& username
)
{
    const char* query =
        "SELECT id, username, password_hash, full_name, role "
        "FROM USER WHERE username = ?";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(), query, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_text(
        statement, 1, username.c_str(), -1, SQLITE_TRANSIENT
    );

    std::unique_ptr<User> result;

    const int step = sqlite3_step(statement);

    if (step == SQLITE_ROW) {
        result = readUser(statement);
    } else if (step != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(database_.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);

    return result;
}

std::vector<std::unique_ptr<User>> UserRepository::findAll()
{
    const char* query =
        "SELECT id, username, password_hash, full_name, role "
        "FROM USER ORDER BY id";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(), query, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    std::vector<std::unique_ptr<User>> users;

    while (true) {
        const int step = sqlite3_step(statement);

        if (step == SQLITE_DONE)
            break;

        if (step != SQLITE_ROW) {
            const std::string error = sqlite3_errmsg(database_.handle());
            sqlite3_finalize(statement);
            throw std::runtime_error(error);
        }

        users.push_back(readUser(statement));
    }

    sqlite3_finalize(statement);

    return users;
}

void UserRepository::updatePasswordHash(
    const std::string& username,
    const std::string& passwordHash
)
{
    const char* query =
        "UPDATE USER SET password_hash = ? WHERE username = ?";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(), query, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_text(
        statement, 1, passwordHash.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 2, username.c_str(), -1, SQLITE_TRANSIENT
    );

    if (sqlite3_step(statement) != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(database_.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
}

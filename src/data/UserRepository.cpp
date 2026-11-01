#include "data/UserRepository.hpp"
#include "services/PasswordHasher.hpp"

#include <sqlite3.h>
#include <stdexcept>

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

    return "";
}

static Role parseRole(const char* value)
{
    std::string role = value ? value : "";

    if (role == "ADMINISTRATOR")
        return Role::Administrator;

    if (role == "LECTURER")
        return Role::Lecturer;

    if (role == "HOD")
        return Role::HeadOfDepartment;

    return Role::Student;
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
                id,
                username,
                passwordHash,
                fullName,
                role
            );

        case Role::Lecturer:
            return std::make_unique<Lecturer>(
                id,
                username,
                passwordHash,
                fullName,
                role
            );

        case Role::HeadOfDepartment:
            return std::make_unique<HeadOfDepartment>(
                id,
                username,
                passwordHash,
                fullName,
                role
            );

        case Role::Student:
            return std::make_unique<Student>(
                id,
                username,
                passwordHash,
                fullName,
                role
            );
    }

    return nullptr;
}

void UserRepository::ensureDemoUser()
{
    const char* query =
        "INSERT OR IGNORE INTO USER "
        "(username, password_hash, full_name, role) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            query,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        throw std::runtime_error(
            sqlite3_errmsg(database_.handle())
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        "admin",
        -1,
        SQLITE_STATIC
    );

    sqlite3_bind_text(
        statement,
        2,
        PasswordHasher::hash("admin123").c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        3,
        "System Administrator",
        -1,
        SQLITE_STATIC
    );

    sqlite3_bind_text(
        statement,
        4,
        "ADMINISTRATOR",
        -1,
        SQLITE_STATIC
    );

    if (sqlite3_step(statement) != SQLITE_DONE) {

        sqlite3_finalize(statement);

        throw std::runtime_error(
            sqlite3_errmsg(database_.handle())
        );
    }

    sqlite3_finalize(statement);
}

std::unique_ptr<User>
UserRepository::findByUsername(
    const std::string& username
)
{
    const char* query =
        "SELECT "
        "id, username, password_hash, full_name, role "
        "FROM USER "
        "WHERE username = ?";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            query,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        throw std::runtime_error(
            sqlite3_errmsg(database_.handle())
        );
    }

    sqlite3_bind_text(
        statement,
        1,
        username.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    std::unique_ptr<User> result;

    if (sqlite3_step(statement) == SQLITE_ROW) {

        result = makeUser(
            sqlite3_column_int(statement, 0),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 1)
            ),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)
            ),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 3)
            ),

            parseRole(
                reinterpret_cast<const char*>(
                    sqlite3_column_text(statement, 4)
                )
            )
        );
    }

    sqlite3_finalize(statement);

    return result;
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

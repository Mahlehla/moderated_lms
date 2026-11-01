#include "data/AssignmentRepository.hpp"

#include <sqlite3.h>
#include <stdexcept>

int AssignmentRepository::create(
    int courseId,
    const std::string& title,
    const std::string& description,
    const std::string& deadline
)
{
    const char* sql =
        "INSERT INTO ASSIGNMENT "
        "(course_id, title, description, deadline) "
        "VALUES (?, ?, ?, ?)";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        throw std::runtime_error(
            sqlite3_errmsg(database_.handle())
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        courseId
    );

    sqlite3_bind_text(
        statement,
        2,
        title.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        3,
        description.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        4,
        deadline.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (sqlite3_step(statement) != SQLITE_DONE)
    {
        std::string error =
            sqlite3_errmsg(database_.handle());

        sqlite3_finalize(statement);

        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);

    return static_cast<int>(
        sqlite3_last_insert_rowid(
            database_.handle()
        )
    );
}

std::vector<Assignment>
AssignmentRepository::findByCourse(
    int courseId
)
{
    const char* sql =
        "SELECT "
        "id, course_id, title, description, "
        "deadline, released "
        "FROM ASSIGNMENT "
        "WHERE course_id = ? "
        "ORDER BY deadline";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK)
    {
        throw std::runtime_error(
            sqlite3_errmsg(database_.handle())
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        courseId
    );

    std::vector<Assignment> assignments;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        assignments.emplace_back(

            sqlite3_column_int(statement, 0),

            sqlite3_column_int(statement, 1),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)
            ),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 3)
            ),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 4)
            ),

            sqlite3_column_int(statement, 5) != 0
        );
    }

    sqlite3_finalize(statement);

    return assignments;
}

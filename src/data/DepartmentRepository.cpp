#include "data/DepartmentRepository.hpp"

#include <sqlite3.h>
#include <stdexcept>

int DepartmentRepository::create(
    const std::string& name
)
{
    const char* sql =
        "INSERT INTO DEPARTMENT (name) "
        "VALUES (?)";

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

    sqlite3_bind_text(
        statement,
        1,
        name.c_str(),
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

std::vector<Department>
DepartmentRepository::findAll()
{
    const char* sql =
        "SELECT id, name "
        "FROM DEPARTMENT "
        "ORDER BY name";

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

    std::vector<Department> departments;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        int id =
            sqlite3_column_int(statement, 0);

        const char* name =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 1)
            );

        departments.emplace_back(
            id,
            name ? name : ""
        );
    }

    sqlite3_finalize(statement);

    return departments;
}

#include "data/CourseRepository.hpp"

#include <sqlite3.h>
#include <stdexcept>

int CourseRepository::create(
    const std::string& code,
    const std::string& name,
    int departmentId
)
{
    const char* sql =
        "INSERT INTO COURSE "
        "(code, name, department_id) "
        "VALUES (?, ?, ?)";

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
        code.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        2,
        name.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_int(
        statement,
        3,
        departmentId
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

std::vector<Course>
CourseRepository::findAll()
{
    const char* sql =
        "SELECT id, code, name, department_id "
        "FROM COURSE "
        "ORDER BY code";

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

    std::vector<Course> courses;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        courses.emplace_back(
            sqlite3_column_int(statement, 0),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 1)
            ),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)
            ),

            sqlite3_column_int(statement, 3)
        );
    }

    sqlite3_finalize(statement);

    return courses;
}

std::vector<Course>
CourseRepository::findByDepartment(
    int departmentId
)
{
    const char* sql =
        "SELECT id, code, name, department_id "
        "FROM COURSE "
        "WHERE department_id = ? "
        "ORDER BY code";

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
        departmentId
    );

    std::vector<Course> courses;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        courses.emplace_back(
            sqlite3_column_int(statement, 0),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 1)
            ),

            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 2)
            ),

            sqlite3_column_int(statement, 3)
        );
    }

    sqlite3_finalize(statement);

    return courses;
}
std::vector<Course>
CourseRepository::findByLecturer(int lecturerId)
{
    if (lecturerId <= 0) {
        throw std::invalid_argument("Invalid lecturer ID.");
    }

    const char* sql =
        "SELECT c.id, c.code, c.name, c.department_id "
        "FROM COURSE c "
        "INNER JOIN LECTURER_COURSE lc "
        "ON lc.course_id = c.id "
        "WHERE lc.lecturer_id = ? "
        "ORDER BY c.code";

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

    sqlite3_bind_int(statement, 1, lecturerId);

    std::vector<Course> courses;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        const auto* code = sqlite3_column_text(statement, 1);
        const auto* name = sqlite3_column_text(statement, 2);

        courses.emplace_back(
            sqlite3_column_int(statement, 0),
            code ? reinterpret_cast<const char*>(code) : "",
            name ? reinterpret_cast<const char*>(name) : "",
            sqlite3_column_int(statement, 3)
        );
    }

    sqlite3_finalize(statement);

    return courses;
}

#include "data/EnrolmentRepository.hpp"

#include <sqlite3.h>
#include <stdexcept>

void EnrolmentRepository::enrolStudent(
    int studentId,
    int courseId
)
{
    const char* sql =
        "INSERT INTO ENROLMENT "
        "(student_id, course_id) "
        "VALUES (?, ?)";

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
        studentId
    );

    sqlite3_bind_int(
        statement,
        2,
        courseId
    );

    if (sqlite3_step(statement) != SQLITE_DONE)
    {
        std::string error =
            sqlite3_errmsg(database_.handle());

        sqlite3_finalize(statement);

        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
}

bool EnrolmentRepository::isEnrolled(
    int studentId,
    int courseId
) const
{
    const char* sql =
        "SELECT 1 "
        "FROM ENROLMENT "
        "WHERE student_id = ? "
        "AND course_id = ? "
        "LIMIT 1";

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
        studentId
    );

    sqlite3_bind_int(
        statement,
        2,
        courseId
    );

    bool found =
        sqlite3_step(statement) == SQLITE_ROW;

    sqlite3_finalize(statement);

    return found;
}

std::vector<int>
EnrolmentRepository::findCoursesForStudent(
    int studentId
)
{
    const char* sql =
        "SELECT course_id "
        "FROM ENROLMENT "
        "WHERE student_id = ? "
        "ORDER BY course_id";

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
        studentId
    );

    std::vector<int> courseIds;

    while (sqlite3_step(statement) == SQLITE_ROW)
    {
        courseIds.push_back(
            sqlite3_column_int(statement, 0)
        );
    }

    sqlite3_finalize(statement);

    return courseIds;
}

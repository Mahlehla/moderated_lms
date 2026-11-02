#include "data/AdministrationRepository.hpp"
#include "data/Database.hpp"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

void executeSql(Database& database, const char* sql)
{
    char* error = nullptr;

    if (sqlite3_exec(
            database.handle(), sql, nullptr, nullptr, &error
        ) != SQLITE_OK) {
        std::string message = error ? error : "SQLite error";
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

template <typename Operation>
bool throwsInvalidArgument(const Operation& operation)
{
    try {
        operation();
        return false;
    } catch (const std::invalid_argument&) {
        return true;
    }
}

template <typename Operation>
bool throwsRuntimeError(const Operation& operation)
{
    try {
        operation();
        return false;
    } catch (const std::runtime_error&) {
        return true;
    }
}

int countRows(Database& database, const char* sql)
{
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database.handle(), sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database.handle()));
    }

    const int result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);
        throw std::runtime_error(sqlite3_errmsg(database.handle()));
    }

    const int count = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return count;
}

void seed(Database& database)
{
    executeSql(database, R"SQL(
        INSERT INTO USER
            (id, username, password_hash, full_name, role)
        VALUES
            (1, 'lecturer1', 'hash1', 'Lecturer One', 'LECTURER'),
            (2, 'hod1', 'hash2', 'HOD One', 'HOD'),
            (3, 'student1', 'hash3', 'Student One', 'STUDENT'),
            (4, 'admin1', 'hash4', 'Administrator One', 'ADMINISTRATOR');

        INSERT INTO DEPARTMENT (id, name)
        VALUES (1, 'Computer Science');

        INSERT INTO COURSE (id, code, name, department_id)
        VALUES (1, 'CSC4433', 'Software Engineering', 1);
    )SQL");
}

int main()
{
    const std::string path =
        "/tmp/moderated_lms_administration_repository_test.db";

    std::remove(path.c_str());

    {
        Database database(path);
        database.initialise();
        seed(database);

        AdministrationRepository administration(database);

        // 1. A lecturer can be assigned to an existing course.
        administration.assignLecturerToCourse(1, 1);

        assert(countRows(
            database,
            "SELECT COUNT(*) FROM LECTURER_COURSE "
            "WHERE lecturer_id = 1 AND course_id = 1"
        ) == 1);

        // 2. An HOD can be assigned to an existing department.
        administration.assignHodToDepartment(2, 1);

        assert(countRows(
            database,
            "SELECT COUNT(*) FROM DEPARTMENT_HOD "
            "WHERE hod_id = 2 AND department_id = 1"
        ) == 1);

        // 3. A student can be enrolled in an existing course.
        administration.enrolStudentInCourse(3, 1);

        assert(countRows(
            database,
            "SELECT COUNT(*) FROM ENROLMENT "
            "WHERE student_id = 3 AND course_id = 1"
        ) == 1);

        // 4. Incorrect user roles must be rejected.
        assert(throwsInvalidArgument([&] {
            administration.assignLecturerToCourse(3, 1);
        }));

        assert(throwsInvalidArgument([&] {
            administration.assignHodToDepartment(1, 1);
        }));

        assert(throwsInvalidArgument([&] {
            administration.enrolStudentInCourse(1, 1);
        }));

        // 5. Nonexistent users must be rejected.
        assert(throwsInvalidArgument([&] {
            administration.assignLecturerToCourse(99, 1);
        }));

        assert(throwsInvalidArgument([&] {
            administration.assignHodToDepartment(99, 1);
        }));

        assert(throwsInvalidArgument([&] {
            administration.enrolStudentInCourse(99, 1);
        }));

        // 6. Nonexistent courses and departments must be rejected.
        assert(throwsInvalidArgument([&] {
            administration.assignLecturerToCourse(1, 99);
        }));

        assert(throwsInvalidArgument([&] {
            administration.assignHodToDepartment(2, 99);
        }));

        assert(throwsInvalidArgument([&] {
            administration.enrolStudentInCourse(3, 99);
        }));

        // 7. Duplicate relationships must be rejected.
        assert(throwsRuntimeError([&] {
            administration.assignLecturerToCourse(1, 1);
        }));

        assert(throwsRuntimeError([&] {
            administration.assignHodToDepartment(2, 1);
        }));

        assert(throwsRuntimeError([&] {
            administration.enrolStudentInCourse(3, 1);
        }));

        // Failed operations must not create extra relationships.
        assert(countRows(
            database, "SELECT COUNT(*) FROM LECTURER_COURSE"
        ) == 1);

        assert(countRows(
            database, "SELECT COUNT(*) FROM DEPARTMENT_HOD"
        ) == 1);

        assert(countRows(
            database, "SELECT COUNT(*) FROM ENROLMENT"
        ) == 1);
    }

    std::remove(path.c_str());

    std::cout
        << "All AdministrationRepository tests passed.\n";

    return 0;
}


#include "data/Database.hpp"
#include "data/AssignmentRepository.hpp"
#include "domain/User.hpp"

#include <sqlite3.h>

#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

void executeSQL(Database& db, const std::string& sql)
{
    char* error = nullptr;

    if (sqlite3_exec(db.handle(), sql.c_str(), nullptr, nullptr, &error)
        != SQLITE_OK) {
        std::string message = error ? error : "Unknown SQLite error";
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

void require(bool condition, const std::string& message)
{
    if (!condition) {
        throw std::runtime_error(message);
    }
}

int main()
{
    const std::string dbPath =
        "/tmp/moderated_lms_lecturer_assignment_test.db";

    std::remove(dbPath.c_str());

    try {
        Database db(dbPath);
        db.initialise();

        AssignmentRepository assignments(db);

        // Create two lecturers, one student and one department.
        executeSQL(db,
            "INSERT INTO USER "
            "(id, username, password_hash, full_name, role) VALUES "
            "(1, 'lecturer1', 'hash1', 'Lecturer One', 'LECTURER'),"
            "(2, 'lecturer2', 'hash2', 'Lecturer Two', 'LECTURER'),"
            "(3, 'student1', 'hash3', 'Student One', 'STUDENT');"

            "INSERT INTO DEPARTMENT (id, name) "
            "VALUES (1, 'Computer Science');"

            "INSERT INTO COURSE "
            "(id, code, name, department_id) VALUES "
            "(1, 'CSC4000', 'Software Engineering', 1),"
            "(2, 'CSC4001', 'Computer Systems', 1);"

            // Lecturer 1 teaches course 1.
            // Lecturer 2 teaches course 2.
            "INSERT INTO LECTURER_COURSE "
            "(lecturer_id, course_id) VALUES "
            "(1, 1), (2, 2);"
        );

        Lecturer lecturer1(
            1, "lecturer1", "hash1", "Lecturer One", Role::Lecturer
        );

        Lecturer lecturer2(
            2, "lecturer2", "hash2", "Lecturer Two", Role::Lecturer
        );

        Student student(
            3, "student1", "hash3", "Student One", Role::Student
        );

        // TEST 1: Assigned lecturer can create an assignment.
        const int assignmentId = assignments.createForLecturer(
            lecturer1,
            1,
            "Software Engineering Assignment",
            "Design and implement an LMS.",
            "2026-12-15 23:59:59",
            100.0
        );

        require(
            assignmentId > 0,
            "TEST 1 FAILED: Assignment was not created."
        );

        std::cout
            << "TEST 1 PASSED: Assigned lecturer created an assignment.\n";

        // TEST 2: Lecturer cannot create an assignment for another course.
        bool ownershipDenied = false;

        try {
            assignments.createForLecturer(
                lecturer1,
                2,
                "Unauthorised Assignment",
                "This should not be created.",
                "2026-12-15 23:59:59",
                100.0
            );
        }
        catch (const std::exception&) {
            ownershipDenied = true;
        }

        require(
            ownershipDenied,
            "TEST 2 FAILED: Unassigned lecturer was allowed to create an assignment."
        );

        std::cout
            << "TEST 2 PASSED: Lecturer cannot create assignments "
            << "for an unassigned course.\n";

        // TEST 3: A student cannot use the lecturer-only method.
        bool studentDenied = false;

        try {
            assignments.createForLecturer(
                student,
                1,
                "Student Assignment",
                "This should not be created.",
                "2026-12-15 23:59:59",
                100.0
            );
        }
        catch (const std::exception&) {
            studentDenied = true;
        }

        require(
            studentDenied,
            "TEST 3 FAILED: Student was allowed to create an assignment."
        );

        std::cout
            << "TEST 3 PASSED: Student cannot create lecturer assignments.\n";

        std::cout << "\nAll lecturer assignment tests passed.\n";

        std::remove(dbPath.c_str());
        return 0;
    }
    catch (const std::exception& error) {
        std::cerr << "\nTEST FAILURE: " << error.what() << '\n';
        std::remove(dbPath.c_str());
        return 1;
    }
}

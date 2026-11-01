#include "data/Database.hpp"
#include "services/AssessmentWorkflowService.hpp"
#include "domain/User.hpp"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

void seed(Database& database) {
    const char* sql = R"SQL(
        INSERT INTO USER (id, username, password_hash, full_name, role)
        VALUES
            (1, 'lecturer', 'hash1', 'Test Lecturer', 'LECTURER'),
            (2, 'hod', 'hash2', 'Test HOD', 'HOD'),
            (3, 'student', 'hash3', 'Test Student', 'STUDENT'),
            (4, 'admin', 'hash4', 'Test Administrator', 'ADMINISTRATOR');

        INSERT INTO DEPARTMENT (id, name)
        VALUES (1, 'Computer Science');

        INSERT INTO COURSE (id, code, name, department_id)
        VALUES (1, 'CSC4433', 'Software Engineering', 1);

        INSERT INTO ASSIGNMENT
            (id, course_id, title, description, deadline, released, max_mark)
        VALUES
            (1, 1, 'Test Assignment', 'Description',
             '2026-11-01 12:00:00', 0, 40);

        INSERT INTO SUBMISSION
            (id, assignment_id, student_id, version,
             submitted_at, status, content)
        VALUES
            (1, 1, 3, 1, '2026-11-01 11:00:00', 'SUBMITTED', 'Test work');
    )SQL";

    char* error = nullptr;

    if (sqlite3_exec(
            database.handle(), sql, nullptr, nullptr, &error
        ) != SQLITE_OK) {
        std::string message = error ? error : "Seed failed";
        sqlite3_free(error);
        throw std::runtime_error(message);
    }
}

template <typename Operation>
bool throwsException(const Operation& operation) {
    try {
        operation();
        return false;
    } catch (const std::exception&) {
        return true;
    }
}

int main() {
    const std::string path = "/tmp/moderated_lms_week5_workflow.db";
    std::remove(path.c_str());

    {
        Database database(path);
        database.initialise();
        seed(database);

        Lecturer lecturer(
            1, "lecturer", "hash1", "Test Lecturer", Role::Lecturer
        );

        HeadOfDepartment hod(
            2, "hod", "hash2", "Test HOD", Role::HeadOfDepartment
        );

        Student student(
            3, "student", "hash3", "Test Student", Role::Student
        );

        Administrator admin(
            4, "admin", "hash4", "Test Administrator", Role::Administrator
        );

        AssessmentWorkflowService workflow(database);

        // Assignment maximum is 40, not 100.
        assert(throwsException([&] {
            workflow.markSubmission(
                lecturer, 1, 41, "Feedback", "2026-11-01 12:30:00"
            );
        }));

        // Feedback is required.
        assert(throwsException([&] {
            workflow.markSubmission(
                lecturer, 1, 30, "", "2026-11-01 12:30:00"
            );
        }));

        // Students cannot mark submissions.
        assert(throwsException([&] {
            workflow.markSubmission(
                student, 1, 30, "Feedback", "2026-11-01 12:30:00"
            );
        }));

        // Lecturer records a valid mark.
        workflow.markSubmission(
            lecturer, 1, 35, "Good work.", "2026-11-01 12:30:00"
        );

        assert(!workflow.isMarkReleased(1));

        // Lecturer cannot moderate or release.
        assert(throwsException([&] {
            workflow.moderateSubmission(
                lecturer, 1, "2026-11-01 13:00:00"
            );
        }));

        assert(throwsException([&] {
            workflow.releaseMark(
                lecturer, 1, "2026-11-01 13:00:00"
            );
        }));

        // Student cannot grant extensions.
        assert(throwsException([&] {
            workflow.grantExtension(
                student, 1, 3,
                "2026-11-05 23:59:59",
                "2026-11-01 13:00:00"
            );
        }));

        // Administrator grants a general extension.
        workflow.grantExtension(
            admin, 1, std::nullopt,
            "2026-11-04 23:59:59",
            "2026-11-01 13:10:00"
        );

        // HOD grants a student-specific extension.
        workflow.grantExtension(
            hod, 1, 3,
            "2026-11-06 23:59:59",
            "2026-11-01 13:15:00"
        );

        assert(
            workflow.effectiveDeadline(
                1, 3, "2026-11-01 12:00:00"
            ) == "2026-11-06 23:59:59"
        );

        // A different student receives the general extension.
        assert(
            workflow.effectiveDeadline(
                1, 99, "2026-11-01 12:00:00"
            ) == "2026-11-04 23:59:59"
        );

        // HOD moderates and releases.
        workflow.moderateSubmission(
            hod, 1, "2026-11-01 14:00:00"
        );

        assert(!workflow.isMarkReleased(1));

        // A second moderation attempt is rejected.
        assert(throwsException([&] {
            workflow.moderateSubmission(
                hod, 1, "2026-11-01 14:05:00"
            );
        }));

        workflow.releaseMark(
            hod, 1, "2026-11-01 14:30:00"
        );

        assert(workflow.isMarkReleased(1));

        // Released marks cannot be changed.
        assert(throwsException([&] {
            workflow.markSubmission(
                lecturer, 1, 38, "Changed mark",
                "2026-11-01 15:00:00"
            );
        }));

        // Verify audit records.
        sqlite3_stmt* statement = nullptr;

        assert(sqlite3_prepare_v2(
            database.handle(),
            "SELECT COUNT(*) FROM AUDIT_LOG",
            -1,
            &statement,
            nullptr
        ) == SQLITE_OK);

        assert(sqlite3_step(statement) == SQLITE_ROW);
        assert(sqlite3_column_int(statement, 0) == 5);
        sqlite3_finalize(statement);

        std::cout << "Week 5 assessment workflow tests passed.\n";
    }

    std::remove(path.c_str());
    return 0;
}

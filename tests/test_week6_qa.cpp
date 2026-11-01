#include "data/Database.hpp"
#include "data/SubmissionRepository.hpp"
#include "services/AssessmentWorkflowService.hpp"
#include "domain/User.hpp"

#include <sqlite3.h>

#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

void execute(Database& database, const std::string& sql) {
    char* error = nullptr;

    if (sqlite3_exec(database.handle(), sql.c_str(),
                     nullptr, nullptr, &error) != SQLITE_OK) {
        std::string message = error ? error : "SQL execution failed";
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

int scalar(Database& database, const std::string& sql) {
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(database.handle(), sql.c_str(),
                           -1, &statement, nullptr) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database.handle()));
    }

    if (sqlite3_step(statement) != SQLITE_ROW) {
        sqlite3_finalize(statement);
        throw std::runtime_error("Scalar query returned no row.");
    }

    int result = sqlite3_column_int(statement, 0);
    sqlite3_finalize(statement);
    return result;
}

void seed(Database& database) {
    execute(database, R"SQL(
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
            (1, 1, 'QA Assignment', 'QA test assignment',
             '2026-11-01 12:00:00', 0, 40);
    )SQL");
}

int main() {
    const std::string path = "/tmp/moderated_lms_week6_qa.db";
    std::remove(path.c_str());

    int firstSubmissionId = 0;
    int secondSubmissionId = 0;

    {
        Database database(path);
        database.initialise();
        seed(database);

        SubmissionRepository submissions(database);

        Lecturer lecturer(
            1, "lecturer", "hash1", "Test Lecturer", Role::Lecturer
        );

        HeadOfDepartment hod(
            2, "hod", "hash2", "Test HOD", Role::HeadOfDepartment
        );

        // --------------------------------------------------
        // TEST 1: Submission versioning and preservation
        // --------------------------------------------------

        assert(submissions.nextVersion(1, 3) == 1);

        firstSubmissionId = submissions.create(
            1, 3, submissions.nextVersion(1, 3),
            "2026-11-01 10:00:00",
            SubmissionStatus::Submitted,
            "Original submission content"
        );

        assert(submissions.nextVersion(1, 3) == 2);

        secondSubmissionId = submissions.create(
            1, 3, submissions.nextVersion(1, 3),
            "2026-11-01 11:00:00",
            SubmissionStatus::Submitted,
            "Revised submission content"
        );

        assert(submissions.nextVersion(1, 3) == 3);

        auto versions = submissions.findByAssignment(1);

        assert(versions.size() == 2);
        assert(versions[0].version() == 1);
        assert(versions[0].content() == "Original submission content");
        assert(versions[1].version() == 2);
        assert(versions[1].content() == "Revised submission content");

        assert(throwsException([&] {
            submissions.create(
                1, 3, 1, "2026-11-01 12:00:00",
                SubmissionStatus::Submitted, "Duplicate version"
            );
        }));

        // --------------------------------------------------
        // TEST 2: Extension selection and precedence
        // --------------------------------------------------

        Administrator admin(
            4, "admin", "hash4", "Test Administrator",
            Role::Administrator
        );

        AssessmentWorkflowService workflow(database);

        workflow.grantExtension(
            admin, 1, std::nullopt,
            "2026-11-04 23:59:59", "2026-11-01 12:00:00"
        );

        workflow.grantExtension(
            hod, 1, 3,
            "2026-11-06 23:59:59", "2026-11-01 12:05:00"
        );

        // A later general extension must not override the
        // student-specific extension.
        workflow.grantExtension(
            admin, 1, std::nullopt,
            "2026-11-08 23:59:59", "2026-11-01 12:10:00"
        );

        assert(
            workflow.effectiveDeadline(
                1, 3, "2026-11-01 12:00:00"
            ) == "2026-11-06 23:59:59"
        );

        // For another student, the newest general extension applies.
        assert(
            workflow.effectiveDeadline(
                1, 99, "2026-11-01 12:00:00"
            ) == "2026-11-08 23:59:59"
        );

        // --------------------------------------------------
        // TEST 3: Invalid workflow transitions
        // --------------------------------------------------

        workflow.markSubmission(
            lecturer, firstSubmissionId, 35,
            "Good work.", "2026-11-01 13:00:00"
        );

        // Releasing before moderation must fail.
        assert(throwsException([&] {
            workflow.releaseMark(
                hod, firstSubmissionId, "2026-11-01 13:05:00"
            );
        }));

        assert(!workflow.isMarkReleased(firstSubmissionId));

        // A nonexistent submission cannot be marked or moderated.
        assert(throwsException([&] {
            workflow.markSubmission(
                lecturer, 999999, 20,
                "Feedback", "2026-11-01 13:10:00"
            );
        }));

        assert(throwsException([&] {
            workflow.moderateSubmission(
                hod, 999999, "2026-11-01 13:10:00"
            );
        }));

        // --------------------------------------------------
        // TEST 4: Transaction rollback on audit failure
        // --------------------------------------------------

        workflow.markSubmission(
            lecturer, secondSubmissionId, 30,
            "Second submission feedback.", "2026-11-01 13:15:00"
        );

        const int auditBeforeRollback =
            scalar(database, "SELECT COUNT(*) FROM AUDIT_LOG");

        // This HOD role has permission, but actor ID 999 does
        // not exist in USER. The audit insert must fail its FK.
        HeadOfDepartment invalidHod(
            999, "ghost", "hash", "Missing HOD", Role::HeadOfDepartment
        );

        assert(throwsException([&] {
            workflow.moderateSubmission(
                invalidHod, secondSubmissionId, "2026-11-01 13:20:00"
            );
        }));

        // The transaction must roll back the moderation update.
        assert(
            scalar(database,
                "SELECT moderated FROM MARK "
                "WHERE submission_id = " + std::to_string(secondSubmissionId)
            ) == 0
        );

        assert(
            scalar(database,
                "SELECT COUNT(*) FROM AUDIT_LOG"
            ) == auditBeforeRollback
        );

        // --------------------------------------------------
        // TEST 5: Successful moderation and release
        // --------------------------------------------------

        workflow.moderateSubmission(
            hod, firstSubmissionId, "2026-11-01 13:30:00"
        );

        assert(throwsException([&] {
            workflow.moderateSubmission(
                hod, firstSubmissionId, "2026-11-01 13:35:00"
            );
        }));

        workflow.releaseMark(
            hod, firstSubmissionId, "2026-11-01 14:00:00"
        );

        assert(workflow.isMarkReleased(firstSubmissionId));

        assert(throwsException([&] {
            workflow.releaseMark(
                hod, firstSubmissionId, "2026-11-01 14:05:00"
            );
        }));

        assert(throwsException([&] {
            workflow.markSubmission(
                lecturer, firstSubmissionId, 38,
                "Changed after release.", "2026-11-01 14:10:00"
            );
        }));

        // --------------------------------------------------
        // TEST 6: Audit record contents
        // --------------------------------------------------

        sqlite3_stmt* audit = nullptr;

        assert(sqlite3_prepare_v2(
            database.handle(),
            "SELECT actor_id, action, entity_type, entity_id, "
            "old_value, new_value, created_at "
            "FROM AUDIT_LOG "
            "WHERE action = 'RELEASE_MARK' "
            "AND entity_id = ?",
            -1, &audit, nullptr
        ) == SQLITE_OK);

        sqlite3_bind_int(audit, 1, firstSubmissionId);

        assert(sqlite3_step(audit) == SQLITE_ROW);
        assert(sqlite3_column_int(audit, 0) == 2);

        assert(std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(audit, 1))
        ) == "RELEASE_MARK");

        assert(std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(audit, 2))
        ) == "MARK");

        assert(sqlite3_column_int(audit, 3) == firstSubmissionId);

        assert(std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(audit, 4))
        ) == "RELEASED:0");

        assert(std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(audit, 5))
        ) == "RELEASED:1");

        assert(std::string(
            reinterpret_cast<const char*>(sqlite3_column_text(audit, 6))
        ) == "2026-11-01 14:00:00");

        sqlite3_finalize(audit);

        std::cout << "Week 6 QA tests passed.\n";
    }

    // ------------------------------------------------------
    // TEST 7: Persistence after closing and reopening database
    // ------------------------------------------------------

    {
        Database reopened(path);
        reopened.initialise();

        SubmissionRepository submissions(reopened);
        AssessmentWorkflowService workflow(reopened);

        const auto versions = submissions.findByStudent(3);

        assert(versions.size() == 2);
        assert(versions[0].content() == "Original submission content");
        assert(versions[1].content() == "Revised submission content");

        assert(workflow.isMarkReleased(firstSubmissionId));

        assert(
            scalar(reopened,
                "SELECT COUNT(*) FROM AUDIT_LOG "
                "WHERE action = 'RELEASE_MARK'"
            ) == 1
        );
    }

    std::remove(path.c_str());

    std::cout << "All Week 6 QA checks passed successfully.\n";
    return 0;
}

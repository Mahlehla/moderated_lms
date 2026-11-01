#include "services/AssessmentWorkflowService.hpp"
#include "services/AccessControlService.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace {

class Statement {
private:
    sqlite3_stmt* statement_ = nullptr;

public:
    Statement(sqlite3* db, const char* sql) {
        if (sqlite3_prepare_v2(
                db, sql, -1, &statement_, nullptr
            ) != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
    }

    ~Statement() {
        if (statement_) {
            sqlite3_finalize(statement_);
        }
    }

    sqlite3_stmt* get() const {
        return statement_;
    }

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
};

class Transaction {
private:
    sqlite3* db_;
    bool committed_ = false;

public:
    explicit Transaction(sqlite3* db) : db_(db) {
        if (sqlite3_exec(db_, "BEGIN IMMEDIATE", nullptr, nullptr, nullptr)
            != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
    }

    void commit() {
        if (sqlite3_exec(db_, "COMMIT", nullptr, nullptr, nullptr)
            != SQLITE_OK) {
            throw std::runtime_error(sqlite3_errmsg(db_));
        }
        committed_ = true;
    }

    ~Transaction() {
        if (!committed_) {
            sqlite3_exec(db_, "ROLLBACK", nullptr, nullptr, nullptr);
        }
    }

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
};

void bindText(sqlite3_stmt* statement, int index, const std::string& value) {
    if (sqlite3_bind_text(
            statement, index, value.c_str(), -1, SQLITE_TRANSIENT
        ) != SQLITE_OK) {
        throw std::runtime_error("Failed to bind text parameter.");
    }
}

void checkStep(sqlite3* db, sqlite3_stmt* statement) {
    if (sqlite3_step(statement) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
}

struct SubmissionInfo {
    int assignmentId;
    int studentId;
    int courseId;
    int departmentId;
    double maximumMark;
};

SubmissionInfo getSubmissionInfo(sqlite3* db, int submissionId) {
    Statement statement(
        db,
        "SELECT s.assignment_id, s.student_id, c.id, "
        "c.department_id, a.max_mark "
        "FROM SUBMISSION s "
        "JOIN ASSIGNMENT a ON a.id = s.assignment_id "
        "JOIN COURSE c ON c.id = a.course_id "
        "WHERE s.id = ?"
    );

    sqlite3_bind_int(statement.get(), 1, submissionId);

    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        throw std::runtime_error("Submission does not exist.");
    }

    return {
        sqlite3_column_int(statement.get(), 0),
        sqlite3_column_int(statement.get(), 1),
        sqlite3_column_int(statement.get(), 2),
        sqlite3_column_int(statement.get(), 3),
        sqlite3_column_double(statement.get(), 4)
    };
}

void requirePermission(Role role, Permission permission) {
    AccessControlService access;

    if (!access.hasPermission(role, permission)) {
        throw std::runtime_error("Permission denied.");
    }
}

void requireLecturerCourse(
    sqlite3* db,
    int lecturerId,
    int courseId
) {
    Statement statement(
        db,
        "SELECT 1 FROM LECTURER_COURSE "
        "WHERE lecturer_id = ? AND course_id = ?"
    );

    sqlite3_bind_int(statement.get(), 1, lecturerId);
    sqlite3_bind_int(statement.get(), 2, courseId);

    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        throw std::runtime_error(
            "Permission denied: lecturer is not assigned to this course."
        );
    }
}

void requireHodDepartment(
    sqlite3* db,
    int hodId,
    int departmentId
) {
    Statement statement(
        db,
        "SELECT 1 FROM DEPARTMENT_HOD "
        "WHERE hod_id = ? AND department_id = ?"
    );

    sqlite3_bind_int(statement.get(), 1, hodId);
    sqlite3_bind_int(statement.get(), 2, departmentId);

    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        throw std::runtime_error(
            "Permission denied: HOD is not assigned to this department."
        );
    }
}

} // namespace

void AssessmentWorkflowService::writeAudit(
    int actorId,
    const std::string& action,
    const std::string& entityType,
    int entityId,
    const std::string& oldValue,
    const std::string& newValue,
    const std::string& timestamp
) {
    Statement statement(
        database_.handle(),
        "INSERT INTO AUDIT_LOG "
        "(actor_id, action, entity_type, entity_id, "
        " old_value, new_value, created_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
    );

    sqlite3_bind_int(statement.get(), 1, actorId);
    bindText(statement.get(), 2, action);
    bindText(statement.get(), 3, entityType);
    sqlite3_bind_int(statement.get(), 4, entityId);
    bindText(statement.get(), 5, oldValue);
    bindText(statement.get(), 6, newValue);
    bindText(statement.get(), 7, timestamp);

    checkStep(database_.handle(), statement.get());
}

void AssessmentWorkflowService::updateSubmissionStatus(
    int submissionId,
    const std::string& status
) {
    Statement statement(
        database_.handle(),
        "UPDATE SUBMISSION SET status = ? WHERE id = ?"
    );

    bindText(statement.get(), 1, status);
    sqlite3_bind_int(statement.get(), 2, submissionId);

    checkStep(database_.handle(), statement.get());

    if (sqlite3_changes(database_.handle()) != 1) {
        throw std::runtime_error("Could not update submission status.");
    }
}

void AssessmentWorkflowService::markSubmission(
    const User& lecturer,
    int submissionId,
    double mark,
    const std::string& feedback,
    const std::string& timestamp
) {
    requirePermission(lecturer.role(), Permission::MarkSubmissions);

    if (submissionId <= 0) {
        throw std::invalid_argument("Invalid submission ID.");
    }

    if (feedback.empty()) {
        throw std::invalid_argument("Feedback cannot be empty.");
    }

    if (timestamp.empty()) {
        throw std::invalid_argument("Timestamp cannot be empty.");
    }

    Transaction transaction(database_.handle());

    const SubmissionInfo info =
        getSubmissionInfo(database_.handle(), submissionId);

    requireLecturerCourse(
        database_.handle(),
        lecturer.id(),
        info.courseId
    );

    if (mark < 0.0 || mark > info.maximumMark) {
        throw std::invalid_argument(
            "Mark must be between zero and the assignment maximum mark."
        );
    }

    Statement existing(
        database_.handle(),
        "SELECT mark, moderated, released "
        "FROM MARK WHERE submission_id = ?"
    );

    sqlite3_bind_int(existing.get(), 1, submissionId);

    bool hasExistingMark = false;
    double oldMark = 0.0;

    if (sqlite3_step(existing.get()) == SQLITE_ROW) {
        hasExistingMark = true;
        oldMark = sqlite3_column_double(existing.get(), 0);

        const bool moderated =
            sqlite3_column_int(existing.get(), 1) != 0;

        const bool released =
            sqlite3_column_int(existing.get(), 2) != 0;

        if (moderated || released) {
            throw std::runtime_error(
                "A moderated or released mark cannot be changed."
            );
        }
    }

    if (hasExistingMark) {
        Statement update(
            database_.handle(),
            "UPDATE MARK SET lecturer_id = ?, mark = ?, feedback = ? "
            "WHERE submission_id = ? AND moderated = 0 AND released = 0"
        );

        sqlite3_bind_int(update.get(), 1, lecturer.id());
        sqlite3_bind_double(update.get(), 2, mark);
        bindText(update.get(), 3, feedback);
        sqlite3_bind_int(update.get(), 4, submissionId);

        checkStep(database_.handle(), update.get());

        if (sqlite3_changes(database_.handle()) != 1) {
            throw std::runtime_error("Could not update mark.");
        }
    } else {
        Statement insert(
            database_.handle(),
            "INSERT INTO MARK "
            "(submission_id, lecturer_id, mark, feedback) "
            "VALUES (?, ?, ?, ?)"
        );

        sqlite3_bind_int(insert.get(), 1, submissionId);
        sqlite3_bind_int(insert.get(), 2, lecturer.id());
        sqlite3_bind_double(insert.get(), 3, mark);
        bindText(insert.get(), 4, feedback);

        checkStep(database_.handle(), insert.get());
    }

    updateSubmissionStatus(submissionId, "MARKED");

    writeAudit(
        lecturer.id(),
        hasExistingMark ? "UPDATE_MARK" : "CREATE_MARK",
        "MARK",
        submissionId,
        hasExistingMark ? std::to_string(oldMark) : "",
        std::to_string(mark),
        timestamp
    );

    transaction.commit();
}

void AssessmentWorkflowService::grantExtension(
    const User& actor,
    int assignmentId,
    std::optional<int> studentId,
    const std::string& newDeadline,
    const std::string& timestamp
) {
    requirePermission(actor.role(), Permission::GrantExtensions);

    if (assignmentId <= 0 || newDeadline.empty() || timestamp.empty()) {
        throw std::invalid_argument("Invalid extension details.");
    }

    if (studentId && *studentId <= 0) {
        throw std::invalid_argument("Invalid student ID.");
    }

    Transaction transaction(database_.handle());

    Statement assignment(
        database_.handle(),
        "SELECT c.department_id "
        "FROM ASSIGNMENT a "
        "JOIN COURSE c ON c.id = a.course_id "
        "WHERE a.id = ?"
    );

    sqlite3_bind_int(assignment.get(), 1, assignmentId);

    if (sqlite3_step(assignment.get()) != SQLITE_ROW) {
        throw std::runtime_error("Assignment does not exist.");
    }

    const int departmentId =
        sqlite3_column_int(assignment.get(), 0);

    if (actor.role() == Role::HeadOfDepartment) {
        requireHodDepartment(
            database_.handle(),
            actor.id(),
            departmentId
        );
    }

    Statement insert(
        database_.handle(),
        "INSERT INTO EXTENSION "
        "(assignment_id, student_id, new_deadline) "
        "VALUES (?, ?, ?)"
    );

    sqlite3_bind_int(insert.get(), 1, assignmentId);

    if (studentId) {
        sqlite3_bind_int(insert.get(), 2, *studentId);
    } else {
        sqlite3_bind_null(insert.get(), 2);
    }

    bindText(insert.get(), 3, newDeadline);
    checkStep(database_.handle(), insert.get());

    const int extensionId =
        static_cast<int>(sqlite3_last_insert_rowid(database_.handle()));

    const std::string target =
        studentId ? "STUDENT:" + std::to_string(*studentId) : "ALL_STUDENTS";

    writeAudit(
        actor.id(),
        "GRANT_EXTENSION",
        "EXTENSION",
        extensionId,
        "",
        "ASSIGNMENT:" + std::to_string(assignmentId)
            + ";" + target + ";DEADLINE:" + newDeadline,
        timestamp
    );

    transaction.commit();
}

std::string AssessmentWorkflowService::effectiveDeadline(
    int assignmentId,
    int studentId,
    const std::string& originalDeadline
) const {
    Statement statement(
        database_.handle(),
        "SELECT new_deadline FROM EXTENSION "
        "WHERE assignment_id = ? "
        "AND (student_id = ? OR student_id IS NULL) "
        "ORDER BY CASE WHEN student_id = ? THEN 0 ELSE 1 END, id DESC "
        "LIMIT 1"
    );

    sqlite3_bind_int(statement.get(), 1, assignmentId);
    sqlite3_bind_int(statement.get(), 2, studentId);
    sqlite3_bind_int(statement.get(), 3, studentId);

    if (sqlite3_step(statement.get()) == SQLITE_ROW) {
        const auto* value = sqlite3_column_text(statement.get(), 0);
        return value
            ? reinterpret_cast<const char*>(value)
            : originalDeadline;
    }

    return originalDeadline;
}

void AssessmentWorkflowService::moderateSubmission(
    const User& headOfDepartment,
    int submissionId,
    const std::string& timestamp
) {
    requirePermission(
        headOfDepartment.role(),
        Permission::ModerateMarks
    );

    if (timestamp.empty()) {
        throw std::invalid_argument("Timestamp cannot be empty.");
    }

    Transaction transaction(database_.handle());

    const SubmissionInfo info =
        getSubmissionInfo(database_.handle(), submissionId);

    requireHodDepartment(
        database_.handle(),
        headOfDepartment.id(),
        info.departmentId
    );

    Statement update(
        database_.handle(),
        "UPDATE MARK SET moderated = 1 "
        "WHERE submission_id = ? AND moderated = 0 AND released = 0"
    );

    sqlite3_bind_int(update.get(), 1, submissionId);
    checkStep(database_.handle(), update.get());

    if (sqlite3_changes(database_.handle()) != 1) {
        throw std::runtime_error(
            "Mark does not exist or cannot be moderated."
        );
    }

    updateSubmissionStatus(submissionId, "MODERATED");

    writeAudit(
        headOfDepartment.id(),
        "MODERATE_MARK",
        "MARK",
        submissionId,
        "MODERATED:0",
        "MODERATED:1",
        timestamp
    );

    transaction.commit();
}

void AssessmentWorkflowService::releaseMark(
    const User& headOfDepartment,
    int submissionId,
    const std::string& timestamp
) {
    requirePermission(
        headOfDepartment.role(),
        Permission::ReleaseMarks
    );

    if (timestamp.empty()) {
        throw std::invalid_argument("Timestamp cannot be empty.");
    }

    Transaction transaction(database_.handle());

    const SubmissionInfo info =
        getSubmissionInfo(database_.handle(), submissionId);

    requireHodDepartment(
        database_.handle(),
        headOfDepartment.id(),
        info.departmentId
    );

    Statement update(
        database_.handle(),
        "UPDATE MARK SET released = 1 "
        "WHERE submission_id = ? AND moderated = 1 AND released = 0"
    );

    sqlite3_bind_int(update.get(), 1, submissionId);
    checkStep(database_.handle(), update.get());

    if (sqlite3_changes(database_.handle()) != 1) {
        throw std::runtime_error(
            "Only a moderated, unreleased mark can be released."
        );
    }

    updateSubmissionStatus(submissionId, "RELEASED");

    writeAudit(
        headOfDepartment.id(),
        "RELEASE_MARK",
        "MARK",
        submissionId,
        "RELEASED:0",
        "RELEASED:1",
        timestamp
    );

    transaction.commit();
}

bool AssessmentWorkflowService::isMarkReleased(int submissionId) const {
    Statement statement(
        database_.handle(),
        "SELECT released FROM MARK WHERE submission_id = ?"
    );

    sqlite3_bind_int(statement.get(), 1, submissionId);

    if (sqlite3_step(statement.get()) != SQLITE_ROW) {
        return false;
    }

    return sqlite3_column_int(statement.get(), 0) == 1;
}

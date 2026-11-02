#include "services/AccessControlService.hpp"
#include "data/AssignmentRepository.hpp"

#include <stdexcept>
#include <sqlite3.h>

int AssignmentRepository::create(
    int courseId,
    const std::string& title,
    const std::string& description,
    const std::string& deadline,
    double maxMark
) {
    if (courseId <= 0) {
        throw std::invalid_argument("Course ID must be positive.");
    }
    if (title.empty()) {
        throw std::invalid_argument("Assignment title cannot be empty.");
    }
    if (deadline.empty()) {
        throw std::invalid_argument("Assignment deadline cannot be empty.");
    }
    if (maxMark <= 0.0 || maxMark > 100.0) {
    throw std::invalid_argument(
        "Maximum mark must be greater than zero and cannot exceed 100."
    );

}

    const char* sql =
        "INSERT INTO ASSIGNMENT "
        "(course_id, title, description, deadline, released, max_mark) "
        "VALUES (?, ?, ?, ?, 0, ?);";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_int(statement, 1, courseId);
    sqlite3_bind_text(statement, 2, title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 3, description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(statement, 4, deadline.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(statement, 5, maxMark);

    const int result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(database_.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
    return static_cast<int>(sqlite3_last_insert_rowid(database_.handle()));
}

std::vector<Assignment> AssignmentRepository::findByCourse(int courseId) {
    const char* sql =
        "SELECT id, course_id, title, description, deadline, released, max_mark "
        "FROM ASSIGNMENT WHERE course_id = ? ORDER BY id;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_int(statement, 1, courseId);

    std::vector<Assignment> assignments;

    while (sqlite3_step(statement) == SQLITE_ROW) {
        assignments.emplace_back(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            reinterpret_cast<const char*>(sqlite3_column_text(statement, 2)),
            reinterpret_cast<const char*>(sqlite3_column_text(statement, 3)),
            reinterpret_cast<const char*>(sqlite3_column_text(statement, 4)),
            sqlite3_column_int(statement, 5) != 0,
            sqlite3_column_double(statement, 6)
        );
    }

    sqlite3_finalize(statement);
    return assignments;
}

std::optional<Assignment> AssignmentRepository::findById(int assignmentId) {
    const char* sql =
        "SELECT id, course_id, title, description, deadline, released, max_mark "
        "FROM ASSIGNMENT WHERE id = ?;";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(database_.handle(), sql, -1, &statement, nullptr)
        != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_int(statement, 1, assignmentId);

    std::optional<Assignment> result;

    if (sqlite3_step(statement) == SQLITE_ROW) {
        result.emplace(
            sqlite3_column_int(statement, 0),
            sqlite3_column_int(statement, 1),
            reinterpret_cast<const char*>(sqlite3_column_text(statement, 2)),
            reinterpret_cast<const char*>(sqlite3_column_text(statement, 3)),
            reinterpret_cast<const char*>(sqlite3_column_text(statement, 4)),
            sqlite3_column_int(statement, 5) != 0,
            sqlite3_column_double(statement, 6)
        );
    }

    sqlite3_finalize(statement);
    return result;
}
int AssignmentRepository::createForLecturer(
    const User& lecturer,
    int courseId,
    const std::string& title,
    const std::string& description,
    const std::string& deadline,
    double maxMark
) {
    AccessControlService access;

    if (!access.isAllowed(
            lecturer,
            Permission::CreateAssignments
        )) {
        throw std::runtime_error(
            "Permission denied: user cannot create assignments."
        );
    }

    if (lecturer.role() != Role::Lecturer || lecturer.id() <= 0) {
        throw std::runtime_error(
            "Only a valid lecturer account can create assignments."
        );
    }

    if (courseId <= 0 || title.empty() || deadline.empty()) {
        throw std::invalid_argument(
            "Invalid assignment details."
        );
    }

    if (maxMark <= 0.0 || maxMark > 100.0) {
        throw std::invalid_argument(
            "Maximum mark must be greater than zero and cannot exceed 100."
        );
    }

    const char* sql =
        "INSERT INTO ASSIGNMENT "
        "(course_id, title, description, deadline, released, max_mark) "
        "SELECT ?, ?, ?, ?, 0, ? "
        "WHERE EXISTS ("
        "    SELECT 1 FROM LECTURER_COURSE "
        "    WHERE lecturer_id = ? AND course_id = ?"
        ");";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(
            sqlite3_errmsg(database_.handle())
        );
    }

    sqlite3_bind_int(statement, 1, courseId);
    sqlite3_bind_text(
        statement, 2, title.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 3, description.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_text(
        statement, 4, deadline.c_str(), -1, SQLITE_TRANSIENT
    );
    sqlite3_bind_double(statement, 5, maxMark);
    sqlite3_bind_int(statement, 6, lecturer.id());
    sqlite3_bind_int(statement, 7, courseId);

    const int result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(database_.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    const int inserted = sqlite3_changes(database_.handle());

    sqlite3_finalize(statement);

    if (inserted != 1) {
        throw std::runtime_error(
            "Permission denied: lecturer is not assigned to this course."
        );
    }

    return static_cast<int>(
        sqlite3_last_insert_rowid(database_.handle())
    );
}

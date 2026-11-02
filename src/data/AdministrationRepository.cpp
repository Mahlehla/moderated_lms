#include "data/AdministrationRepository.hpp"

#include <sqlite3.h>

#include <stdexcept>
#include <string>

namespace {

void executeRelationship(
    Database& database,
    const char* sql,
    int userId,
    int recordId
)
{
    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database.handle(),
            sql,
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database.handle()));
    }

    sqlite3_bind_int(statement, 1, userId);
    sqlite3_bind_int(statement, 2, recordId);

    const int result = sqlite3_step(statement);

    if (result != SQLITE_DONE) {
        const std::string error = sqlite3_errmsg(database.handle());
        sqlite3_finalize(statement);
        throw std::runtime_error(error);
    }

    sqlite3_finalize(statement);
}

} // namespace

void AdministrationRepository::requireUserRole(
    int userId,
    const char* expectedRole
)
{
    const char* sql =
        "SELECT role FROM USER WHERE id = ?";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(), sql, -1, &statement, nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_int(statement, 1, userId);

    const int result = sqlite3_step(statement);

    if (result != SQLITE_ROW) {
        sqlite3_finalize(statement);

        if (result == SQLITE_DONE) {
            throw std::invalid_argument("User account does not exist.");
        }

        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    const auto* value = sqlite3_column_text(statement, 0);
    const std::string actualRole = value
        ? reinterpret_cast<const char*>(value)
        : "";

    sqlite3_finalize(statement);

    if (actualRole != expectedRole) {
        throw std::invalid_argument(
            std::string("Selected user must have the ")
            + expectedRole + " role."
        );
    }
}

void AdministrationRepository::requireRecordExists(
    const char* table,
    int recordId
)
{
    // Only fixed internal table names are passed to this function.
    const std::string sql =
        std::string("SELECT 1 FROM ") + table + " WHERE id = ?";

    sqlite3_stmt* statement = nullptr;

    if (sqlite3_prepare_v2(
            database_.handle(),
            sql.c_str(),
            -1,
            &statement,
            nullptr
        ) != SQLITE_OK) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }

    sqlite3_bind_int(statement, 1, recordId);

    const int result = sqlite3_step(statement);

    sqlite3_finalize(statement);

    if (result == SQLITE_DONE) {
        throw std::invalid_argument(
            std::string("Selected record does not exist in ") + table + "."
        );
    }

    if (result != SQLITE_ROW) {
        throw std::runtime_error(sqlite3_errmsg(database_.handle()));
    }
}

void AdministrationRepository::assignLecturerToCourse(
    int lecturerId,
    int courseId
)
{
    requireUserRole(lecturerId, "LECTURER");
    requireRecordExists("COURSE", courseId);

    executeRelationship(
        database_,
        "INSERT INTO LECTURER_COURSE (lecturer_id, course_id) "
        "VALUES (?, ?)",
        lecturerId,
        courseId
    );
}

void AdministrationRepository::assignHodToDepartment(
    int hodId,
    int departmentId
)
{
    requireUserRole(hodId, "HOD");
    requireRecordExists("DEPARTMENT", departmentId);

    executeRelationship(
        database_,
        "INSERT INTO DEPARTMENT_HOD (hod_id, department_id) "
        "VALUES (?, ?)",
        hodId,
        departmentId
    );
}

void AdministrationRepository::enrolStudentInCourse(
    int studentId,
    int courseId
)
{
    requireUserRole(studentId, "STUDENT");
    requireRecordExists("COURSE", courseId);

    executeRelationship(
        database_,
        "INSERT INTO ENROLMENT (student_id, course_id) "
        "VALUES (?, ?)",
        studentId,
        courseId
    );
}

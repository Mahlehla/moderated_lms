#include "data/SubmissionRepository.hpp"

#include <sqlite3.h>
#include <stdexcept>

namespace
{
    std::string statusToDatabase(
        SubmissionStatus status
    )
    {
        switch (status)
        {
            case SubmissionStatus::Submitted:
                return "SUBMITTED";

            case SubmissionStatus::Late:
                return "LATE";

            case SubmissionStatus::Marked:
                return "MARKED";

            case SubmissionStatus::Moderated:
                return "MODERATED";

            case SubmissionStatus::Released:
                return "RELEASED";
        }

        throw std::runtime_error(
            "Invalid submission status"
        );
    }

    SubmissionStatus statusFromDatabase(
        const char* value
    )
    {
        std::string status = value ? value : "";

        if (status == "SUBMITTED")
            return SubmissionStatus::Submitted;

        if (status == "LATE")
            return SubmissionStatus::Late;

        if (status == "MARKED")
            return SubmissionStatus::Marked;

        if (status == "MODERATED")
            return SubmissionStatus::Moderated;

        if (status == "RELEASED")
            return SubmissionStatus::Released;

        throw std::runtime_error(
            "Unknown submission status: " + status
        );
    }
}

std::string submissionStatusToString(
    SubmissionStatus status
)
{
    return statusToDatabase(status);
}

int SubmissionRepository::create(
    int assignmentId,
    int studentId,
    int version,
    const std::string& submittedAt,
    SubmissionStatus status,
    const std::string& content
)
{
    const char* sql =
        "INSERT INTO SUBMISSION "
        "(assignment_id, student_id, version, "
        " submitted_at, status, content) "
        "VALUES (?, ?, ?, ?, ?, ?)";

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
            "Failed to prepare submission insert"
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        assignmentId
    );

    sqlite3_bind_int(
        statement,
        2,
        studentId
    );

    sqlite3_bind_int(
        statement,
        3,
        version
    );

    sqlite3_bind_text(
        statement,
        4,
        submittedAt.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    std::string statusValue =
        statusToDatabase(status);

    sqlite3_bind_text(
        statement,
        5,
        statusValue.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    sqlite3_bind_text(
        statement,
        6,
        content.c_str(),
        -1,
        SQLITE_TRANSIENT
    );

    if (sqlite3_step(statement) != SQLITE_DONE)
    {
        std::string error =
            sqlite3_errmsg(database_.handle());

        sqlite3_finalize(statement);

        throw std::runtime_error(
            "Failed to create submission: " + error
        );
    }

    sqlite3_finalize(statement);

    return static_cast<int>(
        sqlite3_last_insert_rowid(
            database_.handle()
        )
    );
}

std::vector<Submission>
SubmissionRepository::findByAssignment(
    int assignmentId
)
{
    const char* sql =
        "SELECT id, assignment_id, student_id, "
        "version, submitted_at, status, content "
        "FROM SUBMISSION "
        "WHERE assignment_id = ? "
        "ORDER BY student_id, version";

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
            "Failed to prepare submission query"
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        assignmentId
    );

    std::vector<Submission> submissions;

    while (
        sqlite3_step(statement) ==
        SQLITE_ROW
    )
    {
        int id =
            sqlite3_column_int(statement, 0);

        int assignment =
            sqlite3_column_int(statement, 1);

        int student =
            sqlite3_column_int(statement, 2);

        int version =
            sqlite3_column_int(statement, 3);

        const char* submittedAt =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 4)
            );

        const char* status =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 5)
            );

        const char* content =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 6)
            );

        submissions.emplace_back(
            id,
            assignment,
            student,
            version,
            submittedAt ? submittedAt : "",
            statusFromDatabase(status),
            content ? content : ""
        );
    }

    sqlite3_finalize(statement);

    return submissions;
}

std::vector<Submission>
SubmissionRepository::findByStudent(
    int studentId
)
{
    const char* sql =
        "SELECT id, assignment_id, student_id, "
        "version, submitted_at, status, content "
        "FROM SUBMISSION "
        "WHERE student_id = ? "
        "ORDER BY assignment_id, version";

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
            "Failed to prepare student "
            "submission query"
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        studentId
    );

    std::vector<Submission> submissions;

    while (
        sqlite3_step(statement) ==
        SQLITE_ROW
    )
    {
        int id =
            sqlite3_column_int(statement, 0);

        int assignment =
            sqlite3_column_int(statement, 1);

        int student =
            sqlite3_column_int(statement, 2);

        int version =
            sqlite3_column_int(statement, 3);

        const char* submittedAt =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 4)
            );

        const char* status =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 5)
            );

        const char* content =
            reinterpret_cast<const char*>(
                sqlite3_column_text(statement, 6)
            );

        submissions.emplace_back(
            id,
            assignment,
            student,
            version,
            submittedAt ? submittedAt : "",
            statusFromDatabase(status),
            content ? content : ""
        );
    }

    sqlite3_finalize(statement);

    return submissions;
}

int SubmissionRepository::nextVersion(
    int assignmentId,
    int studentId
)
{
    const char* sql =
        "SELECT COALESCE(MAX(version), 0) + 1 "
        "FROM SUBMISSION "
        "WHERE assignment_id = ? "
        "AND student_id = ?";

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
            "Failed to prepare version query"
        );
    }

    sqlite3_bind_int(
        statement,
        1,
        assignmentId
    );

    sqlite3_bind_int(
        statement,
        2,
        studentId
    );

    int version = 1;

    if (
        sqlite3_step(statement) ==
        SQLITE_ROW
    )
    {
        version =
            sqlite3_column_int(
                statement,
                0
            );
    }

    sqlite3_finalize(statement);

    return version;
}

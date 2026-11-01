#include "data/Database.hpp"

#include <stdexcept>

Database::Database(const std::string& path)
{
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {

        std::string error =
            db_ ? sqlite3_errmsg(db_) : "Unknown database error";

        if (db_) {
            sqlite3_close(db_);
        }

        throw std::runtime_error(
            "Cannot open database: " + error
        );
    }
}

Database::~Database()
{
    if (db_) {
        sqlite3_close(db_);
    }
}

void Database::initialise()
{
    const char* sql = R"SQL(

        PRAGMA foreign_keys = ON;

        CREATE TABLE IF NOT EXISTS USER (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT NOT NULL UNIQUE,
            password_hash TEXT NOT NULL,
            full_name TEXT NOT NULL,

            role TEXT NOT NULL
                CHECK (
                    role IN (
                        'ADMINISTRATOR',
                        'LECTURER',
                        'HOD',
                        'STUDENT'
                    )
                )
        );

        CREATE TABLE IF NOT EXISTS DEPARTMENT (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            name TEXT NOT NULL UNIQUE
        );

        CREATE TABLE IF NOT EXISTS COURSE (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            code TEXT NOT NULL UNIQUE,
            name TEXT NOT NULL,

            department_id INTEGER NOT NULL,

            FOREIGN KEY (department_id)
                REFERENCES DEPARTMENT(id)
        );

        CREATE TABLE IF NOT EXISTS ENROLMENT (
            student_id INTEGER NOT NULL,
            course_id INTEGER NOT NULL,

            PRIMARY KEY (
                student_id,
                course_id
            ),

            FOREIGN KEY (student_id)
                REFERENCES USER(id),

            FOREIGN KEY (course_id)
                REFERENCES COURSE(id)
        );

        CREATE TABLE IF NOT EXISTS ASSIGNMENT (
            id INTEGER PRIMARY KEY AUTOINCREMENT,

            course_id INTEGER NOT NULL,

            title TEXT NOT NULL,
            description TEXT NOT NULL,
            deadline TEXT NOT NULL,

            released INTEGER NOT NULL DEFAULT 0
                CHECK (released IN (0,1)),

            FOREIGN KEY (course_id)
                REFERENCES COURSE(id)
        );

        CREATE TABLE IF NOT EXISTS SUBMISSION (
            id INTEGER PRIMARY KEY AUTOINCREMENT,

            assignment_id INTEGER NOT NULL,
            student_id INTEGER NOT NULL,

            version INTEGER NOT NULL,

            submitted_at TEXT NOT NULL,

            status TEXT NOT NULL,

            content TEXT NOT NULL,

            UNIQUE (
                assignment_id,
                student_id,
                version
            ),

            FOREIGN KEY (assignment_id)
                REFERENCES ASSIGNMENT(id),

            FOREIGN KEY (student_id)
                REFERENCES USER(id)
        );

        CREATE TABLE IF NOT EXISTS MARK (
            id INTEGER PRIMARY KEY AUTOINCREMENT,

            submission_id INTEGER NOT NULL UNIQUE,
            lecturer_id INTEGER NOT NULL,

            mark REAL NOT NULL
                CHECK (mark >= 0 AND mark <= 100),

            feedback TEXT NOT NULL,

            moderated INTEGER NOT NULL DEFAULT 0
                CHECK (moderated IN (0,1)),

            released INTEGER NOT NULL DEFAULT 0
                CHECK (released IN (0,1)),

            FOREIGN KEY (submission_id)
                REFERENCES SUBMISSION(id),

            FOREIGN KEY (lecturer_id)
                REFERENCES USER(id)
        );

        CREATE TABLE IF NOT EXISTS EXTENSION (
            id INTEGER PRIMARY KEY AUTOINCREMENT,

            assignment_id INTEGER NOT NULL,

            student_id INTEGER,

            new_deadline TEXT NOT NULL,

            FOREIGN KEY (assignment_id)
                REFERENCES ASSIGNMENT(id),

            FOREIGN KEY (student_id)
                REFERENCES USER(id)
        );

        CREATE TABLE IF NOT EXISTS AUDIT_LOG (
            id INTEGER PRIMARY KEY AUTOINCREMENT,

            actor_id INTEGER NOT NULL,

            action TEXT NOT NULL,

            entity_type TEXT NOT NULL,

            entity_id INTEGER NOT NULL,

            old_value TEXT,

            new_value TEXT,

            created_at TEXT NOT NULL,

            FOREIGN KEY (actor_id)
                REFERENCES USER(id)
        );

    )SQL";

    char* errorMessage = nullptr;

    if (sqlite3_exec(
            db_,
            sql,
            nullptr,
            nullptr,
            &errorMessage
        ) != SQLITE_OK)
    {
        std::string error =
            errorMessage
                ? errorMessage
                : "Unknown schema error";

        sqlite3_free(errorMessage);

        throw std::runtime_error(
            "Schema error: " + error
        );
    }
}

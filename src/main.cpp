#include "domain/User.hpp"
#include "data/SubmissionRepository.hpp"
#include "data/Database.hpp"
#include "data/UserRepository.hpp"
#include "data/DepartmentRepository.hpp"
#include "data/CourseRepository.hpp"
#include "data/EnrolmentRepository.hpp"
#include "data/AssignmentRepository.hpp"
#include "services/PasswordHasher.hpp"
#include "services/AuthenticationService.hpp"

#include <iostream>

int main()
{
    try {

        Database database("lms.db");

        database.initialise();

        UserRepository users(database);
        DepartmentRepository departments(database);
        CourseRepository courses(database);
        EnrolmentRepository enrolments(database);
        AssignmentRepository assignments(database);
        SubmissionRepository submissions(database);

        users.ensureDemoUser();

        AuthenticationService authentication(users);

        auto user =
            authentication.login(
                "admin",
                "admin123"
            );

        if (!user) {

            std::cout
                << "Login failed\n";

            return 1;
        }

        std::cout
            << "Login successful\n\n";

        std::cout
            << "User: "
            << user->fullName()
            << '\n';

        std::cout
            << "Role: "
            << roleToString(user->role())
            << '\n';

        std::cout
            << "Permissions: "
            << user->permissionsSummary()
            << "\n\n";


        // -------------------------------------------------
        // Department
        // -------------------------------------------------

        int departmentId = -1;

        auto existingDepartments =
            departments.findAll();

        for (const auto& department :
             existingDepartments)
        {
            if (department.name() ==
                "Computer Science")
            {
                departmentId =
                    department.id();

                break;
            }
        }

        if (departmentId == -1)
        {
            departmentId =
                departments.create(
                    "Computer Science"
                );
        }


        // -------------------------------------------------
        // Course
        // -------------------------------------------------

        int courseId = -1;

        auto existingCourses =
            courses.findByDepartment(
                departmentId
            );

        for (const auto& course :
             existingCourses)
        {
            if (course.code() ==
                "CSC4000")
            {
                courseId =
                    course.id();

                break;
            }
        }

        if (courseId == -1)
        {
            courseId =
                courses.create(
                    "CSC4000",
                    "Software Engineering",
                    departmentId
                );
        }


        std::cout
            << "Department created/located: "
            << departmentId
            << '\n';

        std::cout
            << "Course created/located: "
            << courseId
            << '\n';


        // -------------------------------------------------
        // Display courses
        // -------------------------------------------------

        std::cout
            << "\nCourses:\n";

        auto allCourses =
            courses.findAll();

        for (const auto& course :
             allCourses)
        {
            std::cout
                << "  "
                << course.code()
                << " - "
                << course.name()
                << '\n';
        }


        // -------------------------------------------------
        // Student demonstration
        // -------------------------------------------------

        int studentId = -1;

        auto student =
            users.findByUsername("student");

        if (!student)
        {
            const char* sql =
                "INSERT INTO USER "
                "(username, password_hash, "
                "full_name, role) "
                "VALUES (?, ?, ?, ?)";

            sqlite3_stmt* statement = nullptr;

            if (sqlite3_prepare_v2(
                    database.handle(),
                    sql,
                    -1,
                    &statement,
                    nullptr
                ) != SQLITE_OK)
            {
                throw std::runtime_error(
                    sqlite3_errmsg(
                        database.handle()
                    )
                );
            }

            sqlite3_bind_text(
                statement,
                1,
                "student",
                -1,
                SQLITE_STATIC
            );

            const std::string studentPasswordHash =
    PasswordHasher::hash("student123");

sqlite3_bind_text(
    statement,
    2,
    studentPasswordHash.c_str(),
    -1,
    SQLITE_TRANSIENT
);

            sqlite3_bind_text(
                statement,
                3,
                "Demo Student",
                -1,
                SQLITE_STATIC
            );

            sqlite3_bind_text(
                statement,
                4,
                "STUDENT",
                -1,
                SQLITE_STATIC
            );

            if (sqlite3_step(statement)
                != SQLITE_DONE)
            {
                sqlite3_finalize(statement);

                throw std::runtime_error(
                    sqlite3_errmsg(
                        database.handle()
                    )
                );
            }

            sqlite3_finalize(statement);

            student =
                users.findByUsername(
                    "student"
                );
        }

        if (student)
        {
            studentId =
                student->id();

            std::cout
                << "\nStudent: "
                << student->fullName()
                << '\n';

            if (!enrolments.isEnrolled(
                    studentId,
                    courseId))
            {
                enrolments.enrolStudent(
                    studentId,
                    courseId
                );

                std::cout
                    << "Student enrolled successfully.\n";
            }
            else
            {
                std::cout
                    << "Student is already enrolled.\n";
            }
        }


        // -------------------------------------------------
        // Display student's courses
        // -------------------------------------------------

        std::cout
            << "\nStudent course IDs:\n";

        auto studentCourses =
            enrolments.findCoursesForStudent(
                studentId
            );

        for (int id : studentCourses)
        {
            std::cout
                << "  Course ID: "
                << id
                << '\n';
        }
        // -------------------------------------------------
        // Assignment
        // -------------------------------------------------


        auto courseAssignments =
            assignments.findByCourse(courseId);

        if (courseAssignments.empty())
        {
            int assignmentId =
                assignments.create(
                    courseId,
                    "Software Engineering Assignment 1",
                    "Design and implement a moderated "
                    "learning management system.",
                    "2026-10-15 23:59:59"
                );

            std::cout
                << "\nAssignment created: "
                << assignmentId
                << '\n';
        }

        courseAssignments =
            assignments.findByCourse(courseId);

        std::cout
            << "\nAssignments:\n";

        for (const auto& assignment :
             courseAssignments)
        {
            std::cout
                << "  ID: "
                << assignment.id()
                << '\n';

            std::cout
                << "  Title: "
                << assignment.title()
                << '\n';

            std::cout
                << "  Description: "
                << assignment.description()
                << '\n';

            std::cout
                << "  Deadline: "
                << assignment.deadline()
                << '\n';

            std::cout
                << "  Released: "
                << (assignment.released()
                        ? "YES"
                        : "NO")
                << "\n\n";
        }
        // -------------------------------------------------
        // Student Submission
        // -------------------------------------------------

        if (!courseAssignments.empty())
        {
            int assignmentId =
                courseAssignments.front().id();

            int version =
                submissions.nextVersion(
                    assignmentId,
                    studentId
                );

            int submissionId =
                submissions.create(
                    assignmentId,
                    studentId,
                    version,
                    "2026-10-02 10:00:00",
                    SubmissionStatus::Submitted,
                    "This is the student's "
                    "Software Engineering assignment "
                    "submission."
                );

            std::cout
                << "\nSubmission created: "
                << submissionId
                << '\n';

            std::cout
                << "  Assignment ID: "
                << assignmentId
                << '\n';

            std::cout
                << "  Student ID: "
                << studentId
                << '\n';

            std::cout
                << "  Version: "
                << version
                << '\n';

            std::cout
                << "  Status: "
                << submissionStatusToString(
                       SubmissionStatus::Submitted
                   )
                << '\n';

            std::cout
                << "  Content: "
                << "This is the student's "
                   "Software Engineering assignment "
                   "submission."
                << "\n\n";
        }
        // -------------------------------------------------
        // Student Resubmission
        // -------------------------------------------------

        if (!courseAssignments.empty())
        {
            int assignmentId =
                courseAssignments.front().id();

            int version =
                submissions.nextVersion(
                    assignmentId,
                    studentId
                );

            int submissionId =
                submissions.create(
                    assignmentId,
                    studentId,
                    version,
                    "2026-10-02 10:30:00",
                    SubmissionStatus::Submitted,
                    "This is the student's revised "
                    "Software Engineering assignment "
                    "submission."
                );

            std::cout
                << "Resubmission created: "
                << submissionId
                << '\n';

            std::cout
                << "  Assignment ID: "
                << assignmentId
                << '\n';

            std::cout
                << "  Student ID: "
                << studentId
                << '\n';

            std::cout
                << "  Version: "
                << version
                << '\n';

            std::cout
                << "  Status: "
                << submissionStatusToString(
                       SubmissionStatus::Submitted
                   )
                << '\n';

            std::cout
                << "  Content: "
                << "This is the student's revised "
                   "Software Engineering assignment "
                   "submission."
                << "\n\n";
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr
            << "Error: "
            << exception.what()
            << '\n';

        return 1;
    }

    return 0;
}

#include "data/Database.hpp"
#include "utils/DatabasePath.hpp"
#include "data/UserRepository.hpp"
#include "data/DepartmentRepository.hpp"
#include "data/CourseRepository.hpp"
#include "data/AdministrationRepository.hpp"

#include "services/AuthenticationService.hpp"
#include "services/AccessControlService.hpp"

#include "domain/User.hpp"

#include <iostream>
#include <string>
#include <stdexcept>
#include <cstdlib>
#include <filesystem>
#include <limits>
#include <termios.h>
#include <unistd.h>
#include <sys/stat.h>

namespace {

// ============================================================
// INPUT AND SECURITY HELPERS
// ============================================================

std::string databasePath()
{
    return moderatedLmsDatabasePath();
}

std::string readHiddenPassword(const std::string& prompt)
{
    termios oldSettings{};

    if (tcgetattr(STDIN_FILENO, &oldSettings) != 0) {
        throw std::runtime_error(
            "Password input requires an interactive terminal."
        );
    }

    termios newSettings = oldSettings;
    newSettings.c_lflag &= static_cast<tcflag_t>(~ECHO);

    std::cout << prompt << std::flush;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &newSettings) != 0) {
        throw std::runtime_error(
            "Could not configure secure password input."
        );
    }

    std::string password;
    std::getline(std::cin, password);

    const bool restored =
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &oldSettings) == 0;

    std::cout << '\n';

    if (!restored) {
        throw std::runtime_error(
            "Could not restore terminal settings."
        );
    }

    if (!std::cin) {
        throw std::runtime_error("Password input was interrupted.");
    }

    return password;
}

std::string readRequiredLine(const std::string& prompt)
{
    std::string value;

    while (true) {
        std::cout << prompt;
        std::getline(std::cin, value);

        if (!std::cin) {
            throw std::runtime_error("Input was interrupted.");
        }

        if (value.find_first_not_of(" \t\r\n") != std::string::npos) {
            return value;
        }

        std::cout << "This field cannot be empty.\n";
    }
}

int readInteger(const std::string& prompt)
{
    while (true) {
        const std::string input = readRequiredLine(prompt);

        try {
            std::size_t position = 0;
            const int value = std::stoi(input, &position);

            if (position == input.size()) {
                return value;
            }
        } catch (const std::exception&) {
            // Display the validation message below.
        }

        std::cout << "Please enter a valid whole number.\n";
    }
}

int readPositiveInteger(const std::string& prompt)
{
    while (true) {
        const int value = readInteger(prompt);

        if (value > 0) {
            return value;
        }

        std::cout << "The ID must be greater than zero.\n";
    }
}

void requirePermission(
    const User& user,
    Permission permission,
    const AccessControlService& access
)
{
    if (!access.isAllowed(user, permission)) {
        throw std::runtime_error(
            "Access denied. You do not have permission to perform this action."
        );
    }
}

void printHeading(const std::string& heading)
{
    std::cout << "\n========================================\n";
    std::cout << " " << heading << '\n';
    std::cout << "========================================\n";
}

// ============================================================
// INITIAL ADMINISTRATOR SETUP
// ============================================================

void firstAdministratorSetup(UserRepository& users)
{
    printHeading("INITIAL ADMINISTRATOR SETUP");

    std::cout
        << "No user accounts exist. Create the first administrator.\n\n";

    const std::string fullName =
        readRequiredLine("Administrator full name: ");

    const std::string username =
        readRequiredLine("Administrator username: ");

    std::string password;

    while (true) {
        password = readHiddenPassword("Create password: ");
        const std::string confirmation =
            readHiddenPassword("Confirm password: ");

        if (password != confirmation) {
            std::cout << "Passwords do not match. Try again.\n";
            continue;
        }

        if (password.size() < 12) {
            std::cout
                << "Password must contain at least 12 characters.\n";
            continue;
        }

        break;
    }

    const int id = users.createUser(
        username,
        password,
        fullName,
        Role::Administrator
    );

    std::cout
        << "\nAdministrator account created successfully.\n"
        << "Account ID: " << id << '\n';
}

// ============================================================
// USER ACCOUNT MANAGEMENT
// ============================================================

void createUserAccount(
    UserRepository& users,
    Role role,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageUsers,
        access
    );

    printHeading("CREATE USER ACCOUNT");

    const std::string fullName =
        readRequiredLine("Full name: ");

    const std::string username =
        readRequiredLine("Username: ");

    std::string password;

    while (true) {
        password = readHiddenPassword("Create password: ");
        const std::string confirmation =
            readHiddenPassword("Confirm password: ");

        if (password != confirmation) {
            std::cout << "Passwords do not match. Try again.\n";
            continue;
        }

        if (password.size() < 12) {
            std::cout
                << "Password must contain at least 12 characters.\n";
            continue;
        }

        if (password.size() > 1024) {
            std::cout << "Password is too long.\n";
            continue;
        }

        break;
    }

    const int id = users.createUser(
        username,
        password,
        fullName,
        role
    );

    std::cout << "\nAccount created successfully.\n";
    std::cout << "User ID: " << id << '\n';
    std::cout << "Role: " << roleToString(role) << '\n';
}

void listUsers(
    UserRepository& users,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageUsers,
        access
    );

    printHeading("REGISTERED USER ACCOUNTS");

    const auto records = users.findAll();

    if (records.empty()) {
        std::cout << "No user accounts found.\n";
        return;
    }

    for (const auto& user : records) {
        std::cout
            << "ID: " << user->id()
            << " | Name: " << user->fullName()
            << " | Username: " << user->username()
            << " | Role: " << roleToString(user->role())
            << '\n';
    }

    std::cout << "\nTotal accounts: " << records.size() << '\n';
}

void userManagementMenu(
    UserRepository& users,
    const User& administrator,
    const AccessControlService& access
)
{
    while (true) {
        printHeading("USER ACCOUNT MANAGEMENT");

        std::cout
            << "1. Create Administrator\n"
            << "2. Create Lecturer\n"
            << "3. Create Head of Department\n"
            << "4. Create Student\n"
            << "5. List All Users\n"
            << "0. Back\n";

        const int choice = readInteger("\nSelect an option: ");

        try {
            switch (choice) {
                case 1:
                    createUserAccount(
                        users, Role::Administrator, administrator, access
                    );
                    break;

                case 2:
                    createUserAccount(
                        users, Role::Lecturer, administrator, access
                    );
                    break;

                case 3:
                    createUserAccount(
                        users, Role::HeadOfDepartment, administrator, access
                    );
                    break;

                case 4:
                    createUserAccount(
                        users, Role::Student, administrator, access
                    );
                    break;

                case 5:
                    listUsers(users, administrator, access);
                    break;

                case 0:
                    return;

                default:
                    std::cout << "Invalid menu option.\n";
                    break;
            }
        } catch (const std::exception& error) {
            std::cout << "\nOperation failed: "
                      << error.what() << '\n';
        }
    }
}

// ============================================================
// DEPARTMENT MANAGEMENT
// ============================================================

void manageDepartments(
    DepartmentRepository& departments,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageDepartments,
        access
    );

    while (true) {
        printHeading("DEPARTMENT MANAGEMENT");

        std::cout
            << "1. Create Department\n"
            << "2. List Departments\n"
            << "0. Back\n";

        const int choice = readInteger("\nSelect an option: ");

        try {
            if (choice == 0) {
                return;
            }

            if (choice == 1) {
                const std::string name =
                    readRequiredLine("Department name: ");

                const int id = departments.create(name);

                std::cout
                    << "\nDepartment created successfully. ID: "
                    << id << '\n';
            } else if (choice == 2) {
                const auto records = departments.findAll();

                printHeading("REGISTERED DEPARTMENTS");

                if (records.empty()) {
                    std::cout << "No departments found.\n";
                }

                for (const auto& department : records) {
                    std::cout
                        << "ID: " << department.id()
                        << " | Name: " << department.name()
                        << '\n';
                }
            } else {
                std::cout << "Invalid menu option.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "\nOperation failed: "
                      << error.what() << '\n';
        }
    }
}

// ============================================================
// COURSE MANAGEMENT
// ============================================================

void manageCourses(
    CourseRepository& courses,
    DepartmentRepository& departments,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageCourses,
        access
    );

    while (true) {
        printHeading("COURSE MANAGEMENT");

        std::cout
            << "1. Create Course\n"
            << "2. List Courses\n"
            << "0. Back\n";

        const int choice = readInteger("\nSelect an option: ");

        try {
            if (choice == 0) {
                return;
            }

            if (choice == 1) {
                const auto departmentRecords = departments.findAll();

                if (departmentRecords.empty()) {
                    std::cout
                        << "Create a department before creating a course.\n";
                    continue;
                }

                std::cout << "\nAvailable departments:\n";

                for (const auto& department : departmentRecords) {
                    std::cout
                        << department.id() << ". "
                        << department.name() << '\n';
                }

                const std::string code =
                    readRequiredLine("Course code: ");

                const std::string name =
                    readRequiredLine("Course name: ");

                const int departmentId =
                    readPositiveInteger("Department ID: ");

                const int id = courses.create(
                    code,
                    name,
                    departmentId
                );

                std::cout
                    << "\nCourse created successfully. ID: "
                    << id << '\n';

            } else if (choice == 2) {
                const auto records = courses.findAll();

                printHeading("REGISTERED COURSES");

                if (records.empty()) {
                    std::cout << "No courses found.\n";
                }

                for (const auto& course : records) {
                    std::cout
                        << "ID: " << course.id()
                        << " | Code: " << course.code()
                        << " | Name: " << course.name()
                        << " | Department ID: " << course.departmentId()
                        << '\n';
                }
            } else {
                std::cout << "Invalid menu option.\n";
            }
        } catch (const std::exception& error) {
            std::cout << "\nOperation failed: "
                      << error.what() << '\n';
        }
    }
}

// ============================================================
// USER-COURSE AND USER-DEPARTMENT ASSIGNMENTS
// ============================================================

void assignLecturer(
    AdministrationRepository& administration,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageCourses,
        access
    );

    printHeading("ASSIGN LECTURER TO COURSE");

    const int lecturerId =
        readPositiveInteger("Lecturer user ID: ");

    const int courseId =
        readPositiveInteger("Course ID: ");

    administration.assignLecturerToCourse(
        lecturerId,
        courseId
    );

    std::cout << "Lecturer assigned to course successfully.\n";
}

void assignHod(
    AdministrationRepository& administration,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageDepartments,
        access
    );

    printHeading("ASSIGN HOD TO DEPARTMENT");

    const int hodId =
        readPositiveInteger("HOD user ID: ");

    const int departmentId =
        readPositiveInteger("Department ID: ");

    administration.assignHodToDepartment(
        hodId,
        departmentId
    );

    std::cout << "HOD assigned to department successfully.\n";
}

void enrolStudent(
    AdministrationRepository& administration,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageCourses,
        access
    );

    printHeading("ENROL STUDENT IN COURSE");

    const int studentId =
        readPositiveInteger("Student user ID: ");

    const int courseId =
        readPositiveInteger("Course ID: ");

    administration.enrolStudentInCourse(
        studentId,
        courseId
    );

    std::cout << "Student enrolled successfully.\n";
}

// ============================================================
// SYSTEM OVERVIEW
// ============================================================

void showSystemOverview(
    UserRepository& users,
    DepartmentRepository& departments,
    CourseRepository& courses,
    const User& administrator,
    const AccessControlService& access
)
{
    requirePermission(
        administrator,
        Permission::ManageUsers,
        access
    );

    const auto userRecords = users.findAll();
    const auto departmentRecords = departments.findAll();
    const auto courseRecords = courses.findAll();

    printHeading("SYSTEM OVERVIEW");

    std::cout
        << "Total user accounts: " << userRecords.size() << '\n'
        << "Total departments: " << departmentRecords.size() << '\n'
        << "Total courses: " << courseRecords.size() << '\n';

    int administrators = 0;
    int lecturers = 0;
    int hods = 0;
    int students = 0;

    for (const auto& user : userRecords) {
        switch (user->role()) {
            case Role::Administrator:
                ++administrators;
                break;

            case Role::Lecturer:
                ++lecturers;
                break;

            case Role::HeadOfDepartment:
                ++hods;
                break;

            case Role::Student:
                ++students;
                break;
        }
    }

    std::cout
        << "\nUSER BREAKDOWN\n"
        << "Administrators: " << administrators << '\n'
        << "Lecturers: " << lecturers << '\n'
        << "Heads of Department: " << hods << '\n'
        << "Students: " << students << '\n';
}

// ============================================================
// ADMINISTRATOR DASHBOARD
// ============================================================

void administratorDashboard(
    User& administrator,
    UserRepository& users,
    DepartmentRepository& departments,
    CourseRepository& courses,
    AdministrationRepository& administration,
    const AccessControlService& access
)
{
    while (true) {
        printHeading("ADMINISTRATOR DASHBOARD");

        std::cout
            << "Logged in as: " << administrator.fullName() << "\n\n"
            << "1. Manage User Accounts\n"
            << "2. Manage Departments\n"
            << "3. Manage Courses\n"
            << "4. Assign Lecturer to Course\n"
            << "5. Assign HOD to Department\n"
            << "6. Enrol Student in Course\n"
            << "7. View System Overview\n"
            << "0. Logout\n";

        const int choice = readInteger("\nSelect an option: ");

        try {
            switch (choice) {
                case 1:
                    userManagementMenu(
                        users, administrator, access
                    );
                    break;

                case 2:
                    manageDepartments(
                        departments, administrator, access
                    );
                    break;

                case 3:
                    manageCourses(
                        courses,
                        departments,
                        administrator,
                        access
                    );
                    break;

                case 4:
                    assignLecturer(
                        administration, administrator, access
                    );
                    break;

                case 5:
                    assignHod(
                        administration, administrator, access
                    );
                    break;

                case 6:
                    enrolStudent(
                        administration, administrator, access
                    );
                    break;

                case 7:
                    showSystemOverview(
                        users,
                        departments,
                        courses,
                        administrator,
                        access
                    );
                    break;

                case 0:
                    std::cout << "\nLogging out...\n";
                    return;

                default:
                    std::cout << "Invalid menu option.\n";
                    break;
            }
        } catch (const std::exception& error) {
            std::cout << "\nOperation failed: "
                      << error.what() << '\n';
        }
    }
}

} // namespace

// ============================================================
// APPLICATION ENTRY POINT
// ============================================================

int main()
{
    try {
        const mode_t previousMask = umask(0077);

        const std::string path = databasePath();

        Database database(path);
        database.initialise();

        umask(previousMask);

        chmod(path.c_str(), 0600);

        UserRepository users(database);
        DepartmentRepository departments(database);
        CourseRepository courses(database);
        AdministrationRepository administration(database);

        AuthenticationService authentication(users);
        AccessControlService access;

        if (users.countUsers() == 0) {
            firstAdministratorSetup(users);
        }

        while (true) {
            printHeading("MODERATED LEARNING MANAGEMENT SYSTEM");

            std::unique_ptr<User> authenticatedUser;

            while (!authenticatedUser) {
                std::string username;

                std::cout << "Username (or q to quit): ";
                std::getline(std::cin, username);

                if (!std::cin || username == "q" || username == "Q") {
                    std::cout << "Application closed.\n";
                    return 0;
                }

                const std::string password =
                    readHiddenPassword("Password: ");

                authenticatedUser =
                    authentication.login(username, password);

                if (!authenticatedUser) {
                    std::cout
                        << "\nInvalid username or password.\n\n";
                }
            }

            std::cout << "\nLogin successful.\n";
            std::cout << "Welcome, "
                      << authenticatedUser->fullName() << ".\n";

            std::cout << "Role: "
                      << roleToString(authenticatedUser->role()) << '\n';

            std::cout << "Permissions: "
                      << authenticatedUser->permissionsSummary()
                      << "\n";

            if (authenticatedUser->role() == Role::Administrator) {
                administratorDashboard(
                    *authenticatedUser,
                    users,
                    departments,
                    courses,
                    administration,
                    access
                );
            } else {
                std::cout
                    << "\nThe dashboard for this role has not yet "
                    << "been implemented.\n";
                    std::cout << "Please log out and try again later.\n";

                std::cout << "\nPress Enter to return to login.";
                std::string ignored;
                std::getline(std::cin, ignored);
            }

            std::cout << "\nYou have been logged out.\n";
        }
    }
    catch (const std::exception& error) {
        std::cerr << "\nApplication error: "
                  << error.what() << '\n';
        return 1;
    }
}

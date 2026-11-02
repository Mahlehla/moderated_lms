cmake_minimum_required(VERSION 3.16)

project(ModeratedLMS
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)
find_package(OpenSSL REQUIRED)

add_executable(lms
    src/main.cpp
    src/data/Database.cpp
    src/data/UserRepository.cpp
    src/data/AdministrationRepository.cpp
    src/data/DepartmentRepository.cpp
    src/data/CourseRepository.cpp
    src/data/EnrolmentRepository.cpp
    src/data/AssigmentRepository.cpp
    src/data/SubmissionRepository.cpp
    src/services/StandardLateSubmissionStrategy.cpp
    src/services/AuthenticationService.cpp
    src/services/PasswordHasher.cpp
    src/services/AccessControlService.cpp
    src/services/AssessmentWorkflowService.cpp
)

target_include_directories(lms PRIVATE
    include
)

target_link_libraries(lms PRIVATE
    SQLite3::SQLite3 OpenSSL::Crypto
)

target_compile_options(lms PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

enable_testing()

add_executable(lms_tests
    tests/test_week4.cpp
    src/data/Database.cpp
    src/data/AssigmentRepository.cpp
    src/services/StandardLateSubmissionStrategy.cpp
    src/services/AccessControlService.cpp
)

target_include_directories(lms_tests PRIVATE include)
target_link_libraries(lms_tests PRIVATE SQLite3::SQLite3 OpenSSL::Crypto)
target_compile_options(lms_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME Week4Tests COMMAND lms_tests)


add_executable(lms_auth_tests
    tests/test_week5_auth.cpp
    src/data/Database.cpp
    src/data/UserRepository.cpp

    src/services/AuthenticationService.cpp
    src/services/PasswordHasher.cpp
)

target_include_directories(lms_auth_tests PRIVATE include)
target_link_libraries(lms_auth_tests PRIVATE SQLite3::SQLite3 OpenSSL::Crypto)
target_compile_options(lms_auth_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME Week5AuthenticationTests COMMAND lms_auth_tests)


add_executable(lms_access_tests
    tests/test_week5_access.cpp
    src/services/AccessControlService.cpp
)

target_include_directories(lms_access_tests PRIVATE include)
target_compile_options(lms_access_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(
    NAME Week5AccessControlTests
    COMMAND lms_access_tests
)


add_executable(lms_workflow_tests
    tests/test_week5_workflow.cpp
    src/data/Database.cpp
    src/services/AccessControlService.cpp
    src/services/AssessmentWorkflowService.cpp
    src/services/StandardLateSubmissionStrategy.cpp
)

target_include_directories(lms_workflow_tests PRIVATE include)
target_link_libraries(lms_workflow_tests PRIVATE SQLite3::SQLite3)
target_compile_options(lms_workflow_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(
    NAME Week5AssessmentWorkflowTests
    COMMAND lms_workflow_tests
)

# Week 6: Quality Assurance Tests
add_executable(lms_week6_qa_tests
    tests/test_week6_qa.cpp
    src/data/Database.cpp
    src/data/SubmissionRepository.cpp
    src/services/AccessControlService.cpp
    src/services/AssessmentWorkflowService.cpp
    src/services/StandardLateSubmissionStrategy.cpp
)

target_include_directories(lms_week6_qa_tests PRIVATE include)

target_link_libraries(lms_week6_qa_tests PRIVATE
    SQLite3::SQLite3
)

target_compile_options(lms_week6_qa_tests PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

add_test(
    NAME Week6QATests
    COMMAND lms_week6_qa_tests
)

add_executable(lms_admin_repo_tests
    tests/test_administration_repository.cpp
    src/data/AdministrationRepository.cpp
    src/data/Database.cpp
)

target_include_directories(lms_admin_repo_tests PRIVATE include)
target_link_libraries(lms_admin_repo_tests PRIVATE SQLite3::SQLite3)
target_compile_options(lms_admin_repo_tests PRIVATE
    -Wall -Wextra -Wpedantic
)

add_test(
    NAME AdministrationRepositoryTests
    COMMAND lms_admin_repo_tests
)

# ============================================================
# Qt 6 Desktop GUI
# ============================================================

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(lms_gui
    src/gui/main.cpp
    src/data/Database.cpp
    src/data/UserRepository.cpp
    src/data/DepartmentRepository.cpp
    src/data/CourseRepository.cpp
    src/data/EnrolmentRepository.cpp
    src/data/AdministrationRepository.cpp
    src/data/AssigmentRepository.cpp
    src/data/SubmissionRepository.cpp
    src/services/AccessControlService.cpp
    src/services/AssessmentWorkflowService.cpp
    src/services/AuthenticationService.cpp
    src/services/PasswordHasher.cpp
    src/services/StandardLateSubmissionStrategy.cpp
)

target_include_directories(lms_gui PRIVATE include)

target_link_libraries(lms_gui PRIVATE
    SQLite3::SQLite3
    OpenSSL::Crypto
    Qt6::Widgets
)

target_compile_options(lms_gui PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)


# Lecturer assignment ownership tests
add_executable(lms_lecturer_assignment_tests
    tests/test_lecturer_assignment.cpp
    src/data/Database.cpp
    src/data/AssigmentRepository.cpp
    src/services/AccessControlService.cpp
)

target_include_directories(lms_lecturer_assignment_tests PRIVATE include)

target_link_libraries(lms_lecturer_assignment_tests
    PRIVATE SQLite3::SQLite3 OpenSSL::Crypto
)

target_compile_options(lms_lecturer_assignment_tests
    PRIVATE -Wall -Wextra -Wpedantic
)

add_test(
    NAME LecturerAssignmentTests
    COMMAND lms_lecturer_assignment_tests
)

# Phase 6: Optional code coverage instrumentation
option(ENABLE_COVERAGE "Enable GCC code coverage instrumentation" OFF)

if(ENABLE_COVERAGE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        foreach(target IN ITEMS
            lms
            lms_tests

            lms_auth_tests
            lms_access_tests
            lms_workflow_tests
            lms_week6_qa_tests
            lms_admin_repo_tests
            lms_lecturer_assignment_tests
            lms_gui
        )
            target_compile_options(${target} PRIVATE
                --coverage
                -O0
                -g
            )
            target_link_options(${target} PRIVATE --coverage)
        endforeach()
    else()
        message(FATAL_ERROR "Coverage instrumentation is configured for GCC only.")
    endif()
endif()

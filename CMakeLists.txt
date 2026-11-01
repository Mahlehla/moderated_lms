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

cmake_minimum_required(VERSION 3.16)

project(ModeratedLMS
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

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
)

target_include_directories(lms PRIVATE
    include
)

target_link_libraries(lms PRIVATE
    SQLite3::SQLite3
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
target_link_libraries(lms_tests PRIVATE SQLite3::SQLite3)
target_compile_options(lms_tests PRIVATE -Wall -Wextra -Wpedantic)

add_test(NAME Week4Tests COMMAND lms_tests)

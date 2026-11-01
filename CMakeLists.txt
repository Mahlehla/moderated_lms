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

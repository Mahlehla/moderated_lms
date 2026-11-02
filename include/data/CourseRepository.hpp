#pragma once

#include "data/Database.hpp"
#include "domain/Course.hpp"

#include <string>
#include <vector>

class CourseRepository {
private:
    Database& database_;

public:
    explicit CourseRepository(Database& database)
        : database_(database)
    {
    }

    int create(
        const std::string& code,
        const std::string& name,
        int departmentId
    );

    std::vector<Course> findAll();

    std::vector<Course>
    findByDepartment(int departmentId);
    std::vector<Course> findByLecturer(int lecturerId);
};

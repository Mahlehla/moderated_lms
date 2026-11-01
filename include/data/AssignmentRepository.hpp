#pragma once

#include "data/Database.hpp"
#include "domain/Assignment.hpp"

#include <string>
#include <vector>

class AssignmentRepository {
private:
    Database& database_;

public:
    explicit AssignmentRepository(Database& database)
        : database_(database)
    {
    }

    int create(
        int courseId,
        const std::string& title,
        const std::string& description,
        const std::string& deadline
    );

    std::vector<Assignment>
    findByCourse(int courseId);

    Assignment* findById(int assignmentId);
};

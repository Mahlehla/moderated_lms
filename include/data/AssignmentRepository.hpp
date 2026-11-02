#pragma once

#include <optional>
#include <string>
#include <vector>
#include "domain/User.hpp"
#include "data/Database.hpp"
#include "domain/Assignment.hpp"

class AssignmentRepository {
private:
    Database& database_;

public:
    explicit AssignmentRepository(Database& db) : database_(db) {}

    int create(
        int courseId,
        const std::string& title,
        const std::string& description,
        const std::string& deadline,
        double maxMark = 100.0
    );
    int createForLecturer(
    const User& lecturer,
    int courseId,
    const std::string& title,
    const std::string& description,
    const std::string& deadline,
    double maxMark = 100.0
);
    std::vector<Assignment> findByCourse(int courseId);
    std::optional<Assignment> findById(int assignmentId);
};

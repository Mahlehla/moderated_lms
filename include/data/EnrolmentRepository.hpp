#pragma once

#include "data/Database.hpp"

#include <vector>

class EnrolmentRepository {
private:
    Database& database_;

public:
    explicit EnrolmentRepository(Database& database)
        : database_(database)
    {
    }

    void enrolStudent(
        int studentId,
        int courseId
    );

    bool isEnrolled(
        int studentId,
        int courseId
    ) const;

    std::vector<int>
    findCoursesForStudent(int studentId);
};

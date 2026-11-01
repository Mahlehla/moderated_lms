#pragma once

#include "data/Database.hpp"
#include "domain/Submission.hpp"

#include <string>
#include <vector>

class SubmissionRepository
{
private:
    Database& database_;

public:
    explicit SubmissionRepository(Database& database)
        : database_(database)
    {
    }

    int create(
        int assignmentId,
        int studentId,
        int version,
        const std::string& submittedAt,
        SubmissionStatus status,
        const std::string& content
    );

    std::vector<Submission> findByAssignment(
        int assignmentId
    );

    std::vector<Submission> findByStudent(
        int studentId
    );

    int nextVersion(
        int assignmentId,
        int studentId
    );
};

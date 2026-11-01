#pragma once

#include "data/Database.hpp"
#include "domain/User.hpp"

#include <optional>
#include <string>

class AssessmentWorkflowService {
private:
    Database& database_;

    void writeAudit(
        int actorId,
        const std::string& action,
        const std::string& entityType,
        int entityId,
        const std::string& oldValue,
        const std::string& newValue,
        const std::string& timestamp
    );

    void updateSubmissionStatus(
        int submissionId,
        const std::string& status
    );

public:
    explicit AssessmentWorkflowService(Database& database)
        : database_(database) {}

    void markSubmission(
        const User& lecturer,
        int submissionId,
        double mark,
        const std::string& feedback,
        const std::string& timestamp
    );

    void grantExtension(
        const User& actor,
        int assignmentId,
        std::optional<int> studentId,
        const std::string& newDeadline,
        const std::string& timestamp
    );

    std::string effectiveDeadline(
        int assignmentId,
        int studentId,
        const std::string& originalDeadline
    ) const;

    void moderateSubmission(
        const User& headOfDepartment,
        int submissionId,
        const std::string& timestamp
    );

    void releaseMark(
        const User& headOfDepartment,
        int submissionId,
        const std::string& timestamp
    );

    bool isMarkReleased(int submissionId) const;
};

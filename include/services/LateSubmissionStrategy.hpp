#pragma once

#include <string>

class LateSubmissionStrategy {
public:
    virtual ~LateSubmissionStrategy() = default;

    virtual bool isLate(
        const std::string& submissionTime,
        const std::string& deadline
    ) const = 0;

    virtual int penaltyPercent(
        const std::string& submissionTime,
        const std::string& deadline
    ) const = 0;
};

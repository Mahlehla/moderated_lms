#pragma once

#include "services/LateSubmissionStrategy.hpp"

class StandardLateSubmissionStrategy final : public LateSubmissionStrategy {
public:
    bool isLate(
        const std::string& submissionTime,
        const std::string& deadline
    ) const override;

    int penaltyPercent(
        const std::string& submissionTime,
        const std::string& deadline
    ) const override;
};

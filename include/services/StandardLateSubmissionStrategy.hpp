#pragma once

#include "services/LateSubmissionStrategy.hpp"

class StandardLateSubmissionStrategy
    : public LateSubmissionStrategy
{
public:
    bool isLate(
        const std::string& submissionTime,
        const std::string& deadline
    ) const override;
};

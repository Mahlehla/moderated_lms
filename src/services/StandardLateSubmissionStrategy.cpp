#include "services/StandardLateSubmissionStrategy.hpp"

bool StandardLateSubmissionStrategy::isLate(
    const std::string& submissionTime,
    const std::string& deadline
) const
{
    return submissionTime > deadline;
}

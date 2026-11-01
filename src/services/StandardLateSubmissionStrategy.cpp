#include "services/StandardLateSubmissionStrategy.hpp"

#include <ctime>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace {
    std::time_t parseTimestamp(std::string value) {
        // Accept either ISO separator: 'T' or a space.
        if (value.size() >= 11 && value[10] == 'T') {
            value[10] = ' ';
        }

        std::tm parsed{};
        std::istringstream input(value);
        input >> std::get_time(&parsed, "%Y-%m-%d %H:%M:%S");

        if (input.fail()) {
            throw std::invalid_argument(
                "Timestamp must use YYYY-MM-DD HH:MM:SS format."
            );
        }

        input >> std::ws;
        if (!input.eof()) {
            throw std::invalid_argument("Timestamp contains unexpected text.");
        }

        parsed.tm_isdst = 0;
        const std::time_t result = timegm(&parsed);

        if (result == static_cast<std::time_t>(-1)) {
            throw std::invalid_argument("Invalid timestamp.");
        }

        return result;
    }
}

bool StandardLateSubmissionStrategy::isLate(
    const std::string& submissionTime,
    const std::string& deadline
) const {
    return parseTimestamp(submissionTime) > parseTimestamp(deadline);
}

int StandardLateSubmissionStrategy::penaltyPercent(
    const std::string& submissionTime,
    const std::string& deadline
) const {
    const std::time_t submitted = parseTimestamp(submissionTime);
    const std::time_t due = parseTimestamp(deadline);

    const double secondsLate = std::difftime(submitted, due);

    if (secondsLate <= 0) {
        return 0;
    }

    constexpr double secondsPerDay = 24.0 * 60.0 * 60.0;

    // Each commenced 24-hour period attracts a 10 percentage-point penalty.
    const int commencedPeriods =
        static_cast<int>((secondsLate + secondsPerDay - 1) / secondsPerDay);

    if (commencedPeriods > 3) {
        return 100;
    }

    return commencedPeriods * 10;
}

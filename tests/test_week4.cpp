#include "data/AssignmentRepository.hpp"
#include "services/StandardLateSubmissionStrategy.hpp"

#include <cassert>
#include <cstdio>
#include <iostream>
#include <stdexcept>

int main() {
    StandardLateSubmissionStrategy strategy;

    // On-time and late-submission boundaries.
    assert(!strategy.isLate(
        "2026-10-01 09:00:00", "2026-10-01 10:00:00"
    ));
    assert(strategy.penaltyPercent(
        "2026-10-01 10:00:00", "2026-10-01 10:00:00"
    ) == 0);
    assert(strategy.penaltyPercent(
        "2026-10-01 10:00:01", "2026-10-01 10:00:00"
    ) == 10);
    assert(strategy.penaltyPercent(
        "2026-10-02 10:00:00", "2026-10-01 10:00:00"
    ) == 10);
    assert(strategy.penaltyPercent(
        "2026-10-02 10:00:01", "2026-10-01 10:00:00"
    ) == 20);
    assert(strategy.penaltyPercent(
        "2026-10-04 10:00:00", "2026-10-01 10:00:00"
    ) == 30);
    assert(strategy.penaltyPercent(
        "2026-10-04 10:00:01", "2026-10-01 10:00:00"
    ) == 100);

    // Accept the ISO 'T' separator as well.
    assert(strategy.penaltyPercent(
        "2026-10-01T10:00:01", "2026-10-01T10:00:00"
    ) == 10);

    // Invalid timestamps must be rejected.
    bool invalidTimestampRejected = false;
    try {
        strategy.penaltyPercent("not-a-date", "2026-10-01 10:00:00");
    } catch (const std::invalid_argument&) {
        invalidTimestampRejected = true;
    }
    assert(invalidTimestampRejected);

    // Assignment persistence tests use a separate temporary database.
    const char* databasePath = "/tmp/moderated_lms_week4_test.db";
    std::remove(databasePath);

    {
        Database database(databasePath);
        database.initialise();

        // The isolated test database does not need course fixtures.
        sqlite3_exec(database.handle(), "PRAGMA foreign_keys = OFF;",
                     nullptr, nullptr, nullptr);

        AssignmentRepository assignments(database);

        const int id = assignments.create(
            1,
            "Week 4 Test Assignment",
            "Testing assignment persistence",
            "2026-11-15 23:59:59",
            50.0
        );

        const auto found = assignments.findById(id);
        assert(found.has_value());
        assert(found->id() == id);
        assert(found->courseId() == 1);
        assert(found->title() == "Week 4 Test Assignment");
        assert(found->maxMark() == 50.0);

        const auto courseAssignments = assignments.findByCourse(1);
        assert(!courseAssignments.empty());
        assert(courseAssignments.front().maxMark() == 50.0);

        assert(!assignments.findById(999999).has_value());

        bool invalidMarkRejected = false;
        try {
            assignments.create(1, "Invalid", "Test",
                               "2026-11-15 23:59:59", 0.0);
        } catch (const std::invalid_argument&) {
            invalidMarkRejected = true;
        }
        assert(invalidMarkRejected);
                // Maximum mark of 100 is valid.
        const int hundredMarkId = assignments.create(
            1,
            "Maximum Boundary Test",
            "Testing the upper valid boundary",
            "2026-11-15 23:59:59",
            100.0
        );

        const auto hundredMarkAssignment =
            assignments.findById(hundredMarkId);

        assert(hundredMarkAssignment.has_value());
        assert(hundredMarkAssignment->maxMark() == 100.0);

        // Maximum mark above 100 must be rejected.
        bool excessiveMarkRejected = false;
        try {
            assignments.create(
                1, "Excessive", "Test",
                "2026-11-15 23:59:59", 100.01
            );
        } catch (const std::invalid_argument&) {
            excessiveMarkRejected = true;
        }
        assert(excessiveMarkRejected);

        // Negative maximum mark must be rejected.
        bool negativeMarkRejected = false;
        try {
            assignments.create(
                1, "Negative", "Test",
                "2026-11-15 23:59:59", -10.0
            );
        } catch (const std::invalid_argument&) {
            negativeMarkRejected = true;
        }
        assert(negativeMarkRejected);

        bool emptyTitleRejected = false;
        try {
            assignments.create(1, "", "Test",
                               "2026-11-15 23:59:59", 100.0);
        } catch (const std::invalid_argument&) {
            emptyTitleRejected = true;
        }
        assert(emptyTitleRejected);
    }

    std::remove(databasePath);

    std::cout << "All Week 4 tests passed.\n";
    return 0;
}

#pragma once

#include <string>

enum class SubmissionStatus
{
    Submitted,
    Late,
    Marked,
    Moderated,
    Released
};

std::string submissionStatusToString(
    SubmissionStatus status
);

class Submission
{
private:
    int id_;
    int assignmentId_;
    int studentId_;
    int version_;
    std::string submittedAt_;
    SubmissionStatus status_;
    std::string content_;

public:
    Submission(
        int id,
        int assignmentId,
        int studentId,
        int version,
        std::string submittedAt,
        SubmissionStatus status,
        std::string content
    )
        : id_(id),
          assignmentId_(assignmentId),
          studentId_(studentId),
          version_(version),
          submittedAt_(std::move(submittedAt)),
          status_(status),
          content_(std::move(content))
    {
    }

    int id() const
    {
        return id_;
    }

    int assignmentId() const
    {
        return assignmentId_;
    }

    int studentId() const
    {
        return studentId_;
    }

    int version() const
    {
        return version_;
    }

    const std::string& submittedAt() const
    {
        return submittedAt_;
    }

    SubmissionStatus status() const
    {
        return status_;
    }

    const std::string& content() const
    {
        return content_;
    }
};

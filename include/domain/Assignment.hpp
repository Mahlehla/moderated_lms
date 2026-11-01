#pragma once

#include <string>

class Assignment {
private:
    int id_;
    int courseId_;
    std::string title_;
    std::string description_;
    std::string deadline_;
    bool released_;

public:
    Assignment(
        int id,
        int courseId,
        std::string title,
        std::string description,
        std::string deadline,
        bool released
    )
        : id_(id),
          courseId_(courseId),
          title_(std::move(title)),
          description_(std::move(description)),
          deadline_(std::move(deadline)),
          released_(released)
    {
    }

    int id() const {
        return id_;
    }

    int courseId() const {
        return courseId_;
    }

    const std::string& title() const {
        return title_;
    }

    const std::string& description() const {
        return description_;
    }

    const std::string& deadline() const {
        return deadline_;
    }

    bool released() const {
        return released_;
    }
};

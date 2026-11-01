#pragma once

#include <string>

class Course {
private:
    int id_;
    std::string code_;
    std::string name_;
    int departmentId_;

public:
    Course(
        int id,
        std::string code,
        std::string name,
        int departmentId
    )
        : id_(id),
          code_(std::move(code)),
          name_(std::move(name)),
          departmentId_(departmentId)
    {
    }

    int id() const {
        return id_;
    }

    const std::string& code() const {
        return code_;
    }

    const std::string& name() const {
        return name_;
    }

    int departmentId() const {
        return departmentId_;
    }
};

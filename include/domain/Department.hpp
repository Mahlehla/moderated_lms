#pragma once

#include <string>

class Department {
private:
    int id_;
    std::string name_;

public:
    Department(int id, std::string name)
        : id_(id),
          name_(std::move(name))
    {
    }

    int id() const {
        return id_;
    }

    const std::string& name() const {
        return name_;
    }
};

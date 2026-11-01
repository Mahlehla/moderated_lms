#pragma once

#include <memory>
#include <string>

enum class Role {
    Administrator,
    Lecturer,
    HeadOfDepartment,
    Student
};

std::string roleToString(Role role);

class User {
protected:
    int id_;
    std::string username_;
    std::string passwordHash_;
    std::string fullName_;
    Role role_;

public:
    User(
        int id,
        std::string username,
        std::string passwordHash,
        std::string fullName,
        Role role
    )
        : id_(id),
          username_(std::move(username)),
          passwordHash_(std::move(passwordHash)),
          fullName_(std::move(fullName)),
          role_(role) {}

    virtual ~User() = default;

    virtual std::string permissionsSummary() const = 0;

    int id() const {
        return id_;
    }

    const std::string& username() const {
        return username_;
    }

    const std::string& passwordHash() const {
        return passwordHash_;
    }

    const std::string& fullName() const {
        return fullName_;
    }

    Role role() const {
        return role_;
    }
};

class Administrator final : public User {
public:
    using User::User;

    std::string permissionsSummary() const override {
        return "Manage users, courses and departments";
    }
};

class Lecturer final : public User {
public:
    using User::User;

    std::string permissionsSummary() const override {
        return "Create assignments, mark and provide feedback";
    }
};

class HeadOfDepartment final : public User {
public:
    using User::User;

    std::string permissionsSummary() const override {
        return "Moderate and release marks";
    }
};

class Student final : public User {
public:
    using User::User;

    std::string permissionsSummary() const override {
        return "Submit work and view released marks";
    }
};

std::unique_ptr<User> makeUser(
    int id,
    const std::string& username,
    const std::string& passwordHash,
    const std::string& fullName,
    Role role
);

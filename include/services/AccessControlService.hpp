#pragma once

#include "domain/User.hpp"

enum class Permission {
    ManageUsers,
    ManageCourses,
    ManageDepartments,
    CreateAssignments,
    ViewSubmissions,
    MarkSubmissions,
    SubmitAssignments,
    ViewReleasedMarks,
    ModerateMarks,
    ReleaseMarks,
    GrantExtensions
};

class AccessControlService {
public:
    bool hasPermission(
        Role role,
        Permission permission
    ) const;

    bool isAllowed(
        const User& user,
        Permission permission
    ) const;
};

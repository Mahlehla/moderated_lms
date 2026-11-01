#include "services/AccessControlService.hpp"

bool AccessControlService::hasPermission(
    Role role,
    Permission permission
) const {
    switch (role) {

        case Role::Administrator:
            switch (permission) {
                case Permission::ManageUsers:
                case Permission::ManageCourses:
                case Permission::ManageDepartments:
                case Permission::GrantExtensions:
                    return true;
                default:
                    return false;
            }

        case Role::Lecturer:
            switch (permission) {
                case Permission::CreateAssignments:
                case Permission::ViewSubmissions:
                case Permission::MarkSubmissions:
                    return true;
                default:
                    return false;
            }

        case Role::HeadOfDepartment:
            switch (permission) {
                case Permission::ViewSubmissions:
                case Permission::ModerateMarks:
                case Permission::ReleaseMarks:
                case Permission::GrantExtensions:
                    return true;
                default:
                    return false;
            }

        case Role::Student:
            switch (permission) {
                case Permission::SubmitAssignments:
                case Permission::ViewReleasedMarks:
                    return true;
                default:
                    return false;
            }
    }

    return false;
}

bool AccessControlService::isAllowed(
    const User& user,
    Permission permission
) const {
    return hasPermission(user.role(), permission);
}

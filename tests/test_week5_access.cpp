#include "services/AccessControlService.hpp"

#include <cassert>
#include <iostream>

int main() {
    AccessControlService access;

    // Administrator permissions
    assert(access.hasPermission(Role::Administrator, Permission::ManageUsers));
    assert(access.hasPermission(Role::Administrator, Permission::ManageCourses));
    assert(access.hasPermission(Role::Administrator, Permission::ManageDepartments));
    assert(!access.hasPermission(Role::Administrator, Permission::MarkSubmissions));

    // Lecturer permissions
    assert(access.hasPermission(Role::Lecturer, Permission::CreateAssignments));
    assert(access.hasPermission(Role::Lecturer, Permission::ViewSubmissions));
    assert(access.hasPermission(Role::Lecturer, Permission::MarkSubmissions));
    assert(!access.hasPermission(Role::Lecturer, Permission::ReleaseMarks));
    assert(!access.hasPermission(Role::Lecturer, Permission::ModerateMarks));

    // Head of Department permissions
    assert(access.hasPermission(Role::HeadOfDepartment, Permission::ViewSubmissions));
    assert(access.hasPermission(Role::HeadOfDepartment, Permission::ModerateMarks));
    assert(access.hasPermission(Role::HeadOfDepartment, Permission::ReleaseMarks));
    assert(!access.hasPermission(Role::HeadOfDepartment, Permission::MarkSubmissions));

    // Student permissions
    assert(access.hasPermission(Role::Student, Permission::SubmitAssignments));
    assert(access.hasPermission(Role::Student, Permission::ViewReleasedMarks));
    assert(!access.hasPermission(Role::Student, Permission::MarkSubmissions));
    assert(!access.hasPermission(Role::Student, Permission::ManageUsers));

    // Test authorization using a real User object.
    Student student(
        1, "student1", "hashed-password", "Test Student", Role::Student
    );

    assert(access.isAllowed(student, Permission::SubmitAssignments));
    assert(!access.isAllowed(student, Permission::ReleaseMarks));

    std::cout << "Week 5 access-control tests passed.\n";
    return 0;
}

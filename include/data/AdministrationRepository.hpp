#pragma once

#include "data/Database.hpp"

class AdministrationRepository {
private:
    Database& database_;

    void requireUserRole(int userId, const char* expectedRole);
    void requireRecordExists(
        const char* table,
        int recordId
    );

public:
    explicit AdministrationRepository(Database& database)
        : database_(database)
    {
    }

    void assignLecturerToCourse(int lecturerId, int courseId);

    void assignHodToDepartment(int hodId, int departmentId);

    void enrolStudentInCourse(int studentId, int courseId);
};

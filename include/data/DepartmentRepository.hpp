#pragma once

#include "data/Database.hpp"
#include "domain/Department.hpp"

#include <string>
#include <vector>

class DepartmentRepository {
private:
    Database& database_;

public:
    explicit DepartmentRepository(Database& database)
        : database_(database)
    {
    }

    int create(const std::string& name);

    std::vector<Department> findAll();
};

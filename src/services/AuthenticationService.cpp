#include "services/AuthenticationService.hpp"

std::unique_ptr<User>
AuthenticationService::login(
    const std::string& username,
    const std::string& password
)
{
    auto user =
        users_.findByUsername(username);

    if (!user) {
        return nullptr;
    }

    if (user->passwordHash() != password) {
        return nullptr;
    }

    return user;
}

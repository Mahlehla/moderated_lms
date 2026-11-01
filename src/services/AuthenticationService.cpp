#include "services/AuthenticationService.hpp"
#include "services/PasswordHasher.hpp"

std::unique_ptr<User>
AuthenticationService::login(
    const std::string& username,
    const std::string& password
)
{
    auto user = users_.findByUsername(username);

    if (!user || !PasswordHasher::verify(password, user->passwordHash())) {
        return nullptr;
    }

    // Upgrade legacy plaintext credentials after successful verification.
    if (!PasswordHasher::isHashed(user->passwordHash())) {
        users_.updatePasswordHash(username, PasswordHasher::hash(password));
    }

    return user;
}

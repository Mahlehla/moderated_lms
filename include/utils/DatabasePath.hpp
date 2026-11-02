#pragma once

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <sys/stat.h>
#include <string>

inline std::string moderatedLmsDatabasePath()
{
    const char* xdgData = std::getenv("XDG_DATA_HOME");
    const char* home = std::getenv("HOME");

    std::filesystem::path base;

    if (xdgData && *xdgData) {
        base = xdgData;
    } else if (home && *home) {
        base = std::filesystem::path(home) / ".local" / "share";
    } else {
        throw std::runtime_error(
            "Cannot determine the user's data directory."
        );
    }

    const auto directory = base / "moderated-lms";

    std::filesystem::create_directories(directory);

    if (chmod(directory.c_str(), 0700) != 0) {
        throw std::runtime_error(
            "Cannot secure the LMS data directory."
        );
    }

    return (directory / "lms.db").string();
}

#pragma once

#include <string>
#include <optional>

struct User {
    int id{0};
    std::string username;
    std::string password;
    bool isActive{true};
    std::string role{"user"};
    std::string createdAt;
    std::optional<std::string> updatedAt;
    std::optional<std::string> lastLogin;
};

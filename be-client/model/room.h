#pragma once

#include <string>
#include <optional>

struct Room {
    int id{0};
    std::string roomName;
    int productId{0};
    int duration{0};
    std::string status{"waiting"};
    int hostUserId{0};
    std::optional<std::string> createdAt;
    std::optional<std::string> startedAt;
    std::optional<std::string> endedAt;
    int basePrice{0};
};

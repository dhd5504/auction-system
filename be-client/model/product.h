#pragma once

#include <string>
#include <optional>

struct Product {
    int id{0};
    std::string name;
    std::optional<std::string> description;
    int startPrice{0};
    std::string status{"available"};
    int ownerUserId{0};
    std::optional<std::string> createdAt;
    std::optional<std::string> updatedAt;
    std::optional<std::string> imageUrl;
    std::optional<std::string> category;
};

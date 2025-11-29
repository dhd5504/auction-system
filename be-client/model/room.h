#pragma once

#include <string>
#include <vector>

struct Room {
    std::string id;
    std::string name;
    std::string startTime;
    std::vector<std::string> productIds;
};

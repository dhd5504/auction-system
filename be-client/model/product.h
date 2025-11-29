#pragma once

#include <string>

struct Product {
    std::string id;
    std::string name;
    double startPrice{0.0};
    double buyPrice{0.0};
    double step{0.0};
    std::string description;
};

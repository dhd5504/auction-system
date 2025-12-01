#pragma once

#include <string>

class JwtService {
public:
    JwtService(std::string secret, int expirySeconds = 3600);

    std::string issueToken(const std::string& userId, const std::string& role) const;
    bool verifyToken(const std::string& token, std::string& userId, std::string& role) const;

private:
    std::string secret;
    int expirySeconds;
};

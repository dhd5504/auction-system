#pragma once

#include <vector>

#include "Protocol.h"
#include "database.h"
#include "session_manager.h"

class PacketRouter {
public:
    PacketRouter(Database& db, SessionManager& sessions);

    bool route(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload);

private:
    bool handleLogin(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload);
    bool handleJoinRoom(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload);
    bool handleBid(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload);
    bool handleBuyNow(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload);
    bool handleNotifyMessage(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload);

    Database& db;
    SessionManager& sessions;
};

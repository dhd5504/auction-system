#pragma once

#include <atomic>
#include <thread>
#include <vector>

#include "packet_router.h"

class TcpServer {
public:
    TcpServer(uint16_t port, Database& db, SessionManager& sessions);
    ~TcpServer();

    bool start();
    void stop();

private:
    void handleClient(int clientFd);

    int serverFd{-1};
    uint16_t port;
    std::atomic<bool> running{false};
    PacketRouter router;
    std::vector<std::thread> workers;
};

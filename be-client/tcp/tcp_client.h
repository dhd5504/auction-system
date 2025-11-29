#pragma once

#include <mutex>
#include <string>

class TcpClient {
public:
    TcpClient(const std::string& host, int port);
    ~TcpClient();

    bool sendCommand(const std::string& command);

private:
    bool connectIfNeeded();
    void closeSocket();

    std::string host;
    int port;
    int sock{-1};
    std::mutex mutex;
};

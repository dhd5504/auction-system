#include "tcp_client.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <cstring>
#include <iostream>

TcpClient::TcpClient(const std::string& host_, int port_)
    : host(host_), port(port_) {}

TcpClient::~TcpClient() {
    closeSocket();
}

bool TcpClient::connectIfNeeded() {
    if (sock != -1) {
        return true;
    }

    sock = ::socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << std::endl;
        closeSocket();
        return false;
    }

    if (::connect(sock, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) < 0) {
        std::cerr << "Failed to connect to TCP server at " << host << ":" << port << std::endl;
        closeSocket();
        return false;
    }
    return true;
}

void TcpClient::closeSocket() {
    if (sock != -1) {
        ::close(sock);
        sock = -1;
    }
}

bool TcpClient::sendCommand(const std::string& command) {
    std::lock_guard<std::mutex> lock(mutex);
    if (!connectIfNeeded()) {
        return false;
    }

    std::string payload = command + "\n";
    ssize_t sent = ::send(sock, payload.c_str(), payload.size(), 0);
    if (sent < 0) {
        std::cerr << "Failed to send command, reconnecting..." << std::endl;
        closeSocket();
        return false;
    }
    return true;
}

#include "tcp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <iostream>
#include <sstream>
#include <iomanip>

#include "packet_io.h"

TcpServer::TcpServer(uint16_t port_, Database& db, SessionManager& sessions)
    : port(port_), router(db, sessions) {}

TcpServer::~TcpServer() {
    stop();
}

bool TcpServer::start() {
    if (running.exchange(true)) {
        return false;
    }

    serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        running = false;
        return false;
    }

    int opt = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (::bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "Bind failed" << std::endl;
        stop();
        return false;
    }
    if (::listen(serverFd, 16) < 0) {
        std::cerr << "Listen failed" << std::endl;
        stop();
        return false;
    }

    std::cout << "TCP server listening on port " << port << std::endl;

    std::thread acceptThread([this]() {
        while (running.load()) {
            int clientFd = ::accept(serverFd, nullptr, nullptr);
            if (clientFd < 0) {
                if (running.load()) {
                    std::cerr << "Accept failed" << std::endl;
                }
                continue;
            }
            workers.emplace_back(&TcpServer::handleClient, this, clientFd);
        }
    });
    workers.emplace_back(std::move(acceptThread));
    return true;
}

void TcpServer::stop() {
    if (!running.exchange(false)) {
        return;
    }
    if (serverFd >= 0) {
        ::shutdown(serverFd, SHUT_RDWR);
        ::close(serverFd);
        serverFd = -1;
    }
    for (auto& t : workers) {
        if (t.joinable()) t.join();
    }
    workers.clear();
}

void TcpServer::handleClient(int clientFd) {
    while (running.load()) {
        PacketHeader header{};
        if (!readHeader(clientFd, header)) {
            break;
        }
        std::cout << "[GW->TCP] recv opcode=0x" << std::hex << header.opcode << std::dec
                  << " len=" << header.length
                  << " session=" << header.sessionId
                  << std::endl;
        std::vector<uint8_t> payload;
        if (!readPayload(clientFd, payload, header.length)) {
            break;
        }
        if (!payload.empty()) {
            std::ostringstream out;
            size_t n = std::min(payload.size(), static_cast<size_t>(256));
            for (size_t i = 0; i < n; ++i) {
                out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(payload[i]);
                if (i + 1 < n) out << " ";
            }
            if (payload.size() > n) out << " ...";
            std::cout << "[GW->TCP] payload (" << payload.size() << " bytes) "
                      << out.str() << std::endl;
        }
        if (!router.route(clientFd, header, payload)) {
            break;
        }
    }
    ::close(clientFd);
}

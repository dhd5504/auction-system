#include "tcp_client.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <sstream>
#include <cstring>
#include <iostream>
#include <iomanip>

TcpClient::TcpClient(std::string host_, uint16_t port_)
    : host(std::move(host_)), port(port_) {}

namespace {

const char* opcodeName(uint16_t opcode) {
    switch (opcode) {
    case Opcode::LOGIN_REQ: return "LOGIN_REQ";
    case Opcode::LOGIN_RES: return "LOGIN_RES";
    case Opcode::JOIN_ROOM_REQ: return "JOIN_ROOM_REQ";
    case Opcode::JOIN_ROOM_RES: return "JOIN_ROOM_RES";
    case Opcode::BID_REQ: return "BID_REQ";
    case Opcode::BID_RES: return "BID_RES";
    case Opcode::NOTIFY_MESSAGE: return "NOTIFY_MESSAGE";
    case Opcode::TIMER_TICK: return "TIMER_TICK";
    case Opcode::ITEM_START: return "ITEM_START";
    case Opcode::ITEM_END: return "ITEM_END";
    case Opcode::BUY_NOW_REQ: return "BUY_NOW_REQ";
    case Opcode::BUY_NOW_RES: return "BUY_NOW_RES";
    default: return "UNKNOWN";
    }
}

std::string hexDump(const void* data, size_t len, size_t maxLen = 256) {
    const auto* bytes = static_cast<const uint8_t*>(data);
    size_t n = std::min(len, maxLen);
    std::ostringstream out;
    for (size_t i = 0; i < n; ++i) {
        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
        if (i + 1 < n) out << " ";
    }
    if (len > maxLen) out << " ...";
    return out.str();
}

} // namespace

bool TcpClient::connectSocket(int& fd) {
    fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        std::cerr << "TCPClient: failed to create socket\n";
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (::inet_pton(AF_INET, host.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "TCPClient: invalid address " << host << "\n";
        ::close(fd);
        return false;
    }
    if (::connect(fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "TCPClient: connect failed to " << host << ":" << port << "\n";
        ::close(fd);
        return false;
    }
    return true;
}

bool TcpClient::readExact(int fd, void* buffer, size_t length) {
    size_t total = 0;
    auto* ptr = static_cast<uint8_t*>(buffer);
    while (total < length) {
        ssize_t n = ::recv(fd, ptr + total, length - total, 0);
        if (n <= 0) return false;
        total += static_cast<size_t>(n);
    }
    return true;
}

bool TcpClient::writeExact(int fd, const void* buffer, size_t length) {
    size_t total = 0;
    auto* ptr = static_cast<const uint8_t*>(buffer);
    while (total < length) {
        ssize_t n = ::send(fd, ptr + total, length - total, 0);
        if (n <= 0) return false;
        total += static_cast<size_t>(n);
    }
    return true;
}

bool TcpClient::transact(uint16_t opcode,
                         uint32_t sessionId,
                         const void* payload,
                         size_t payloadSize,
                         PacketHeader& responseHeader,
                         std::vector<uint8_t>& responsePayload) {
    int fd = -1;
    if (!connectSocket(fd)) {
        return false;
    }

    PacketHeader header{};
    header.opcode = opcode;
    header.length = static_cast<uint32_t>(payloadSize);
    header.sessionId = sessionId;
    header.timestamp = static_cast<uint16_t>(std::chrono::system_clock::now().time_since_epoch().count() & 0xFFFF);
    header.reserved = 0;

    PacketHeader net = header;
    net.opcode = htons(header.opcode);
    net.length = htonl(header.length);
    net.sessionId = htonl(header.sessionId);
    net.timestamp = htons(header.timestamp);
    net.reserved = htons(header.reserved);

    std::cout << "[GW->TCP] opcode=" << opcodeName(header.opcode)
              << " (0x" << std::hex << header.opcode << std::dec << ")"
              << " len=" << header.length
              << " session=" << header.sessionId
              << std::endl;

    bool ok = writeExact(fd, &net, sizeof(net));
    if (ok && payloadSize > 0) {
        ok = writeExact(fd, payload, payloadSize);
        if (ok) {
            std::cout << "[GW->TCP] payload (" << payloadSize << " bytes) "
                      << hexDump(payload, payloadSize) << std::endl;
        }
    }
    if (!ok) {
        ::close(fd);
        return false;
    }

    PacketHeader respNet{};
    if (!readExact(fd, &respNet, sizeof(respNet))) {
        ::close(fd);
        return false;
    }
    responseHeader.opcode = ntohs(respNet.opcode);
    responseHeader.length = ntohl(respNet.length);
    responseHeader.sessionId = ntohl(respNet.sessionId);
    responseHeader.timestamp = ntohs(respNet.timestamp);
    responseHeader.reserved = ntohs(respNet.reserved);

    std::cout << "[TCP->GW] opcode=" << opcodeName(responseHeader.opcode)
              << " (0x" << std::hex << responseHeader.opcode << std::dec << ")"
              << " len=" << responseHeader.length
              << " session=" << responseHeader.sessionId
              << std::endl;

    responsePayload.assign(responseHeader.length, 0);
    if (responseHeader.length > 0) {
        if (!readExact(fd, responsePayload.data(), responsePayload.size())) {
            ::close(fd);
            return false;
        }
        std::cout << "[TCP->GW] payload (" << responsePayload.size() << " bytes) "
                  << hexDump(responsePayload.data(), responsePayload.size()) << std::endl;
    }

    ::close(fd);
    return true;
}

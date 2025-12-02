#include "packet_io.h"

#include <arpa/inet.h>
#include <unistd.h>

#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>

bool readExact(int fd, void* buffer, size_t length) {
    size_t total = 0;
    auto* ptr = static_cast<uint8_t*>(buffer);
    while (total < length) {
        ssize_t n = ::recv(fd, ptr + total, length - total, 0);
        if (n <= 0) {
            return false;
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

bool writeExact(int fd, const void* buffer, size_t length) {
    size_t total = 0;
    auto* ptr = static_cast<const uint8_t*>(buffer);
    while (total < length) {
        ssize_t n = ::send(fd, ptr + total, length - total, 0);
        if (n <= 0) {
            return false;
        }
        total += static_cast<size_t>(n);
    }
    return true;
}

bool readHeader(int fd, PacketHeader& header) {
    PacketHeader net{};
    if (!readExact(fd, &net, sizeof(net))) {
        return false;
    }
    header.opcode = ntohs(net.opcode);
    header.length = ntohl(net.length);
    header.sessionId = ntohl(net.sessionId);
    header.timestamp = ntohs(net.timestamp);
    header.reserved = ntohs(net.reserved);
    return true;
}

bool readPayload(int fd, std::vector<uint8_t>& payload, size_t length) {
    payload.assign(length, 0);
    return length == 0 || readExact(fd, payload.data(), length);
}

bool sendPacket(int fd, const PacketHeader& header, const std::vector<uint8_t>& payload) {
    PacketHeader net = header;
    net.opcode = htons(header.opcode);
    net.length = htonl(header.length);
    net.sessionId = htonl(header.sessionId);
    net.timestamp = htons(header.timestamp);
    net.reserved = htons(header.reserved);

    auto hexDump = [](const std::vector<uint8_t>& data, size_t maxLen = 256) {
        std::ostringstream out;
        size_t n = std::min(data.size(), maxLen);
        for (size_t i = 0; i < n; ++i) {
            out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(data[i]);
            if (i + 1 < n) out << " ";
        }
        if (data.size() > maxLen) out << " ...";
        return out.str();
    };

    std::cout << "[TCP->GW] send opcode=0x" << std::hex << header.opcode << std::dec
              << " len=" << payload.size()
              << " session=" << header.sessionId
              << std::endl;
    if (!payload.empty()) {
        std::cout << "[TCP->GW] payload (" << payload.size() << " bytes) "
                  << hexDump(payload) << std::endl;
    }

    if (!writeExact(fd, &net, sizeof(net))) {
        return false;
    }
    if (!payload.empty()) {
        if (!writeExact(fd, payload.data(), payload.size())) {
            return false;
        }
    }
    return true;
}

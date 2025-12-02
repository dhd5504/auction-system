#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "Protocol.h"

class TcpClient {
public:
    TcpClient(std::string host, uint16_t port);
    bool transact(uint16_t opcode,
                  uint32_t sessionId,
                  const void* payload,
                  size_t payloadSize,
                  PacketHeader& responseHeader,
                  std::vector<uint8_t>& responsePayload);

private:
    bool connectSocket(int& fd);
    bool readExact(int fd, void* buffer, size_t length);
    bool writeExact(int fd, const void* buffer, size_t length);

    std::string host;
    uint16_t port;
};

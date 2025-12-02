#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "Protocol.h"

bool readExact(int fd, void* buffer, size_t length);
bool writeExact(int fd, const void* buffer, size_t length);
bool readHeader(int fd, PacketHeader& header);
bool readPayload(int fd, std::vector<uint8_t>& payload, size_t length);
bool sendPacket(int fd, const PacketHeader& header, const std::vector<uint8_t>& payload);

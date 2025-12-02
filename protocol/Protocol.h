#pragma once

#include <cstdint>

namespace Opcode {
    constexpr uint16_t LOGIN_REQ = 0x01;
    constexpr uint16_t LOGIN_RES = 0x02;
    constexpr uint16_t JOIN_ROOM_REQ = 0x03;
    constexpr uint16_t JOIN_ROOM_RES = 0x04;
    constexpr uint16_t BID_REQ = 0x05;
    constexpr uint16_t BID_RES = 0x06;
    constexpr uint16_t NOTIFY_MESSAGE = 0x07;
    constexpr uint16_t TIMER_TICK = 0x08;
    constexpr uint16_t ITEM_START = 0x09;
    constexpr uint16_t ITEM_END = 0x0A;
    constexpr uint16_t BUY_NOW_REQ = 0x0B;
    constexpr uint16_t BUY_NOW_RES = 0x0C;
}

enum class NotifyCode : uint32_t {
    NONE = 0,
    PRODUCT_LIST = 1,
    PRODUCT_GET = 2,
    PRODUCT_CREATE = 3,
    PRODUCT_UPDATE = 4,
    PRODUCT_DELETE = 5,
    PRODUCT_LIST_OWN = 6,
    ROOM_LIST_PUBLIC = 20,
    ROOM_LIST_OWN = 21,
    ROOM_CREATE = 22,
    ROOM_DELETE = 23,
    ROOM_START = 24,
    ROOM_CANCEL = 25
};

#pragma pack(push, 1)

struct PacketHeader {
    uint16_t opcode;
    uint32_t length;
    uint32_t sessionId;
    uint16_t timestamp;
    uint16_t reserved;
};
static_assert(sizeof(PacketHeader) == 14, "PacketHeader must be 14 bytes");

struct LoginReq {
    char username[32];
    char password[32];
    uint8_t registerFlag;
    uint8_t reserved[3];
};

struct LoginRes {
    uint32_t userId;
    uint8_t success;
    char role[16];
    char message[64];
};

struct JoinRoomReq {
    uint32_t roomId;
    uint32_t sessionId;
    uint8_t action; // 0=list public, 1=get detail, 2=join, 3=list own
    uint8_t reserved[3];
};

struct RoomInfo {
    uint32_t roomId;
    uint32_t productId;
    uint32_t hostUserId;
    uint32_t currentPrice;
    uint32_t basePrice;
    uint32_t durationSeconds;
    char roomName[64];
    char status[16];
};

struct JoinRoomRes {
    uint8_t result;
    uint8_t roomCount;
    uint16_t reserved;
    RoomInfo rooms[16];
};

struct BidReq {
    uint32_t sessionId;
    uint32_t roomId;
    uint32_t amount;
};

struct BidRes {
    uint32_t roomId;
    uint32_t highestBid;
    uint32_t highestBidderId;
    uint8_t success;
    char message[64];
};

struct BuyNowReq {
    uint32_t sessionId;
    uint32_t roomId;
    uint32_t price;
};

struct BuyNowRes {
    uint32_t roomId;
    uint32_t buyerId;
    uint32_t finalPrice;
    uint8_t success;
    char message[64];
};

struct NotifyMessage {
    uint32_t roomId;
    uint32_t code;   // matches NotifyCode
    char message[16384];
};

struct TimerTick {
    uint32_t roomId;
    uint32_t secondsRemaining;
};

struct ItemStart {
    uint32_t roomId;
    uint32_t productId;
    uint32_t startPrice;
};

struct ItemEnd {
    uint32_t roomId;
    uint32_t productId;
    uint32_t winningPrice;
    uint32_t winnerUserId;
};

#pragma pack(pop)

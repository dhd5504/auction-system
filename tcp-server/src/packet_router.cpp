#include "packet_router.h"

#include <algorithm>
#include <chrono>
#include <ctime>
#include <cstring>
#include <regex>
#include <sstream>
#include <string>

#include "packet_io.h"

namespace {

std::string fixedToString(const char* data, size_t maxLen) {
    return std::string(data, strnlen(data, maxLen));
}

template <size_t N>
void copyToFixed(const std::string& input, char (&target)[N]) {
    std::memset(target, 0, N);
    std::strncpy(target, input.c_str(), N - 1);
}

std::string getStringField(const std::string& body, const std::string& key) {
    std::regex rgx("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(body, match, rgx)) {
        return match[1];
    }
    return {};
}

int getIntField(const std::string& body, const std::string& key) {
    std::regex rgx("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    std::smatch match;
    if (std::regex_search(body, match, rgx)) {
        return std::stoi(match[1]);
    }
    return 0;
}

std::string nowIso() {
    using namespace std::chrono;
    auto now = system_clock::now();
    auto t = system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    char buf[64];
    std::strftime(buf, sizeof(buf), "%FT%TZ", &tm);
    return buf;
}

template <typename T>
std::vector<uint8_t> toBuffer(const T& obj) {
    std::vector<uint8_t> out(sizeof(T));
    std::memcpy(out.data(), &obj, sizeof(T));
    return out;
}

std::string escapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
}

} // namespace

PacketRouter::PacketRouter(Database& db_, SessionManager& sessions_)
    : db(db_), sessions(sessions_) {}

bool PacketRouter::route(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload) {
    switch (header.opcode) {
    case Opcode::LOGIN_REQ:
        return handleLogin(clientFd, header, payload);
    case Opcode::JOIN_ROOM_REQ:
        return handleJoinRoom(clientFd, header, payload);
    case Opcode::BID_REQ:
        return handleBid(clientFd, header, payload);
    case Opcode::BUY_NOW_REQ:
        return handleBuyNow(clientFd, header, payload);
    case Opcode::NOTIFY_MESSAGE:
        return handleNotifyMessage(clientFd, header, payload);
    default:
        return false;
    }
}

bool PacketRouter::handleLogin(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(LoginReq)) return false;
    LoginReq req{};
    std::memcpy(&req, payload.data(), sizeof(LoginReq));
    std::string username = fixedToString(req.username, sizeof(req.username));
    std::string password = fixedToString(req.password, sizeof(req.password));
    bool doRegister = req.registerFlag != 0;

    LoginRes res{};
    res.success = 0;
    copyToFixed(std::string("invalid credentials"), res.message);

    auto existing = db.getUserByUsername(username);
    if (doRegister) {
        if (existing) {
            copyToFixed(std::string("user exists"), res.message);
        } else {
            User user;
            user.username = username;
            user.password = password;
            int newId = 0;
            if (db.addUser(user, newId)) {
                res.userId = static_cast<uint32_t>(newId);
                res.success = 1;
                copyToFixed(std::string("registered"), res.message);
                copyToFixed(std::string("user"), res.role);
            } else {
                copyToFixed(std::string("registration failed"), res.message);
            }
        }
    } else {
        if (existing && existing->password == password) {
            res.userId = static_cast<uint32_t>(existing->id);
            res.success = 1;
            copyToFixed(existing->role, res.role);
            copyToFixed(std::string("ok"), res.message);
            db.updateLastLogin(existing->id);
        } else {
            copyToFixed(std::string("unauthorized"), res.message);
        }
    }

    PacketHeader outHeader = header;
    outHeader.opcode = Opcode::LOGIN_RES;
    outHeader.length = sizeof(LoginRes);
    outHeader.timestamp = static_cast<uint16_t>(std::time(nullptr));
    if (res.success) {
        outHeader.sessionId = sessions.createSession(static_cast<int>(res.userId));
    } else {
        outHeader.sessionId = 0;
    }
    auto buffer = toBuffer(res);
    return sendPacket(clientFd, outHeader, buffer);
}

bool PacketRouter::handleJoinRoom(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(JoinRoomReq)) return false;
    JoinRoomReq req{};
    std::memcpy(&req, payload.data(), sizeof(JoinRoomReq));
    uint32_t sessionId = header.sessionId ? header.sessionId : req.sessionId;
    auto userOpt = sessions.userForSession(sessionId);

    std::vector<Room> rooms;
    uint8_t result = 0;
    switch (req.action) {
    case 0: // public list
        rooms = db.getRoomsPublic();
        break;
    case 3: // own list
        if (!userOpt) {
            result = 1;
        } else {
            rooms = db.getRoomsByOwner(*userOpt);
        }
        break;
    case 1: // get detail
    case 2: { // join
        auto room = db.getRoomById(static_cast<int>(req.roomId));
        if (room) {
            rooms.push_back(*room);
        } else {
            result = 1;
        }
        break;
    }
    default:
        result = 1;
        break;
    }

    JoinRoomRes res{};
    res.result = result;
    constexpr size_t MAX_ROOMS = sizeof(res.rooms) / sizeof(res.rooms[0]);
    res.roomCount = static_cast<uint8_t>(std::min<size_t>(rooms.size(), MAX_ROOMS));
    for (size_t i = 0; i < res.roomCount; ++i) {
        const auto& r = rooms[i];
        auto& info = res.rooms[i];
        info.roomId = static_cast<uint32_t>(r.id);
        info.productId = static_cast<uint32_t>(r.productId);
        info.hostUserId = static_cast<uint32_t>(r.hostUserId);
        info.basePrice = static_cast<uint32_t>(r.basePrice);
        info.durationSeconds = static_cast<uint32_t>(r.duration);
        info.currentPrice = static_cast<uint32_t>(std::max(r.basePrice, db.getHighestBid(r.id)));
        copyToFixed(r.roomName, info.roomName);
        copyToFixed(r.status, info.status);
    }

    PacketHeader outHeader = header;
    outHeader.opcode = Opcode::JOIN_ROOM_RES;
    outHeader.length = sizeof(JoinRoomRes);
    outHeader.timestamp = static_cast<uint16_t>(std::time(nullptr));
    auto buffer = toBuffer(res);
    return sendPacket(clientFd, outHeader, buffer);
}

bool PacketRouter::handleBid(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(BidReq)) return false;
    BidReq req{};
    std::memcpy(&req, payload.data(), sizeof(BidReq));
    uint32_t sessionId = header.sessionId ? header.sessionId : req.sessionId;
    auto userOpt = sessions.userForSession(sessionId);

    BidRes res{};
    res.roomId = req.roomId;
    res.success = 0;
    copyToFixed(std::string("unauthorized"), res.message);
    if (!userOpt) {
        PacketHeader outHeader = header;
        outHeader.opcode = Opcode::BID_RES;
        outHeader.length = sizeof(BidRes);
        outHeader.timestamp = static_cast<uint16_t>(std::time(nullptr));
        auto buf = toBuffer(res);
        return sendPacket(clientFd, outHeader, buf);
    }

    auto room = db.getRoomById(static_cast<int>(req.roomId));
    if (!room) {
        copyToFixed(std::string("room not found"), res.message);
    } else {
        int current = std::max(room->basePrice, db.getHighestBid(req.roomId));
        if (static_cast<int>(req.amount) <= current) {
            copyToFixed(std::string("bid too low"), res.message);
        } else {
            if (db.recordBid(req.roomId, *userOpt, static_cast<int>(req.amount))) {
                res.success = 1;
                res.highestBid = req.amount;
                res.highestBidderId = static_cast<uint32_t>(*userOpt);
                copyToFixed(std::string("accepted"), res.message);
            } else {
                copyToFixed(std::string("db error"), res.message);
            }
        }
    }

    PacketHeader outHeader = header;
    outHeader.opcode = Opcode::BID_RES;
    outHeader.length = sizeof(BidRes);
    outHeader.timestamp = static_cast<uint16_t>(std::time(nullptr));
    auto buf = toBuffer(res);
    return sendPacket(clientFd, outHeader, buf);
}

bool PacketRouter::handleBuyNow(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(BuyNowReq)) return false;
    BuyNowReq req{};
    std::memcpy(&req, payload.data(), sizeof(BuyNowReq));
    uint32_t sessionId = header.sessionId ? header.sessionId : req.sessionId;
    auto userOpt = sessions.userForSession(sessionId);

    BuyNowRes res{};
    res.roomId = req.roomId;
    res.success = 0;
    copyToFixed(std::string("unauthorized"), res.message);

    if (userOpt) {
        auto room = db.getRoomById(static_cast<int>(req.roomId));
        if (!room) {
            copyToFixed(std::string("room not found"), res.message);
        } else if (room->status == "sold") {
            copyToFixed(std::string("already sold"), res.message);
        } else {
            res.success = 1;
            res.buyerId = static_cast<uint32_t>(*userOpt);
            res.finalPrice = req.price;
            copyToFixed(std::string("sold"), res.message);
            db.updateRoomStatus(room->id, room->hostUserId, "sold", room->startedAt, nowIso());
            db.updateProductStatus(room->productId, "sold");
        }
    }

    PacketHeader outHeader = header;
    outHeader.opcode = Opcode::BUY_NOW_RES;
    outHeader.length = sizeof(BuyNowRes);
    outHeader.timestamp = static_cast<uint16_t>(std::time(nullptr));
    auto buf = toBuffer(res);
    return sendPacket(clientFd, outHeader, buf);
}

bool PacketRouter::handleNotifyMessage(int clientFd, const PacketHeader& header, const std::vector<uint8_t>& payload) {
    if (payload.size() < sizeof(NotifyMessage)) return false;
    NotifyMessage req{};
    std::memcpy(&req, payload.data(), sizeof(NotifyMessage));
    uint32_t sessionId = header.sessionId;
    auto userOpt = sessions.userForSession(sessionId);
    std::string body = fixedToString(req.message, sizeof(req.message));
    NotifyCode code = static_cast<NotifyCode>(req.code);

    auto sendJson = [&](const std::string& jsonText, NotifyCode responseCode) {
        NotifyMessage res{};
        res.roomId = req.roomId;
        res.code = static_cast<uint32_t>(responseCode);
        if (jsonText.size() >= sizeof(res.message)) {
            copyToFixed(std::string("{\"error\":\"response too large\"}"), res.message);
        } else {
            copyToFixed(jsonText, res.message);
        }
        PacketHeader outHeader = header;
        outHeader.opcode = Opcode::NOTIFY_MESSAGE;
        outHeader.length = sizeof(NotifyMessage);
        outHeader.timestamp = static_cast<uint16_t>(std::time(nullptr));
        auto buf = toBuffer(res);
        return sendPacket(clientFd, outHeader, buf);
    };

    switch (code) {
    case NotifyCode::PRODUCT_LIST: {
        auto products = db.getProducts(std::nullopt);
        std::ostringstream out;
        out << "[";
        for (size_t i = 0; i < products.size(); ++i) {
            const auto& p = products[i];
            out << "{"
                << "\"id\":" << p.id << ","
                << "\"name\":\"" << escapeJson(p.name) << "\","
                << "\"description\":\"" << escapeJson(p.description.value_or("")) << "\","
                << "\"startPrice\":" << p.startPrice << ","
                << "\"status\":\"" << escapeJson(p.status) << "\","
                << "\"ownerUserId\":" << p.ownerUserId << ","
                << "\"imageUrl\":\"" << escapeJson(p.imageUrl.value_or("")) << "\","
                << "\"category\":\"" << escapeJson(p.category.value_or("")) << "\","
                << "\"createdAt\":\"" << escapeJson(p.createdAt.value_or("")) << "\""
                << "}";
            if (i + 1 < products.size()) out << ",";
        }
        out << "]";
        return sendJson(out.str(), code);
    }
    case NotifyCode::PRODUCT_LIST_OWN: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        auto products = db.getProducts(*userOpt);
        std::ostringstream out;
        out << "[";
        for (size_t i = 0; i < products.size(); ++i) {
            const auto& p = products[i];
            out << "{"
                << "\"id\":" << p.id << ","
                << "\"name\":\"" << escapeJson(p.name) << "\","
                << "\"description\":\"" << escapeJson(p.description.value_or("")) << "\","
                << "\"startPrice\":" << p.startPrice << ","
                << "\"status\":\"" << escapeJson(p.status) << "\","
                << "\"ownerUserId\":" << p.ownerUserId << ","
                << "\"imageUrl\":\"" << escapeJson(p.imageUrl.value_or("")) << "\","
                << "\"category\":\"" << escapeJson(p.category.value_or("")) << "\","
                << "\"createdAt\":\"" << escapeJson(p.createdAt.value_or("")) << "\""
                << "}";
            if (i + 1 < products.size()) out << ",";
        }
        out << "]";
        return sendJson(out.str(), code);
    }
    case NotifyCode::PRODUCT_GET: {
        int id = getIntField(body, "id");
        auto product = db.getProductById(id);
        if (!product) return sendJson("{\"error\":\"not found\"}", code);
        std::ostringstream out;
        out << "{"
            << "\"id\":" << product->id << ","
            << "\"name\":\"" << escapeJson(product->name) << "\","
            << "\"description\":\"" << escapeJson(product->description.value_or("")) << "\","
            << "\"startPrice\":" << product->startPrice << ","
            << "\"status\":\"" << escapeJson(product->status) << "\","
            << "\"ownerUserId\":" << product->ownerUserId << ","
            << "\"imageUrl\":\"" << escapeJson(product->imageUrl.value_or("")) << "\","
            << "\"category\":\"" << escapeJson(product->category.value_or("")) << "\","
            << "\"createdAt\":\"" << escapeJson(product->createdAt.value_or("")) << "\""
            << "}";
        return sendJson(out.str(), code);
    }
    case NotifyCode::PRODUCT_CREATE: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        Product product;
        product.name = getStringField(body, "name");
        product.description = getStringField(body, "description");
        product.startPrice = getIntField(body, "startPrice");
        product.status = "available";
        product.ownerUserId = *userOpt;
        int newId = 0;
        if (product.name.empty() || product.startPrice <= 0 || !db.addProduct(product, newId)) {
            return sendJson("{\"error\":\"invalid product\"}", code);
        }
        std::ostringstream out;
        out << "{\"id\":" << newId << "}";
        return sendJson(out.str(), code);
    }
    case NotifyCode::PRODUCT_UPDATE: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        int id = getIntField(body, "id");
        auto product = db.getProductById(id);
        if (!product || product->ownerUserId != *userOpt) return sendJson("{\"error\":\"not found\"}", code);
        product->name = getStringField(body, "name").empty() ? product->name : getStringField(body, "name");
        product->description = getStringField(body, "description").empty() ? product->description : std::optional<std::string>(getStringField(body, "description"));
        int price = getIntField(body, "startPrice");
        if (price > 0) product->startPrice = price;
        std::string status = getStringField(body, "status");
        if (!status.empty()) product->status = status;
        bool ok = db.updateProduct(*product);
        return sendJson(ok ? "{\"updated\":true}" : "{\"error\":\"update failed\"}", code);
    }
    case NotifyCode::PRODUCT_DELETE: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        int id = getIntField(body, "id");
        bool ok = db.deleteProduct(id, *userOpt);
        return sendJson(ok ? "{\"deleted\":true}" : "{\"error\":\"delete failed\"}", code);
    }
    case NotifyCode::ROOM_LIST_PUBLIC: {
        auto rooms = db.getRoomsPublic();
        std::ostringstream out;
        out << "[";
        for (size_t i = 0; i < rooms.size(); ++i) {
            const auto& r = rooms[i];
            out << "{"
                << "\"id\":" << r.id << ","
                << "\"roomName\":\"" << escapeJson(r.roomName) << "\","
                << "\"productId\":" << r.productId << ","
                << "\"status\":\"" << escapeJson(r.status) << "\","
                << "\"hostUserId\":" << r.hostUserId << ","
                << "\"basePrice\":" << r.basePrice << ","
                << "\"currentPrice\":" << std::max(r.basePrice, db.getHighestBid(r.id))
                << "}";
            if (i + 1 < rooms.size()) out << ",";
        }
        out << "]";
        return sendJson(out.str(), code);
    }
    case NotifyCode::ROOM_LIST_OWN: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        auto rooms = db.getRoomsByOwner(*userOpt);
        std::ostringstream out;
        out << "[";
        for (size_t i = 0; i < rooms.size(); ++i) {
            const auto& r = rooms[i];
            out << "{"
                << "\"id\":" << r.id << ","
                << "\"roomName\":\"" << escapeJson(r.roomName) << "\","
                << "\"productId\":" << r.productId << ","
                << "\"status\":\"" << escapeJson(r.status) << "\","
                << "\"hostUserId\":" << r.hostUserId << ","
                << "\"basePrice\":" << r.basePrice << ","
                << "\"currentPrice\":" << std::max(r.basePrice, db.getHighestBid(r.id))
                << "}";
            if (i + 1 < rooms.size()) out << ",";
        }
        out << "]";
        return sendJson(out.str(), code);
    }
    case NotifyCode::ROOM_CREATE: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        Room room;
        room.roomName = getStringField(body, "roomName");
        room.productId = getIntField(body, "productId");
        room.duration = std::max(1, getIntField(body, "duration"));
        room.status = "waiting";
        room.hostUserId = *userOpt;
        room.basePrice = std::max(0, getIntField(body, "basePrice"));
        auto product = db.getProductById(room.productId);
        if (!product || product->ownerUserId != *userOpt || product->status != "available") {
            return sendJson("{\"error\":\"product not available\"}", code);
        }
        int newId = 0;
        if (room.roomName.empty() || room.productId <= 0 || !db.addRoom(room, newId)) {
            return sendJson("{\"error\":\"create failed\"}", code);
        }
        db.updateProductStatus(room.productId, "pending", room.hostUserId);
        std::ostringstream out;
        out << "{\"id\":" << newId << "}";
        return sendJson(out.str(), code);
    }
    case NotifyCode::ROOM_DELETE: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        int roomId = getIntField(body, "roomId");
        bool ok = db.deleteRoom(roomId, *userOpt);
        return sendJson(ok ? "{\"deleted\":true}" : "{\"error\":\"delete failed\"}", code);
    }
    case NotifyCode::ROOM_START: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        int roomId = getIntField(body, "roomId");
        bool ok = db.updateRoomStatus(roomId, *userOpt, "running", nowIso());
        return sendJson(ok ? "{\"started\":true}" : "{\"error\":\"start failed\"}", code);
    }
    case NotifyCode::ROOM_CANCEL: {
        if (!userOpt) return sendJson("{\"error\":\"unauthorized\"}", code);
        int roomId = getIntField(body, "roomId");
        bool ok = db.updateRoomStatus(roomId, *userOpt, "cancelled", std::nullopt, nowIso());
        return sendJson(ok ? "{\"cancelled\":true}" : "{\"error\":\"cancel failed\"}", code);
    }
    default:
        return sendJson("{\"error\":\"unsupported\"}", code);
    }
}

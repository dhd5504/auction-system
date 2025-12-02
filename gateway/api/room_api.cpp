#include "room_api.h"

#include <sstream>
#include <cstring>
#include <cctype>

using namespace Pistache;

namespace {

template <size_t N>
void copyToFixed(const std::string& input, char (&target)[N]) {
    std::memset(target, 0, N);
    std::strncpy(target, input.c_str(), N - 1);
}

int getIntField(const std::string& body, const std::string& key) {
    auto pos = body.find(key);
    if (pos == std::string::npos) return 0;
    pos = body.find(':', pos);
    if (pos == std::string::npos) return 0;
    ++pos;
    while (pos < body.size() && (body[pos] == ' ' || body[pos] == '\"')) ++pos;
    std::string number;
    while (pos < body.size() && (isdigit(body[pos]) || body[pos] == '-')) {
        number.push_back(body[pos]);
        ++pos;
    }
    if (number.empty()) return 0;
    try {
        return std::stoi(number);
    } catch (...) {
        return 0;
    }
}

std::string roomToJson(const RoomInfo& info) {
    std::ostringstream out;
    out << "{"
        << "\"id\":" << info.roomId << ","
        << "\"roomName\":\"" << std::string(info.roomName, strnlen(info.roomName, sizeof(info.roomName))) << "\","
        << "\"productId\":" << info.productId << ","
        << "\"hostUserId\":" << info.hostUserId << ","
        << "\"basePrice\":" << info.basePrice << ","
        << "\"currentPrice\":" << info.currentPrice << ","
        << "\"duration\":" << info.durationSeconds << ","
        << "\"status\":\"" << std::string(info.status, strnlen(info.status, sizeof(info.status))) << "\""
        << "}";
    return out.str();
}

std::string joinResToJson(const JoinRoomRes& res) {
    std::ostringstream out;
    out << "[";
    for (size_t i = 0; i < res.roomCount; ++i) {
        out << roomToJson(res.rooms[i]);
        if (i + 1 < res.roomCount) out << ",";
    }
    out << "]";
    return out.str();
}

std::string wrapRoomId(int roomId) {
    std::ostringstream out;
    out << "{\"roomId\":" << roomId << "}";
    return out.str();
}

} // namespace

RoomApi::RoomApi(Rest::Router& router, TcpClient& tcp, WsServer& ws)
    : tcpClient(tcp), wsServer(ws) {
    Rest::Routes::Get(router, "/api/rooms", Rest::Routes::bind(&RoomApi::handleListPublic, this));
    Rest::Routes::Get(router, "/api/rooms/:id", Rest::Routes::bind(&RoomApi::handleGetPublic, this));
    Rest::Routes::Get(router, "/api/me/rooms", Rest::Routes::bind(&RoomApi::handleListOwn, this));
    Rest::Routes::Get(router, "/api/me/rooms/:id", Rest::Routes::bind(&RoomApi::handleGetOwn, this));
    Rest::Routes::Post(router, "/api/me/rooms", Rest::Routes::bind(&RoomApi::handleCreateOwn, this));
    Rest::Routes::Delete(router, "/api/me/rooms/:id", Rest::Routes::bind(&RoomApi::handleDeleteOwn, this));
    Rest::Routes::Post(router, "/api/me/rooms/:id/start", Rest::Routes::bind(&RoomApi::handleStartOwn, this));
    Rest::Routes::Post(router, "/api/me/rooms/:id/cancel", Rest::Routes::bind(&RoomApi::handleCancelOwn, this));
    Rest::Routes::Post(router, "/api/rooms/:id/bid", Rest::Routes::bind(&RoomApi::handleBid, this));
    Rest::Routes::Post(router, "/api/rooms/:id/buy", Rest::Routes::bind(&RoomApi::handleBuyNow, this));
    Rest::Routes::Options(router, "/api/rooms", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/rooms/:id", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/rooms", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/rooms/:id", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/rooms/:id/start", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/rooms/:id/cancel", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/rooms/:id/bid", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/rooms/:id/buy", Rest::Routes::bind(&RoomApi::handleOptions, this));
}

bool RoomApi::authorize(const Rest::Request& request, Http::ResponseWriter& response, uint32_t& sessionId) {
    auto authHeader = request.headers().tryGetRaw("Authorization");
    if (!authHeader.has_value()) {
        addCors(response);
        response.send(Http::Code::Unauthorized, "Missing token");
        return false;
    }
    std::string value = authHeader->value();
    const std::string prefix = "Bearer ";
    if (value.rfind(prefix, 0) != 0 || value.size() <= prefix.size()) {
        addCors(response);
        response.send(Http::Code::Unauthorized, "Missing token");
        return false;
    }
    try {
        sessionId = static_cast<uint32_t>(std::stoul(value.substr(prefix.size())));
    } catch (...) {
        addCors(response);
        response.send(Http::Code::Unauthorized, "Invalid token");
        return false;
    }
    return true;
}

void RoomApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, DELETE, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
}

void RoomApi::handleListPublic(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    JoinRoomReq req{};
    req.action = 0;

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::JOIN_ROOM_REQ, 0, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(JoinRoomRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to fetch rooms");
        return;
    }
    JoinRoomRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(JoinRoomRes));
    std::string json = joinResToJson(res);
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, json);
}

void RoomApi::handleGetPublic(const Rest::Request& request, Http::ResponseWriter response) {
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }
    JoinRoomReq req{};
    req.action = 1;
    req.roomId = static_cast<uint32_t>(id);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::JOIN_ROOM_REQ, 0, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(JoinRoomRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to fetch room");
        return;
    }
    JoinRoomRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(JoinRoomRes));
    if (res.roomCount == 0) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Room not found");
        return;
    }
    std::string json = roomToJson(res.rooms[0]);
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, json);
}

void RoomApi::handleListOwn(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    JoinRoomReq req{};
    req.action = 3;
    req.sessionId = sessionId;

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::JOIN_ROOM_REQ, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(JoinRoomRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to fetch rooms");
        return;
    }
    JoinRoomRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(JoinRoomRes));
    std::string json = joinResToJson(res);
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, json);
}

void RoomApi::handleGetOwn(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }
    JoinRoomReq req{};
    req.action = 1;
    req.roomId = static_cast<uint32_t>(id);
    req.sessionId = sessionId;

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::JOIN_ROOM_REQ, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(JoinRoomRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to fetch room");
        return;
    }
    JoinRoomRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(JoinRoomRes));
    if (res.roomCount == 0) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Room not found");
        return;
    }
    std::string json = roomToJson(res.rooms[0]);
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, json);
}

void RoomApi::handleCreateOwn(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::ROOM_CREATE);
    copyToFixed(request.body(), req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to create room");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = json.find("error") != std::string::npos ? Http::Code::Bad_Request : Http::Code::Created;
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, json);
}

void RoomApi::handleDeleteOwn(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }
    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::ROOM_DELETE);
    copyToFixed(wrapRoomId(id), req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to delete room");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = json.find("error") != std::string::npos ? Http::Code::Bad_Request : Http::Code::Ok;
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, json);
}

void RoomApi::handleStartOwn(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }
    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::ROOM_START);
    copyToFixed(wrapRoomId(id), req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to start room");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = json.find("error") != std::string::npos ? Http::Code::Bad_Request : Http::Code::Ok;
    if (code == Http::Code::Ok) {
        wsServer.broadcast("{\"type\":\"room_started\",\"roomId\":" + std::to_string(id) + "}");
    }
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, json);
}

void RoomApi::handleCancelOwn(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }
    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::ROOM_CANCEL);
    copyToFixed(wrapRoomId(id), req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to cancel room");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = json.find("error") != std::string::npos ? Http::Code::Bad_Request : Http::Code::Ok;
    if (code == Http::Code::Ok) {
        wsServer.broadcast("{\"type\":\"room_cancelled\",\"roomId\":" + std::to_string(id) + "}");
    }
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, json);
}

void RoomApi::handleBid(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int roomId = 0;
    try { roomId = request.param(":id").as<int>(); } catch (...) { roomId = 0; }
    int amount = getIntField(request.body(), "amount");
    if (amount <= 0) amount = getIntField(request.body(), "bid");

    BidReq req{};
    req.sessionId = sessionId;
    req.roomId = static_cast<uint32_t>(roomId);
    req.amount = static_cast<uint32_t>(amount);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::BID_REQ, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(BidRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to place bid");
        return;
    }
    BidRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(BidRes));
    std::string message = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = res.success ? Http::Code::Ok : Http::Code::Bad_Request;

    std::ostringstream out;
    out << "{"
        << "\"roomId\":" << res.roomId << ","
        << "\"highestBid\":" << res.highestBid << ","
        << "\"highestBidderId\":" << res.highestBidderId << ","
        << "\"success\":" << (res.success ? "true" : "false") << ","
        << "\"message\":\"" << message << "\""
        << "}";

    if (res.success) {
        wsServer.broadcast("{\"type\":\"bid\",\"roomId\":" + std::to_string(res.roomId) +
                           ",\"amount\":" + std::to_string(res.highestBid) + "}");
    }

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, out.str());
}

void RoomApi::handleBuyNow(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int roomId = 0;
    try { roomId = request.param(":id").as<int>(); } catch (...) { roomId = 0; }
    int price = getIntField(request.body(), "price");
    if (price <= 0) price = getIntField(request.body(), "amount");

    BuyNowReq req{};
    req.sessionId = sessionId;
    req.roomId = static_cast<uint32_t>(roomId);
    req.price = static_cast<uint32_t>(price);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::BUY_NOW_REQ, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(BuyNowRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to buy now");
        return;
    }
    BuyNowRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(BuyNowRes));
    std::string message = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = res.success ? Http::Code::Ok : Http::Code::Bad_Request;

    std::ostringstream out;
    out << "{"
        << "\"roomId\":" << res.roomId << ","
        << "\"buyerId\":" << res.buyerId << ","
        << "\"finalPrice\":" << res.finalPrice << ","
        << "\"success\":" << (res.success ? "true" : "false") << ","
        << "\"message\":\"" << message << "\""
        << "}";

    if (res.success) {
        wsServer.broadcast("{\"type\":\"buy_now\",\"roomId\":" + std::to_string(res.roomId) +
                           ",\"finalPrice\":" + std::to_string(res.finalPrice) + "}");
    }

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, out.str());
}

void RoomApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

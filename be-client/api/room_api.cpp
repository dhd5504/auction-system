#include "room_api.h"

#include <regex>
#include <sstream>
#include <optional>

using namespace Pistache;

namespace {

std::string escapeJson(const std::string& input) {
    std::string out;
    out.reserve(input.size());
    for (char c : input) {
        switch (c) {
        case '\\': out += "\\\\"; break;
        case '\"': out += "\\\""; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: out += c; break;
        }
    }
    return out;
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

} // namespace

RoomApi::RoomApi(Rest::Router& router, SQLiteDb& db_, JwtService& jwt_)
    : db(db_), jwt(jwt_) {
    // Owner routes
    Rest::Routes::Get(router, "/api/me/rooms", Rest::Routes::bind(&RoomApi::handleListOwn, this));
    Rest::Routes::Get(router, "/api/me/rooms/:id", Rest::Routes::bind(&RoomApi::handleGetOwn, this));
    Rest::Routes::Post(router, "/api/me/rooms", Rest::Routes::bind(&RoomApi::handleCreateOwn, this));
    Rest::Routes::Delete(router, "/api/me/rooms/:id", Rest::Routes::bind(&RoomApi::handleDeleteOwn, this));
    Rest::Routes::Options(router, "/api/me/rooms", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/rooms/:id", Rest::Routes::bind(&RoomApi::handleOptions, this));

    // Public routes
    Rest::Routes::Get(router, "/api/rooms", Rest::Routes::bind(&RoomApi::handleListPublic, this));
    Rest::Routes::Get(router, "/api/rooms/:id", Rest::Routes::bind(&RoomApi::handleGetPublic, this));
    Rest::Routes::Options(router, "/api/rooms", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/rooms/:id", Rest::Routes::bind(&RoomApi::handleOptions, this));
}

bool RoomApi::authorize(const Rest::Request& request, Http::ResponseWriter& response, int& userId, std::string& role) {
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
    std::string token = value.substr(prefix.size());

    std::string userIdStr;
    if (!jwt.verifyToken(token, userIdStr, role)) {
        addCors(response);
        response.send(Http::Code::Unauthorized, "Invalid token");
        return false;
    }
    try {
        userId = std::stoi(userIdStr);
    } catch (...) {
        addCors(response);
        response.send(Http::Code::Unauthorized, "Invalid user id");
        return false;
    }
    return true;
}

void RoomApi::handleCreateOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    auto body = request.body();
    Room room;
    room.roomName = getStringField(body, "roomName");
    if (room.roomName.empty()) room.roomName = getStringField(body, "name");
    room.productId = getIntField(body, "productId");
    room.duration = getIntField(body, "duration");
    room.status = "pending"; // product was available, set pending
    room.hostUserId = userId;
    room.basePrice = getIntField(body, "basePrice");
    std::string startedAt = getStringField(body, "startedAt");
    if (!startedAt.empty()) room.startedAt = startedAt;
    std::string endedAt = getStringField(body, "endedAt");
    if (!endedAt.empty()) room.endedAt = endedAt;

    if (room.roomName.empty() || room.productId <= 0 || room.duration <= 0) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Invalid room payload");
        return;
    }

    auto product = db.getProductByIdForOwner(room.productId, userId);
    if (!product.has_value()) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Product not owned by user");
        return;
    }
    if (product->status != "available") {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Product not available");
        return;
    }

    int newId = 0;
    if (!db.addRoom(room, newId)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to create room");
        return;
    }
    room.id = newId;
    // update product status to pending
    db.updateProductStatus(room.productId, "pending", userId);

    addCors(response);
    std::ostringstream resp;
    resp << "{"
         << "\"id\":" << room.id
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Created, resp.str());
}

void RoomApi::handleListOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    auto rooms = db.getRooms(userId);
    std::ostringstream stream;
    stream << "[";
    for (size_t i = 0; i < rooms.size(); ++i) {
        const auto& r = rooms[i];
        stream << "{"
               << "\"id\":" << r.id << ","
               << "\"roomName\":\"" << escapeJson(r.roomName) << "\","
               << "\"productId\":" << r.productId << ","
               << "\"duration\":" << r.duration << ","
               << "\"status\":\"" << escapeJson(r.status) << "\","
               << "\"hostUserId\":" << r.hostUserId << ","
               << "\"createdAt\":\"" << escapeJson(r.createdAt.value_or("")) << "\","
               << "\"startedAt\":\"" << escapeJson(r.startedAt.value_or("")) << "\","
               << "\"endedAt\":\"" << escapeJson(r.endedAt.value_or("")) << "\","
               << "\"basePrice\":" << r.basePrice
               << "}";
        if (i + 1 < rooms.size()) stream << ",";
    }
    stream << "]";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, stream.str());
}

void RoomApi::handleGetOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    int id = request.param(":id").as<int>();
    auto room = db.getRoomByIdForUser(id, userId);
    if (!room.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }
    const auto& r = *room;
    std::ostringstream resp;
    resp << "{"
         << "\"id\":" << r.id << ","
         << "\"roomName\":\"" << escapeJson(r.roomName) << "\","
         << "\"productId\":" << r.productId << ","
         << "\"duration\":" << r.duration << ","
         << "\"status\":\"" << escapeJson(r.status) << "\","
         << "\"hostUserId\":" << r.hostUserId << ","
         << "\"createdAt\":\"" << escapeJson(r.createdAt.value_or("")) << "\","
         << "\"startedAt\":\"" << escapeJson(r.startedAt.value_or("")) << "\","
         << "\"endedAt\":\"" << escapeJson(r.endedAt.value_or("")) << "\","
         << "\"basePrice\":" << r.basePrice
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, resp.str());
}

void RoomApi::handleDeleteOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    int id = request.param(":id").as<int>();
    auto room = db.getRoomByIdForUser(id, userId);
    if (!room.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }

    // restore product status if waiting
    if (room->status == "waiting") {
        db.updateProductStatus(room->productId, "available");
    }

    if (!db.deleteRoom(id, userId)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Delete failed");
        return;
    }
    addCors(response);
    response.send(Http::Code::Ok, "Deleted");
}

void RoomApi::handleListPublic(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    auto rooms = db.getRoomsPublic();
    std::ostringstream stream;
    stream << "[";
    for (size_t i = 0; i < rooms.size(); ++i) {
        const auto& r = rooms[i];
        stream << "{"
               << "\"id\":" << r.id << ","
               << "\"roomName\":\"" << escapeJson(r.roomName) << "\","
               << "\"productId\":" << r.productId << ","
               << "\"duration\":" << r.duration << ","
               << "\"status\":\"" << escapeJson(r.status) << "\","
               << "\"hostUserId\":" << r.hostUserId << ","
               << "\"createdAt\":\"" << escapeJson(r.createdAt.value_or("")) << "\","
               << "\"startedAt\":\"" << escapeJson(r.startedAt.value_or("")) << "\","
               << "\"endedAt\":\"" << escapeJson(r.endedAt.value_or("")) << "\","
               << "\"basePrice\":" << r.basePrice
               << "}";
        if (i + 1 < rooms.size()) stream << ",";
    }
    stream << "]";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, stream.str());
}

void RoomApi::handleGetPublic(const Rest::Request& request, Http::ResponseWriter response) {
    int id = request.param(":id").as<int>();
    auto room = db.getRoomById(id);
    if (!room.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }
    const auto& r = *room;
    std::ostringstream resp;
    resp << "{"
         << "\"id\":" << r.id << ","
         << "\"roomName\":\"" << escapeJson(r.roomName) << "\","
         << "\"productId\":" << r.productId << ","
         << "\"duration\":" << r.duration << ","
         << "\"status\":\"" << escapeJson(r.status) << "\","
         << "\"hostUserId\":" << r.hostUserId << ","
         << "\"createdAt\":\"" << escapeJson(r.createdAt.value_or("")) << "\","
         << "\"startedAt\":\"" << escapeJson(r.startedAt.value_or("")) << "\","
         << "\"endedAt\":\"" << escapeJson(r.endedAt.value_or("")) << "\","
         << "\"basePrice\":" << r.basePrice
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, resp.str());
}

void RoomApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

void RoomApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, DELETE, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
}

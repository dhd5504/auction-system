#include "room_api.h"

#include <random>
#include <regex>
#include <sstream>
#include <vector>

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

std::vector<std::string> parseArray(const std::string& body, const std::string& key) {
    std::vector<std::string> result;
    auto pos = body.find("\"" + key + "\"");
    if (pos == std::string::npos) return result;
    pos = body.find("[", pos);
    if (pos == std::string::npos) return result;
    auto end = body.find("]", pos);
    if (end == std::string::npos) return result;
    std::string content = body.substr(pos + 1, end - pos - 1);
    // match either "text" or bare number token
    std::regex tokenRgx("\"([^\"]+)\"|(-?\\d+)");
    std::smatch match;
    auto begin = content.cbegin();
    while (std::regex_search(begin, content.cend(), match, tokenRgx)) {
        if (match[1].matched) {
            result.push_back(match[1]);
        } else if (match[2].matched) {
            result.push_back(match[2]);
        }
        begin = match.suffix().first;
    }
    return result;
}

std::string randomId(const std::string& prefix) {
    static std::mt19937 rng{std::random_device{}()};
    static const char chars[] =
        "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::uniform_int_distribution<> dist(0, static_cast<int>(sizeof(chars) - 2));
    std::string out = prefix;
    out.push_back('-');
    for (int i = 0; i < 8; ++i) {
        out.push_back(chars[dist(rng)]);
    }
    return out;
}

} // namespace

RoomApi::RoomApi(Rest::Router& router, SQLiteDb& db_, TcpClient& tcpClient_, WsServer& ws_)
    : db(db_), tcpClient(tcpClient_), ws(ws_) {
    Rest::Routes::Post(router, "/api/rooms", Rest::Routes::bind(&RoomApi::handleCreate, this));
    Rest::Routes::Get(router, "/api/rooms", Rest::Routes::bind(&RoomApi::handleList, this));
    Rest::Routes::Post(router, "/api/rooms/:id/start", Rest::Routes::bind(&RoomApi::handleStart, this));
    Rest::Routes::Options(router, "/api/rooms", Rest::Routes::bind(&RoomApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/rooms/:id/start", Rest::Routes::bind(&RoomApi::handleOptions, this));
}

void RoomApi::handleCreate(const Rest::Request& request, Http::ResponseWriter response) {
    auto body = request.body();
    Room room;
    room.id = getStringField(body, "id");
    room.name = getStringField(body, "name");
    room.startTime = getStringField(body, "startTime");
    room.productIds = parseArray(body, "products");

    if (room.id.empty()) {
        room.id = randomId("room");
    }

    if (room.name.empty() || room.startTime.empty()) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Invalid room payload");
        return;
    }

    if (!db.addRoom(room)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to create room");
        return;
    }

    std::ostringstream createCmd;
    createCmd << "ROOM_CREATE " << room.id << " " << room.startTime;
    tcpClient.sendCommand(createCmd.str());

    for (const auto& productId : room.productIds) {
        db.linkRoomProduct(room.id, productId);
        std::ostringstream addCmd;
        addCmd << "ROOM_ADD_PRODUCT " << room.id << " " << productId;
        tcpClient.sendCommand(addCmd.str());
    }

    ws.broadcast("{\"event\":\"room_created\",\"id\":\"" + room.id + "\"}");

    addCors(response);
    std::ostringstream resp;
    resp << "{"
         << "\"id\":\"" << escapeJson(room.id) << "\""
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Created, resp.str());
}

void RoomApi::handleList(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    auto rooms = db.getRooms();
    std::ostringstream stream;
    stream << "[";
    for (size_t i = 0; i < rooms.size(); ++i) {
        const auto& r = rooms[i];
        stream << "{"
               << "\"id\":\"" << escapeJson(r.id) << "\","
               << "\"name\":\"" << escapeJson(r.name) << "\","
               << "\"startTime\":\"" << escapeJson(r.startTime) << "\","
               << "\"products\":[";
        for (size_t j = 0; j < r.productIds.size(); ++j) {
            stream << "\"" << escapeJson(r.productIds[j]) << "\"";
            if (j + 1 < r.productIds.size()) stream << ",";
        }
        stream << "]"
               << "}";
        if (i + 1 < rooms.size()) {
            stream << ",";
        }
    }
    stream << "]";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, stream.str());
}

void RoomApi::handleStart(const Rest::Request& request, Http::ResponseWriter response) {
    auto idParam = request.param(":id");
    std::string roomId = idParam.as<std::string>();
    std::ostringstream cmd;
    cmd << "START_ROOM " << roomId;
    tcpClient.sendCommand(cmd.str());

    ws.broadcast("{\"event\":\"room_started\",\"id\":\"" + roomId + "\"}");
    addCors(response);
    response.send(Http::Code::Ok, "Room started");
}

void RoomApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

void RoomApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type");
}

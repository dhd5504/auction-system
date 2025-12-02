#include "auth_api.h"

#include <regex>
#include <sstream>
#include <cstring>

using namespace Pistache;

namespace {

std::string getStringField(const std::string& body, const std::string& key) {
    std::regex rgx("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    std::smatch match;
    if (std::regex_search(body, match, rgx)) {
        return match[1];
    }
    return {};
}

template <size_t N>
void copyToFixed(const std::string& input, char (&target)[N]) {
    std::memset(target, 0, N);
    std::strncpy(target, input.c_str(), N - 1);
}

} // namespace

AuthApi::AuthApi(Rest::Router& router, TcpClient& tcp)
    : tcpClient(tcp) {
    Rest::Routes::Post(router, "/api/login", Rest::Routes::bind(&AuthApi::handleLogin, this));
    Rest::Routes::Post(router, "/api/register", Rest::Routes::bind(&AuthApi::handleRegister, this));
    Rest::Routes::Options(router, "/api/login", Rest::Routes::bind(&AuthApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/register", Rest::Routes::bind(&AuthApi::handleOptions, this));
}

void AuthApi::handleLogin(const Rest::Request& request, Http::ResponseWriter response) {
    auto body = request.body();
    std::string username = getStringField(body, "username");
    std::string password = getStringField(body, "password");

    if (username.empty() || password.empty()) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Missing username or password");
        return;
    }

    LoginReq req{};
    copyToFixed(username, req.username);
    copyToFixed(password, req.password);
    req.registerFlag = 0;

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::LOGIN_REQ, 0, &req, sizeof(req), respHeader, respPayload) ||
        respHeader.opcode != Opcode::LOGIN_RES ||
        respPayload.size() < sizeof(LoginRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Gateway login failed");
        return;
    }

    LoginRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(LoginRes));

    if (!res.success) {
        addCors(response);
        response.send(Http::Code::Unauthorized, std::string(res.message, strnlen(res.message, sizeof(res.message))));
        return;
    }

    std::ostringstream out;
    out << "{"
        << "\"token\":\"" << respHeader.sessionId << "\","
        << "\"tokenType\":\"Session\","
        << "\"user\":{"
        << "\"id\":" << res.userId << ","
        << "\"username\":\"" << username << "\","
        << "\"role\":\"" << std::string(res.role, strnlen(res.role, sizeof(res.role))) << "\""
        << "}"
        << "}";

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, out.str());
}

void AuthApi::handleRegister(const Rest::Request& request, Http::ResponseWriter response) {
    auto body = request.body();
    std::string username = getStringField(body, "username");
    std::string password = getStringField(body, "password");

    if (username.empty() || password.size() < 4) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Invalid username or password");
        return;
    }

    LoginReq req{};
    copyToFixed(username, req.username);
    copyToFixed(password, req.password);
    req.registerFlag = 1;

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::LOGIN_REQ, 0, &req, sizeof(req), respHeader, respPayload) ||
        respHeader.opcode != Opcode::LOGIN_RES ||
        respPayload.size() < sizeof(LoginRes)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Gateway register failed");
        return;
    }

    LoginRes res{};
    std::memcpy(&res, respPayload.data(), sizeof(LoginRes));

    if (!res.success) {
        addCors(response);
        response.send(Http::Code::Conflict, std::string(res.message, strnlen(res.message, sizeof(res.message))));
        return;
    }

    std::ostringstream out;
    out << "{"
        << "\"token\":\"" << respHeader.sessionId << "\","
        << "\"tokenType\":\"Session\","
        << "\"user\":{"
        << "\"id\":" << res.userId << ","
        << "\"username\":\"" << username << "\","
        << "\"role\":\"" << std::string(res.role, strnlen(res.role, sizeof(res.role))) << "\""
        << "}"
        << "}";

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Created, out.str());
}

void AuthApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

void AuthApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, PATCH, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
}

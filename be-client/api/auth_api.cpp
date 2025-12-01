#include "auth_api.h"

#include <openssl/sha.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <regex>
#include <sstream>

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

std::string nowIso() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&time, &tm);
    char buffer[64];
    std::strftime(buffer, sizeof(buffer), "%FT%TZ", &tm);
    return buffer;
}

} // namespace

AuthApi::AuthApi(Rest::Router& router, SQLiteDb& db_, JwtService& jwt_)
    : db(db_), jwt(jwt_) {
    Rest::Routes::Post(router, "/api/register", Rest::Routes::bind(&AuthApi::handleRegister, this));
    Rest::Routes::Post(router, "/api/login", Rest::Routes::bind(&AuthApi::handleLogin, this));
    Rest::Routes::Options(router, "/api/register", Rest::Routes::bind(&AuthApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/login", Rest::Routes::bind(&AuthApi::handleOptions, this));
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

    if (db.getUserByUsername(username).has_value()) {
        addCors(response);
        response.send(Http::Code::Conflict, "User already exists");
        return;
    }

    User user;
    user.username = username;
    user.password = hashPassword(password);
    user.role = "user";
    user.createdAt = nowIso();

    int newId = 0;
    if (!db.addUser(user, newId)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to create user");
        return;
    }
    user.id = newId;

    std::string token = jwt.issueToken(std::to_string(user.id), user.role);
    std::ostringstream resp;
    resp << "{"
         << "\"token\":\"" << token << "\","
         << "\"tokenType\":\"Bearer\","
         << "\"user\":{"
         << "\"id\":" << user.id << ","
         << "\"username\":\"" << user.username << "\","
         << "\"role\":\"" << user.role << "\""
         << "}"
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Created, resp.str());
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

    auto found = db.getUserByUsername(username);
    if (!found.has_value()) {
        addCors(response);
        response.send(Http::Code::Unauthorized, "Invalid credentials");
        return;
    }
    const auto& user = *found;
    std::string hashed = hashPassword(password);
    if (hashed != user.password) {
        addCors(response);
        response.send(Http::Code::Unauthorized, "Invalid credentials");
        return;
    }

    std::string token = jwt.issueToken(std::to_string(user.id), user.role);
    std::ostringstream resp;
    resp << "{"
         << "\"token\":\"" << token << "\","
         << "\"tokenType\":\"Bearer\","
         << "\"user\":{"
         << "\"id\":" << user.id << ","
         << "\"username\":\"" << user.username << "\","
         << "\"role\":\"" << user.role << "\""
         << "}"
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, resp.str());
}

void AuthApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

void AuthApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
}

std::string AuthApi::hashPassword(const std::string& password) {
    std::string combined = password;
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(combined.data()), combined.size(), hash);

    std::ostringstream out;
    for (unsigned char c : hash) {
        out << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(c);
    }
    return out.str();
}

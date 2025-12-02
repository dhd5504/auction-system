#include "product_api.h"

#include <regex>
#include <sstream>
#include <cstring>

using namespace Pistache;

namespace {

template <size_t N>
void copyToFixed(const std::string& input, char (&target)[N]) {
    std::memset(target, 0, N);
    std::strncpy(target, input.c_str(), N - 1);
}

std::string ensureIdWrapped(int id, const std::string& body) {
    if (body.empty()) {
        std::ostringstream out;
        out << "{\"id\":" << id << "}";
        return out.str();
    }
    std::string trimmed = body;
    if (trimmed.front() == '{' && trimmed.back() == '}') {
        trimmed = trimmed.substr(1, trimmed.size() - 2);
    }
    std::ostringstream out;
    out << "{\"id\":" << id;
    if (!trimmed.empty()) out << "," << trimmed;
    out << "}";
    return out.str();
}

} // namespace

ProductApi::ProductApi(Rest::Router& router, TcpClient& tcp, WsServer& ws)
    : tcpClient(tcp), wsServer(ws) {
    Rest::Routes::Get(router, "/api/products", Rest::Routes::bind(&ProductApi::handleListPublic, this));
    Rest::Routes::Get(router, "/api/me/products", Rest::Routes::bind(&ProductApi::handleListOwn, this));
    Rest::Routes::Get(router, "/api/products/:id", Rest::Routes::bind(&ProductApi::handleGet, this));
    Rest::Routes::Post(router, "/api/me/products", Rest::Routes::bind(&ProductApi::handleCreate, this));
    Rest::Routes::Put(router, "/api/me/products/:id", Rest::Routes::bind(&ProductApi::handleUpdate, this));
    Rest::Routes::Patch(router, "/api/me/products/:id/status", Rest::Routes::bind(&ProductApi::handleUpdateStatus, this));
    Rest::Routes::Delete(router, "/api/me/products/:id", Rest::Routes::bind(&ProductApi::handleDelete, this));
    Rest::Routes::Options(router, "/api/products", Rest::Routes::bind(&ProductApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/products/:id", Rest::Routes::bind(&ProductApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/products", Rest::Routes::bind(&ProductApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/products/:id", Rest::Routes::bind(&ProductApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/products/:id/status", Rest::Routes::bind(&ProductApi::handleOptions, this));
}

bool ProductApi::authorize(const Rest::Request& request, Http::ResponseWriter& response, uint32_t& sessionId) {
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

void ProductApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, DELETE, PATCH, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
}

void ProductApi::handleListPublic(const Rest::Request& request, Http::ResponseWriter response) {
    (void)request;
    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::PRODUCT_LIST);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, 0, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to fetch products");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));

    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, json);
}

void ProductApi::handleListOwn(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;

    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::PRODUCT_LIST_OWN);
    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to fetch products");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, json);
}

void ProductApi::handleGet(const Rest::Request& request, Http::ResponseWriter response) {
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }
    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::PRODUCT_GET);
    copyToFixed("{\"id\":" + std::to_string(id) + "}", req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, 0, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to fetch product");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = json.find("error") != std::string::npos ? Http::Code::Not_Found : Http::Code::Ok;
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, json);
}

void ProductApi::handleCreate(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::PRODUCT_CREATE);
    copyToFixed(request.body(), req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to create product");
        return;
    }
    NotifyMessage res{};
    std::memcpy(&res, respPayload.data(), sizeof(NotifyMessage));
    std::string json = std::string(res.message, strnlen(res.message, sizeof(res.message)));
    Http::Code code = json.find("error") != std::string::npos ? Http::Code::Bad_Request : Http::Code::Created;
    if (code == Http::Code::Created) {
        wsServer.broadcast("{\"type\":\"product_created\",\"payload\":" + json + "}");
    }
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(code, json);
}

void ProductApi::handleUpdate(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }

    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::PRODUCT_UPDATE);
    copyToFixed(ensureIdWrapped(id, request.body()), req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to update product");
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

void ProductApi::handleUpdateStatus(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }

    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::PRODUCT_UPDATE);
    copyToFixed(ensureIdWrapped(id, request.body()), req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to update status");
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

void ProductApi::handleDelete(const Rest::Request& request, Http::ResponseWriter response) {
    uint32_t sessionId = 0;
    if (!authorize(request, response, sessionId)) return;
    int id = 0;
    try { id = request.param(":id").as<int>(); } catch (...) { id = 0; }

    NotifyMessage req{};
    req.code = static_cast<uint32_t>(NotifyCode::PRODUCT_DELETE);
    copyToFixed("{\"id\":" + std::to_string(id) + "}", req.message);

    PacketHeader respHeader{};
    std::vector<uint8_t> respPayload;
    if (!tcpClient.transact(Opcode::NOTIFY_MESSAGE, sessionId, &req, sizeof(req), respHeader, respPayload) ||
        respPayload.size() < sizeof(NotifyMessage)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to delete product");
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

void ProductApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

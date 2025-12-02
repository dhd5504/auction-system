#include "product_api.h"

#include <regex>
#include <sstream>
#include <optional>
#include <vector>
#include <algorithm>

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

ProductApi::ProductApi(Rest::Router& router, SQLiteDb& db_, JwtService& jwt_)
    : db(db_), jwt(jwt_) {
    // Owner routes
    Rest::Routes::Get(router, "/api/me/products", Rest::Routes::bind(&ProductApi::handleListOwn, this));
    Rest::Routes::Get(router, "/api/me/products/:id", Rest::Routes::bind(&ProductApi::handleGetOwn, this));
    Rest::Routes::Post(router, "/api/me/products", Rest::Routes::bind(&ProductApi::handleCreateOwn, this));
    Rest::Routes::Put(router, "/api/me/products/:id", Rest::Routes::bind(&ProductApi::handleUpdateOwn, this));
    Rest::Routes::Delete(router, "/api/me/products/:id", Rest::Routes::bind(&ProductApi::handleDeleteOwn, this));
    Rest::Routes::Patch(router, "/api/me/products/:id/status", Rest::Routes::bind(&ProductApi::handleUpdateStatus, this));
    Rest::Routes::Options(router, "/api/me/products", Rest::Routes::bind(&ProductApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/products/:id", Rest::Routes::bind(&ProductApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/me/products/:id/status", Rest::Routes::bind(&ProductApi::handleOptions, this));

    // Public routes
    Rest::Routes::Get(router, "/api/products", Rest::Routes::bind(&ProductApi::handleListPublic, this));
    Rest::Routes::Get(router, "/api/products/:id", Rest::Routes::bind(&ProductApi::handleGetPublic, this));
    Rest::Routes::Options(router, "/api/products", Rest::Routes::bind(&ProductApi::handleOptions, this));
    Rest::Routes::Options(router, "/api/products/:id", Rest::Routes::bind(&ProductApi::handleOptions, this));
}

bool ProductApi::authorize(const Rest::Request& request, Http::ResponseWriter& response, int& userId, std::string& role) {
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

void ProductApi::handleCreateOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    auto body = request.body();
    Product product;
    product.name = getStringField(body, "name");
    product.description = getStringField(body, "description");
    product.startPrice = getIntField(body, "startPrice");
    product.status = "available";
    product.ownerUserId = userId;
    product.imageUrl = getStringField(body, "imageUrl");
    product.category = getStringField(body, "category");

    if (product.name.empty() || product.startPrice <= 0) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Invalid product payload");
        return;
    }

    int newId = 0;
    if (!db.addProduct(product, newId)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to save product");
        return;
    }
    product.id = newId;

    addCors(response);
    std::ostringstream resp;
    resp << "{"
         << "\"id\":" << product.id
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Created, resp.str());
}

void ProductApi::handleListOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    auto products = db.getProducts(userId);
    std::ostringstream stream;
    stream << "[";
    for (size_t i = 0; i < products.size(); ++i) {
        const auto& p = products[i];
        stream << "{"
               << "\"id\":" << p.id << ","
               << "\"name\":\"" << escapeJson(p.name) << "\","
               << "\"description\":\"" << escapeJson(p.description.value_or("")) << "\","
               << "\"startPrice\":" << p.startPrice << ","
               << "\"status\":\"" << escapeJson(p.status) << "\","
               << "\"ownerUserId\":" << p.ownerUserId << ","
               << "\"createdAt\":\"" << escapeJson(p.createdAt.value_or("")) << "\","
               << "\"imageUrl\":\"" << escapeJson(p.imageUrl.value_or("")) << "\","
               << "\"category\":\"" << escapeJson(p.category.value_or("")) << "\""
               << "}";
        if (i + 1 < products.size()) stream << ",";
    }
    stream << "]";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, stream.str());
}

void ProductApi::handleGetOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    int id = request.param(":id").as<int>();
    auto product = db.getProductByIdForOwner(id, userId);
    if (!product.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }

    const auto& p = *product;
    std::ostringstream resp;
    resp << "{"
         << "\"id\":" << p.id << ","
         << "\"name\":\"" << escapeJson(p.name) << "\","
         << "\"description\":\"" << escapeJson(p.description.value_or("")) << "\","
         << "\"startPrice\":" << p.startPrice << ","
         << "\"status\":\"" << escapeJson(p.status) << "\","
         << "\"ownerUserId\":" << p.ownerUserId << ","
         << "\"createdAt\":\"" << escapeJson(p.createdAt.value_or("")) << "\","
         << "\"imageUrl\":\"" << escapeJson(p.imageUrl.value_or("")) << "\","
         << "\"category\":\"" << escapeJson(p.category.value_or("")) << "\""
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, resp.str());
}

void ProductApi::handleUpdateOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    int id = request.param(":id").as<int>();
    auto existing = db.getProductByIdForOwner(id, userId);
    if (!existing.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }
    if (existing->status != "available") {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Product not editable");
        return;
    }

    auto body = request.body();
    Product updated = *existing;
    std::string name = getStringField(body, "name");
    if (!name.empty()) updated.name = name;
    std::string desc = getStringField(body, "description");
    if (!desc.empty()) updated.description = desc;
    int startPrice = getIntField(body, "startPrice");
    if (startPrice > 0) updated.startPrice = startPrice;
    std::string status = getStringField(body, "status");
    if (!status.empty()) updated.status = status;
    std::string imageUrl = getStringField(body, "imageUrl");
    if (!imageUrl.empty()) updated.imageUrl = imageUrl;
    std::string category = getStringField(body, "category");
    if (!category.empty()) updated.category = category;

    if (!db.updateProduct(updated)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Update failed");
        return;
    }
    addCors(response);
    response.send(Http::Code::Ok, "Updated");
}

void ProductApi::handleDeleteOwn(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) return;
    (void)role;

    int id = request.param(":id").as<int>();
    auto existing = db.getProductByIdForOwner(id, userId);
    if (!existing.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }
    if (existing->status != "available") {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Product not deletable");
        return;
    }
    if (!db.deleteProduct(id, userId)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Delete failed");
        return;
    }
    addCors(response);
    response.send(Http::Code::Ok, "Deleted");
}

void ProductApi::handleUpdateStatus(const Rest::Request& request, Http::ResponseWriter response) {
    int userId;
    std::string role;
    if (!authorize(request, response, userId, role)) {
        return;
    }
    (void)role;

    int id = request.param(":id").as<int>();
    auto existing = db.getProductByIdForOwner(id, userId);
    if (!existing.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }

    std::string body = request.body();
    std::string status = getStringField(body, "status");
    const std::vector<std::string> allowed = {"available", "pending", "running", "sold", "cancelled"};
    if (std::find(allowed.begin(), allowed.end(), status) == allowed.end()) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Invalid status");
        return;
    }

    if (!db.updateProductStatus(id, status, userId)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Update status failed");
        return;
    }
    addCors(response);
    response.send(Http::Code::Ok, "Status updated");
}

void ProductApi::handleListPublic(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    auto products = db.getProductsPublic();
    std::ostringstream stream;
    stream << "[";
    for (size_t i = 0; i < products.size(); ++i) {
        const auto& p = products[i];
        stream << "{"
               << "\"id\":" << p.id << ","
               << "\"name\":\"" << escapeJson(p.name) << "\","
               << "\"description\":\"" << escapeJson(p.description.value_or("")) << "\","
               << "\"startPrice\":" << p.startPrice << ","
               << "\"status\":\"" << escapeJson(p.status) << "\","
               << "\"ownerUserId\":" << p.ownerUserId << ","
               << "\"createdAt\":\"" << escapeJson(p.createdAt.value_or("")) << "\","
               << "\"imageUrl\":\"" << escapeJson(p.imageUrl.value_or("")) << "\","
               << "\"category\":\"" << escapeJson(p.category.value_or("")) << "\""
               << "}";
        if (i + 1 < products.size()) stream << ",";
    }
    stream << "]";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, stream.str());
}

void ProductApi::handleGetPublic(const Rest::Request& request, Http::ResponseWriter response) {
    int id = request.param(":id").as<int>();
    auto product = db.getProductById(id);
    if (!product.has_value()) {
        addCors(response);
        response.send(Http::Code::Not_Found, "Not found");
        return;
    }
    const auto& p = *product;
    std::ostringstream resp;
    resp << "{"
         << "\"id\":" << p.id << ","
         << "\"name\":\"" << escapeJson(p.name) << "\","
         << "\"description\":\"" << escapeJson(p.description.value_or("")) << "\","
         << "\"startPrice\":" << p.startPrice << ","
         << "\"status\":\"" << escapeJson(p.status) << "\","
         << "\"ownerUserId\":" << p.ownerUserId << ","
         << "\"createdAt\":\"" << escapeJson(p.createdAt.value_or("")) << "\","
         << "\"imageUrl\":\"" << escapeJson(p.imageUrl.value_or("")) << "\","
         << "\"category\":\"" << escapeJson(p.category.value_or("")) << "\""
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, resp.str());
}

void ProductApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

void ProductApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, PUT, PATCH, DELETE, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type, Authorization");
}

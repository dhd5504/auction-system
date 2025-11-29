#include "product_api.h"

#include <random>
#include <regex>
#include <sstream>

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

double getNumberField(const std::string& body, const std::string& key) {
    std::regex rgx("\"" + key + "\"\\s*:\\s*([-+]?[0-9]*\\.?[0-9]+)");
    std::smatch match;
    if (std::regex_search(body, match, rgx)) {
        return std::stod(match[1]);
    }
    return 0.0;
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

ProductApi::ProductApi(Rest::Router& router, SQLiteDb& db_, TcpClient& tcpClient_, WsServer& ws_)
    : db(db_), tcpClient(tcpClient_), ws(ws_) {
    Rest::Routes::Post(router, "/api/products", Rest::Routes::bind(&ProductApi::handleCreate, this));
    Rest::Routes::Get(router, "/api/products", Rest::Routes::bind(&ProductApi::handleList, this));
    Rest::Routes::Options(router, "/api/products", Rest::Routes::bind(&ProductApi::handleOptions, this));
}

void ProductApi::handleCreate(const Rest::Request& request, Http::ResponseWriter response) {
    auto body = request.body();
    Product product;
    product.id = getStringField(body, "id");
    product.name = getStringField(body, "name");
    product.startPrice = getNumberField(body, "startPrice");
    product.buyPrice = getNumberField(body, "buyPrice");
    product.step = getNumberField(body, "step");
    product.description = getStringField(body, "description");

    if (product.id.empty()) {
        product.id = randomId("prod");
    }

    if (product.name.empty()) {
        addCors(response);
        response.send(Http::Code::Bad_Request, "Invalid product payload");
        return;
    }

    if (!db.addProduct(product)) {
        addCors(response);
        response.send(Http::Code::Internal_Server_Error, "Failed to save product");
        return;
    }

    std::ostringstream cmd;
    cmd << "PRODUCT_ADD " << product.id << " " << product.name << " "
        << product.startPrice << " " << product.buyPrice << " " << product.step
        << " " << product.description;
    tcpClient.sendCommand(cmd.str());

    ws.broadcast("{\"event\":\"product_added\",\"id\":\"" + product.id + "\"}");

    addCors(response);
    std::ostringstream resp;
    resp << "{"
         << "\"id\":\"" << escapeJson(product.id) << "\""
         << "}";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    response.send(Http::Code::Created, resp.str());
}

void ProductApi::handleList(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    auto products = db.getProducts();
    std::ostringstream stream;
    stream << "[";
    for (size_t i = 0; i < products.size(); ++i) {
        const auto& p = products[i];
        stream << "{"
               << "\"id\":\"" << escapeJson(p.id) << "\","
               << "\"name\":\"" << escapeJson(p.name) << "\","
               << "\"startPrice\":" << p.startPrice << ","
               << "\"buyPrice\":" << p.buyPrice << ","
               << "\"step\":" << p.step << ","
               << "\"description\":\"" << escapeJson(p.description) << "\""
               << "}";
        if (i + 1 < products.size()) {
            stream << ",";
        }
    }
    stream << "]";
    response.headers().add<Http::Header::ContentType>(MIME(Application, Json));
    addCors(response);
    response.send(Http::Code::Ok, stream.str());
}

void ProductApi::handleOptions(const Rest::Request& /*request*/, Http::ResponseWriter response) {
    addCors(response);
    response.send(Http::Code::Ok);
}

void ProductApi::addCors(Http::ResponseWriter& response) {
    response.headers()
        .add<Http::Header::AccessControlAllowOrigin>("*")
        .add<Http::Header::AccessControlAllowMethods>("GET, POST, OPTIONS")
        .add<Http::Header::AccessControlAllowHeaders>("Content-Type");
}

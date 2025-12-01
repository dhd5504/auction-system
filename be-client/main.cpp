#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

#include <iostream>

#include "api/product_api.h"
#include "api/room_api.h"
#include "api/auth_api.h"
#include "db/sqlite.h"
#include "security/jwt.h"
#include <cstdlib>

int main() {
    SQLiteDb database("data.db");
    if (!database.open()) {
        std::cerr << "Unable to open SQLite database" << std::endl;
        return 1;
    }
    if (!database.initSchema()) {
        std::cerr << "Unable to initialize SQLite schema" << std::endl;
        return 1;
    }

    const char* envSecret = std::getenv("JWT_SECRET");
    const char* envExpiry = std::getenv("JWT_EXPIRES_IN");
    int expirySeconds = envExpiry ? std::atoi(envExpiry) : 3600;
    if (expirySeconds <= 0) expirySeconds = 3600;
    JwtService jwt(envSecret ? envSecret : "", expirySeconds);

    Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8080));
    Pistache::Http::Endpoint httpEndpoint(addr);
    Pistache::Rest::Router router;

    ProductApi productApi(router, database, jwt);
    RoomApi roomApi(router, database, jwt);
    AuthApi authApi(router, database, jwt);
    (void)productApi;
    (void)roomApi;
    (void)authApi;

    auto opts = Pistache::Http::Endpoint::options()
                    .threads(2)
                    .flags(Pistache::Tcp::Options::ReuseAddr);
    httpEndpoint.init(opts);
    httpEndpoint.setHandler(router.handler());

    std::cout << "Starting REST API on http://localhost:8080" << std::endl;
    std::cout << "WebSocket server at ws://localhost:8081/ws" << std::endl;
    httpEndpoint.serveThreaded();

    std::cout << "Press ENTER to exit" << std::endl;
    std::string line;
    std::getline(std::cin, line);

    httpEndpoint.shutdown();
    database.close();
    return 0;
}

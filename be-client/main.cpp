#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

#include <iostream>

#include "api/product_api.h"
#include "api/room_api.h"
#include "db/sqlite.h"
#include "tcp/tcp_client.h"
#include "ws/ws_server.h"

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

    WsServer wsServer(8081);
    wsServer.start();

    TcpClient tcpClient("127.0.0.1", 9000);

    Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8080));
    Pistache::Http::Endpoint httpEndpoint(addr);
    Pistache::Rest::Router router;

    ProductApi productApi(router, database, tcpClient, wsServer);
    RoomApi roomApi(router, database, tcpClient, wsServer);
    (void)productApi;
    (void)roomApi;

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
    wsServer.stop();
    database.close();
    return 0;
}

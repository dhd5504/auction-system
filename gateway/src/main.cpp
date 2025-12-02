#include <pistache/endpoint.h>
#include <pistache/http.h>
#include <pistache/router.h>

#include <cstdlib>
#include <iostream>
#include <string>

#include "api/auth_api.h"
#include "api/product_api.h"
#include "api/room_api.h"
#include "tcp_client.h"
#include "ws/ws_server.h"

int main() {
    const char* hostEnv = std::getenv("TCP_SERVER_HOST");
    const char* portEnv = std::getenv("TCP_SERVER_PORT");
    std::string tcpHost = hostEnv ? hostEnv : "127.0.0.1";
    uint16_t tcpPort = portEnv ? static_cast<uint16_t>(std::stoi(portEnv)) : 9000;

    TcpClient tcpClient(tcpHost, tcpPort);
    WsServer wsServer(8081);
    wsServer.start();

    Pistache::Rest::Router router;
    AuthApi authApi(router, tcpClient);
    ProductApi productApi(router, tcpClient, wsServer);
    RoomApi roomApi(router, tcpClient, wsServer);
    (void)authApi;
    (void)productApi;
    (void)roomApi;

    Pistache::Address addr(Pistache::Ipv4::any(), Pistache::Port(8080));
    Pistache::Http::Endpoint endpoint(addr);
    auto opts = Pistache::Http::Endpoint::options()
                    .threads(2)
                    .flags(Pistache::Tcp::Options::ReuseAddr);
    endpoint.init(opts);
    endpoint.setHandler(router.handler());

    std::cout << "Gateway started. REST http://localhost:8080, WS ws://localhost:8081/ws\n";
    endpoint.serveThreaded();

    std::cout << "Press ENTER to stop gateway" << std::endl;
    std::string line;
    std::getline(std::cin, line);

    endpoint.shutdown();
    wsServer.stop();
    return 0;
}

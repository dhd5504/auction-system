#pragma once

#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>

#include "tcp_client.h"
#include "ws/ws_server.h"

class RoomApi {
public:
    RoomApi(Pistache::Rest::Router& router, TcpClient& tcp, WsServer& ws);

private:
    bool authorize(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter& response, uint32_t& sessionId);
    void addCors(Pistache::Http::ResponseWriter& response);

    void handleListPublic(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleGetPublic(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleListOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleGetOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleCreateOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleDeleteOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleStartOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleCancelOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleBid(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleBuyNow(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleOptions(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);

    TcpClient& tcpClient;
    WsServer& wsServer;
};

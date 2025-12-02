#pragma once

#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>

#include "tcp_client.h"
#include "ws/ws_server.h"

class ProductApi {
public:
    ProductApi(Pistache::Rest::Router& router, TcpClient& tcp, WsServer& ws);

private:
    bool authorize(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter& response, uint32_t& sessionId);
    void addCors(Pistache::Http::ResponseWriter& response);

    void handleListPublic(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleListOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleGet(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleCreate(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleUpdate(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleUpdateStatus(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleDelete(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleOptions(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);

    TcpClient& tcpClient;
    WsServer& wsServer;
};

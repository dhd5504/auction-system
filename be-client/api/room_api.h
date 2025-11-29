#pragma once

#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>

#include "db/sqlite.h"
#include "tcp/tcp_client.h"
#include "ws/ws_server.h"

class RoomApi {
public:
    RoomApi(Pistache::Rest::Router& router, SQLiteDb& db, TcpClient& tcpClient, WsServer& ws);

private:
    void handleCreate(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleList(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleStart(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleOptions(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void addCors(Pistache::Http::ResponseWriter& response);

    SQLiteDb& db;
    TcpClient& tcpClient;
    WsServer& ws;
};

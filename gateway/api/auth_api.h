#pragma once

#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>

#include "tcp_client.h"

class AuthApi {
public:
    AuthApi(Pistache::Rest::Router& router, TcpClient& tcp);

private:
    void handleLogin(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleRegister(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleOptions(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);

    void addCors(Pistache::Http::ResponseWriter& response);
    TcpClient& tcpClient;
};

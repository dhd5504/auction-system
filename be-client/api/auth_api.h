#pragma once

#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>

#include "db/sqlite.h"
#include "security/jwt.h"

class AuthApi {
public:
    AuthApi(Pistache::Rest::Router& router, SQLiteDb& db, JwtService& jwt);

private:
    void handleRegister(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleLogin(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleOptions(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);

    void addCors(Pistache::Http::ResponseWriter& response);
    std::string hashPassword(const std::string& password);

    SQLiteDb& db;
    JwtService& jwt;
};

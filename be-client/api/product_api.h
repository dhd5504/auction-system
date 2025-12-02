#pragma once

#include <pistache/http.h>
#include <pistache/http_headers.h>
#include <pistache/router.h>

#include "db/sqlite.h"
#include "security/jwt.h"

class ProductApi {
public:
    ProductApi(Pistache::Rest::Router& router, SQLiteDb& db, JwtService& jwt);

private:
    bool authorize(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter& response, int& userId, std::string& role);
    void handleCreateOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleListOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleGetOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleUpdateOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleDeleteOwn(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleUpdateStatus(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);

    void handleListPublic(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleGetPublic(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void handleOptions(const Pistache::Rest::Request& request, Pistache::Http::ResponseWriter response);
    void addCors(Pistache::Http::ResponseWriter& response);

    SQLiteDb& db;
    JwtService& jwt;
};

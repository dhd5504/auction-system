#pragma once

#include <mutex>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "model/product.h"
#include "model/room.h"

class SQLiteDb {
public:
    explicit SQLiteDb(const std::string& path);
    ~SQLiteDb();

    bool open();
    void close();
    bool initSchema();

    bool addProduct(const Product& product);
    std::vector<Product> getProducts();

    bool addRoom(const Room& room);
    std::vector<Room> getRooms();

    bool linkRoomProduct(const std::string& roomId, const std::string& productId);
    std::vector<std::string> getRoomProducts(const std::string& roomId);

private:
    bool execute(const std::string& sql);
    std::string loadSchema();
    std::string loadSeed();

    std::string dbPath;
    sqlite3* db{nullptr};
    std::recursive_mutex mutex;
};

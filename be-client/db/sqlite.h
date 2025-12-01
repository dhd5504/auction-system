#pragma once

#include <optional>
#include <mutex>
#include <string>
#include <vector>

#include <sqlite3.h>

#include "model/product.h"
#include "model/room.h"
#include "model/user.h"

class SQLiteDb {
public:
    explicit SQLiteDb(const std::string& path);
    ~SQLiteDb();

    bool open();
    void close();
    bool initSchema();

    bool addProduct(const Product& product, int& newId);
    std::vector<Product> getProducts(int ownerUserId);
    std::vector<Product> getProductsPublic();
    std::optional<Product> getProductById(int id);
    std::optional<Product> getProductByIdForOwner(int id, int ownerUserId);
    bool updateProduct(const Product& product);
    bool deleteProduct(int id, int ownerUserId);
    bool updateProductStatus(int id, const std::string& status, std::optional<int> ownerUserId = std::nullopt);

    bool addRoom(const Room& room, int& newId);
    std::vector<Room> getRooms(int hostUserId);
    std::vector<Room> getRoomsPublic();
    std::optional<Room> getRoomByIdForUser(int id, int hostUserId);
    std::optional<Room> getRoomById(int id);
    bool deleteRoom(int id, int hostUserId);

    bool addUser(const User& user, int& newId);
    std::optional<User> getUserByUsername(const std::string& username);
    std::optional<User> getUserById(int id);

private:
    bool execute(const std::string& sql);
    std::string loadSchema();
    std::string loadSeed();

    std::string dbPath;
    sqlite3* db{nullptr};
    std::recursive_mutex mutex;
};

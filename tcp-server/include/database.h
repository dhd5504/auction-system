#pragma once

#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <sqlite3.h>

struct User {
    int id{0};
    std::string username;
    std::string password;
    std::string role{"user"};
    bool isActive{true};
};

struct Product {
    int id{0};
    std::string name;
    std::optional<std::string> description;
    int startPrice{0};
    std::string status{"available"};
    int ownerUserId{0};
    std::optional<std::string> createdAt;
    std::optional<std::string> updatedAt;
    std::optional<std::string> imageUrl;
    std::optional<std::string> category;
};

struct Room {
    int id{0};
    std::string roomName;
    int productId{0};
    int duration{0};
    std::string status{"waiting"};
    int hostUserId{0};
    std::optional<std::string> createdAt;
    std::optional<std::string> updatedAt;
    std::optional<std::string> startedAt;
    std::optional<std::string> endedAt;
    int basePrice{0};
};

class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    bool open();
    void close();
    bool initSchema();

    std::optional<User> getUserByUsername(const std::string& username);
    std::optional<User> getUserById(int id);
    bool addUser(const User& user, int& newId);
    bool updateLastLogin(int id);

    bool addProduct(const Product& product, int& newId);
    std::vector<Product> getProducts(std::optional<int> ownerUserId = std::nullopt);
    std::optional<Product> getProductById(int id);
    bool updateProduct(const Product& product);
    bool deleteProduct(int id, int ownerUserId);
    bool updateProductStatus(int id, const std::string& status, std::optional<int> ownerUserId = std::nullopt);

    bool addRoom(const Room& room, int& newId);
    std::vector<Room> getRoomsPublic();
    std::vector<Room> getRoomsByOwner(int hostUserId);
    std::optional<Room> getRoomById(int id);
    bool deleteRoom(int id, int hostUserId);
    bool updateRoomStatus(int id, int hostUserId, const std::string& status, std::optional<std::string> startedAt = std::nullopt, std::optional<std::string> endedAt = std::nullopt);

    bool recordBid(int roomId, int userId, int amount);
    int getHighestBid(int roomId);

private:
    bool execute(const std::string& sql);
    std::string loadSchema();
    std::string loadSeed();

    std::string dbPath;
    sqlite3* db{nullptr};
    std::recursive_mutex mutex;
};

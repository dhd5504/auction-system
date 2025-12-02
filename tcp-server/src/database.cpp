#include "database.h"

#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

void bindOptional(sqlite3_stmt* stmt, int idx, const std::optional<std::string>& value) {
    if (value && !value->empty()) {
        sqlite3_bind_text(stmt, idx, value->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, idx);
    }
}

User mapUser(sqlite3_stmt* stmt) {
    User u;
    u.id = sqlite3_column_int(stmt, 0);
    if (const unsigned char* t = sqlite3_column_text(stmt, 1)) u.username = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 2)) u.password = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 3)) u.role = reinterpret_cast<const char*>(t);
    u.isActive = sqlite3_column_int(stmt, 4) != 0;
    return u;
}

Product mapProduct(sqlite3_stmt* stmt) {
    Product p;
    p.id = sqlite3_column_int(stmt, 0);
    if (const unsigned char* t = sqlite3_column_text(stmt, 1)) p.name = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 2)) p.description = reinterpret_cast<const char*>(t);
    p.startPrice = sqlite3_column_int(stmt, 3);
    if (const unsigned char* t = sqlite3_column_text(stmt, 4)) p.status = reinterpret_cast<const char*>(t);
    p.ownerUserId = sqlite3_column_int(stmt, 5);
    if (const unsigned char* t = sqlite3_column_text(stmt, 6)) p.createdAt = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 7)) p.updatedAt = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 8)) p.imageUrl = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 9)) p.category = reinterpret_cast<const char*>(t);
    return p;
}

Room mapRoom(sqlite3_stmt* stmt) {
    Room r;
    r.id = sqlite3_column_int(stmt, 0);
    if (const unsigned char* t = sqlite3_column_text(stmt, 1)) r.roomName = reinterpret_cast<const char*>(t);
    r.productId = sqlite3_column_int(stmt, 2);
    r.duration = sqlite3_column_int(stmt, 3);
    if (const unsigned char* t = sqlite3_column_text(stmt, 4)) r.status = reinterpret_cast<const char*>(t);
    r.hostUserId = sqlite3_column_int(stmt, 5);
    if (const unsigned char* t = sqlite3_column_text(stmt, 6)) r.createdAt = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 7)) r.updatedAt = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 8)) r.startedAt = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 9)) r.endedAt = reinterpret_cast<const char*>(t);
    r.basePrice = sqlite3_column_int(stmt, 10);
    return r;
}

} // namespace

Database::Database(const std::string& path) : dbPath(path) {}

Database::~Database() {
    close();
}

bool Database::open() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (db) {
        return true;
    }

    if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Failed to open database: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }
    return true;
}

void Database::close() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

std::string Database::loadSchema() {
    std::vector<std::filesystem::path> candidates = {
        "db/schema.sql",
        "../db/schema.sql",
        "tcp-server/db/schema.sql",
        "../tcp-server/db/schema.sql",
        std::filesystem::path(dbPath).parent_path() / "db/schema.sql",
        std::filesystem::current_path() / "tcp-server/db/schema.sql"
    };

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (file.good()) {
            return std::string(std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>());
        }
    }
    return {};
}

std::string Database::loadSeed() {
    std::vector<std::filesystem::path> candidates = {
        "db/seed.sql",
        "../db/seed.sql",
        "tcp-server/db/seed.sql",
        "../tcp-server/db/seed.sql",
        std::filesystem::path(dbPath).parent_path() / "db/seed.sql",
        std::filesystem::current_path() / "tcp-server/db/seed.sql"
    };

    for (const auto& candidate : candidates) {
        std::ifstream file(candidate);
        if (file.good()) {
            return std::string(std::istreambuf_iterator<char>(file),
                               std::istreambuf_iterator<char>());
        }
    }
    return {};
}

bool Database::initSchema() {
    std::string schema = loadSchema();
    if (schema.empty()) {
        std::cerr << "Could not load schema.sql" << std::endl;
        return false;
    }
    if (!execute(schema)) {
        return false;
    }

    std::string seed = loadSeed();
    if (seed.empty()) {
        std::cerr << "Could not load seed.sql" << std::endl;
        return false;
    }
    return execute(seed);
}

bool Database::execute(const std::string& sql) {
    char* errMsg = nullptr;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) {
        std::cerr << "Database not opened" << std::endl;
        return false;
    }

    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "SQLite error: " << (errMsg ? errMsg : "unknown") << std::endl;
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

std::optional<User> Database::getUserByUsername(const std::string& username) {
    const char* sql = R"(
        SELECT id, username, password, role, is_active
        FROM users WHERE username = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = mapUser(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

std::optional<User> Database::getUserById(int id) {
    const char* sql = R"(
        SELECT id, username, password, role, is_active
        FROM users WHERE id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int(stmt, 1, id);
    std::optional<User> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = mapUser(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Database::addUser(const User& user, int& newId) {
    const char* sql = R"(
        INSERT INTO users(username, password, role, is_active)
        VALUES(?,?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, user.role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, user.isActive ? 1 : 0);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    return true;
}

bool Database::updateLastLogin(int id) {
    const char* sql = R"(
        UPDATE users SET last_login = CURRENT_TIMESTAMP WHERE id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::addProduct(const Product& product, int& newId) {
    const char* sql = R"(
        INSERT INTO products(name, description, start_price, status, owner_user_id, image_url, category)
        VALUES(?,?,?,?,?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, product.name.c_str(), -1, SQLITE_TRANSIENT);
    bindOptional(stmt, 2, product.description);
    sqlite3_bind_int(stmt, 3, product.startPrice);
    sqlite3_bind_text(stmt, 4, product.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, product.ownerUserId);
    bindOptional(stmt, 6, product.imageUrl);
    bindOptional(stmt, 7, product.category);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert product failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    return true;
}

std::vector<Product> Database::getProducts(std::optional<int> ownerUserId) {
    const char* sqlOwned = R"(
        SELECT id, name, description, start_price, status, owner_user_id, created_at, updated_at, image_url, category
        FROM products
        WHERE owner_user_id = ?;
    )";
    const char* sqlAll = R"(
        SELECT id, name, description, start_price, status, owner_user_id, created_at, updated_at, image_url, category
        FROM products;
    )";
    std::vector<Product> products;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return products;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = ownerUserId.has_value() ? sqlOwned : sqlAll;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return products;
    }
    if (ownerUserId) sqlite3_bind_int(stmt, 1, *ownerUserId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        products.push_back(mapProduct(stmt));
    }
    sqlite3_finalize(stmt);
    return products;
}

std::optional<Product> Database::getProductById(int id) {
    const char* sql = R"(
        SELECT id, name, description, start_price, status, owner_user_id, created_at, updated_at, image_url, category
        FROM products WHERE id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int(stmt, 1, id);
    std::optional<Product> result;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = mapProduct(stmt);
    }
    sqlite3_finalize(stmt);
    return result;
}

bool Database::updateProduct(const Product& product) {
    const char* sql = R"(
        UPDATE products
        SET name = ?, description = ?, start_price = ?, status = ?, image_url = ?, category = ?, updated_at = CURRENT_TIMESTAMP
        WHERE id = ? AND owner_user_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, product.name.c_str(), -1, SQLITE_TRANSIENT);
    bindOptional(stmt, 2, product.description);
    sqlite3_bind_int(stmt, 3, product.startPrice);
    sqlite3_bind_text(stmt, 4, product.status.c_str(), -1, SQLITE_TRANSIENT);
    bindOptional(stmt, 5, product.imageUrl);
    bindOptional(stmt, 6, product.category);
    sqlite3_bind_int(stmt, 7, product.id);
    sqlite3_bind_int(stmt, 8, product.ownerUserId);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::deleteProduct(int id, int ownerUserId) {
    const char* sql = R"(
        DELETE FROM products WHERE id = ? AND owner_user_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, ownerUserId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::updateProductStatus(int id, const std::string& status, std::optional<int> ownerUserId) {
    const char* sql = ownerUserId.has_value()
        ? "UPDATE products SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND owner_user_id = ?;"
        : "UPDATE products SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    int idx = 3;
    if (ownerUserId) {
        sqlite3_bind_int(stmt, idx, *ownerUserId);
    }
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::addRoom(const Room& room, int& newId) {
    const char* sql = R"(
        INSERT INTO rooms(room_name, product_id, duration, status, host_user_id, base_price)
        VALUES(?,?,?,?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, room.roomName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, room.productId);
    sqlite3_bind_int(stmt, 3, room.duration);
    sqlite3_bind_text(stmt, 4, room.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, room.hostUserId);
    sqlite3_bind_int(stmt, 6, room.basePrice);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        sqlite3_finalize(stmt);
        return false;
    }
    newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    return true;
}

std::vector<Room> Database::getRoomsPublic() {
    const char* sql = R"(
        SELECT id, room_name, product_id, duration, status, host_user_id, created_at, updated_at, started_at, ended_at, base_price
        FROM rooms;
    )";
    std::vector<Room> rooms;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return rooms;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return rooms;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        rooms.push_back(mapRoom(stmt));
    }
    sqlite3_finalize(stmt);
    return rooms;
}

std::vector<Room> Database::getRoomsByOwner(int hostUserId) {
    const char* sql = R"(
        SELECT id, room_name, product_id, duration, status, host_user_id, created_at, updated_at, started_at, ended_at, base_price
        FROM rooms WHERE host_user_id = ?;
    )";
    std::vector<Room> rooms;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return rooms;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return rooms;
    }
    sqlite3_bind_int(stmt, 1, hostUserId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        rooms.push_back(mapRoom(stmt));
    }
    sqlite3_finalize(stmt);
    return rooms;
}

std::optional<Room> Database::getRoomById(int id) {
    const char* sql = R"(
        SELECT id, room_name, product_id, duration, status, host_user_id, created_at, updated_at, started_at, ended_at, base_price
        FROM rooms WHERE id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return std::nullopt;
    }
    sqlite3_bind_int(stmt, 1, id);
    std::optional<Room> room;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        room = mapRoom(stmt);
    }
    sqlite3_finalize(stmt);
    return room;
}

bool Database::deleteRoom(int id, int hostUserId) {
    const char* sql = R"(
        DELETE FROM rooms WHERE id = ? AND host_user_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, hostUserId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::updateRoomStatus(int id, int hostUserId, const std::string& status, std::optional<std::string> startedAt, std::optional<std::string> endedAt) {
    const char* sql = R"(
        UPDATE rooms
        SET status = ?, started_at = COALESCE(?, started_at), ended_at = COALESCE(?, ended_at), updated_at = CURRENT_TIMESTAMP
        WHERE id = ? AND host_user_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    bindOptional(stmt, 2, startedAt);
    bindOptional(stmt, 3, endedAt);
    sqlite3_bind_int(stmt, 4, id);
    sqlite3_bind_int(stmt, 5, hostUserId);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::recordBid(int roomId, int userId, int amount) {
    const char* sql = R"(
        INSERT INTO bids(room_id, user_id, amount) VALUES(?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, roomId);
    sqlite3_bind_int(stmt, 2, userId);
    sqlite3_bind_int(stmt, 3, amount);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

int Database::getHighestBid(int roomId) {
    const char* sql = R"(
        SELECT COALESCE(MAX(amount), 0) FROM bids WHERE room_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return 0;
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        return 0;
    }
    sqlite3_bind_int(stmt, 1, roomId);
    int result = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        result = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return result;
}

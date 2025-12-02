#include "sqlite.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>

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
    u.isActive = sqlite3_column_int(stmt, 3) != 0;
    if (const unsigned char* t = sqlite3_column_text(stmt, 4)) u.role = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 5)) u.createdAt = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 6)) u.updatedAt = reinterpret_cast<const char*>(t);
    if (const unsigned char* t = sqlite3_column_text(stmt, 7)) u.lastLogin = reinterpret_cast<const char*>(t);
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

SQLiteDb::SQLiteDb(const std::string& path) : dbPath(path) {}

SQLiteDb::~SQLiteDb() {
    close();
}

bool SQLiteDb::open() {
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

void SQLiteDb::close() {
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
}

std::string SQLiteDb::loadSchema() {
    std::vector<std::filesystem::path> candidates = {
        "db/schema.sql",
        "../db/schema.sql",
        std::filesystem::path(dbPath).parent_path() / "db/schema.sql"
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

std::string SQLiteDb::loadSeed() {
    std::vector<std::filesystem::path> candidates = {
        "db/seed.sql",
        "../db/seed.sql",
        std::filesystem::path(dbPath).parent_path() / "db/seed.sql"
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

bool SQLiteDb::initSchema() {
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

bool SQLiteDb::execute(const std::string& sql) {
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

bool SQLiteDb::addProduct(const Product& product, int& newId) {
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

std::vector<Product> SQLiteDb::getProducts(int ownerUserId) {
    const char* sql = R"(
        SELECT id, name, description, start_price, status, owner_user_id, created_at, updated_at, image_url, category
        FROM products
        WHERE owner_user_id = ?;
    )";
    std::vector<Product> products;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return products;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return products;
    }

    sqlite3_bind_int(stmt, 1, ownerUserId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        products.push_back(mapProduct(stmt));
    }
    sqlite3_finalize(stmt);
    return products;
}

std::vector<Product> SQLiteDb::getProductsPublic() {
    const char* sql = R"(
        SELECT id, name, description, start_price, status, owner_user_id, created_at, updated_at, image_url, category
        FROM products;
    )";
    std::vector<Product> products;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return products;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return products;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        products.push_back(mapProduct(stmt));
    }
    sqlite3_finalize(stmt);
    return products;
}

std::optional<Product> SQLiteDb::getProductById(int id) {
    const char* sql = R"(
        SELECT id, name, description, start_price, status, owner_user_id, created_at, updated_at, image_url, category
        FROM products WHERE id = ? LIMIT 1;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);
    std::optional<Product> product;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        product = mapProduct(stmt);
    }
    sqlite3_finalize(stmt);
    return product;
}

std::optional<Product> SQLiteDb::getProductByIdForOwner(int id, int ownerUserId) {
    const char* sql = R"(
        SELECT id, name, description, start_price, status, owner_user_id, created_at, updated_at, image_url, category
        FROM products WHERE id = ? AND owner_user_id = ? LIMIT 1;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, ownerUserId);
    std::optional<Product> product;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        product = mapProduct(stmt);
    }
    sqlite3_finalize(stmt);
    return product;
}

bool SQLiteDb::updateProduct(const Product& product) {
    const char* sql = R"(
        UPDATE products
        SET name = ?, description = ?, start_price = ?, status = ?, image_url = ?, category = ?, updated_at = CURRENT_TIMESTAMP
        WHERE id = ? AND owner_user_id = ?;
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
    bindOptional(stmt, 5, product.imageUrl);
    bindOptional(stmt, 6, product.category);
    sqlite3_bind_int(stmt, 7, product.id);
    sqlite3_bind_int(stmt, 8, product.ownerUserId);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Update product failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return sqlite3_changes(db) > 0;
}

bool SQLiteDb::deleteProduct(int id, int ownerUserId) {
    const char* sql = R"(
        DELETE FROM products WHERE id = ? AND owner_user_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, ownerUserId);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Delete product failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return sqlite3_changes(db) > 0;
}

bool SQLiteDb::updateProductStatus(int id, const std::string& status, std::optional<int> ownerUserId) {
    const char* sqlOwner = R"(
        UPDATE products SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ? AND owner_user_id = ?;
    )";
    const char* sqlAny = R"(
        UPDATE products SET status = ?, updated_at = CURRENT_TIMESTAMP WHERE id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = ownerUserId.has_value() ? sqlOwner : sqlAny;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, id);
    if (ownerUserId.has_value()) {
        sqlite3_bind_int(stmt, 3, *ownerUserId);
    }

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Update product status failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return sqlite3_changes(db) > 0;
}

bool SQLiteDb::addRoom(const Room& room, int& newId) {
    const char* sql = R"(
        INSERT INTO rooms(room_name, product_id, duration, status, host_user_id, started_at, ended_at, base_price)
        VALUES(?,?,?,?,?,?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, room.roomName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, room.productId);
    sqlite3_bind_int(stmt, 3, room.duration);
    sqlite3_bind_text(stmt, 4, room.status.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, room.hostUserId);
    bindOptional(stmt, 6, room.startedAt);
    bindOptional(stmt, 7, room.endedAt);
    if (room.basePrice > 0) {
        sqlite3_bind_int(stmt, 8, room.basePrice);
    } else {
        sqlite3_bind_null(stmt, 8);
    }

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert room failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    return true;
}

std::vector<Room> SQLiteDb::getRooms(int hostUserId) {
    const char* sql = R"(
        SELECT id, room_name, product_id, duration, status, host_user_id, created_at, updated_at, started_at, ended_at, base_price
        FROM rooms WHERE host_user_id = ?;
    )";
    std::vector<Room> rooms;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return rooms;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return rooms;
    }

    sqlite3_bind_int(stmt, 1, hostUserId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        rooms.push_back(mapRoom(stmt));
    }
    sqlite3_finalize(stmt);
    return rooms;
}

std::vector<Room> SQLiteDb::getRoomsPublic() {
    const char* sql = R"(
        SELECT id, room_name, product_id, duration, status, host_user_id, created_at, updated_at, started_at, ended_at, base_price
        FROM rooms;
    )";
    std::vector<Room> rooms;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return rooms;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return rooms;
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        rooms.push_back(mapRoom(stmt));
    }
    sqlite3_finalize(stmt);
    return rooms;
}

std::optional<Room> SQLiteDb::getRoomByIdForUser(int id, int hostUserId) {
    const char* sql = R"(
        SELECT id, room_name, product_id, duration, status, host_user_id, created_at, updated_at, started_at, ended_at, base_price
        FROM rooms WHERE id = ? AND host_user_id = ? LIMIT 1;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, hostUserId);

    std::optional<Room> room;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        room = mapRoom(stmt);
    }
    sqlite3_finalize(stmt);
    return room;
}

std::optional<Room> SQLiteDb::getRoomById(int id) {
    const char* sql = R"(
        SELECT id, room_name, product_id, duration, status, host_user_id, created_at, updated_at, started_at, ended_at, base_price
        FROM rooms WHERE id = ? LIMIT 1;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
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

bool SQLiteDb::deleteRoom(int id, int hostUserId) {
    const char* sql = R"(
        DELETE FROM rooms WHERE id = ? AND host_user_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_bind_int(stmt, 2, hostUserId);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Delete room failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return sqlite3_changes(db) > 0;
}

bool SQLiteDb::updateRoomStatus(int id, int hostUserId, const std::string& status, std::optional<std::string> startedAt, std::optional<std::string> endedAt) {
    const char* sql = R"(
        UPDATE rooms
        SET status = ?, started_at = ?, ended_at = ?, updated_at = CURRENT_TIMESTAMP
        WHERE id = ? AND host_user_id = ?;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, status.c_str(), -1, SQLITE_TRANSIENT);
    if (startedAt.has_value()) {
        sqlite3_bind_text(stmt, 2, startedAt->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 2);
    }
    if (endedAt.has_value()) {
        sqlite3_bind_text(stmt, 3, endedAt->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }
    sqlite3_bind_int(stmt, 4, id);
    sqlite3_bind_int(stmt, 5, hostUserId);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Update room status failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return sqlite3_changes(db) > 0;
}

bool SQLiteDb::addUser(const User& user, int& newId) {
    const char* sql = R"(
        INSERT INTO users(username, password, is_active, role, last_login)
        VALUES(?,?,?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, user.username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user.password.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 3, user.isActive ? 1 : 0);
    sqlite3_bind_text(stmt, 4, user.role.c_str(), -1, SQLITE_TRANSIENT);
    if (user.lastLogin.has_value()) {
        sqlite3_bind_text(stmt, 5, user.lastLogin->c_str(), -1, SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 5);
    }

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert user failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    newId = static_cast<int>(sqlite3_last_insert_rowid(db));
    sqlite3_finalize(stmt);
    return true;
}

std::optional<User> SQLiteDb::getUserByUsername(const std::string& username) {
    const char* sql = R"(
        SELECT id, username, password, is_active, role, created_at, updated_at, last_login
        FROM users WHERE username = ? LIMIT 1;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    std::optional<User> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = mapUser(stmt);
    }
    sqlite3_finalize(stmt);
    return user;
}

std::optional<User> SQLiteDb::getUserById(int id) {
    const char* sql = R"(
        SELECT id, username, password, is_active, role, created_at, updated_at, last_login
        FROM users WHERE id = ? LIMIT 1;
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return std::nullopt;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, id);

    std::optional<User> user;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        user = mapUser(stmt);
    }
    sqlite3_finalize(stmt);
    return user;
}

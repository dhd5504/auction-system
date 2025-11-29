#include "sqlite.h"

#include <filesystem>
#include <fstream>
#include <iostream>

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

bool SQLiteDb::addProduct(const Product& product) {
    const char* sql = R"(
        INSERT INTO products(id, name, start_price, buy_price, step, description)
        VALUES(?,?,?,?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, product.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, product.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, product.startPrice);
    sqlite3_bind_double(stmt, 4, product.buyPrice);
    sqlite3_bind_double(stmt, 5, product.step);
    sqlite3_bind_text(stmt, 6, product.description.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert product failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

std::vector<Product> SQLiteDb::getProducts() {
    const char* sql = R"(
        SELECT id, name, start_price, buy_price, step, description FROM products;
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
        Product p;
        const unsigned char* idText = sqlite3_column_text(stmt, 0);
        if (idText) p.id = reinterpret_cast<const char*>(idText);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        p.startPrice = sqlite3_column_double(stmt, 2);
        p.buyPrice = sqlite3_column_double(stmt, 3);
        p.step = sqlite3_column_double(stmt, 4);
        const unsigned char* desc = sqlite3_column_text(stmt, 5);
        if (desc) p.description = reinterpret_cast<const char*>(desc);
        products.push_back(std::move(p));
    }
    sqlite3_finalize(stmt);
    return products;
}

bool SQLiteDb::addRoom(const Room& room) {
    const char* sql = R"(
        INSERT INTO rooms(id, name, start_time)
        VALUES(?,?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, room.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, room.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, room.startTime.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert room failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

std::vector<Room> SQLiteDb::getRooms() {
    const char* sql = R"(
        SELECT id, name, start_time FROM rooms;
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
        Room r;
        const unsigned char* idText = sqlite3_column_text(stmt, 0);
        if (idText) r.id = reinterpret_cast<const char*>(idText);
        r.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        r.startTime = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.productIds = getRoomProducts(r.id);
        rooms.push_back(std::move(r));
    }
    sqlite3_finalize(stmt);
    return rooms;
}

bool SQLiteDb::linkRoomProduct(const std::string& roomId, const std::string& productId) {
    const char* sql = R"(
        INSERT INTO room_products(room_id, product_id) VALUES(?,?);
    )";
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return false;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return false;
    }

    sqlite3_bind_text(stmt, 1, roomId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, productId.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert room_product failed: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_finalize(stmt);
        return false;
    }
    sqlite3_finalize(stmt);
    return true;
}

std::vector<std::string> SQLiteDb::getRoomProducts(const std::string& roomId) {
    const char* sql = R"(
        SELECT product_id FROM room_products WHERE room_id = ?;
    )";
    std::vector<std::string> ids;
    std::lock_guard<std::recursive_mutex> lock(mutex);
    if (!db) return ids;

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << std::endl;
        return ids;
    }

    sqlite3_bind_text(stmt, 1, roomId.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* pid = sqlite3_column_text(stmt, 0);
        if (pid) ids.emplace_back(reinterpret_cast<const char*>(pid));
    }
    sqlite3_finalize(stmt);
    return ids;
}

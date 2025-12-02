#include <filesystem>
#include <iostream>
#include <thread>

#include "database.h"
#include "session_manager.h"
#include "tcp_server.h"

int main() {
    std::filesystem::create_directories("data");
    Database db("data/data.db");
    if (!db.open()) {
        std::cerr << "Unable to open SQLite database" << std::endl;
        return 1;
    }
    if (!db.initSchema()) {
        std::cerr << "Unable to initialize SQLite schema" << std::endl;
        return 1;
    }

    SessionManager sessions;
    TcpServer server(9000, db, sessions);
    if (!server.start()) {
        std::cerr << "Failed to start TCP server" << std::endl;
        return 1;
    }

    std::cout << "TCP server ready. Press ENTER to exit." << std::endl;
    std::string line;
    std::getline(std::cin, line);

    server.stop();
    db.close();
    return 0;
}

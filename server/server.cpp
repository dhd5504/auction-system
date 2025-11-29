#include <iostream>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(9000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(server_fd, (sockaddr*)&addr, sizeof(addr));
    listen(server_fd, 3);

    std::cout << "TCP server running on 9000...\n";

    while (true) {
        int client_fd = accept(server_fd, nullptr, nullptr);

        char buffer[1024];
        int len = recv(client_fd, buffer, sizeof(buffer), 0);
        buffer[len] = '\0';

        std::cout << "Received: " << buffer;

        std::string response = "OK speed=5\n";
        send(client_fd, response.c_str(), response.size(), 0);

        close(client_fd);
    }
}

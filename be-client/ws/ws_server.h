#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include <pistache/listener.h>
#include <pistache/peer.h>
#include <pistache/tcp.h>

class WsServer {
public:
    explicit WsServer(uint16_t port = 8081);
    ~WsServer();

    void start();
    void stop();

    void broadcast(const std::string& message);

private:
    class WsHandler;
    friend class WsHandler;

    struct ClientState {
        std::shared_ptr<Pistache::Tcp::Peer> peer;
        bool handshaken{false};
        std::string buffer;
    };

    void handleHandshake(const std::shared_ptr<Pistache::Tcp::Peer>& peer, ClientState& state);
    void handleFrames(const std::shared_ptr<Pistache::Tcp::Peer>& peer, ClientState& state);
    void sendTextFrame(const std::shared_ptr<Pistache::Tcp::Peer>& peer, const std::string& message);
    void sendPong(const std::shared_ptr<Pistache::Tcp::Peer>& peer, const std::string& payload);

    uint16_t port;
    Pistache::Tcp::Listener listener;
    std::shared_ptr<WsHandler> handler;
    std::atomic<bool> running{false};

    std::mutex clientsMutex;
    std::unordered_map<size_t, ClientState> clients;
};

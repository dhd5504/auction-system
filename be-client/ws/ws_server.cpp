#include "ws_server.h"

#include <pistache/base64.h>
#include <pistache/stream.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

namespace {

uint32_t rotl(uint32_t value, size_t bits) {
    return (value << bits) | (value >> (32 - bits));
}

std::array<uint8_t, 20> sha1Bytes(const std::string& input) {
    uint32_t h0 = 0x67452301;
    uint32_t h1 = 0xEFCDAB89;
    uint32_t h2 = 0x98BADCFE;
    uint32_t h3 = 0x10325476;
    uint32_t h4 = 0xC3D2E1F0;

    std::vector<uint8_t> data(input.begin(), input.end());
    uint64_t bitLen = static_cast<uint64_t>(data.size()) * 8;

    // append the bit '1' to the message
    data.push_back(0x80);
    // append 0 <= k < 512 bits '0', pad to 56 mod 64 bytes
    while ((data.size() % 64) != 56) {
        data.push_back(0x00);
    }

    // append length as 64-bit big-endian integer
    for (int i = 7; i >= 0; --i) {
        data.push_back(static_cast<uint8_t>((bitLen >> (i * 8)) & 0xFF));
    }

    for (size_t chunk = 0; chunk < data.size(); chunk += 64) {
        uint32_t w[80];
        std::memset(w, 0, sizeof(w));

        for (int i = 0; i < 16; ++i) {
            size_t idx = chunk + i * 4;
            w[i] = (data[idx] << 24) | (data[idx + 1] << 16) |
                   (data[idx + 2] << 8) | (data[idx + 3]);
        }

        for (int i = 16; i < 80; ++i) {
            w[i] = rotl(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = h0;
        uint32_t b = h1;
        uint32_t c = h2;
        uint32_t d = h3;
        uint32_t e = h4;

        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = rotl(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rotl(b, 30);
            b = a;
            a = temp;
        }

        h0 += a;
        h1 += b;
        h2 += c;
        h3 += d;
        h4 += e;
    }

    std::array<uint8_t, 20> digest{
        static_cast<uint8_t>((h0 >> 24) & 0xFF), static_cast<uint8_t>((h0 >> 16) & 0xFF),
        static_cast<uint8_t>((h0 >> 8) & 0xFF),  static_cast<uint8_t>(h0 & 0xFF),
        static_cast<uint8_t>((h1 >> 24) & 0xFF), static_cast<uint8_t>((h1 >> 16) & 0xFF),
        static_cast<uint8_t>((h1 >> 8) & 0xFF),  static_cast<uint8_t>(h1 & 0xFF),
        static_cast<uint8_t>((h2 >> 24) & 0xFF), static_cast<uint8_t>((h2 >> 16) & 0xFF),
        static_cast<uint8_t>((h2 >> 8) & 0xFF),  static_cast<uint8_t>(h2 & 0xFF),
        static_cast<uint8_t>((h3 >> 24) & 0xFF), static_cast<uint8_t>((h3 >> 16) & 0xFF),
        static_cast<uint8_t>((h3 >> 8) & 0xFF),  static_cast<uint8_t>(h3 & 0xFF),
        static_cast<uint8_t>((h4 >> 24) & 0xFF), static_cast<uint8_t>((h4 >> 16) & 0xFF),
        static_cast<uint8_t>((h4 >> 8) & 0xFF),  static_cast<uint8_t>(h4 & 0xFF)};
    return digest;
}

std::string trim(const std::string& input) {
    auto begin = input.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return {};
    auto end = input.find_last_not_of(" \t\r\n");
    return input.substr(begin, end - begin + 1);
}

} // namespace

class WsServer::WsHandler : public Pistache::Tcp::Handler {
public:
    explicit WsHandler(WsServer* server) : server_(server) {}

    std::shared_ptr<Pistache::Tcp::Handler> clone() const override {
        return std::make_shared<WsHandler>(server_);
    }

    void onInput(const char* buffer, size_t len, const std::shared_ptr<Pistache::Tcp::Peer>& peer) override {
        std::lock_guard<std::mutex> lock(server_->clientsMutex);
        auto id = peer->getID();
        auto it = server_->clients.find(id);
        if (it == server_->clients.end()) {
            WsServer::ClientState state;
            state.peer = peer;
            it = server_->clients.emplace(id, std::move(state)).first;
        }
        auto& state = it->second;
        state.buffer.append(buffer, len);

        if (!state.handshaken) {
            server_->handleHandshake(peer, state);
        }
        if (state.handshaken) {
            server_->handleFrames(peer, state);
        }
    }

    void onConnection(const std::shared_ptr<Pistache::Tcp::Peer>& peer) override {
        std::lock_guard<std::mutex> lock(server_->clientsMutex);
        WsServer::ClientState state;
        state.peer = peer;
        server_->clients.emplace(peer->getID(), std::move(state));
    }

    void onDisconnection(const std::shared_ptr<Pistache::Tcp::Peer>& peer) override {
        std::lock_guard<std::mutex> lock(server_->clientsMutex);
        server_->clients.erase(peer->getID());
    }

private:
    WsServer* server_;
};

WsServer::WsServer(uint16_t port_)
    : port(port_), listener(Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(port_))) {}

WsServer::~WsServer() {
    stop();
}

void WsServer::start() {
    if (running.exchange(true)) {
        return;
    }
    handler = std::make_shared<WsHandler>(this);
    auto opts = Pistache::Flags<Pistache::Tcp::Options>(Pistache::Tcp::Options::ReuseAddr) |
                Pistache::Tcp::Options::ReusePort;
    listener.init(1, opts);
    listener.setHandler(handler);
    listener.bind();
    listener.runThreaded();
}

void WsServer::stop() {
    if (!running.exchange(false)) {
        return;
    }
    listener.shutdown();
    std::lock_guard<std::mutex> lock(clientsMutex);
    clients.clear();
}

void WsServer::broadcast(const std::string& message) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto it = clients.begin(); it != clients.end();) {
        auto& state = it->second;
        if (!state.handshaken || !state.peer) {
            it = clients.erase(it);
            continue;
        }
        sendTextFrame(state.peer, message);
        ++it;
    }
}

void WsServer::handleHandshake(const std::shared_ptr<Pistache::Tcp::Peer>& peer, ClientState& state) {
    const std::string endSeq = "\r\n\r\n";
    auto headerEnd = state.buffer.find(endSeq);
    if (headerEnd == std::string::npos) {
        return;
    }

    std::string request = state.buffer.substr(0, headerEnd);
    state.buffer.erase(0, headerEnd + endSeq.size());

    if (request.find("GET /ws") != 0) {
        const std::string bad = "HTTP/1.1 400 Bad Request\r\n\r\n";
        peer->send(Pistache::RawBuffer(bad.c_str(), bad.size()));
        return;
    }

    auto keyPos = request.find("Sec-WebSocket-Key:");
    if (keyPos == std::string::npos) {
        const std::string bad = "HTTP/1.1 400 Bad Request\r\n\r\n";
        peer->send(Pistache::RawBuffer(bad.c_str(), bad.size()));
        return;
    }

    auto keyLineEnd = request.find("\r\n", keyPos);
    std::string keyLine = request.substr(keyPos + std::strlen("Sec-WebSocket-Key:"),
                                        keyLineEnd == std::string::npos ? std::string::npos : keyLineEnd - (keyPos + std::strlen("Sec-WebSocket-Key:")));
    std::string clientKey = trim(keyLine);
    const std::string guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
    auto digest = sha1Bytes(clientKey + guid);
    std::vector<std::byte> digestBytes;
    digestBytes.reserve(digest.size());
    for (auto b : digest) {
        digestBytes.push_back(static_cast<std::byte>(b));
    }
    Base64Encoder encoder(digestBytes);
    const std::string& acceptKey = encoder.Encode();

    std::ostringstream resp;
    resp << "HTTP/1.1 101 Switching Protocols\r\n"
         << "Upgrade: websocket\r\n"
         << "Connection: Upgrade\r\n"
         << "Sec-WebSocket-Accept: " << acceptKey << "\r\n\r\n";
    auto responseStr = resp.str();
    peer->send(Pistache::RawBuffer(responseStr.c_str(), responseStr.size()));
    state.handshaken = true;
}

void WsServer::sendTextFrame(const std::shared_ptr<Pistache::Tcp::Peer>& peer, const std::string& message) {
    std::vector<uint8_t> frame;
    frame.push_back(0x81); // FIN + text frame
    size_t len = message.size();
    if (len <= 125) {
        frame.push_back(static_cast<uint8_t>(len));
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    frame.insert(frame.end(), message.begin(), message.end());

    Pistache::RawBuffer buffer(reinterpret_cast<const char*>(frame.data()), frame.size());
    peer->send(buffer);
}

void WsServer::sendPong(const std::shared_ptr<Pistache::Tcp::Peer>& peer, const std::string& payload) {
    std::vector<uint8_t> frame;
    frame.push_back(0x8A); // FIN + pong
    size_t len = payload.size();
    if (len <= 125) {
        frame.push_back(static_cast<uint8_t>(len));
    } else if (len <= 65535) {
        frame.push_back(126);
        frame.push_back((len >> 8) & 0xFF);
        frame.push_back(len & 0xFF);
    } else {
        frame.push_back(127);
        for (int i = 7; i >= 0; --i) {
            frame.push_back((len >> (i * 8)) & 0xFF);
        }
    }
    frame.insert(frame.end(), payload.begin(), payload.end());

    Pistache::RawBuffer buffer(reinterpret_cast<const char*>(frame.data()), frame.size());
    peer->send(buffer);
}

void WsServer::handleFrames(const std::shared_ptr<Pistache::Tcp::Peer>& peer, ClientState& state) {
    while (state.buffer.size() >= 2) {
        uint8_t b1 = static_cast<uint8_t>(state.buffer[0]);
        uint8_t b2 = static_cast<uint8_t>(state.buffer[1]);

        bool fin = b1 & 0x80;
        uint8_t opcode = b1 & 0x0F;
        bool masked = b2 & 0x80;
        uint64_t payloadLen = b2 & 0x7F;
        size_t offset = 2;

        if (payloadLen == 126) {
            if (state.buffer.size() < offset + 2) return;
            payloadLen = (static_cast<uint8_t>(state.buffer[offset]) << 8) |
                         static_cast<uint8_t>(state.buffer[offset + 1]);
            offset += 2;
        } else if (payloadLen == 127) {
            if (state.buffer.size() < offset + 8) return;
            payloadLen = 0;
            for (int i = 0; i < 8; ++i) {
                payloadLen = (payloadLen << 8) | static_cast<uint8_t>(state.buffer[offset + i]);
            }
            offset += 8;
        }

        uint8_t mask[4] = {0, 0, 0, 0};
        if (masked) {
            if (state.buffer.size() < offset + 4) return;
            for (int i = 0; i < 4; ++i) {
                mask[i] = static_cast<uint8_t>(state.buffer[offset + i]);
            }
            offset += 4;
        }

        if (state.buffer.size() < offset + payloadLen) {
            return;
        }

        std::string payload = state.buffer.substr(offset, payloadLen);
        if (masked) {
            for (size_t i = 0; i < payload.size(); ++i) {
                payload[i] = static_cast<char>(payload[i] ^ mask[i % 4]);
            }
        }

        state.buffer.erase(0, offset + payloadLen);

        if (!fin) {
            continue; // Fragmented messages are ignored in this minimal server
        }

        if (opcode == 0x9) { // ping
            sendPong(peer, payload);
        } else if (opcode == 0x8) { // close
            break;
        }
    }
}

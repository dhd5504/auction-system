# Auction System – Binary Gateway + TCP Server

This repository now uses a binary protocol between the HTTP **gateway** and the **tcp-server**. The React FE stays unchanged and still speaks JSON over REST + WebSocket; the gateway converts JSON ⇄ binary packets and forwards everything to the tcp-server, which owns all business logic and SQLite data.

## Architecture
- **fe-client**: unchanged React UI (REST + WS).
- **gateway**: Pistache REST server + WebSocket relay. Builds `PacketHeader + Payload` using `protocol/Protocol.h`, opens a TCP connection, forwards the packet, parses the binary response, and returns JSON. No database, no business logic.
- **tcp-server**: pure binary server with SQLite. Reads `PacketHeader`, routes by opcode, executes handlers, and returns binary payloads. Handles auctions, bids, buy-now, room/product CRUD, and emits notify/timer/item packets.

### Binary protocol
Header (14 bytes, packed):
```cpp
struct PacketHeader {
    uint16_t opcode;
    uint32_t length;
    uint32_t sessionId;
    uint16_t timestamp;
    uint16_t reserved;
};
```
Payload = raw bytes of one struct from `protocol/Protocol.h`.

Opcodes (mandatory):
`LOGIN_REQ/RES`, `JOIN_ROOM_REQ/RES`, `BID_REQ/RES`, `BUY_NOW_REQ/RES`, `NOTIFY_MESSAGE`, `TIMER_TICK`, `ITEM_START`, `ITEM_END`. Extra routing for product/room CRUD is multiplexed via `NotifyCode` inside `NOTIFY_MESSAGE`.

## Folder layout
- `protocol/Protocol.h` – shared header + payload structs/opcodes.
- `gateway/` – REST routes, Packet builder/parser, TCP client, WebSocket relay.
- `tcp-server/` – packet reader/router, SQLite handlers, session manager.
- `fe-client/` – unchanged React app.

## Build & run
### Docker Compose
```bash
docker-compose up --build
# Gateway: http://localhost:8080
# WebSocket: ws://localhost:8081/ws
# TCP server: localhost:9000
```

### Local build
Gateway:
```bash
cmake -S gateway -B build/gateway
cmake --build build/gateway
./build/gateway/gateway
```

TCP server:
```bash
cmake -S tcp-server -B build/tcp-server
cmake --build build/tcp-server
./build/tcp-server/tcp_server
```

## API quickstart (end-to-end)
- **Login** → binary `LOGIN_REQ/RES`:
  ```bash
  curl -X POST http://localhost:8080/api/login \
    -H "Content-Type: application/json" \
    -d '{"username":"admin","password":"admin"}'
  # returns {token: <sessionId>, user:{...}}
  ```
- **Products** (proxy via `NOTIFY_MESSAGE`):
  - List public: `GET /api/products`
  - List own: `GET /api/me/products` (Bearer <sessionId>)
  - Create: `POST /api/me/products`
- **Rooms**:
  - List public: `GET /api/rooms` (uses binary `JOIN_ROOM_REQ/RES`)
  - Create room: `POST /api/me/rooms` (Bearer <sessionId>)
  - Bid: `POST /api/rooms/:id/bid` with body `{"amount":123}`
  - Buy-now: `POST /api/rooms/:id/buy` with body `{"price":999}`

WebSocket broadcasts (`ws://localhost:8081/ws`):
- `{"type":"room_started","roomId":...}`
- `{"type":"room_cancelled","roomId":...}`
- `{"type":"bid","roomId":...,"amount":...}`
- `{"type":"buy_now","roomId":...,"finalPrice":...}`

## Notes
- Gateway holds no state besides forwarding the `sessionId` it receives from `LOGIN_RES`.
- TCP server owns SQLite (schema + seed in `tcp-server/db`), including bids and room/product lifecycle.
- All packets are little-to-big endian converted on the wire; header size is fixed at 14 bytes.

# NetworkApp (Qt client/server demo)

Ứng dụng mẫu client–server dùng Qt, giao thức header + JSON, SQLite lưu user.

## Cấu trúc
- `client/`: Qt Widgets UI, TcpClient gói/giải gói CMD/REQ/LEN + JSON.
- `server/`: QtCore/TcpServer + SQLite, CommandHandler trả LOGIN_OK/FAIL, REGISTER_OK/FAIL, PING/PONG.
- `common/protocol.txt`: mô tả ngắn format.

## Yêu cầu
- Qt5/Qt6 có modules Widgets, Network, Sql (SQLite driver).
- CMake, compiler C++17.

## Build & run
### Server
```bash
cmake -S server -B server/build
cmake --build server/build
cd server && ./build/server_app
```
- Lắng nghe `127.0.0.1:5555`, tạo `users.db` cạnh binary, nạp schema `server/db/schema.sql`.

### Client
```bash
cmake -S client -B client/build
cmake --build client/build
cd client && ./build/client
```
- Mặc định kết nối `127.0.0.1:5555`. Đổi host/port trong `client/src/mainwindow.h`.

## Giao thức tóm tắt
- Frame = header + JSON (UTF-8).
- Header: `CMD=<COMMAND>;REQ=<id>;LEN=<bytes>\n` (REQ=0 cho server push).
- Payload: JSON, LEN là số byte.
- LOGIN: client gửi username/password; server trả `LOGIN_OK` với token hoặc `LOGIN_FAIL`.
- REGISTER: client gửi username/password/fullName/phone; server trả `REGISTER_OK/FAIL`.
- PING: echo PONG với field message.

## Logging
- Client/server in console: `[CLIENT->SERVER] ...`, `[SERVER->CLIENT] ...` để theo dõi gói.

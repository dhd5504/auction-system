# 🚀 **Auction System – Realtime TCP + REST + WebSocket (React + C++)**

Dự án đấu giá realtime gồm 3 phần chính:

* **Frontend (React)** — Gửi REST API & nhận WebSocket realtime
* **Backend Client (C++ Pistache)** — Trung tâm xử lý REST ↔ TCP ↔ SQLite ↔ WebSocket
* **TCP Server (C++)** — Xử lý lệnh đấu giá dạng text (PRODUCT_ADD, ROOM_CREATE, START_ROOM, BID...)

Kiến trúc tuân thủ 100% yêu cầu:

```
React FE
   ↓ REST JSON
Backend Client (C++)
   ↓ TCP text commands
TCP Server (C++)
   ↓ response text
Backend Client
   ↓ SQLite save
   ↓ WebSocket broadcast
React FE realtime
```

---

# 🔧 **1. Backend Client (C++)**

### Công nghệ:

* **Pistache** → REST API + WebSocket
* **SQLite3** → database
* **POSIX sockets** → TCP client
* **nlohmann/json** → JSON parse

### Chạy BE Client:

```
cd be-client
mkdir build
cd build
cmake ..
make
./be_client
```

Backend chạy:

* REST: **[http://localhost:8080](http://localhost:8080)**
* WebSocket: **ws://localhost:8081/ws**

---

# 🔌 **3. TCP Server (C++)**

TCP Server xử lý text command:

Chạy:

```
cd server
make
./server
```

TCP server chạy tại:

```
localhost:9000
```

---

# 🖥 **3. Frontend (React)**

FE gọi API BE Client + nhận realtime:

### WebSocket:

```
const ws = new WebSocket("ws://localhost:8081/ws")
```

### Chạy FE

```
cd fe-client
npm install
npm start
```

Trang FE:

```
http://localhost:3000
```

---

# 📡 **4. WebSocket Events**

BE Client broadcast:

* `product_created:{...}`
* `room_created:{...}`
* (sắp tới: `price_update`, `winner`, `countdown`, …)

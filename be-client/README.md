# 🚀 Build Project

```bash
cd be-client
mkdir build
cd build
cmake ..
make
````

---

# ▶️ File Chạy Sau Khi Build

```
build/be_client
```

Chạy server:

```bash
./be_client
```

---

# 💾 Làm Việc Với SQLite

## 📌 1. Mở Database

```bash
cd build
sqlite3 data.db
```

## 📌 2. Xem Danh Sách Bảng

```sql
.tables;
```

## 📌 3. Xem Dữ Liệu Trong Bảng `users`

```sql
SELECT * FROM users;
```

## 📌 4. Thoát SQLite

```sql
.quit
```
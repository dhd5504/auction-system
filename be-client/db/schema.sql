CREATE TABLE IF NOT EXISTS products(
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    start_price REAL NOT NULL,
    buy_price REAL NOT NULL,
    step REAL NOT NULL,
    description TEXT
);

CREATE TABLE IF NOT EXISTS rooms(
    id TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    start_time TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS room_products(
    room_id TEXT NOT NULL,
    product_id TEXT NOT NULL,
    FOREIGN KEY(room_id) REFERENCES rooms(id),
    FOREIGN KEY(product_id) REFERENCES products(id)
);

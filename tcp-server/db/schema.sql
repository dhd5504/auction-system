CREATE TABLE IF NOT EXISTS users(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT UNIQUE NOT NULL,
    password TEXT NOT NULL,
    role TEXT DEFAULT 'user',
    is_active INTEGER DEFAULT 1,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    last_login DATETIME
);

CREATE TABLE IF NOT EXISTS products(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    description TEXT,
    start_price INTEGER NOT NULL,
    status TEXT DEFAULT 'available',
    owner_user_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    image_url TEXT,
    category TEXT,
    FOREIGN KEY(owner_user_id) REFERENCES users(id)
);

CREATE TABLE IF NOT EXISTS rooms(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_name TEXT NOT NULL,
    product_id INTEGER NOT NULL,
    duration INTEGER NOT NULL,
    status TEXT DEFAULT 'waiting',
    host_user_id INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    started_at DATETIME,
    ended_at DATETIME,
    base_price INTEGER,
    FOREIGN KEY(product_id) REFERENCES products(id),
    FOREIGN KEY(host_user_id) REFERENCES users(id)
);

CREATE TABLE IF NOT EXISTS bids(
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    room_id INTEGER NOT NULL,
    user_id INTEGER NOT NULL,
    amount INTEGER NOT NULL,
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY(room_id) REFERENCES rooms(id),
    FOREIGN KEY(user_id) REFERENCES users(id)
);

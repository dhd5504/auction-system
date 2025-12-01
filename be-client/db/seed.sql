-- Users (passwords stored as plain text placeholders, replace with hashes in production)
INSERT OR IGNORE INTO users(id, username, password, is_active, role) VALUES
  (1, 'admin', 'admin', 1, 'admin'),
  (2, 'alice', 'alice123', 1, 'user');

-- Products owned by users
INSERT OR IGNORE INTO products(id, name, description, start_price, status, owner_user_id, created_at, image_url, category) VALUES
  (1, 'Vintage Clock', '1930s mantel clock', 120, 'available', 1, CURRENT_TIMESTAMP, NULL, 'antique'),
  (2, 'Gaming Laptop', 'RTX 4070, 16GB RAM', 900, 'available', 1, CURRENT_TIMESTAMP, NULL, 'electronics'),
  (3, 'Oil Painting', 'Landscape original', 500, 'available', 2, CURRENT_TIMESTAMP, NULL, 'art');

-- Rooms referencing a single product
INSERT OR IGNORE INTO rooms(id, room_name, product_id, duration, status, host_user_id, created_at, started_at, ended_at, base_price) VALUES
  (1, 'Morning Auction', 1, 3600, 'waiting', 1, CURRENT_TIMESTAMP, NULL, NULL, 100),
  (2, 'Evening Auction', 3, 5400, 'waiting', 2, CURRENT_TIMESTAMP, NULL, NULL, 200);

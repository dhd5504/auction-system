INSERT OR IGNORE INTO users(id, username, password, role, is_active) VALUES
  (1, 'admin', 'admin', 'admin', 1),
  (2, 'alice', 'alice123', 'user', 1),
  (3, 'bob', 'bob123', 'user', 1);

INSERT OR IGNORE INTO products(id, name, description, start_price, status, owner_user_id, created_at, image_url, category) VALUES
  (1, 'Vintage Clock', '1930s mantel clock', 120, 'available', 1, CURRENT_TIMESTAMP, NULL, 'antique'),
  (2, 'Gaming Laptop', 'RTX 4070, 16GB RAM', 900, 'available', 1, CURRENT_TIMESTAMP, NULL, 'electronics'),
  (3, 'Oil Painting', 'Landscape original', 500, 'available', 2, CURRENT_TIMESTAMP, NULL, 'art');

INSERT OR IGNORE INTO rooms(id, room_name, product_id, duration, status, host_user_id, created_at, base_price) VALUES
  (1, 'Morning Auction', 1, 3600, 'waiting', 1, CURRENT_TIMESTAMP, 100),
  (2, 'Evening Auction', 3, 5400, 'waiting', 2, CURRENT_TIMESTAMP, 200);

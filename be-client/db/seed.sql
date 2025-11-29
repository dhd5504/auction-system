-- Sample products (string ids)
INSERT OR IGNORE INTO products(id, name, start_price, buy_price, step, description) VALUES
  ('p1', 'Vintage Clock', 120.0, 300.0, 10.0, '1930s mantel clock'),
  ('p2', 'Gaming Laptop', 900.0, 1500.0, 50.0, 'RTX 4070, 16GB RAM'),
  ('p3', 'Oil Painting', 500.0, 1200.0, 25.0, 'Landscape original');

-- Sample rooms
INSERT OR IGNORE INTO rooms(id, name, start_time) VALUES
  ('r100', 'Morning Auction', '2025-05-01T09:00:00Z'),
  ('r101', 'Evening Auction', '2025-05-01T19:00:00Z');

-- Link products to rooms
INSERT OR IGNORE INTO room_products(room_id, product_id) VALUES
  ('r100', 'p1'),
  ('r100', 'p2'),
  ('r101', 'p3');

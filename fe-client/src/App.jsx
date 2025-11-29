import React, { useEffect, useState } from 'react';
import { BrowserRouter, Routes, Route, Navigate, NavLink } from 'react-router-dom';
import ProductCreatePage from './pages/ProductCreatePage';
import ProductListPage from './pages/ProductListPage';
import RoomCreatePage from './pages/RoomCreatePage';
import RoomListPage from './pages/RoomListPage';
import EventLog from './components/EventLog';
import { connectWebSocket } from './websocket';

export default function App() {
  const [events, setEvents] = useState([]);

  useEffect(() => {
    const socket = connectWebSocket((event) => {
      setEvents((prev) => {
        const next = [
          {
            time: new Date().toLocaleTimeString(),
            event,
            raw: typeof event === 'string' ? event : null,
          },
          ...prev,
        ];
        return next.slice(0, 50);
      });
    });

    return () => socket.close();
  }, []);

  return (
    <BrowserRouter>
      <div
        style={{
          fontFamily: '"Space Grotesk", "Helvetica Neue", sans-serif',
          background: 'linear-gradient(180deg, #f8fafc 0%, #e0f2fe 40%, #f8fafc 100%)',
          minHeight: '100vh',
        }}
      >
        <header
          style={{
            background: 'linear-gradient(135deg, #0f172a, #0d9488)',
            color: '#fff',
            padding: '16px 24px',
          }}
        >
          <h1 style={{ margin: 0 }}>Auction Dashboard</h1>
          <nav style={{ marginTop: 12, display: 'flex', gap: 16, flexWrap: 'wrap' }}>
            <NavLink
              to="/products"
              style={({ isActive }) => ({
                color: isActive ? '#5eead4' : '#e5e7eb',
                textDecoration: 'none',
                fontWeight: 600,
              })}
            >
              Products
            </NavLink>
            <NavLink
              to="/products/create"
              style={({ isActive }) => ({
                color: isActive ? '#5eead4' : '#e5e7eb',
                textDecoration: 'none',
                fontWeight: 600,
              })}
            >
              Create Product
            </NavLink>
            <NavLink
              to="/rooms"
              style={({ isActive }) => ({
                color: isActive ? '#5eead4' : '#e5e7eb',
                textDecoration: 'none',
                fontWeight: 600,
              })}
            >
              Rooms
            </NavLink>
            <NavLink
              to="/rooms/create"
              style={({ isActive }) => ({
                color: isActive ? '#5eead4' : '#e5e7eb',
                textDecoration: 'none',
                fontWeight: 600,
              })}
            >
              Create Room
            </NavLink>
          </nav>
        </header>

        <main style={{ padding: 24, display: 'grid', gridTemplateColumns: '2fr 1fr', gap: 16 }}>
          <div>
            <Routes>
              <Route path="/products/create" element={<ProductCreatePage />} />
              <Route path="/products" element={<ProductListPage />} />
              <Route path="/rooms/create" element={<RoomCreatePage />} />
              <Route path="/rooms" element={<RoomListPage />} />
              <Route path="*" element={<Navigate to="/products" replace />} />
            </Routes>
          </div>
          <EventLog events={events} />
        </main>
      </div>
    </BrowserRouter>
  );
}

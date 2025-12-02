import React from 'react';
import { BrowserRouter, Routes, Route, Navigate, NavLink } from 'react-router-dom';
import { AuthProvider, useAuth } from './AuthContext';

import LoginPage from './pages/LoginPage';
import RegisterPage from './pages/RegisterPage';

import MyProductList from './pages/MyProductList';
import MyProductDetail from './pages/MyProductDetail';
import ProductCreate from './pages/ProductCreate';
import ProductEdit from './pages/ProductEdit';
import ProductList from './pages/ProductList';
import ProductDetail from './pages/ProductDetail';

import MyRoomList from './pages/MyRoomList';
import MyRoomDetail from './pages/MyRoomDetail';
import RoomCreate from './pages/RoomCreate';
import RoomList from './pages/RoomList';
import RoomDetail from './pages/RoomDetail';
import AuctionRoomPage from './pages/AuctionRoomPage';

function NavBar() {
  const { user, logout } = useAuth();
  return (
    <header className="nav">
      <div className="logo">Auction Lite</div>
      <nav>
        <NavLink to="/products">Products</NavLink>
        <NavLink to="/rooms">Rooms</NavLink>
        {user && (
          <>
            <NavLink to="/me/products">My Products</NavLink>
            <NavLink to="/me/rooms">My Rooms</NavLink>
          </>
        )}
      </nav>
      <div className="auth">
        {user ? (
          <>
            <span>{user.username}</span>
            <button onClick={logout}>Logout</button>
          </>
        ) : (
          <>
            <NavLink to="/login">Login</NavLink>
            <NavLink to="/register">Register</NavLink>
          </>
        )}
      </div>
    </header>
  );
}

function AppShell() {
  return (
    <BrowserRouter>
      <NavBar />
      <main className="container">
        <Routes>
          <Route path="/login" element={<LoginPage />} />
          <Route path="/register" element={<RegisterPage />} />

          <Route path="/me/products" element={<MyProductList />} />
          <Route path="/me/products/create" element={<ProductCreate />} />
          <Route path="/me/products/:id" element={<MyProductDetail />} />
          <Route path="/me/products/:id/edit" element={<ProductEdit />} />

          <Route path="/products" element={<ProductList />} />
          <Route path="/products/:id" element={<ProductDetail />} />

          <Route path="/me/rooms" element={<MyRoomList />} />
          <Route path="/me/rooms/create" element={<RoomCreate />} />
          <Route path="/me/rooms/:id" element={<MyRoomDetail />} />

          <Route path="/rooms" element={<RoomList />} />
          <Route path="/rooms/:id" element={<RoomDetail />} />
          <Route path="/rooms/:id/auction" element={<AuctionRoomPage />} />

          <Route path="*" element={<Navigate to="/products" replace />} />
        </Routes>
      </main>
    </BrowserRouter>
  );
}

export default function App() {
  return (
    <AuthProvider>
      <AppShell />
    </AuthProvider>
  );
}

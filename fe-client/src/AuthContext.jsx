import React, { createContext, useContext, useEffect, useMemo, useState } from 'react';
import { login as apiLogin, register as apiRegister } from './api/auth';

const AuthContext = createContext(null);
const TOKEN_KEY = 'access_token';

function loadToken() {
  if (typeof localStorage === 'undefined') return null;
  return localStorage.getItem(TOKEN_KEY);
}

function persistToken(token) {
  if (typeof localStorage === 'undefined') return;
  if (token) {
    localStorage.setItem(TOKEN_KEY, token);
  } else {
    localStorage.removeItem(TOKEN_KEY);
  }
}

export function AuthProvider({ children }) {
  const [user, setUser] = useState(null);
  const [token, setToken] = useState(loadToken());
  const [loading, setLoading] = useState(false);

  useEffect(() => {
    const existing = loadToken();
    if (!existing) return;
    setToken(existing);
    setUser((prev) => prev || { username: 'me', id: null });
  }, []);

  const login = async (username, password) => {
    setLoading(true);
    try {
      const result = await apiLogin(username, password);
      const nextUser = result.user || { username };
      setUser(nextUser);
      setToken(result.token);
      persistToken(result.token);
      return result;
    } finally {
      setLoading(false);
    }
  };

  const register = async (username, password) => {
    setLoading(true);
    try {
      const result = await apiRegister(username, password);
      const nextUser = result.user || { username };
      setUser(nextUser);
      setToken(result.token);
      persistToken(result.token);
      return result;
    } finally {
      setLoading(false);
    }
  };

  const logout = () => {
    setUser(null);
    setToken(null);
    persistToken(null);
  };

  const value = useMemo(
    () => ({ user, token, loading, login, register, logout }),
    [user, token, loading]
  );

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export function useAuth() {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error('useAuth must be used within AuthProvider');
  return ctx;
}

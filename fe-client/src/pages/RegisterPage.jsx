import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { useAuth } from '../AuthContext';

export default function RegisterPage() {
  const { register, loading } = useAuth();
  const [form, setForm] = useState({ username: '', password: '' });
  const [error, setError] = useState('');
  const navigate = useNavigate();

  const handleSubmit = async (e) => {
    e.preventDefault();
    setError('');
    try {
      await register(form.username.trim(), form.password);
      navigate('/me/products');
    } catch (err) {
      setError(err.message);
    }
  };

  const handleChange = (e) => {
    const { name, value } = e.target;
    setForm((prev) => ({ ...prev, [name]: value }));
  };

  return (
    <div style={{ background: '#fff', padding: 20, borderRadius: 10, boxShadow: '0 10px 30px rgba(0,0,0,0.05)' }}>
      <h2 style={{ marginTop: 0 }}>Register</h2>
      <form onSubmit={handleSubmit} style={{ display: 'grid', gap: 12 }}>
        <label style={{ display: 'grid', gap: 6 }}>
          <span>Username</span>
          <input
            name="username"
            value={form.username}
            onChange={handleChange}
            required
            placeholder="yourname"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>
        <label style={{ display: 'grid', gap: 6 }}>
          <span>Password</span>
          <input
            type="password"
            name="password"
            value={form.password}
            onChange={handleChange}
            required
            placeholder="••••••"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>
        <button
          type="submit"
          disabled={loading}
          style={{
            background: '#0ea5e9',
            color: '#fff',
            border: 'none',
            padding: '12px 16px',
            borderRadius: 8,
            cursor: loading ? 'not-allowed' : 'pointer',
            fontWeight: 700,
          }}
        >
          {loading ? 'Signing up…' : 'Register'}
        </button>
      </form>
      {error && <p style={{ marginTop: 12, color: '#dc2626' }}>{error}</p>}
    </div>
  );
}

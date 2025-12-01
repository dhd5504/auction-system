import React, { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { createProduct } from '../api/products';

export default function ProductCreate() {
  const [form, setForm] = useState({ name: '', description: '', startPrice: '', imageUrl: '', category: '' });
  const [error, setError] = useState('');
  const navigate = useNavigate();

  const submit = async (e) => {
    e.preventDefault();
    setError('');
    try {
      await createProduct({ ...form, startPrice: Number(form.startPrice) });
      navigate('/me/products');
    } catch (err) {
      setError(err.message);
    }
  };

  return (
    <div className="card">
      <h2>Create Product</h2>
      <form onSubmit={submit} className="form">
        <input placeholder="Name" value={form.name} onChange={(e) => setForm({ ...form, name: e.target.value })} required />
        <textarea placeholder="Description" value={form.description} onChange={(e) => setForm({ ...form, description: e.target.value })} />
        <input type="number" placeholder="Start price" value={form.startPrice} onChange={(e) => setForm({ ...form, startPrice: e.target.value })} required />
        <input placeholder="Image URL" value={form.imageUrl} onChange={(e) => setForm({ ...form, imageUrl: e.target.value })} />
        <input placeholder="Category" value={form.category} onChange={(e) => setForm({ ...form, category: e.target.value })} />
        <button type="submit">Create</button>
      </form>
      {error && <p className="error">{error}</p>}
    </div>
  );
}

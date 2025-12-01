import React, { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { createRoom } from '../api/rooms';
import { getMyProducts } from '../api/products';

export default function RoomCreate() {
  const [products, setProducts] = useState([]);
  const [form, setForm] = useState({ roomName: '', productId: '', duration: '', basePrice: '', startedAt: '', endedAt: '' });
  const [error, setError] = useState('');
  const navigate = useNavigate();

  useEffect(() => {
    (async () => {
      try {
        const data = await getMyProducts();
        const available = (data || []).filter((p) => p.status === 'available');
        setProducts(available);
      } catch (err) {
        setError(err.message);
      }
    })();
  }, []);

  const submit = async (e) => {
    e.preventDefault();
    setError('');
    try {
      await createRoom({
        roomName: form.roomName,
        productId: Number(form.productId),
        duration: Number(form.duration),
        basePrice: Number(form.basePrice),
        startedAt: form.startedAt ? new Date(form.startedAt).toISOString() : undefined,
        endedAt: form.endedAt ? new Date(form.endedAt).toISOString() : undefined,
      });
      navigate('/me/rooms');
    } catch (err) {
      setError(err.message);
    }
  };

  return (
    <div className="card">
      <h2>Create Room</h2>
      <form onSubmit={submit} className="form">
        <input placeholder="Room name" value={form.roomName} onChange={(e) => setForm({ ...form, roomName: e.target.value })} required />
        <input type="number" placeholder="Duration (seconds)" value={form.duration} onChange={(e) => setForm({ ...form, duration: e.target.value })} required />
        <input type="number" placeholder="Base price" value={form.basePrice} onChange={(e) => setForm({ ...form, basePrice: e.target.value })} />
        <label>
          <div>Start date/time</div>
          <input type="datetime-local" value={form.startedAt} onChange={(e) => setForm({ ...form, startedAt: e.target.value })} />
        </label>
        <label>
          <div>End date/time</div>
          <input type="datetime-local" value={form.endedAt} onChange={(e) => setForm({ ...form, endedAt: e.target.value })} />
        </label>
        <select value={form.productId} onChange={(e) => setForm({ ...form, productId: e.target.value })} required>
          <option value="">-- Select available product --</option>
          {products.map((p) => (
            <option key={p.id} value={p.id}>{p.name} (#{p.id})</option>
          ))}
        </select>
        <button type="submit">Create</button>
      </form>
      {error && <p className="error">{error}</p>}
    </div>
  );
}

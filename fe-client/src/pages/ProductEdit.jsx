import React, { useEffect, useState } from 'react';
import { useNavigate, useParams } from 'react-router-dom';
import { getMyProduct, updateProduct } from '../api/products';

export default function ProductEdit() {
  const { id } = useParams();
  const [form, setForm] = useState({ name: '', description: '', startPrice: '', imageUrl: '', category: '', status: '' });
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(true);
  const navigate = useNavigate();

  useEffect(() => {
    (async () => {
      try {
        const data = await getMyProduct(id);
        setForm({
          name: data.name || '',
          description: data.description || '',
          startPrice: data.startPrice || '',
          imageUrl: data.imageUrl || '',
          category: data.category || '',
          status: data.status || '',
        });
      } catch (err) {
        setError(err.message);
      } finally {
        setLoading(false);
      }
    })();
  }, [id]);

  const submit = async (e) => {
    e.preventDefault();
    if (form.status !== 'available') {
      setError('Only edit when status = available');
      return;
    }
    setError('');
    try {
      await updateProduct(id, { ...form, startPrice: Number(form.startPrice) });
      navigate(`/me/products/${id}`);
    } catch (err) {
      setError(err.message);
    }
  };

  if (loading) return <p>Loading...</p>;
  return (
    <div className="card">
      <h2>Edit Product</h2>
      <form onSubmit={submit} className="form">
        <input value={form.name} onChange={(e) => setForm({ ...form, name: e.target.value })} required />
        <textarea value={form.description} onChange={(e) => setForm({ ...form, description: e.target.value })} />
        <input type="number" value={form.startPrice} onChange={(e) => setForm({ ...form, startPrice: e.target.value })} required />
        <input value={form.imageUrl} onChange={(e) => setForm({ ...form, imageUrl: e.target.value })} placeholder="Image URL" />
        <input value={form.category} onChange={(e) => setForm({ ...form, category: e.target.value })} placeholder="Category" />
        <button type="submit">Save</button>
      </form>
      {error && <p className="error">{error}</p>}
    </div>
  );
}

import React, { useEffect, useState } from 'react';
import { createRoom } from '../api/room';
import { getProducts } from '../api/product';

const initialForm = {
  name: '',
  startTime: '',
  products: [],
};

const getProductId = (product) => {
  return product.id ?? product.product_id ?? product.name;
};

export default function RoomCreatePage() {
  const [form, setForm] = useState(initialForm);
  const [products, setProducts] = useState([]);
  const [status, setStatus] = useState({ type: '', message: '' });
  const [loadingProducts, setLoadingProducts] = useState(true);
  const [submitting, setSubmitting] = useState(false);

  useEffect(() => {
    const loadProducts = async () => {
      try {
        setLoadingProducts(true);
        const data = await getProducts();
        setProducts(Array.isArray(data) ? data : []);
      } catch (err) {
        setStatus({ type: 'error', message: err.message });
      } finally {
        setLoadingProducts(false);
      }
    };

    loadProducts();
  }, []);

  const handleChange = (e) => {
    const { name, value } = e.target;
    setForm((prev) => ({ ...prev, [name]: value }));
  };

  const toggleProduct = (productId) => {
    setForm((prev) => {
      const exists = prev.products.includes(productId);
      return {
        ...prev,
        products: exists ? prev.products.filter((id) => id !== productId) : [...prev.products, productId],
      };
    });
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSubmitting(true);
    setStatus({ type: '', message: '' });

    try {
      const payload = {
        name: form.name.trim(),
        startTime: form.startTime ? new Date(form.startTime).toISOString() : null,
        products: form.products,
      };

      await createRoom(payload);
      setStatus({ type: 'success', message: 'Room created successfully.' });
      setForm({ ...initialForm });
    } catch (err) {
      setStatus({ type: 'error', message: err.message });
    } finally {
      setSubmitting(false);
    }
  };

  return (
    <div style={{ background: '#fff', padding: 20, borderRadius: 10, boxShadow: '0 10px 30px rgba(0,0,0,0.05)' }}>
      <h2 style={{ marginTop: 0 }}>Create Room</h2>
      <form onSubmit={handleSubmit} style={{ display: 'grid', gap: 12 }}>
        <label style={{ display: 'grid', gap: 6 }}>
          <span>Room Name</span>
          <input
            name="name"
            value={form.name}
            onChange={handleChange}
            required
            placeholder="Evening Auction"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>

        <label style={{ display: 'grid', gap: 6 }}>
          <span>Start Time</span>
          <input
            type="datetime-local"
            name="startTime"
            value={form.startTime}
            onChange={handleChange}
            required
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>

        <div style={{ display: 'grid', gap: 8 }}>
          <span>Products</span>
          {loadingProducts ? (
            <p>Loading products...</p>
          ) : products.length === 0 ? (
            <p style={{ color: '#6b7280' }}>No products available. Create a product first.</p>
          ) : (
            <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fit, minmax(180px, 1fr))', gap: 8 }}>
              {products.map((p) => {
                const pid = getProductId(p);
                const isChecked = form.products.includes(pid);
                return (
                  <label
                    key={pid}
                    style={{
                      border: '1px solid #e5e7eb',
                      borderRadius: 8,
                      padding: 10,
                      display: 'flex',
                      alignItems: 'center',
                      gap: 8,
                      background: isChecked ? '#ecfeff' : '#fff',
                    }}
                  >
                    <input type="checkbox" checked={isChecked} onChange={() => toggleProduct(pid)} />
                    <div>
                      <div style={{ fontWeight: 600 }}>{p.name}</div>
                      <div style={{ fontSize: 12, color: '#6b7280' }}>
                        Start {p.startPrice ?? p.start_price ?? p.start}
                      </div>
                    </div>
                  </label>
                );
              })}
            </div>
          )}
        </div>

        <button
          type="submit"
          disabled={submitting || loadingProducts}
          style={{
            background: '#2563eb',
            color: '#fff',
            border: 'none',
            padding: '12px 16px',
            borderRadius: 8,
            cursor: submitting ? 'not-allowed' : 'pointer',
            fontWeight: 700,
            letterSpacing: 0.2,
          }}
        >
          {submitting ? 'Saving…' : 'Create Room'}
        </button>
      </form>
      {status.message && (
        <p style={{ marginTop: 12, color: status.type === 'error' ? '#dc2626' : '#16a34a' }}>
          {status.message}
        </p>
      )}
    </div>
  );
}

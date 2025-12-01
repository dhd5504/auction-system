import React, { useState } from 'react';
import { createProduct } from '../api/product';
import { useAuth } from '../AuthContext';

const initialForm = {
  name: '',
  startPrice: '',
  buyPrice: '',
  step: '',
  description: '',
};

export default function ProductCreatePage() {
  const [form, setForm] = useState(initialForm);
  const [status, setStatus] = useState({ type: '', message: '' });
  const [submitting, setSubmitting] = useState(false);
  const { user } = useAuth();

  const handleChange = (e) => {
    const { name, value } = e.target;
    setForm((prev) => ({ ...prev, [name]: value }));
  };

  const handleSubmit = async (e) => {
    e.preventDefault();
    setSubmitting(true);
    setStatus({ type: '', message: '' });

    try {
      const payload = {
        name: form.name.trim(),
        startPrice: parseFloat(form.startPrice),
        step: parseFloat(form.step),
        description: form.description.trim(),
      };
      if (form.buyPrice !== '') {
        payload.buyPrice = parseFloat(form.buyPrice);
      }

      await createProduct(payload);
      setStatus({ type: 'success', message: 'Product created successfully.' });
      setForm({ ...initialForm });
    } catch (err) {
      setStatus({ type: 'error', message: err.message });
    } finally {
      setSubmitting(false);
    }
  };

  if (!user) {
    return (
      <div style={{ background: '#fff', padding: 20, borderRadius: 10, boxShadow: '0 10px 30px rgba(0,0,0,0.05)' }}>
        <h2 style={{ marginTop: 0 }}>Create Product</h2>
        <p style={{ color: '#ef4444' }}>Please login to create a product.</p>
      </div>
    );
  }

  return (
    <div style={{ background: '#fff', padding: 20, borderRadius: 10, boxShadow: '0 10px 30px rgba(0,0,0,0.05)' }}>
      <h2 style={{ marginTop: 0 }}>Create Product</h2>
      <form onSubmit={handleSubmit} style={{ display: 'grid', gap: 12 }}>
        <label style={{ display: 'grid', gap: 6 }}>
          <span>Name</span>
          <input
            name="name"
            value={form.name}
            onChange={handleChange}
            required
            placeholder="Vintage watch"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>

        <label style={{ display: 'grid', gap: 6 }}>
          <span>Start Price</span>
          <input
            type="number"
            name="startPrice"
            value={form.startPrice}
            onChange={handleChange}
            required
            step="0.01"
            placeholder="0.00"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>

        <label style={{ display: 'grid', gap: 6 }}>
          <span>Buy Now Price</span>
          <input
            type="number"
            name="buyPrice"
            value={form.buyPrice}
            onChange={handleChange}
            step="0.01"
            placeholder="0.00"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>

        <label style={{ display: 'grid', gap: 6 }}>
          <span>Bid Step</span>
          <input
            type="number"
            name="step"
            value={form.step}
            onChange={handleChange}
            required
            step="0.01"
            placeholder="0.00"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db' }}
          />
        </label>

        <label style={{ display: 'grid', gap: 6 }}>
          <span>Description</span>
          <textarea
            name="description"
            value={form.description}
            onChange={handleChange}
            rows={3}
            placeholder="Details that help bidders understand the item"
            style={{ padding: 10, borderRadius: 6, border: '1px solid #d1d5db', resize: 'vertical' }}
          />
        </label>

        <button
          type="submit"
          disabled={submitting}
          style={{
            background: '#0ea5e9',
            color: '#fff',
            border: 'none',
            padding: '12px 16px',
            borderRadius: 8,
            cursor: submitting ? 'not-allowed' : 'pointer',
            fontWeight: 700,
            letterSpacing: 0.2,
          }}
        >
          {submitting ? 'Saving…' : 'Create Product'}
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

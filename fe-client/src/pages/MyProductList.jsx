import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { getMyProducts, deleteProduct } from '../api/products';
import StatusBadge from '../components/StatusBadge';

export default function MyProductList() {
  const [items, setItems] = useState([]);
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(true);

  const load = async () => {
    try {
      setLoading(true);
      const data = await getMyProducts();
      setItems(Array.isArray(data) ? data : []);
      setError('');
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    load();
  }, []);

  const handleDelete = async (id, status) => {
    if (status !== 'available') {
      alert('Only delete when status = available');
      return;
    }
    if (!window.confirm('Delete this product?')) return;
    try {
      await deleteProduct(id);
      await load();
    } catch (err) {
      alert(err.message);
    }
  };

  return (
    <div className="card">
      <div className="card-header">
        <h2>My Products</h2>
        <Link to="/me/products/create">Create</Link>
      </div>
      {loading && <p>Loading...</p>}
      {error && <p className="error">{error}</p>}
      <div className="list">
        {items.map((p) => (
          <div key={p.id} className="list-item">
            <div>
              <div className="title-row">
                <Link to={`/me/products/${p.id}`}>{p.name}</Link>
                <StatusBadge status={p.status} />
              </div>
              <div className="muted">Start price: {p.startPrice}</div>
            </div>
            <div className="actions">
              <Link to={`/me/products/${p.id}/edit`} className={p.status !== 'available' ? 'disabled' : ''}>Edit</Link>
              <button onClick={() => handleDelete(p.id, p.status)} disabled={p.status !== 'available'}>
                Delete
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

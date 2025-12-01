import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { getAllProducts } from '../api/products';
import StatusBadge from '../components/StatusBadge';

export default function ProductList() {
  const [items, setItems] = useState([]);
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    (async () => {
      try {
        const data = await getAllProducts();
        setItems(Array.isArray(data) ? data : []);
      } catch (err) {
        setError(err.message);
      } finally {
        setLoading(false);
      }
    })();
  }, []);

  return (
    <div className="card">
      <h2>Products</h2>
      {loading && <p>Loading...</p>}
      {error && <p className="error">{error}</p>}
      <div className="list">
        {items.map((p) => (
          <div key={p.id} className="list-item">
            <div className="title-row">
              <Link to={`/products/${p.id}`}>{p.name}</Link>
              <StatusBadge status={p.status} />
            </div>
            <div className="muted">Start price: {p.startPrice}</div>
          </div>
        ))}
      </div>
    </div>
  );
}

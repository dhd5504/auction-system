import React, { useEffect, useState } from 'react';
import { useParams, Link } from 'react-router-dom';
import { getProduct } from '../api/products';
import StatusBadge from '../components/StatusBadge';

export default function ProductDetail() {
  const { id } = useParams();
  const [item, setItem] = useState(null);
  const [error, setError] = useState('');

  useEffect(() => {
    (async () => {
      try {
        const data = await getProduct(id);
        setItem(data);
      } catch (err) {
        setError(err.message);
      }
    })();
  }, [id]);

  if (error) return <p className="error">{error}</p>;
  if (!item) return <p>Loading...</p>;

  return (
    <div className="card">
      <div className="title-row">
        <h2>{item.name}</h2>
        <StatusBadge status={item.status} />
      </div>
      <p>{item.description}</p>
      <p>Start price: {item.startPrice}</p>
      <p>Category: {item.category}</p>
      <p>Owner: {item.ownerUserId}</p>
      <Link to="/products">Back</Link>
    </div>
  );
}

import React, { useEffect, useState } from 'react';
import { useParams, Link } from 'react-router-dom';
import { getMyProduct } from '../api/products';
import StatusBadge from '../components/StatusBadge';

export default function MyProductDetail() {
  const { id } = useParams();
  const [item, setItem] = useState(null);
  const [error, setError] = useState('');

  useEffect(() => {
    (async () => {
      try {
        const data = await getMyProduct(id);
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
      <p>Image: {item.imageUrl}</p>
      <div className="actions">
        <Link to={`/me/products/${id}/edit`} className={item.status !== 'available' ? 'disabled' : ''}>Edit</Link>
        <Link to="/me/products">Back</Link>
      </div>
    </div>
  );
}

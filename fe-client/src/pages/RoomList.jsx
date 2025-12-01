import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { getAllRooms } from '../api/rooms';
import StatusBadge from '../components/StatusBadge';

export default function RoomList() {
  const [items, setItems] = useState([]);
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    (async () => {
      try {
        const data = await getAllRooms();
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
      <h2>Rooms</h2>
      {loading && <p>Loading...</p>}
      {error && <p className="error">{error}</p>}
      <div className="list">
        {items.map((r) => (
          <div key={r.id} className="list-item">
            <div className="title-row">
              <Link to={`/rooms/${r.id}`}>{r.roomName}</Link>
              <StatusBadge status={r.status} />
            </div>
            <div className="muted">Product #{r.productId} | Duration: {r.duration}s</div>
          </div>
        ))}
      </div>
    </div>
  );
}

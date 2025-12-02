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
          <div
            key={r.id}
            className="list-item"
            style={{
              display: 'grid',
              gridTemplateColumns: '1.2fr 1fr auto',
              gap: '12px',
              alignItems: 'center',
            }}
          >
            <div className="title-row">
              <Link to={`/rooms/${r.id}`}>{r.roomName}</Link>
              <StatusBadge status={r.status} />
            </div>
            <div className="muted">Product #{r.productId} | Duration: {r.duration}s</div>
            <div className="flex justify-end">
              {r.status === 'running' ? (
                <Link
                  to={`/rooms/${r.id}/auction`}
                  className="px-3 py-2 bg-emerald-500 text-white rounded-lg font-semibold"
                >
                  Join Room
                </Link>
              ) : (
                <span className="text-sm text-slate-400">Not running</span>
              )}
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

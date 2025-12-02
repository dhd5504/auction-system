import React, { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import { getMyRooms, deleteRoom, startRoom } from '../api/rooms';
import StatusBadge from '../components/StatusBadge';

export default function MyRoomList() {
  const [items, setItems] = useState([]);
  const [error, setError] = useState('');
  const [loading, setLoading] = useState(true);

  const load = async () => {
    try {
      setLoading(true);
      const data = await getMyRooms();
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
    if (status !== 'waiting') {
      alert('Only delete when status = waiting');
      return;
    }
    if (!window.confirm('Delete this room?')) return;
    try {
      await deleteRoom(id);
      await load();
    } catch (err) {
      alert(err.message);
    }
  };

  const handleStart = async (id, status) => {
    if (status !== 'waiting' && status !== 'pending') {
      alert('Only start when status = waiting/pending');
      return;
    }
    try {
      await startRoom(id);
      await load();
    } catch (err) {
      alert(err.message);
    }
  };

  return (
    <div className="card">
      <div className="card-header">
        <h2>My Rooms</h2>
        <Link to="/me/rooms/create">Create</Link>
      </div>
      {loading && <p>Loading...</p>}
      {error && <p className="error">{error}</p>}
      <div className="list">
        {items.map((r) => (
          <div key={r.id} className="list-item">
            <div>
              <div className="title-row">
                <Link to={`/me/rooms/${r.id}`}>{r.roomName}</Link>
                <StatusBadge status={r.status} />
              </div>
              <div className="muted">Product #{r.productId} | Duration: {r.duration}s | Base: {r.basePrice}</div>
            </div>
            <div className="actions">
              <button onClick={() => handleStart(r.id, r.status)} disabled={r.status !== 'waiting' && r.status !== 'pending'}>
                Start
              </button>
              <button onClick={() => handleDelete(r.id, r.status)} disabled={r.status !== 'waiting'}>
                Delete
              </button>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

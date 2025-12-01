import React, { useEffect, useState } from 'react';
import { getRooms, startRoom } from '../api/room';
import { useAuth } from '../AuthContext';

export default function RoomListPage() {
  const [rooms, setRooms] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');
  const [actionMessage, setActionMessage] = useState('');
  const [startingId, setStartingId] = useState(null);
  const { user } = useAuth();

  const loadRooms = async () => {
    if (!user) {
      setRooms([]);
      setError('Please login to view rooms.');
      setLoading(false);
      return;
    }
    try {
      setLoading(true);
      setError('');
      const data = await getRooms();
      setRooms(Array.isArray(data) ? data : []);
    } catch (err) {
      setError(err.message);
    } finally {
      setLoading(false);
    }
  };

  useEffect(() => {
    loadRooms();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [user]);

  const handleStart = async (id) => {
    setStartingId(id);
    setActionMessage('');
    try {
      await startRoom(id);
      setActionMessage(`Room ${id} started.`);
      await loadRooms();
    } catch (err) {
      setActionMessage(err.message);
    } finally {
      setStartingId(null);
    }
  };

  return (
    <div style={{ background: '#fff', padding: 20, borderRadius: 10, boxShadow: '0 10px 30px rgba(0,0,0,0.05)' }}>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <h2 style={{ marginTop: 0 }}>Rooms</h2>
        <button
          onClick={loadRooms}
          style={{
            background: '#e5e7eb',
            border: 'none',
            padding: '8px 12px',
            borderRadius: 8,
            cursor: 'pointer',
            fontWeight: 600,
          }}
        >
          Refresh
        </button>
      </div>
      {loading && <p>Loading rooms...</p>}
      {error && <p style={{ color: '#dc2626' }}>{error}</p>}
      {actionMessage && <p style={{ color: '#2563eb' }}>{actionMessage}</p>}
      {!user && <p style={{ color: '#ef4444' }}>Login to start rooms.</p>}
      {!loading && !error && (
        <div style={{ overflowX: 'auto' }}>
          <table style={{ width: '100%', borderCollapse: 'collapse' }}>
            <thead>
              <tr style={{ textAlign: 'left', borderBottom: '2px solid #e5e7eb' }}>
                <th style={{ padding: '8px 6px' }}>ID</th>
                <th style={{ padding: '8px 6px' }}>Name</th>
                <th style={{ padding: '8px 6px' }}>Start Time</th>
                <th style={{ padding: '8px 6px' }}>Products</th>
                <th style={{ padding: '8px 6px' }}>Status</th>
                <th style={{ padding: '8px 6px' }}>Actions</th>
              </tr>
            </thead>
            <tbody>
              {rooms.length === 0 ? (
                <tr>
                  <td colSpan="6" style={{ padding: 12, textAlign: 'center', color: '#6b7280' }}>
                    No rooms yet.
                  </td>
                </tr>
              ) : (
                rooms.map((room) => {
                  const id = room.id ?? room.room_id ?? room.roomId ?? room.name;
                  const startTime =
                    room.startTime ??
                    room.start_time ??
                    room.start ??
                    room.start_datetime ??
                    room.startDateTime ??
                    room.start_at;
                  const status = room.status ?? room.state ?? 'pending';

                  const rawProducts =
                    room.products ??
                    room.productIds ??
                    room.product_ids ??
                    room.productId ??
                    room.product_list ??
                    room.productList;

                  let productLabel = '—';
                  if (Array.isArray(rawProducts) && rawProducts.length > 0) {
                    productLabel = rawProducts
                      .map((p) => {
                        if (typeof p === 'object' && p !== null) {
                          return p.name ?? p.title ?? p.id ?? p.product_id ?? '?';
                        }
                        return p;
                      })
                      .join(', ');
                  } else if (typeof rawProducts === 'string' || typeof rawProducts === 'number') {
                    productLabel = rawProducts;
                  }

                  const canStart = status ? status !== 'started' : true;
                  const canStartWithAuth = !!user && canStart;

                  return (
                    <tr key={id} style={{ borderBottom: '1px solid #f3f4f6' }}>
                      <td style={{ padding: '10px 6px' }}>{id}</td>
                      <td style={{ padding: '10px 6px' }}>{room.name}</td>
                      <td style={{ padding: '10px 6px' }}>{startTime ?? '—'}</td>
                      <td style={{ padding: '10px 6px', maxWidth: 240 }}>{productLabel}</td>
                      <td style={{ padding: '10px 6px' }}>{status}</td>
                      <td style={{ padding: '10px 6px' }}>
                        <button
                          onClick={() => handleStart(id)}
                          disabled={!canStartWithAuth || startingId === id}
                          style={{
                            background: '#22c55e',
                            color: '#fff',
                            border: 'none',
                            padding: '8px 12px',
                            borderRadius: 8,
                            cursor: !canStartWithAuth ? 'not-allowed' : 'pointer',
                            fontWeight: 700,
                          }}
                        >
                          {startingId === id ? 'Starting…' : 'Start'}
                        </button>
                      </td>
                    </tr>
                  );
                })
              )}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}

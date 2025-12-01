import React, { useEffect, useState } from 'react';
import { useParams, Link } from 'react-router-dom';
import { getRoom } from '../api/rooms';
import StatusBadge from '../components/StatusBadge';

export default function RoomDetail() {
  const { id } = useParams();
  const [room, setRoom] = useState(null);
  const [error, setError] = useState('');

  useEffect(() => {
    (async () => {
      try {
        const data = await getRoom(id);
        setRoom(data);
      } catch (err) {
        setError(err.message);
      }
    })();
  }, [id]);

  if (error) return <p className="error">{error}</p>;
  if (!room) return <p>Loading...</p>;

  return (
    <div className="card">
      <div className="title-row">
        <h2>{room.roomName}</h2>
        <StatusBadge status={room.status} />
      </div>
      <p>Product ID: {room.productId}</p>
      <p>Duration: {room.duration}s</p>
      <p>Base price: {room.basePrice}</p>
      <p>Host: {room.hostUserId}</p>
      <Link to="/rooms">Back</Link>
    </div>
  );
}

import React from 'react';

export default function EventLog({ events }) {
  return (
    <div style={{ border: '1px solid #e5e7eb', borderRadius: 10, padding: 14, background: '#fff', maxHeight: 360, overflowY: 'auto', boxShadow: '0 10px 30px rgba(0,0,0,0.04)' }}>
      <h3 style={{ marginTop: 0, marginBottom: 12 }}>Realtime Events</h3>
      {events.length === 0 ? (
        <p style={{ color: '#6b7280' }}>Waiting for WebSocket updates...</p>
      ) : (
        events.map((entry, idx) => (
          <pre
            key={idx}
            style={{
              background: '#f8fafc',
              padding: 10,
              borderRadius: 8,
              fontSize: 12,
              overflowX: 'auto',
              marginBottom: 8,
            }}
          >
            <strong>{entry.time}</strong> - {entry.raw || JSON.stringify(entry.event, null, 2)}
          </pre>
        ))
      )}
    </div>
  );
}

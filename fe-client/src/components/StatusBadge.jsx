import React from 'react';

const colors = {
  available: '#22c55e',
  pending: '#f59e0b',
  running: '#3b82f6',
  sold: '#a855f7',
  waiting: '#6b7280',
};

export default function StatusBadge({ status }) {
  const color = colors[status] || '#9ca3af';
  return (
    <span
      style={{
        padding: '2px 8px',
        borderRadius: 12,
        background: color,
        color: '#fff',
        fontSize: 12,
        fontWeight: 700,
        textTransform: 'capitalize',
      }}
    >
      {status || 'unknown'}
    </span>
  );
}

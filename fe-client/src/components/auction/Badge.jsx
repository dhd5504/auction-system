import React from 'react';

export default function Badge({ children, color = 'bg-slate-100 text-slate-800' }) {
  return <span className={`px-3 py-1 text-xs font-semibold rounded-full ${color}`}>{children}</span>;
}

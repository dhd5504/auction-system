import React from 'react';
import Card from './Card';

export default function BidHistory({ title, items }) {
  return (
    <Card className="space-y-2">
      <h3 className="font-semibold text-slate-900">{title}</h3>
      <div className="space-y-2 max-h-64 overflow-auto pr-1">
        {items.map((h) => (
          <div key={h.id} className="flex items-center justify-between p-2 rounded-lg border border-slate-100">
            <div className="text-sm">
              <span className="font-semibold text-slate-900">{h.user}</span> {h.action || 'bid'} {h.amount}
            </div>
            <div className="text-xs text-slate-500">{h.time}</div>
          </div>
        ))}
      </div>
    </Card>
  );
}

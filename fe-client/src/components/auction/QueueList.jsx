import React from 'react';
import Card from './Card';

export default function QueueList({ items }) {
  return (
    <Card className="space-y-2">
      <div className="flex items-center justify-between">
        <h3 className="font-semibold text-slate-900">Queue ({items.length})</h3>
      </div>
      <div className="space-y-2">
        {items.map((q) => (
          <div key={q.id} className="flex items-center gap-3 p-2 rounded-lg border border-slate-100">
            <div className="w-10 h-10 rounded-lg bg-slate-100 overflow-hidden flex items-center justify-center">
              <img src={q.image} alt={q.name} className="w-full h-full object-cover" />
            </div>
            <div className="flex-1">
              <div className="text-sm font-semibold text-slate-900">{q.name}</div>
              <div className="text-xs text-slate-500">Start Price: {q.startPrice}</div>
            </div>
            <div className="text-sm font-semibold text-slate-700">{q.price}</div>
          </div>
        ))}
      </div>
    </Card>
  );
}

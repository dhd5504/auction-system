import React from 'react';
import Card from './Card';

export default function Ranking({ items }) {
  return (
    <Card className="space-y-2">
      <h3 className="font-semibold text-slate-900">Ranking</h3>
      <div className="space-y-2">
        {items.map((r) => (
          <div key={r.id} className="flex items-center gap-3 p-2 rounded-lg border border-slate-100">
            <div className="w-8 h-8 rounded-full bg-slate-200 flex items-center justify-center text-slate-700 text-sm font-semibold">
              {r.name.slice(0, 1).toUpperCase()}
            </div>
            <div className="flex-1">
              <div className="text-sm font-semibold text-slate-900">{r.name}</div>
            </div>
            <div className="text-sm font-semibold text-slate-800">{r.amount}</div>
          </div>
        ))}
      </div>
    </Card>
  );
}

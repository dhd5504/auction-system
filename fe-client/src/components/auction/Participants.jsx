import React from 'react';
import Card from './Card';
import Badge from './Badge';

export default function Participants({ users }) {
  return (
    <Card className="space-y-2">
      <div className="flex items-center justify-between">
        <h3 className="font-semibold text-slate-900">Participants ({users.length})</h3>
      </div>
      <div className="space-y-2">
        {users.map((u) => (
          <div key={u.id} className="flex items-center gap-3 p-2 rounded-lg border border-slate-100">
            <div className="w-8 h-8 rounded-full bg-slate-200 flex items-center justify-center text-slate-700 text-sm font-semibold">
              {u.name.slice(0, 2).toUpperCase()}
            </div>
            <div className="flex-1">
              <div className="text-sm font-semibold text-slate-900">{u.name}</div>
              <div className="text-xs text-slate-500">{u.online ? 'Online' : 'Offline'}</div>
            </div>
            {u.host && <Badge color="bg-purple-100 text-purple-800">Host</Badge>}
          </div>
        ))}
      </div>
    </Card>
  );
}

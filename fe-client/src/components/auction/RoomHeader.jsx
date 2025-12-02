import React from 'react';
import Badge from './Badge';
import Card from './Card';

export default function RoomHeader({ room }) {
  return (
    <Card className="flex items-center justify-between">
      <div>
        <h1 className="text-2xl font-bold text-slate-900">AuctionRoomPage</h1>
        <p className="text-sm text-slate-500">
          Host: <span className="font-semibold text-slate-700">{room.host}</span>
        </p>
      </div>
      <div className="flex items-center gap-2">
        <Badge color="bg-emerald-100 text-emerald-800">{room.status}</Badge>
        <button className="px-3 py-2 bg-slate-200 hover:bg-slate-300 text-slate-800 rounded-lg text-sm font-semibold">
          Leave
        </button>
      </div>
    </Card>
  );
}

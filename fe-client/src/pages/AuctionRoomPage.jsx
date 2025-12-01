import React, { useMemo } from 'react';

const Badge = ({ children, color = 'bg-emerald-100 text-emerald-800' }) => (
  <span className={`px-3 py-1 rounded-full text-xs font-semibold ${color}`}>{children}</span>
);

function RoomHeader({ room }) {
  return (
    <div className="bg-white shadow rounded-xl p-4 flex flex-col md:flex-row md:items-center md:justify-between gap-3">
      <div>
        <h1 className="text-2xl font-bold text-slate-900">{room.name}</h1>
        <p className="text-sm text-slate-500">Host: {room.host}</p>
      </div>
      <div className="flex flex-wrap items-center gap-2">
        <Badge color="bg-blue-100 text-blue-800">Status: {room.status}</Badge>
        <Badge color="bg-orange-100 text-orange-800">Time left: {room.countdown}</Badge>
        <button className="px-4 py-2 bg-red-500 hover:bg-red-600 text-white rounded-lg font-semibold transition">
          Leave Room
        </button>
      </div>
    </div>
  );
}

function ActiveProductPanel({ product }) {
  return (
    <div className="bg-white shadow rounded-xl p-4 space-y-4">
      <div className="flex gap-4">
        <div className="w-40 h-40 rounded-lg overflow-hidden bg-slate-100">
          <img src={product.image} alt={product.name} className="w-full h-full object-cover" />
        </div>
        <div className="flex-1 space-y-2">
          <h2 className="text-xl font-semibold text-slate-900">{product.name}</h2>
          <p className="text-sm text-slate-500">{product.description}</p>
          <div className="grid grid-cols-2 gap-2 text-sm">
            <div className="text-slate-500">Start Price</div>
            <div className="font-semibold">{product.startPrice}</div>
            <div className="text-slate-500">Current Price</div>
            <div className="font-semibold text-emerald-600">{product.currentPrice}</div>
            <div className="text-slate-500">Leading</div>
            <div className="font-semibold">{product.leading}</div>
          </div>
        </div>
      </div>
      <div className="bg-slate-50 rounded-lg p-4 flex flex-col md:flex-row md:items-center gap-3">
        <input
          type="number"
          className="flex-1 border border-slate-200 rounded-lg px-3 py-2"
          placeholder={`Enter bid (min +${product.minStep})`}
        />
        <button className="px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg font-semibold transition">
          Bid +{product.minStep}
        </button>
      </div>
    </div>
  );
}

function QueueList({ items }) {
  return (
    <div className="bg-white shadow rounded-xl p-3 space-y-2">
      <div className="flex items-center justify-between">
        <h3 className="font-semibold text-slate-900">Queue List</h3>
        <Badge>{items.length} items</Badge>
      </div>
      <div className="space-y-2 max-h-64 overflow-auto pr-1">
        {items.map((q) => (
          <div key={q.id} className="flex items-center gap-3 p-2 rounded-lg border border-slate-100 hover:bg-slate-50">
            <img src={q.image} alt={q.name} className="w-12 h-12 rounded object-cover" />
            <div className="flex-1">
              <div className="text-sm font-semibold">{q.name}</div>
              <div className="text-xs text-slate-500">Start: {q.startPrice}</div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

function ParticipantList({ users }) {
  return (
    <div className="bg-white shadow rounded-xl p-3 space-y-2">
      <div className="flex items-center justify-between">
        <h3 className="font-semibold text-slate-900">Participants</h3>
        <Badge color="bg-emerald-100 text-emerald-800">{users.length} online</Badge>
      </div>
      <div className="space-y-2 max-h-64 overflow-auto pr-1">
        {users.map((u) => (
          <div key={u.id} className="flex items-center gap-3 p-2 rounded-lg border border-slate-100 hover:bg-slate-50">
            <img src={u.avatar} alt={u.name} className="w-10 h-10 rounded-full object-cover" />
            <div className="flex-1">
              <div className="text-sm font-semibold flex items-center gap-2">
                {u.name}
                {u.host && <Badge color="bg-purple-100 text-purple-800">Host</Badge>}
              </div>
              <div className={`text-xs ${u.online ? 'text-emerald-600' : 'text-slate-400'}`}>
                {u.online ? 'Online' : 'Offline'}
              </div>
            </div>
          </div>
        ))}
      </div>
    </div>
  );
}

function RankingList({ items }) {
  return (
    <div className="bg-white shadow rounded-xl p-3 space-y-2">
      <div className="flex items-center justify-between">
        <h3 className="font-semibold text-slate-900">Top Bidders</h3>
        <Badge color="bg-red-100 text-red-700">Live</Badge>
      </div>
      <div className="space-y-2">
        {items.map((b, idx) => (
          <div key={b.id} className="flex items-center gap-3 p-2 rounded-lg border border-slate-100 hover:bg-slate-50">
            <div className="w-8 h-8 rounded-full bg-slate-100 flex items-center justify-center text-slate-700 font-semibold">{idx + 1}</div>
            <img src={b.avatar} alt={b.name} className="w-10 h-10 rounded-full object-cover" />
            <div className="flex-1">
              <div className="text-sm font-semibold">{b.name}</div>
              <div className="text-xs text-slate-500">{b.time}</div>
            </div>
            <div className="text-sm font-bold text-emerald-600">{b.amount}</div>
          </div>
        ))}
      </div>
    </div>
  );
}

function BidHistory({ items }) {
  return (
    <div className="bg-white shadow rounded-xl p-3 space-y-2">
      <div className="flex items-center justify-between">
        <h3 className="font-semibold text-slate-900">Bid History</h3>
        <Badge>{items.length}</Badge>
      </div>
      <div className="space-y-2 max-h-64 overflow-auto pr-1">
        {items.map((h) => (
          <div key={h.id} className="flex justify-between p-2 rounded-lg border border-slate-100 hover:bg-slate-50 text-sm">
            <div>
              <span className="font-semibold">{h.user}</span> → <span className="text-emerald-600 font-semibold">+{h.delta}</span>
            </div>
            <div className="text-xs text-slate-500">{h.time}</div>
          </div>
        ))}
      </div>
    </div>
  );
}

export default function AuctionRoomPage() {
  const mock = useMemo(
    () => ({
      room: { name: 'Morning Auction', host: 'admin', status: 'RUNNING', countdown: '12:34' },
      product: {
        name: 'Vintage Clock',
        description: '1930s mantel clock with mahogany finish.',
        startPrice: '$120',
        currentPrice: '$240',
        leading: 'alice',
        minStep: '$10',
        image: 'https://images.unsplash.com/photo-1504384308090-c894fdcc538d?auto=format&fit=crop&w=600&q=80',
      },
      queue: [
        { id: 1, name: 'Gaming Laptop', startPrice: '$900', image: 'https://images.unsplash.com/photo-1517336714731-489689fd1ca8?auto=format&fit=crop&w=200&q=80' },
        { id: 2, name: 'Oil Painting', startPrice: '$500', image: 'https://images.unsplash.com/photo-1504198453319-5ce911bafcde?auto=format&fit=crop&w=200&q=80' },
        { id: 3, name: 'Designer Chair', startPrice: '$300', image: 'https://images.unsplash.com/photo-1489515217757-5fd1be406fef?auto=format&fit=crop&w=200&q=80' },
      ],
      participants: [
        { id: 1, name: 'admin', avatar: 'https://i.pravatar.cc/100?img=1', online: true, host: true },
        { id: 2, name: 'alice', avatar: 'https://i.pravatar.cc/100?img=2', online: true },
        { id: 3, name: 'bob', avatar: 'https://i.pravatar.cc/100?img=3', online: false },
        { id: 4, name: 'carol', avatar: 'https://i.pravatar.cc/100?img=4', online: true },
      ],
      ranking: [
        { id: 1, name: 'alice', avatar: 'https://i.pravatar.cc/100?img=2', amount: '$240', time: '10:01' },
        { id: 2, name: 'bob', avatar: 'https://i.pravatar.cc/100?img=3', amount: '$230', time: '09:58' },
        { id: 3, name: 'carol', avatar: 'https://i.pravatar.cc/100?img=4', amount: '$220', time: '09:55' },
      ],
      history: [
        { id: 1, user: 'alice', delta: '$20', time: '10:10:02' },
        { id: 2, user: 'bob', delta: '$10', time: '10:09:30' },
        { id: 3, user: 'carol', delta: '$10', time: '10:09:10' },
        { id: 4, user: 'dave', delta: '$20', time: '10:08:55' },
        { id: 5, user: 'erin', delta: '$10', time: '10:08:20' },
      ],
    }),
    []
  );

  return (
    <div className="min-h-screen bg-slate-100 p-4 md:p-6">
      <div className="max-w-6xl mx-auto space-y-4">
        <RoomHeader room={mock.room} />

        <div className="grid lg:grid-cols-3 gap-4">
          {/* Left column */}
          <div className="space-y-4">
            <QueueList items={mock.queue} />
            <RankingList items={mock.ranking} />
          </div>

          {/* Middle column */}
          <div>
            <ActiveProductPanel product={mock.product} />
          </div>

          {/* Right column */}
          <div className="space-y-4">
            <ParticipantList users={mock.participants} />
            <BidHistory items={mock.history} />
          </div>
        </div>
      </div>
    </div>
  );
}

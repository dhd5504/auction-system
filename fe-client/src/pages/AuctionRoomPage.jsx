import React, { useMemo } from 'react';
import RoomHeader from '../components/auction/RoomHeader';
import ActiveProduct from '../components/auction/ActiveProduct';
import QueueList from '../components/auction/QueueList';
import Participants from '../components/auction/Participants';
import Ranking from '../components/auction/Ranking';
import BidHistory from '../components/auction/BidHistory';

export default function AuctionRoomPage() {
  const mock = useMemo(
    () => ({
      room: { host: 'username', status: 'RUNNING', countdown: '12:34' },
      product: {
        name: 'Product Name',
        description: 'Premium headphones with noise cancelling.',
        startPrice: '$300',
        currentPrice: '$350',
        leading: 'user123',
        minStep: '$10',
        image: 'https://images.unsplash.com/photo-1512310604669-443f26c35f52?auto=format&fit=crop&w=500&q=80',
      },
      queue: [
        { id: 1, name: 'Second Product', startPrice: '$100', price: '$100', image: 'https://images.unsplash.com/photo-1523275335684-37898b6baf30?auto=format&fit=crop&w=200&q=80' },
        { id: 2, name: 'Third Product', startPrice: '$200', price: '$200', image: 'https://images.unsplash.com/photo-1517245386807-bb43f82c33c4?auto=format&fit=crop&w=200&q=80' },
        { id: 3, name: 'Fourth Product', startPrice: '$150', price: '$150', image: 'https://images.unsplash.com/photo-1526170375885-4d8ecf77b99f?auto=format&fit=crop&w=200&q=80' },
      ],
      participants: [
        { id: 1, name: 'user789', online: true, host: false },
        { id: 2, name: 'user456', online: true, host: true },
        { id: 3, name: 'user123', online: true, host: false },
        { id: 4, name: 'user000', online: false, host: false },
      ],
      ranking: [
        { id: 1, name: 'user123', amount: '$350' },
        { id: 2, name: 'user456', amount: '$340' },
        { id: 3, name: 'user189', amount: '$330' },
      ],
      history1: [
        { id: 1, user: 'user123', amount: '$350', time: '08:45' },
        { id: 2, user: 'user456', amount: '$340', time: '08:42' },
        { id: 3, user: 'user789', amount: '$400', time: '08:41' },
      ],
      history2: [
        { id: 4, user: '+2', action: '', amount: '', time: '08:42' },
        { id: 5, user: '+$50', action: '', amount: '', time: '08:42' },
      ],
    }),
    []
  );

  return (
    <div className="min-h-screen bg-slate-100 p-4 md:p-6">
      <div className="max-w-5xl mx-auto space-y-4">
        <RoomHeader room={mock.room} />

        <div className="grid lg:grid-cols-3 gap-4">
          <div className="space-y-4">
            <QueueList items={mock.queue} />
            <Ranking items={mock.ranking} />
          </div>

          <div>
            <ActiveProduct product={mock.product} />
          </div>

          <div className="space-y-4">
            <Participants users={mock.participants} />
            <BidHistory title="Bid History" items={mock.history1} />
            <BidHistory title="Bid History +" items={mock.history2} />
          </div>
        </div>
      </div>
    </div>
  );
}

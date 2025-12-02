import React from 'react';
import Card from './Card';

export default function ActiveProduct({ product }) {
  return (
    <Card>
      <div className="flex flex-col items-center gap-3">
        <div className="w-48 h-48 rounded-xl bg-slate-100 overflow-hidden">
          <img src={product.image} alt={product.name} className="w-full h-full object-cover" />
        </div>
        <div className="w-full space-y-1">
          <h2 className="text-lg font-semibold text-slate-900">{product.name}</h2>
          <p className="text-sm text-slate-500">
            Start Price: <span className="font-semibold text-slate-800">{product.startPrice}</span>
          </p>
          <p className="text-sm text-slate-500">
            Current Price: <span className="font-semibold text-slate-800">{product.currentPrice}</span>
          </p>
          <p className="text-sm text-slate-500">
            Leading User: <span className="font-semibold text-slate-800">{product.leading}</span>
          </p>
        </div>
        <div className="w-full flex gap-2">
          <input
            type="number"
            className="flex-1 border border-slate-200 rounded-lg px-3 py-2 text-sm"
            placeholder="Enter bid amount"
          />
          <button className="px-4 py-2 bg-blue-600 hover:bg-blue-700 text-white rounded-lg text-sm font-semibold">
            Bid Now
          </button>
        </div>
      </div>
    </Card>
  );
}

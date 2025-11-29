import React, { useEffect, useState } from 'react';
import { getProducts } from '../api/product';

export default function ProductListPage() {
  const [products, setProducts] = useState([]);
  const [loading, setLoading] = useState(true);
  const [error, setError] = useState('');

  useEffect(() => {
    const load = async () => {
      try {
        setLoading(true);
        setError('');
        const data = await getProducts();
        setProducts(Array.isArray(data) ? data : []);
      } catch (err) {
        setError(err.message);
      } finally {
        setLoading(false);
      }
    };

    load();
  }, []);

  return (
    <div style={{ background: '#fff', padding: 20, borderRadius: 10, boxShadow: '0 10px 30px rgba(0,0,0,0.05)' }}>
      <div style={{ display: 'flex', alignItems: 'center', justifyContent: 'space-between' }}>
        <h2 style={{ marginTop: 0 }}>Products</h2>
      </div>
      {loading && <p>Loading products...</p>}
      {error && <p style={{ color: '#dc2626' }}>{error}</p>}
      {!loading && !error && (
        <div style={{ overflowX: 'auto' }}>
          <table style={{ width: '100%', borderCollapse: 'collapse' }}>
            <thead>
              <tr style={{ textAlign: 'left', borderBottom: '2px solid #e5e7eb' }}>
                <th style={{ padding: '8px 6px' }}>ID</th>
                <th style={{ padding: '8px 6px' }}>Name</th>
                <th style={{ padding: '8px 6px' }}>Start Price</th>
                <th style={{ padding: '8px 6px' }}>Buy Price</th>
                <th style={{ padding: '8px 6px' }}>Step</th>
                <th style={{ padding: '8px 6px' }}>Description</th>
              </tr>
            </thead>
            <tbody>
              {products.length === 0 ? (
                <tr>
                  <td colSpan="6" style={{ padding: 12, textAlign: 'center', color: '#6b7280' }}>
                    No products yet.
                  </td>
                </tr>
              ) : (
                products.map((p) => {
                  const id = p.id ?? p.product_id ?? p.productId ?? p.name;
                  const startPrice =
                    p.startPrice ??
                    p.start_price ??
                    p.startprice ??
                    p.start ??
                    p.startingPrice ??
                    p.starting_price;
                  const buyPrice = p.buyPrice ?? p.buy_price ?? p.buyprice ?? p.buy ?? p.buyNowPrice ?? p.buy_now_price;
                  const step = p.step ?? p.step_price ?? p.stepPrice ?? p.stepprice;
                  const description = p.description ?? p.desc;
                  return (
                    <tr key={id} style={{ borderBottom: '1px solid #f3f4f6' }}>
                      <td style={{ padding: '10px 6px' }}>{id}</td>
                      <td style={{ padding: '10px 6px' }}>{p.name}</td>
                      <td style={{ padding: '10px 6px' }}>{startPrice ?? '—'}</td>
                      <td style={{ padding: '10px 6px' }}>{buyPrice ?? '—'}</td>
                      <td style={{ padding: '10px 6px' }}>{step ?? '—'}</td>
                      <td style={{ padding: '10px 6px', maxWidth: 320 }}>{description}</td>
                    </tr>
                  );
                })
              )}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}

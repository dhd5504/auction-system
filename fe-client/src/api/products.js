const BASE = 'http://localhost:8080/api';

function authHeader() {
  const token = localStorage.getItem('access_token');
  return token ? { Authorization: `Bearer ${token}` } : {};
}

async function handleResponse(response) {
  if (!response.ok) {
    const text = await response.text();
    throw new Error(text || `HTTP ${response.status}`);
  }
  const ct = response.headers.get('content-type') || '';
  return ct.includes('application/json') ? response.json() : response.text();
}

// Owner APIs
export async function getMyProducts() {
  const res = await fetch(`${BASE}/me/products`, { headers: { ...authHeader() } });
  return handleResponse(res);
}

export async function getMyProduct(id) {
  const res = await fetch(`${BASE}/me/products/${id}`, { headers: { ...authHeader() } });
  return handleResponse(res);
}

export async function createProduct(body) {
  const res = await fetch(`${BASE}/me/products`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', ...authHeader() },
    body: JSON.stringify(body),
  });
  return handleResponse(res);
}

export async function updateProduct(id, body) {
  const res = await fetch(`${BASE}/me/products/${id}`, {
    method: 'PUT',
    headers: { 'Content-Type': 'application/json', ...authHeader() },
    body: JSON.stringify(body),
  });
  return handleResponse(res);
}

export async function deleteProduct(id) {
  const res = await fetch(`${BASE}/me/products/${id}`, {
    method: 'DELETE',
    headers: { ...authHeader() },
  });
  return handleResponse(res);
}

export async function updateProductStatus(id, status) {
  const res = await fetch(`${BASE}/me/products/${id}/status`, {
    method: 'PATCH',
    headers: { 'Content-Type': 'application/json', ...authHeader() },
    body: JSON.stringify({ status }),
  });
  return handleResponse(res);
}

// Public APIs
export async function getAllProducts() {
  const res = await fetch(`${BASE}/products`);
  return handleResponse(res);
}

export async function getProduct(id) {
  const res = await fetch(`${BASE}/products/${id}`);
  return handleResponse(res);
}

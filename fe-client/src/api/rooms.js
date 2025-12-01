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
export async function getMyRooms() {
  const res = await fetch(`${BASE}/me/rooms`, { headers: { ...authHeader() } });
  return handleResponse(res);
}

export async function getMyRoom(id) {
  const res = await fetch(`${BASE}/me/rooms/${id}`, { headers: { ...authHeader() } });
  return handleResponse(res);
}

export async function createRoom(body) {
  const res = await fetch(`${BASE}/me/rooms`, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', ...authHeader() },
    body: JSON.stringify(body),
  });
  return handleResponse(res);
}

export async function deleteRoom(id) {
  const res = await fetch(`${BASE}/me/rooms/${id}`, {
    method: 'DELETE',
    headers: { ...authHeader() },
  });
  return handleResponse(res);
}

// Public APIs
export async function getAllRooms() {
  const res = await fetch(`${BASE}/rooms`);
  return handleResponse(res);
}

export async function getRoom(id) {
  const res = await fetch(`${BASE}/rooms/${id}`);
  return handleResponse(res);
}

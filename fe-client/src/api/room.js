const ROOM_BASE = 'http://localhost:8080/api/rooms';

async function handleResponse(response) {
  if (!response.ok) {
    const text = await response.text();
    throw new Error(text || `Request failed with status ${response.status}`);
  }
  const contentType = response.headers.get('content-type') || '';
  if (contentType.includes('application/json')) {
    return response.json();
  }
  const text = await response.text();
  return text || null;
}

export async function createRoom(room) {
  const response = await fetch(ROOM_BASE, {
    method: 'POST',
    headers: { 'Content-Type': 'application/json' },
    body: JSON.stringify(room),
  });
  return handleResponse(response);
}

export async function getRooms() {
  const response = await fetch(ROOM_BASE);
  return handleResponse(response);
}

export async function startRoom(roomId) {
  const response = await fetch(`${ROOM_BASE}/${roomId}/start`, {
    method: 'POST',
  });
  return handleResponse(response);
}

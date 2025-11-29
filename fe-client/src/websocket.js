const WS_URL = 'ws://localhost:8081/ws';

export function connectWebSocket(onEvent) {
  const socket = new WebSocket(WS_URL);

  socket.onopen = () => {
    console.info('[WebSocket] Connected to', WS_URL);
  };

  socket.onmessage = (event) => {
    try {
      const data = JSON.parse(event.data);
      console.info('[WebSocket] event', data);
      if (typeof onEvent === 'function') {
        onEvent(data);
      }
    } catch (err) {
      console.error('[WebSocket] parse error', err);
      if (typeof onEvent === 'function') {
        onEvent({ type: 'raw', payload: event.data });
      }
    }
  };

  socket.onerror = (err) => {
    console.error('[WebSocket] error', err);
  };

  socket.onclose = () => {
    console.warn('[WebSocket] closed');
  };

  return socket;
}

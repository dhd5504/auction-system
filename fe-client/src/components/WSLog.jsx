import React, { useEffect, useState } from "react";

export default function WSLog() {
  const [messages, setMessages] = useState([]);

  useEffect(() => {
    const ws = new WebSocket("ws://localhost:9001");

    ws.onmessage = (msg) => {
      setMessages((prev) => [...prev, msg.data]);
    };

    return () => ws.close();
  }, []);

  return (
    <div style={{ background: "#111", color: "#0f0", padding: 10, height: 150, overflow: "auto", fontSize: 14 }}>
      <div>WebSocket Messages:</div>
      {messages.map((m, i) => (
        <div key={i}>• {m}</div>
      ))}
    </div>
  );
}

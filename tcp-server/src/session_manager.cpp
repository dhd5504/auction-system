#include "session_manager.h"

SessionManager::SessionManager() : nextSession(1000) {}

uint32_t SessionManager::createSession(int userId) {
    uint32_t id = nextSession.fetch_add(1);
    std::lock_guard<std::mutex> lock(mutex);
    sessions[id] = userId;
    return id;
}

std::optional<int> SessionManager::userForSession(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(mutex);
    auto it = sessions.find(sessionId);
    if (it == sessions.end()) return std::nullopt;
    return it->second;
}

void SessionManager::invalidate(uint32_t sessionId) {
    std::lock_guard<std::mutex> lock(mutex);
    sessions.erase(sessionId);
}

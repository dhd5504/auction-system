#pragma once

#include <atomic>
#include <optional>
#include <unordered_map>
#include <mutex>
#include <cstdint>

class SessionManager {
public:
    SessionManager();

    uint32_t createSession(int userId);
    std::optional<int> userForSession(uint32_t sessionId);
    void invalidate(uint32_t sessionId);

private:
    std::atomic<uint32_t> nextSession;
    std::unordered_map<uint32_t, int> sessions;
    mutable std::mutex mutex;
};

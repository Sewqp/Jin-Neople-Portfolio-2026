#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include "Session.h"

class Room {
public:
    static constexpr int MAX_PLAYERS = 4;

    Room(uint32_t roomId);

    bool     Enter(std::shared_ptr<Session> session);
    void     Leave(uint64_t sessionId);
    void     Broadcast(char* data, size_t size);
    uint32_t GetRoomId() const;
    int      GetPlayerCount() const;

private:
    uint32_t m_roomId;
    std::unordered_map<uint64_t, std::shared_ptr<Session>> m_sessions;
    mutable std::shared_mutex m_lock;
};

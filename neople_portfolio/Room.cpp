#include "Room.h"
#include "AsyncLogger.h"

Room::Room(uint32_t roomId)
    : m_roomId(roomId) {
}

bool Room::Enter(std::shared_ptr<Session> session) {
    std::unique_lock lock(m_lock);
    if (m_sessions.size() >= MAX_PLAYERS) {
        return false;
    }
    m_sessions[session->GetId()] = session;
    AsyncLogger::GetInstance().Log(
        "Room " + std::to_string(m_roomId) +
        " ÀÔÀå. SessionID: " + std::to_string(session->GetId()));
    return true;
}

void Room::Leave(uint64_t sessionId) {
    std::unique_lock lock(m_lock);
    m_sessions.erase(sessionId);
    AsyncLogger::GetInstance().Log(
        "Room " + std::to_string(m_roomId) +
        " ÅðÀå. SessionID: " + std::to_string(sessionId));
}

void Room::Broadcast(char* data, size_t size) {
    std::shared_lock lock(m_lock);
    for (const auto& pair : m_sessions) {
        pair.second->PostSend(data, size);
    }
}

uint32_t Room::GetRoomId() const {
    return m_roomId;
}

int Room::GetPlayerCount() const {
    std::shared_lock lock(m_lock);
    return static_cast<int>(m_sessions.size());
}

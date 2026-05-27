#include "SessionManager.h"
#include "Session.h"

void SessionManager::AddSession(std::shared_ptr<Session> session) {
    std::unique_lock<std::shared_mutex> writeLock(m_lock);
    m_sessions[session->GetId()] = session;
}

void SessionManager::RemoveSession(uint64_t sessionId) {
    std::unique_lock<std::shared_mutex> writeLock(m_lock);
    m_sessions.erase(sessionId);
}

void SessionManager::Broadcast(char* data, size_t size) {
    std::shared_lock<std::shared_mutex> readLock(m_lock);
    for (auto& pair : m_sessions) {
        pair.second->PostSend(data, size);
    }
}

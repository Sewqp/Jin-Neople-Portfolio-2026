#include "SessionManager.h"
#include "Session.h" // Session의 실제 정의가 필요함

void SessionManager::AddSession(std::shared_ptr<Session> session) {
    // [쓰기 락] 나 혼자만 쓸 거야! 다른 스레드 다 멈춰!
    std::unique_lock<std::shared_mutex> writeLock(m_lock);

    // session 내부의 고유 ID를 키값으로 맵에 등록
    m_sessions[session->GetId()] = session;
}

void SessionManager::RemoveSession(uint64_t sessionId) {
    // [쓰기 락] 나 혼자만 지울 거야! 다른 스레드 다 멈춰!
    std::unique_lock<std::shared_mutex> writeLock(m_lock);

    m_sessions.erase(sessionId); // map은 없는 걸 지워도 에러가 안 나서 안전함
}

void SessionManager::Broadcast(char* data, size_t size) {
    // [읽기 락] 내용 고치는 거 아니니까, 읽으러 온 스레드끼리는 다 같이 들어와!
    std::shared_lock<std::shared_mutex> readLock(m_lock);

    for (auto& pair : m_sessions) {
        pair.second->PostSend(data, size); // 모든 유저에게 패킷 전송 예약
    }
}
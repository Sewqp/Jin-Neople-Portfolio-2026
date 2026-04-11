#pragma once
#include <cstdint>
#include <unordered_map>
#include <memory>
#include <shared_mutex> // 필수!

class Session; // 전방 선언

class SessionManager {
public:
    // [설명: Magic Static을 활용한 스레드 세이프 싱글톤 반환]
    static SessionManager& GetInstance() {
        static SessionManager instance;
        return instance;
    }

    void AddSession(std::shared_ptr<Session> session);
    void RemoveSession(uint64_t sessionId);
    void Broadcast(char* data, size_t size);

private:
    // 싱글톤이므로 외부에서 생성/복사 금지
    SessionManager() = default;
    ~SessionManager() = default;
    SessionManager(const SessionManager&) = delete;
    SessionManager& operator=(const SessionManager&) = delete;

    std::unordered_map<uint64_t, std::shared_ptr<Session>> m_sessions;
    std::shared_mutex m_lock; // C++17 읽기/쓰기 락
};
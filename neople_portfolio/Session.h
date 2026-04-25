#pragma once
#include <winsock2.h>
#include <windows.h>
#include <memory>
#include <array>
#include <queue>    // 👈 큐 추가
#include <vector>   // 👈 벡터 추가
#include <mutex>    // 👈 자물쇠 추가
#include <atomic>   // 👈 원자적 연산 추가


enum class IO_TYPE { RECV, SEND, ACCEPT };

struct ExOverlapped {
    OVERLAPPED base;
    IO_TYPE io_type;
    WSABUF wsa_buf;
    std::shared_ptr<class Session> keepAlive; // OS 완료 통보까지 세션 생존 보장
};

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(SOCKET sock);
    ~Session();

    uint64_t GetId() const { return m_sessionId; }

    void PostRecv();
    void PostSend(char* data, size_t size);

    // [설명: IOCP에서 Send가 '완료'되었을 때 호출할 함수 (나중에 IocpCore에서 씀)]

    void SendCompleted();
    void OnRecvCompleted(int bytes);

private:
    void RegisterSend(); // 👈 실제 WSASend를 호출하는 내부 비공개 함수

private:
    SOCKET m_sock;
    uint64_t m_sessionId = 0;

    std::array<char, 8192> m_recvBuffer;
    ExOverlapped m_recvOverlapped;
    ExOverlapped m_sendOverlapped;

    // 🚨 [새로 추가된 Send 전용 방어 장치들] 🚨
    std::mutex m_sendLock;                         // 큐를 보호할 자물쇠
    std::queue<std::vector<char>> m_sendQueue;     // 안전하게 복사된 패킷들이 대기하는 큐
    std::atomic<bool> m_isSending{ false };        // 지금 OS가 전송 작업을 하고 있는지 체크하는 깃발
};
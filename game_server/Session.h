#pragma once
#include <winsock2.h>
#include <windows.h>
#include <memory>
#include <array>
#include <queue>
#include <vector>
#include <mutex>
#include <atomic>
#include "RingBuffer.h"

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

    void SendCompleted();
    void OnRecvCompleted(int bytes);

private:
    void RegisterSend();

private:
    SOCKET   m_sock;
    uint64_t m_sessionId = 0;

    std::array<char, 8192> m_recvBuffer;
    RingBuffer             m_ringBuffer;
    ExOverlapped m_recvOverlapped;
    ExOverlapped m_sendOverlapped;

    std::mutex                   m_sendLock;
    std::queue<std::vector<char>> m_sendQueue;
    std::atomic<bool>            m_isSending{ false };
};

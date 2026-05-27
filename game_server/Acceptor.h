#pragma once
#include <winsock2.h>
#include <windows.h>
#include <memory>
#include "Session.h"

class IocpCore;

struct AcceptOverlapped {
    OVERLAPPED base;           
    IO_TYPE    io_type;
    SOCKET     clientSock;
    char       addrBuf[64 * 2];
};

class Acceptor {
public:
    explicit Acceptor(IocpCore& iocpCore);
    ~Acceptor();

    void Init(uint16_t port);
    void OnAcceptCompleted(AcceptOverlapped* over);

private:
    void RegisterAccept();

    static constexpr int LISTEN_BACKLOG   = SOMAXCONN; // OS TCP 대기열 최대
    static constexpr int PENDING_ACCEPTS  = 64;        // 사전 예약할 AcceptEx 수
    static constexpr int ADDR_BUFFER_SIZE = 64;

    IocpCore& m_iocpCore;
    SOCKET    m_listenSock = INVALID_SOCKET;
};
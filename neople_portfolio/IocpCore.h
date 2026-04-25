#pragma once
#include <cstdint>
#include <vector>
#include <thread>
#include <atomic>
#include <winsock2.h>
#include <windows.h>
#include "Acceptor.h"

class IocpCore {
public:
    IocpCore();
    ~IocpCore();

    void Init();
    void Start();
    void SetAcceptor(Acceptor* acceptor) { m_acceptor = acceptor; }

    // [설명: IOCP 핸들 반환 (나중에 SessionManager가 소켓을 묶을 때 사용)]
    HANDLE GetHandle() { return m_hIocp; }

private:
    HANDLE m_hIocp;
    std::vector<std::thread> m_workerThreads;

    // [설명: 서버가 실행 중인지 체크하는 스레드 안전 깃발]
    std::atomic<bool> m_isRunning;

    // [설명: 워커 스레드들이 실제로 뺑뺑이 도는 무한 루프 함수]
    void WorkerThreadMain();

    Acceptor* m_acceptor = nullptr;
};
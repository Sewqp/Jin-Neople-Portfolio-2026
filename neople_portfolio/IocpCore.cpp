#include "IocpCore.h"
#include <iostream>

IocpCore::IocpCore() : m_hIocp(INVALID_HANDLE_VALUE), m_isRunning(false) {}

IocpCore::~IocpCore() {
    // 1. 서버 종료 깃발을 내림
    m_isRunning = false;

    // 2. 대기 중인 종업원(스레드)들을 깨우기 위해 가짜 알림(Post)을 보냄
    if (m_hIocp != INVALID_HANDLE_VALUE) {
        for (size_t i = 0; i < m_workerThreads.size(); ++i) {
            PostQueuedCompletionStatus(m_hIocp, 0, 0, NULL);
        }
    }

    // 3. 종업원들이 하던 일을 다 마치고 퇴근할 때까지 기다려줌 (join)
    for (auto& thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // 4. 식당 문 닫기
    if (m_hIocp != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hIocp);
    }
}

void IocpCore::Init() {
    // 윈도우 OS에게 비동기 알림판(IOCP) 생성을 요청
    m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
}

void IocpCore::Start() {
    m_isRunning = true;

    // 내 컴퓨터의 논리 코어 수 x 2 만큼 종업원을 고용 (가장 이상적인 비율)
    const int numThreads = std::thread::hardware_concurrency() * 2;
    for (int i = 0; i < numThreads; ++i) {
        m_workerThreads.emplace_back(&IocpCore::WorkerThreadMain, this);
    }
}

void IocpCore::WorkerThreadMain() {
    while (m_isRunning) {
        DWORD bytesTransferred = 0;
        ULONG_PTR completionKey = 0;
        LPOVERLAPPED overlapped = nullptr;

        // [핵심] 알림판 앞에서 무한 대기 (INFINITE)
        BOOL ret = GetQueuedCompletionStatus(
            m_hIocp,
            &bytesTransferred,
            &completionKey,
            &overlapped,
            INFINITE
        );

        // 서버 종료를 위해 억지로 깨운 경우 탈출
        if (m_isRunning == false) {
            break;
        }

        if (ret == FALSE) {
            // 유저 강제 종료 등의 에러 처리 로직
            continue;
        }

        // TODO: completionKey를 Session* 으로 캐스팅해서 데이터 처리!
        // 어제 만든 ExOverlapped의 io_type을 확인해서 RECV/SEND 분기 처리!
    }
}
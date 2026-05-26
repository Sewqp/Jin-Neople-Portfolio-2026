#include "IocpCore.h"
#include "AsyncLogger.h"
#include "SessionManager.h"
#include "Session.h"
#include "Acceptor.h"

IocpCore::IocpCore() : m_hIocp(INVALID_HANDLE_VALUE), m_isRunning(false) {}

IocpCore::~IocpCore() {
    // [서버 종료 깃발 내림]
    m_isRunning = false;

    // [대기 중인 워커 스레드들을 깨우기 위해 가짜 알림 전송]
    if (m_hIocp != INVALID_HANDLE_VALUE) {
        for (size_t i = 0; i < m_workerThreads.size(); ++i) {
            PostQueuedCompletionStatus(m_hIocp, 0, 0, NULL);
        }
    }

    // [모든 스레드가 종료될 때까지 대기]
    for (auto& thread : m_workerThreads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    // [IOCP 핸들 닫기]
    if (m_hIocp != INVALID_HANDLE_VALUE) {
        CloseHandle(m_hIocp);
    }
}

void IocpCore::Init() {
    // [IOCP 알림판 생성]
    m_hIocp = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
    if (m_hIocp == INVALID_HANDLE_VALUE) {
        AsyncLogger::GetInstance().LogError("IOCP 생성 실패");
    }
}

void IocpCore::Start() {
    m_isRunning = true;

    // [논리 코어 수 × 2만큼 워커 스레드 생성]
    const int numThreads = std::thread::hardware_concurrency() * 2;
    for (int i = 0; i < numThreads; ++i) {
        m_workerThreads.emplace_back(&IocpCore::WorkerThreadMain, this);
    }
}

void IocpCore::WorkerThreadMain() {
    while (m_isRunning) {
        DWORD      bytesTransferred = 0;
        ULONG_PTR  completionKey = 0;
        LPOVERLAPPED overlapped = nullptr;

        // [알림판 앞에서 무한 대기]
        BOOL ret = GetQueuedCompletionStatus(
            m_hIocp,
            &bytesTransferred,
            &completionKey,
            &overlapped,
            INFINITE
        );

        // [종료 신호 확인]
        if (!m_isRunning) break;

        try {
            // [overlapped가 nullptr이면 에러]
            if (overlapped == nullptr) {
                AsyncLogger::GetInstance().LogError("overlapped가 nullptr입니다.");
                continue;
            }

            ExOverlapped* exOver = reinterpret_cast<ExOverlapped*>(overlapped);

            // [ACCEPT 완료 처리]
            if (exOver->io_type == IO_TYPE::ACCEPT) {
                AcceptOverlapped* acceptOver = reinterpret_cast<AcceptOverlapped*>(exOver);
                m_acceptor->OnAcceptCompleted(acceptOver);
                continue;
            }

            // [연결 끊김 처리 — bytesTransferred == 0]
            if (bytesTransferred == 0) {
                std::shared_ptr<Session> session = exOver->keepAlive;
                if (session) {
                    SessionManager::GetInstance().RemoveSession(session->GetId());
                }
                continue;
            }

            // [RECV / SEND 분기]
            std::shared_ptr<Session> session = exOver->keepAlive;
            if (!session) continue;

            switch (exOver->io_type) {
            case IO_TYPE::RECV:
                session->OnRecvCompleted(static_cast<int>(bytesTransferred));
                session->PostRecv();
                break;
            case IO_TYPE::SEND:
                session->SendCompleted();
                break;
            default:
                AsyncLogger::GetInstance().LogError("알 수 없는 IO_TYPE입니다.");
                break;
            }
        }
        catch (const std::exception& e) {
            AsyncLogger::GetInstance().LogError(
                "WorkerThread 예외 발생: " + std::string(e.what()));
        }
    }
}
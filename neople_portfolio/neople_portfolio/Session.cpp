#include "Session.h"
#include <iostream>

extern std::atomic<uint64_t> GSessionIdAllocator; // (아까 추가했던 전역 ID 발급기)

// ... (생성자, 소멸자, PostRecv는 아까 짠 그대로 유지) ...

void Session::PostSend(char* data, size_t size) {
    // 1. [메모리 증발 방지] 외부의 불안정한 char* 데이터를 내 안전한 vector로 딥카피(복사)!
    std::vector<char> sendData(data, data + size);

    {
        // 2. 여러 스레드가 동시에 Send를 요청할 수 있으니 자물쇠 채우고 큐에 밀어넣기
        std::lock_guard<std::mutex> lock(m_sendLock);
        m_sendQueue.push(std::move(sendData));
    }

    // 3. [Overlapped 덮어쓰기 방지] 
    // "아무도 전송 중이 아니면(false), 내가 깃발을 들고(true) 전송을 시작하겠다!"
    if (m_isSending.exchange(true) == false) {
        RegisterSend();
    }
}

void Session::RegisterSend() {
    std::lock_guard<std::mutex> lock(m_sendLock);

    // 보낼 게 없으면 깃발 내리고 퇴근
    if (m_sendQueue.empty()) {
        m_isSending = false;
        return;
    }

    // 🚨 핵심: pop()으로 빼버리면 안 됨! OS가 다 쏠 때까지 메모리가 살아있어야 하니까 front()로 앞면만 봄
    std::vector<char>& data = m_sendQueue.front();

    WSABUF wsaBuf;
    wsaBuf.buf = data.data();
    wsaBuf.len = static_cast<ULONG>(data.size());

    // Send Overlapped 초기화 및 세팅
    ZeroMemory(&m_sendOverlapped, sizeof(OVERLAPPED)); // shared_ptr까지 밀면 안 되므로 base만 초기화
    m_sendOverlapped.io_type = IO_TYPE::SEND;

    // 비동기 작업 중 세션 파괴 방지 (Overlapped가 살아있는 한 세션도 생존)
    m_sendOverlapped.keepAlive = shared_from_this();

    DWORD sendBytes = 0;

    // 윈도우 OS에게 전송 예약!
    int result = WSASend(m_sock, &wsaBuf, 1, &sendBytes, 0,
        reinterpret_cast<LPWSAOVERLAPPED>(&m_sendOverlapped), NULL);

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error != WSA_IO_PENDING) {
            // 진짜 에러가 났다면 전송 실패이므로 깃발을 다시 내림
            m_isSending = false;
            // TODO: 연결 끊기 처리
            AsyncLogger::GetInstance().Log("WSASend Failed. Error Code: " + std::to_string(error));
        }
    }
}

void Session::SendCompleted() {
    // OS가 "아까 맡긴 데이터 전송 끝났어!"라고 알려주면 호출됨
    {
        std::lock_guard<std::mutex> lock(m_sendLock);
        // 이제 진짜로 다 보냈으니 큐에서 삭제해서 메모리 해제!
        m_sendQueue.pop();
    }

    // 큐에 아직 보낼 게 남아있다면 연쇄적으로 다시 전송 시작
    RegisterSend();
}
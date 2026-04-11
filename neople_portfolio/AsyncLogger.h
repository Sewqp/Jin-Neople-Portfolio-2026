#pragma once
#include <string>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

class AsyncLogger {
public:
    // [싱글톤 인스턴스 반환]
    static AsyncLogger& GetInstance() {
        static AsyncLogger instance;
        return instance;
    }

    // [논블로킹 로그 — 큐에 push하고 즉시 리턴]
    void Log(const std::string& message);

    // [에러 로그 — 큐에 push + AI 전송]
    void LogError(const std::string& message);

private:
    AsyncLogger();
    ~AsyncLogger();
    AsyncLogger(const AsyncLogger&) = delete;
    AsyncLogger& operator=(const AsyncLogger&) = delete;

    // [로거 스레드 루프 — 큐에서 꺼내 파일에 기록]
    void ProcessLoop();

    // [AI 전송 — HTTP 전송은 추후 구현]
    void SendToAI(const std::string& message);

    // [현재 시각을 "[YYYY-MM-DD HH:MM:SS]" 형식으로 반환]
    std::string GetTimestamp() const;

    // [오늘 날짜 기준 로그 파일명 반환 "logs/server_YYYYMMDD.log"]
    std::string GetLogFileName() const;

    std::queue<std::string>    m_logQueue;
    std::mutex                 m_mutex;
    std::condition_variable    m_cv;
    std::thread                m_loggerThread;
    std::atomic<bool>          m_isRunning{ true };
};
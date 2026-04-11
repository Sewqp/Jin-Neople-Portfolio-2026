#include "AsyncLogger.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <filesystem>

AsyncLogger::AsyncLogger() {
    // [생성자에서 바로 로거 스레드 시작 — 싱글톤이라 외부에서 Start() 호출 불필요]
    m_loggerThread = std::thread(&AsyncLogger::ProcessLoop, this);
}

AsyncLogger::~AsyncLogger() {
    {
        // [종료 깃발 내리고 스레드 깨움]
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isRunning = false;
    }
    m_cv.notify_all(); // [대기 중인 스레드 깨우기]

    if (m_loggerThread.joinable()) {
        m_loggerThread.join(); // [스레드 완전 종료 대기]
    }
}

void AsyncLogger::Log(const std::string& message) {
    {
        // [mutex로 큐 보호 — 멀티스레드 안전]
        std::lock_guard<std::mutex> lock(m_mutex);
        m_logQueue.push(GetTimestamp() + message);
    }
    m_cv.notify_one(); // [로거 스레드 깨우기]
}

void AsyncLogger::LogError(const std::string& message) {
    Log(message);
    SendToAI(message);
}

void AsyncLogger::ProcessLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(m_mutex);

        // [큐에 데이터가 있거나 종료 신호가 오면 깨어남]
        m_cv.wait(lock, [this] {
            return !m_logQueue.empty() || !m_isRunning;
            });

        // [종료 신호 + 큐가 비었으면 루프 탈출]
        if (!m_isRunning && m_logQueue.empty()) {
            break;
        }

        // [큐에서 꺼내기]
        std::string message = m_logQueue.front();
        m_logQueue.pop();
        lock.unlock(); // [파일 I/O 중에는 락 불필요 — 먼저 해제]

        // [날짜별 로그 파일에 기록]
        std::filesystem::create_directories("logs");
        std::ofstream logFile(GetLogFileName(),
            std::ios_base::app | std::ios_base::out);
        if (logFile.is_open()) {
            logFile << message << "\n";
        }
    }
}

void AsyncLogger::SendToAI(const std::string& message) {
    // TODO: HTTP POST로 로컬 AI 서버에 에러 메시지 전송
    // 예: POST http://localhost:11434/api/analyze { "error": message }
}

std::string AsyncLogger::GetTimestamp() const {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream oss;
    oss << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] ";
    return oss.str();
}

std::string AsyncLogger::GetLogFileName() const {
    std::time_t now = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif
    std::ostringstream oss;
    oss << "logs/server_" << std::put_time(&tm, "%Y%m%d") << ".log";
    return oss.str();
}
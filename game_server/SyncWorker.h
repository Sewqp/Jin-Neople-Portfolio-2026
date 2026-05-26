#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

class SyncWorker {
public:
    static SyncWorker& GetInstance();

    void Start();
    void Stop();

private:
    SyncWorker() = default;
    ~SyncWorker();
    SyncWorker(const SyncWorker&) = delete;
    SyncWorker& operator=(const SyncWorker&) = delete;

    static constexpr int SYNC_INTERVAL_SECONDS = 30;

    std::thread            m_syncThread;
    std::atomic<bool>      m_isRunning{ false };
    std::mutex             m_mutex;
    std::condition_variable m_cv;

    void SyncLoop();
    void FlushAll();
};

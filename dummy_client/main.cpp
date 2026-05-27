#pragma comment(lib, "ws2_32.lib")

#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <latch>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstring>

#include "../game_server/Packet.h"
#include "DummyClient.h"

// -------------------------------------------------------
// 설정
// -------------------------------------------------------
constexpr int      TARGET_CONNECTIONS = 10000;
constexpr int      PACKETS_PER_CLIENT = 100;
constexpr char     SERVER_IP[]        = "127.0.0.1";
constexpr uint16_t SERVER_PORT        = 9000;

// 100개씩 3ms 간격 — 300ms 램프업 (100 배치 × 3ms)
constexpr int BATCH_SIZE     = 100;
constexpr int BATCH_DELAY_MS = 3;

// 스레드 풀 크기 — 최소 32, 최대 hw_concurrency*2
// 각 스레드가 TARGET_CONNECTIONS/WORKER_THREADS 개의 소켓을 순차 관리
const int WORKER_THREADS = std::max(32, (int)std::thread::hardware_concurrency() * 2);

// -------------------------------------------------------
// 전역 통계 / 동기화
// -------------------------------------------------------
std::atomic<int>  g_connected   { 0 };
std::atomic<int>  g_failed      { 0 };
std::atomic<int>  g_packetsSent { 0 };
std::atomic<int>  g_sendErrors  { 0 };
std::atomic<bool> g_done        { false };

// 연결 페이즈 완료 후 전송 페이즈 시작
std::latch g_connectLatch(TARGET_CONNECTIONS);

// -------------------------------------------------------
// 워커 스레드
//   [페이즈 1] startId~endId 클라이언트를 순차 접속
//   [페이즈 2] latch 해제 후 모든 소켓에 패킷 전송
// -------------------------------------------------------
void WorkerThread(int workerId, std::chrono::steady_clock::time_point testStart) {
    int slice    = TARGET_CONNECTIONS / WORKER_THREADS;
    int startId  = workerId * slice;
    int endId    = (workerId == WORKER_THREADS - 1) ? TARGET_CONNECTIONS : startId + slice;

    std::vector<DummyClient> clients;
    clients.reserve(endId - startId);

    // ---- 페이즈 1: 접속 ----
    for (int clientId = startId; clientId < endId; ++clientId) {
        // 절대시각 기준 대기 — sleep_for는 호출마다 누적되므로 사용 금지
        int batchIndex = clientId / BATCH_SIZE;
        auto batchTime = testStart + std::chrono::milliseconds(batchIndex * BATCH_DELAY_MS);
        std::this_thread::sleep_until(batchTime);

        DummyClient client(clientId);
        if (client.Connect(SERVER_IP, SERVER_PORT)) {
            ++g_connected;
            clients.push_back(std::move(client));
        } else {
            ++g_failed;
        }
        g_connectLatch.count_down();
    }

    g_connectLatch.wait(); // 전체 접속 완료까지 대기

    // ---- 페이즈 2: 패킷 전송 ----
    for (auto& client : clients) {
        PKT_CharacterStat pkt{};
        pkt.header.size  = static_cast<uint16_t>(sizeof(PKT_CharacterStat));
        pkt.header.id    = PacketID::CHARACTER_STAT_INFO;
        pkt.character_id = static_cast<uint64_t>(client.GetId() % 10) + 1;
        pkt.hp_max       = 10000;
        pkt.mp_max       = 5000;
        pkt.is_alive     = Status::NORMAL;
        pkt.last_map_id  = static_cast<uint32_t>(1 + (client.GetId() % 20));

        for (int i = 0; i < PACKETS_PER_CLIENT; ++i) {
            pkt.level = static_cast<uint32_t>(1 + (i % 100));
            pkt.hp    = static_cast<uint32_t>(pkt.hp_max - (i * 10) % pkt.hp_max);
            pkt.mp    = static_cast<uint32_t>(pkt.mp_max - (i * 5)  % pkt.mp_max);

            if (!client.SendPacket(&pkt, static_cast<int>(sizeof(pkt)))) {
                ++g_sendErrors;
                break;
            }
            ++g_packetsSent;
        }
    }
}

// -------------------------------------------------------
// 모니터 스레드: 1초마다 실시간 현황 출력
// -------------------------------------------------------
void MonitorThread(std::chrono::steady_clock::time_point testStart) {
    while (!g_done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - testStart).count();

        int    sent = g_packetsSent.load();
        double pps  = elapsedMs > 0 ? (sent * 1000.0 / elapsedMs) : 0.0;

        std::cout
            << std::fixed << std::setprecision(1)
            << "[" << std::setw(5) << elapsedMs / 1000.0 << "s]"
            << "  접속: "   << std::setw(5) << g_connected.load()
            << "  실패: "   << std::setw(4) << g_failed.load()
            << "  패킷: "   << std::setw(8) << sent
            << "  pkt/s: " << std::setw(7) << static_cast<int>(pps)
            << "  오류: "   << g_sendErrors.load()
            << "\n";
    }
}

// -------------------------------------------------------
// main
// -------------------------------------------------------
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "WSAStartup 실패\n";
        return 1;
    }

    std::cout << "=== RPG 게임 서버 더미 클라이언트 ===\n";
    std::cout << "서버           : " << SERVER_IP << ":" << SERVER_PORT << "\n";
    std::cout << "클라이언트     : " << TARGET_CONNECTIONS << "\n";
    std::cout << "워커 스레드    : " << WORKER_THREADS << "\n";
    std::cout << "클라이언트당   : " << PACKETS_PER_CLIENT << " 패킷\n";
    std::cout << "예상 총 패킷   : " << TARGET_CONNECTIONS * PACKETS_PER_CLIENT << "\n";
    std::cout << "---\n";

    auto testStart = std::chrono::steady_clock::now();

    std::thread monitor(MonitorThread, testStart);

    std::vector<std::thread> workers;
    workers.reserve(WORKER_THREADS);
    for (int i = 0; i < WORKER_THREADS; ++i)
        workers.emplace_back(WorkerThread, i, testStart);

    for (auto& t : workers)
        t.join();

    g_done.store(true);
    monitor.join();

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - testStart).count();

    int     totalPkts  = g_packetsSent.load();
    double  pps        = elapsedMs > 0 ? (totalPkts * 1000.0 / elapsedMs) : 0.0;
    int64_t totalBytes = static_cast<int64_t>(totalPkts) * sizeof(PKT_CharacterStat);

    std::cout << "\n=== 최종 결과 ===\n";
    std::cout << std::fixed << std::setprecision(3);
    std::cout << "소요 시간    : " << elapsedMs / 1000.0 << " 초\n";
    std::cout << "접속 성공    : " << g_connected.load() << " / " << TARGET_CONNECTIONS << "\n";
    std::cout << "접속 실패    : " << g_failed.load()    << "\n";
    std::cout << "총 패킷 송신 : " << totalPkts          << "\n";
    std::cout << "송신 오류    : " << g_sendErrors.load() << "\n";
    std::cout << "평균 처리량  : " << static_cast<int>(pps) << " pkt/s\n";
    std::cout << "총 데이터    : " << totalBytes / 1024   << " KB ("
              << totalBytes << " bytes)\n";

    WSACleanup();
    return 0;
}

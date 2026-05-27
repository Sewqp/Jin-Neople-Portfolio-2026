#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <iostream>
#include <iomanip>
#include <thread>
#include <atomic>
#include <vector>
#include <chrono>
#include <cstring>

#include "../game_server/Packet.h"
#include "DummyClient.h"

// -------------------------------------------------------
// 설정
// -------------------------------------------------------
constexpr int      TARGET_CONNECTIONS = 1000;
constexpr int      PACKETS_PER_CLIENT = 100;
constexpr char     SERVER_IP[]        = "127.0.0.1";
constexpr uint16_t SERVER_PORT        = 9000;

// -------------------------------------------------------
// 전역 통계
// -------------------------------------------------------
std::atomic<int>  g_connected   { 0 };
std::atomic<int>  g_failed      { 0 };
std::atomic<int>  g_packetsSent { 0 };
std::atomic<int>  g_sendErrors  { 0 };
std::atomic<bool> g_done        { false };

// -------------------------------------------------------
// 워커 스레드: 접속 → 패킷 전송 → 종료
// -------------------------------------------------------
void WorkerThread(int clientId) {
    DummyClient client(clientId);

    if (!client.Connect(SERVER_IP, SERVER_PORT)) {
        ++g_failed;
        return;
    }
    ++g_connected;

    // character_id 1~10 순환 (DB에 존재하는 ID)
    uint64_t charId = static_cast<uint64_t>(clientId % 10) + 1;

    PKT_CharacterStat pkt{};
    pkt.header.size  = static_cast<uint16_t>(sizeof(PKT_CharacterStat));
    pkt.header.id    = PacketID::CHARACTER_STAT_INFO;
    pkt.character_id = charId;
    pkt.hp_max       = 10000;
    pkt.mp_max       = 5000;
    pkt.is_alive     = Status::NORMAL;
    pkt.last_map_id  = static_cast<uint32_t>(1 + (clientId % 20));

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

// -------------------------------------------------------
// 모니터 스레드: 1초마다 실시간 현황 출력
// -------------------------------------------------------
void MonitorThread(std::chrono::steady_clock::time_point startTime) {
    while (!g_done.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - startTime).count();

        int  sent = g_packetsSent.load();
        double pps = elapsedMs > 0 ? (sent * 1000.0 / elapsedMs) : 0.0;

        std::cout
            << std::fixed << std::setprecision(1)
            << "[" << std::setw(5) << elapsedMs / 1000.0 << "s]"
            << "  접속: "    << std::setw(4) << g_connected.load()
            << "  실패: "    << std::setw(3) << g_failed.load()
            << "  패킷: "    << std::setw(7) << sent
            << "  pkt/s: "  << std::setw(6) << static_cast<int>(pps)
            << "  오류: "    << g_sendErrors.load()
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
    std::cout << "서버  : " << SERVER_IP << ":" << SERVER_PORT << "\n";
    std::cout << "클라이언트  : " << TARGET_CONNECTIONS << "\n";
    std::cout << "클라이언트당 패킷  : " << PACKETS_PER_CLIENT << "\n";
    std::cout << "예상 총 패킷  : " << TARGET_CONNECTIONS * PACKETS_PER_CLIENT << "\n";
    std::cout << "---\n";

    auto startTime = std::chrono::steady_clock::now();

    std::thread monitor(MonitorThread, startTime);

    std::vector<std::thread> workers;
    workers.reserve(TARGET_CONNECTIONS);
    for (int i = 0; i < TARGET_CONNECTIONS; ++i)
        workers.emplace_back(WorkerThread, i);

    for (auto& t : workers)
        t.join();

    g_done.store(true);
    monitor.join();

    auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - startTime).count();

    int  totalPkts = g_packetsSent.load();
    double pps     = elapsedMs > 0 ? (totalPkts * 1000.0 / elapsedMs) : 0.0;
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

#include "DummyClient.h"
#include <ws2tcpip.h>

DummyClient::DummyClient(int clientId) : m_clientId(clientId) {}

DummyClient::DummyClient(DummyClient&& other) noexcept
    : m_sock(other.m_sock), m_clientId(other.m_clientId) {
    other.m_sock = INVALID_SOCKET;
}

DummyClient::~DummyClient() { Disconnect(); }

bool DummyClient::Connect(const char* ip, uint16_t port) {
    m_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sock == INVALID_SOCKET) return false;

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    inet_pton(AF_INET, ip, &addr.sin_addr);

    if (connect(m_sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) != 0) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
        return false;
    }
    return true;
}

bool DummyClient::SendPacket(const void* data, int size) {
    const char* ptr = static_cast<const char*>(data);
    int remaining   = size;
    while (remaining > 0) {
        int sent = send(m_sock, ptr, remaining, 0);
        if (sent <= 0) return false;
        ptr       += sent;
        remaining -= sent;
    }
    return true;
}

void DummyClient::Disconnect() {
    if (m_sock != INVALID_SOCKET) {
        closesocket(m_sock);
        m_sock = INVALID_SOCKET;
    }
}

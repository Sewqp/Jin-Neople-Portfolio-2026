#pragma once
#include <winsock2.h>
#include <cstdint>

class DummyClient {
public:
    explicit DummyClient(int clientId);
    ~DummyClient();

    bool Connect(const char* ip, uint16_t port);
    bool SendPacket(const void* data, int size);
    void Disconnect();

    bool IsConnected() const { return m_sock != INVALID_SOCKET; }

private:
    SOCKET m_sock    = INVALID_SOCKET;
    int    m_clientId;
};

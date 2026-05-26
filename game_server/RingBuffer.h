#pragma once
#include <cstdint>
#include <array>
#include <vector>
#include <optional>
#include "Packet.h"

class RingBuffer {
public:
    static constexpr size_t BUFFER_SIZE = 65536;

    // [데이터를 버퍼에 씀. 공간 부족 시 false 반환]
    bool Write(const char* data, size_t len);

    // [읽기 포인터 움직이지 않고 앞에서 len바이트 복사]
    bool Peek(char* dest, size_t len) const;

    // [앞에서 len바이트 읽고 읽기 포인터 전진]
    bool Read(char* dest, size_t len);

    // [읽기 포인터만 len만큼 전진 — 실제 복사 없이 버림]
    void Ignore(size_t len);

    // [읽을 수 있는 바이트 수]
    size_t GetReadableSize() const;

    // [쓸 수 있는 바이트 수]
    size_t GetWritableSize() const;

    // [PacketHeader.size만큼 모이면 완성된 패킷 반환. 아직이면 nullopt]
    std::optional<std::vector<char>> TryAssemblePacket();

private:
    std::array<char, BUFFER_SIZE> m_buffer{};
    size_t m_readPos = 0;
    size_t m_writePos = 0;
    size_t m_dataSize = 0;
};
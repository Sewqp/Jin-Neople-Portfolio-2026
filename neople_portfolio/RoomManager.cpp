#include "RoomManager.h"
#include "AsyncLogger.h"

RoomManager& RoomManager::GetInstance() {
    static RoomManager instance;
    return instance;
}

std::shared_ptr<Room> RoomManager::CreateRoom() {
    std::unique_lock lock(m_lock);
    uint32_t roomId = m_roomIdAllocator.fetch_add(1);
    auto room = std::make_shared<Room>(roomId);
    m_rooms[roomId] = room;
    AsyncLogger::GetInstance().Log(
        "Room 생성. ID: " + std::to_string(roomId));
    return room;
}

void RoomManager::DestroyRoom(uint32_t roomId) {
    std::unique_lock lock(m_lock);
    m_rooms.erase(roomId);
    AsyncLogger::GetInstance().Log(
        "Room 제거. ID: " + std::to_string(roomId));
}

std::shared_ptr<Room> RoomManager::GetRoom(uint32_t roomId) {
    std::shared_lock lock(m_lock);
    auto it = m_rooms.find(roomId);
    if (it != m_rooms.end()) {
        return it->second;
    }
    return nullptr;
}

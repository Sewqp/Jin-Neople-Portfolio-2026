#include "PacketHandler.h"
#include "AsyncLogger.h"

PacketHandler& PacketHandler::GetInstance() {
    static PacketHandler instance;
    return instance;
}

void PacketHandler::Handle(std::shared_ptr<Session> session, std::vector<char>& packet) {
    // [패킷 크기가 헤더보다 작으면 비정상 패킷]
    if (packet.size() < sizeof(PacketHeader)) {
        AsyncLogger::GetInstance().LogError(
            "패킷 크기 오류. Size: " + std::to_string(packet.size()));
        return;
    }

    // [패킷 데이터를 PacketHeader*로 캐스팅]
    PacketHeader* header = reinterpret_cast<PacketHeader*>(packet.data());

    // [패킷 ID별 분기]
    switch (header->id) {
    case PacketID::ADVENTURE_INFO:
        OnAdventureInfo(session, reinterpret_cast<PKT_Adventure*>(packet.data()));
        break;
    case PacketID::GUILD_INFO:
        OnGuildInfo(session, reinterpret_cast<PKT_Guild*>(packet.data()));
        break;
    case PacketID::CHARACTER_INFO:
        OnCharacterInfo(session, reinterpret_cast<PKT_Character*>(packet.data()));
        break;
    case PacketID::CHARACTER_STAT_INFO:
        OnCharacterStat(session, reinterpret_cast<PKT_CharacterStat*>(packet.data()));
        break;
    case PacketID::ITEM_DICTIONARY_INFO:
        OnItemDictionary(session, reinterpret_cast<PKT_ItemDictionary*>(packet.data()));
        break;
    case PacketID::ITEM_INSTANCE_INFO:
        OnItemInstance(session, reinterpret_cast<PKT_ItemInstance*>(packet.data()));
        break;
    case PacketID::INVENTORY_INFO:
        OnInventory(session, reinterpret_cast<PKT_Inventory*>(packet.data()));
        break;
    case PacketID::AUCTION_INFO:
        OnAuction(session, reinterpret_cast<PKT_Auction*>(packet.data()));
        break;
    default:
        AsyncLogger::GetInstance().LogError(
            "알 수 없는 PacketID: " + std::to_string(static_cast<uint16_t>(header->id)));
    }
}

void PacketHandler::OnAdventureInfo(std::shared_ptr<Session> session, PKT_Adventure* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: ADVENTURE_INFO");
    // TODO: 실제 처리 로직
}

void PacketHandler::OnGuildInfo(std::shared_ptr<Session> session, PKT_Guild* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: GUILD_INFO");
    // TODO: 실제 처리 로직
}

void PacketHandler::OnCharacterInfo(std::shared_ptr<Session> session, PKT_Character* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: CHARACTER_INFO");
    // TODO: 실제 처리 로직
}

void PacketHandler::OnCharacterStat(std::shared_ptr<Session> session, PKT_CharacterStat* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: CHARACTER_STAT_INFO");
    // TODO: 실제 처리 로직
}

void PacketHandler::OnItemDictionary(std::shared_ptr<Session> session, PKT_ItemDictionary* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: ITEM_DICTIONARY_INFO");
    // TODO: 실제 처리 로직
}

void PacketHandler::OnItemInstance(std::shared_ptr<Session> session, PKT_ItemInstance* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: ITEM_INSTANCE_INFO");
    // TODO: 실제 처리 로직
}

void PacketHandler::OnInventory(std::shared_ptr<Session> session, PKT_Inventory* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: INVENTORY_INFO");
    // TODO: 실제 처리 로직
}

void PacketHandler::OnAuction(std::shared_ptr<Session> session, PKT_Auction* packet) {
    AsyncLogger::GetInstance().Log("패킷 수신: AUCTION_INFO");
    // TODO: 실제 처리 로직
}
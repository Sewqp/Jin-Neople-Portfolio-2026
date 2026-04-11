#pragma once
#include <cstdint>

// ---------------------------------------------------------
// 1. Enum 정의 (파일 최상단 배치)
// ---------------------------------------------------------
enum class PacketID : uint16_t {
    ADVENTURE_INFO = 1000,
    GUILD_INFO = 1001,
    CHARACTER_INFO = 1002,
    CHARACTER_STAT_INFO = 1003,
    ITEM_DICTIONARY_INFO = 1004,
    ITEM_INSTANCE_INFO = 1005,
    INVENTORY_INFO = 1006,
    AUCTION_INFO = 1007
};

enum class Status : uint8_t { NORMAL = 0, BANNED = 1 };
enum class JobCode : uint32_t { SLAYER = 1, FIGHTER = 2, GUNNER = 3, MAGE = 4 };
enum class CharacterState : uint32_t { ALIVE = 0, DEAD = 1 };
enum class ItemType : uint32_t { WEAPON = 100, ARMOR = 200, CONSUMABLE = 300 };
enum class TabType : uint8_t { EQUIP = 0, USE = 1 };
enum class TradeStatus : uint8_t { PENDING = 0, COMPLETE = 1 };

// ---------------------------------------------------------
// 2. 패킷 구조체 정의 (메모리 1바이트 정렬 시작)
// ---------------------------------------------------------
#pragma pack(push, 1)

// [설명: 네트워크 패킷 공통 헤더]
struct PacketHeader {
    uint16_t size;
    PacketID id;
};

// [설명: 모험단 네트워크 패킷]
struct PKT_Adventure {
    PacketHeader header;
    uint64_t adventure_id;
    char adventure_name[21];
    Status status;
    char created_at[20];
    char updated_at[20];
};

// [설명: 길드 네트워크 패킷]
struct PKT_Guild {
    PacketHeader header;
    uint64_t guild_id;
    char guild_name[31];
    uint32_t guild_level;
    Status status;
    char created_at[20];
    char updated_at[20];
};

// [설명: 캐릭터 네트워크 패킷]
struct PKT_Character {
    PacketHeader header;
    uint64_t character_id;
    uint64_t adventure_id;
    uint64_t guild_id; // 0일 경우 소속 길드 없음으로 처리
    JobCode job_code;
    char nickname[31];
    CharacterState state_code;
    char created_at[20];
    char updated_at[20];
};

// [설명: 캐릭터 스탯 네트워크 패킷]
struct PKT_CharacterStat {
    PacketHeader header;
    uint64_t character_id;
    uint32_t level;
    uint32_t hp_max;
    uint32_t hp;
    uint32_t mp_max;
    uint32_t mp;
    Status is_alive;
    uint32_t last_map_id;
};

// [설명: 아이템 사전 네트워크 패킷]
struct PKT_ItemDictionary {
    PacketHeader header;
    uint32_t item_dict_id;
    ItemType item_type;
    char item_name[51];
    char description[256];
    char created_at[20];
    char updated_at[20];
};

// [설명: 아이템 인스턴스 네트워크 패킷]
struct PKT_ItemInstance {
    PacketHeader header;
    uint64_t item_instance_id;
    uint32_t item_dict_id;
    uint32_t count;
    uint32_t enhance_level;
    char created_at[20];
    char updated_at[20];
};

// [설명: 인벤토리 네트워크 패킷]
struct PKT_Inventory {
    PacketHeader header;
    uint64_t character_id;
    TabType tab_type;
    uint32_t slot_index;
    uint64_t item_instance_id;
};

// [설명: 거래소 네트워크 패킷]
struct PKT_Auction {
    PacketHeader header;
    uint64_t auction_id;
    uint64_t seller_id;
    uint64_t item_instance_id;
    uint32_t price;
    TradeStatus trade_status;
    char created_at[20];
    char expired_at[20];
};

#pragma pack(pop)
// ---------------------------------------------------------
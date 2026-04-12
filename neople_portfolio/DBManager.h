#pragma once
#include <mysql/jdbc.h>
#include <vector>
#include <mutex>
#include "Packet.h"

class DBManager {
public:
    static DBManager& GetInstance();
    ~DBManager();

    static constexpr int CONNECTION_POOL_SIZE = 10;

    void Init(const std::string& host, const std::string& user,
        const std::string& password, const std::string& schema);

    // [캐릭터]
    bool InsertCharacter(const PKT_Character& pkt);
    bool UpdateCharacterStat(const PKT_CharacterStat& pkt);
    bool SelectCharacter(uint64_t characterId, PKT_Character& outPkt);

    // [인벤토리]
    bool InsertInventory(const PKT_Inventory& pkt);
    bool SelectInventory(uint64_t characterId, std::vector<PKT_Inventory>& outList);

    // [거래소]
    bool InsertAuction(const PKT_Auction& pkt);
    bool UpdateAuctionStatus(uint64_t auctionId, TradeStatus status);
    bool SelectAuction(uint64_t auctionId, PKT_Auction& outPkt);

private:
    DBManager();
    sql::Driver* m_driver = nullptr;
    std::vector<sql::Connection*> m_connectionPool;
    std::mutex                    m_poolLock;

    sql::Connection* GetConnection();
    void ReturnConnection(sql::Connection*);
};

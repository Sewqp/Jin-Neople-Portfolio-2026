#include "DBManager.h"
#include "AsyncLogger.h"

DBManager& DBManager::GetInstance() {
    static DBManager instance;
    return instance;
}

DBManager::DBManager() {}

DBManager::~DBManager() {
    // [커넥션 풀 전체 정리]
    for (auto conn : m_connectionPool) {
        delete conn;
    }
    m_connectionPool.clear();
}

void DBManager::Init(const std::string& host, const std::string& user,
    const std::string& password, const std::string& schema) {
    try {
        m_driver = sql::mysql::get_mysql_driver_instance();

        // [CONNECTION_POOL_SIZE만큼 연결 생성]
        for (int i = 0; i < CONNECTION_POOL_SIZE; ++i) {
            sql::Connection* conn = m_driver->connect(host, user, password);
            conn->setSchema(schema);
            m_connectionPool.push_back(conn);
        }
        AsyncLogger::GetInstance().Log("DBManager 초기화 완료");
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "MySQL 초기화 오류: " + std::string(e.what()));
    }
}

sql::Connection* DBManager::GetConnection() {
    std::lock_guard<std::mutex> lock(m_poolLock);
    if (!m_connectionPool.empty()) {
        sql::Connection* conn = m_connectionPool.back();
        m_connectionPool.pop_back();
        return conn;
    }
    AsyncLogger::GetInstance().LogError("DBManager 커넥션 풀이 비어 있습니다.");
    return nullptr;
}

void DBManager::ReturnConnection(sql::Connection* conn) {
    std::lock_guard<std::mutex> lock(m_poolLock);
    m_connectionPool.push_back(conn);
}

bool DBManager::InsertCharacter(const PKT_Character& pkt) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 캐릭터 삽입 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "캐릭터 삽입 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}

bool DBManager::UpdateCharacterStat(const PKT_CharacterStat& pkt) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 캐릭터 스탯 수정 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "캐릭터 스탯 수정 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}

bool DBManager::SelectCharacter(uint64_t characterId, PKT_Character& outPkt) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 캐릭터 조회 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "캐릭터 조회 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}

bool DBManager::InsertInventory(const PKT_Inventory& pkt) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 인벤토리 삽입 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "인벤토리 삽입 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}

bool DBManager::SelectInventory(uint64_t characterId, std::vector<PKT_Inventory>& outList) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 인벤토리 조회 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "인벤토리 조회 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}

bool DBManager::InsertAuction(const PKT_Auction& pkt) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 거래소 등록 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "거래소 등록 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}

bool DBManager::UpdateAuctionStatus(uint64_t auctionId, TradeStatus status) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 거래소 상태 수정 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "거래소 상태 수정 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}

bool DBManager::SelectAuction(uint64_t auctionId, PKT_Auction& outPkt) {
    sql::Connection* conn = GetConnection();
    if (!conn) return false;
    try {
        // TODO: 거래소 조회 쿼리
        ReturnConnection(conn);
        return true;
    }
    catch (sql::SQLException& e) {
        AsyncLogger::GetInstance().LogError(
            "거래소 조회 오류: " + std::string(e.what()));
        ReturnConnection(conn);
        return false;
    }
}
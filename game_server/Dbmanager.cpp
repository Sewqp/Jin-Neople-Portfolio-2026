#include "DBManager.h"
#include "AsyncLogger.h"
#include <cstring>

// ---------------------------------------------------------------
// 싱글톤 & 생명주기
// ---------------------------------------------------------------

DBManager& DBManager::GetInstance() {
    static DBManager instance;
    return instance;
}

DBManager::~DBManager() {
    for (auto* conn : m_connectionPool)
        mysql_close(conn);
    m_connectionPool.clear();
    mysql_library_end();
}

void DBManager::Init(const std::string& host, const std::string& user,
    const std::string& password, const std::string& schema) {
    m_host = host; m_user = user; m_password = password; m_schema = schema;

    mysql_library_init(0, nullptr, nullptr);

    for (int i = 0; i < CONNECTION_POOL_SIZE; ++i) {
        MYSQL* conn = mysql_init(nullptr);
        if (!conn) {
            AsyncLogger::GetInstance().LogError("mysql_init 실패");
            return;
        }
        if (!mysql_real_connect(conn, host.c_str(), user.c_str(),
                password.c_str(), schema.c_str(), 3306, nullptr, 0)) {
            AsyncLogger::GetInstance().LogError(
                "MySQL 연결 실패: " + std::string(mysql_error(conn)));
            mysql_close(conn);
            return;
        }
        mysql_set_character_set(conn, "utf8mb4");
        m_connectionPool.push_back(conn);
    }
    AsyncLogger::GetInstance().Log("DBManager 초기화 완료");
}

MYSQL* DBManager::GetConnection() {
    std::unique_lock<std::mutex> lock(m_poolLock);
    if (!m_poolCondition.wait_for(lock, std::chrono::seconds(5),
            [this] { return !m_connectionPool.empty(); })) {
        AsyncLogger::GetInstance().LogError("DBManager 커넥션 획득 타임아웃 (5초)");
        return nullptr;
    }
    auto* conn = m_connectionPool.back();
    m_connectionPool.pop_back();
    return conn;
}

void DBManager::ReturnConnection(MYSQL* conn) {
    {
        std::lock_guard<std::mutex> lock(m_poolLock);
        m_connectionPool.push_back(conn);
    }
    m_poolCondition.notify_one();
}

// ---------------------------------------------------------------
// 캐릭터
// ---------------------------------------------------------------

bool DBManager::InsertCharacter(const PKT_Character& pkt) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql =
        "INSERT INTO `character` "
        "(character_id, adventure_id, guild_id, job_code, nickname, state_code, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, NOW(), NOW())";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        AsyncLogger::GetInstance().LogError("캐릭터 삽입 준비 실패");
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint64_t char_id  = pkt.character_id;
    uint64_t adv_id   = pkt.adventure_id;
    uint64_t guild_id = pkt.guild_id;
    uint32_t job      = static_cast<uint32_t>(pkt.job_code);
    uint32_t state    = static_cast<uint32_t>(pkt.state_code);
    unsigned long nick_len = (unsigned long)strlen(pkt.nickname);

    MYSQL_BIND b[6];
    memset(b, 0, sizeof(b));
    b[0].buffer_type = MYSQL_TYPE_LONGLONG; b[0].buffer = &char_id;  b[0].is_unsigned = 1;
    b[1].buffer_type = MYSQL_TYPE_LONGLONG; b[1].buffer = &adv_id;   b[1].is_unsigned = 1;
    b[2].buffer_type = MYSQL_TYPE_LONGLONG; b[2].buffer = &guild_id; b[2].is_unsigned = 1;
    b[3].buffer_type = MYSQL_TYPE_LONG;     b[3].buffer = &job;      b[3].is_unsigned = 1;
    b[4].buffer_type = MYSQL_TYPE_STRING;   b[4].buffer = (void*)pkt.nickname;
    b[4].buffer_length = sizeof(pkt.nickname); b[4].length = &nick_len;
    b[5].buffer_type = MYSQL_TYPE_LONG;     b[5].buffer = &state;    b[5].is_unsigned = 1;

    bool ok = (mysql_stmt_bind_param(stmt, b) == 0 && mysql_stmt_execute(stmt) == 0);
    if (!ok)
        AsyncLogger::GetInstance().LogError(
            "캐릭터 삽입 실패: " + std::string(mysql_stmt_error(stmt)));

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return ok;
}

bool DBManager::UpdateCharacterStat(const PKT_CharacterStat& pkt) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql =
        "UPDATE character_stat "
        "SET level=?, hp_max=?, hp=?, mp_max=?, mp=?, is_alive=?, last_map_id=? "
        "WHERE character_id=?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint32_t level    = pkt.level;
    uint32_t hp_max   = pkt.hp_max;
    uint32_t hp       = pkt.hp;
    uint32_t mp_max   = pkt.mp_max;
    uint32_t mp       = pkt.mp;
    uint8_t  is_alive = static_cast<uint8_t>(pkt.is_alive);
    uint32_t last_map = pkt.last_map_id;
    uint64_t char_id  = pkt.character_id;

    MYSQL_BIND b[8];
    memset(b, 0, sizeof(b));
    b[0].buffer_type = MYSQL_TYPE_LONG;     b[0].buffer = &level;    b[0].is_unsigned = 1;
    b[1].buffer_type = MYSQL_TYPE_LONG;     b[1].buffer = &hp_max;   b[1].is_unsigned = 1;
    b[2].buffer_type = MYSQL_TYPE_LONG;     b[2].buffer = &hp;       b[2].is_unsigned = 1;
    b[3].buffer_type = MYSQL_TYPE_LONG;     b[3].buffer = &mp_max;   b[3].is_unsigned = 1;
    b[4].buffer_type = MYSQL_TYPE_LONG;     b[4].buffer = &mp;       b[4].is_unsigned = 1;
    b[5].buffer_type = MYSQL_TYPE_TINY;     b[5].buffer = &is_alive; b[5].is_unsigned = 1;
    b[6].buffer_type = MYSQL_TYPE_LONG;     b[6].buffer = &last_map; b[6].is_unsigned = 1;
    b[7].buffer_type = MYSQL_TYPE_LONGLONG; b[7].buffer = &char_id;  b[7].is_unsigned = 1;

    bool ok = (mysql_stmt_bind_param(stmt, b) == 0 && mysql_stmt_execute(stmt) == 0);
    if (!ok)
        AsyncLogger::GetInstance().LogError(
            "캐릭터 스탯 업데이트 실패: " + std::string(mysql_stmt_error(stmt)));

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return ok;
}

bool DBManager::SelectCharacter(uint64_t characterId, PKT_Character& outPkt) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql =
        "SELECT character_id, adventure_id, guild_id, job_code, nickname, "
        "state_code, created_at, updated_at "
        "FROM `character` WHERE character_id=?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    MYSQL_BIND in_b[1];
    memset(in_b, 0, sizeof(in_b));
    in_b[0].buffer_type = MYSQL_TYPE_LONGLONG;
    in_b[0].buffer      = &characterId;
    in_b[0].is_unsigned  = 1;
    mysql_stmt_bind_param(stmt, in_b);

    if (mysql_stmt_execute(stmt) || mysql_stmt_store_result(stmt)) {
        AsyncLogger::GetInstance().LogError(
            "캐릭터 조회 실패: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint64_t char_id = 0, adv_id = 0, guild_id = 0;
    uint32_t job = 0, state = 0;
    char     nickname[31]   = {};
    char     created_at[20] = {};
    char     updated_at[20] = {};
    unsigned long nick_len = 0, ca_len = 0, ua_len = 0;
    bool     guild_null = false;

    MYSQL_BIND out_b[8];
    memset(out_b, 0, sizeof(out_b));
    out_b[0].buffer_type = MYSQL_TYPE_LONGLONG; out_b[0].buffer = &char_id;    out_b[0].is_unsigned = 1;
    out_b[1].buffer_type = MYSQL_TYPE_LONGLONG; out_b[1].buffer = &adv_id;     out_b[1].is_unsigned = 1;
    out_b[2].buffer_type = MYSQL_TYPE_LONGLONG; out_b[2].buffer = &guild_id;   out_b[2].is_unsigned = 1;
    out_b[2].is_null     = &guild_null;
    out_b[3].buffer_type = MYSQL_TYPE_LONG;     out_b[3].buffer = &job;        out_b[3].is_unsigned = 1;
    out_b[4].buffer_type = MYSQL_TYPE_STRING;   out_b[4].buffer = nickname;
    out_b[4].buffer_length = sizeof(nickname);  out_b[4].length = &nick_len;
    out_b[5].buffer_type = MYSQL_TYPE_LONG;     out_b[5].buffer = &state;      out_b[5].is_unsigned = 1;
    out_b[6].buffer_type = MYSQL_TYPE_STRING;   out_b[6].buffer = created_at;
    out_b[6].buffer_length = sizeof(created_at); out_b[6].length = &ca_len;
    out_b[7].buffer_type = MYSQL_TYPE_STRING;   out_b[7].buffer = updated_at;
    out_b[7].buffer_length = sizeof(updated_at); out_b[7].length = &ua_len;

    mysql_stmt_bind_result(stmt, out_b);

    int ret = mysql_stmt_fetch(stmt);
    if (ret != 0 && ret != MYSQL_DATA_TRUNCATED) {
        mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    outPkt.character_id = char_id;
    outPkt.adventure_id = adv_id;
    outPkt.guild_id     = guild_null ? 0 : guild_id;
    outPkt.job_code     = static_cast<JobCode>(job);
    strncpy_s(outPkt.nickname, nickname, _TRUNCATE);
    outPkt.state_code   = static_cast<CharacterState>(state);
    strncpy_s(outPkt.created_at, created_at, _TRUNCATE);
    strncpy_s(outPkt.updated_at, updated_at, _TRUNCATE);

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return true;
}

// ---------------------------------------------------------------
// 인벤토리
// ---------------------------------------------------------------

bool DBManager::InsertInventory(const PKT_Inventory& pkt) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql =
        "INSERT INTO inventory (character_id, tab_type, slot_index, item_instance_id) "
        "VALUES (?, ?, ?, ?)";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint64_t char_id = pkt.character_id;
    uint8_t  tab     = static_cast<uint8_t>(pkt.tab_type);
    uint32_t slot    = pkt.slot_index;
    uint64_t item_id = pkt.item_instance_id;

    MYSQL_BIND b[4];
    memset(b, 0, sizeof(b));
    b[0].buffer_type = MYSQL_TYPE_LONGLONG; b[0].buffer = &char_id; b[0].is_unsigned = 1;
    b[1].buffer_type = MYSQL_TYPE_TINY;     b[1].buffer = &tab;     b[1].is_unsigned = 1;
    b[2].buffer_type = MYSQL_TYPE_LONG;     b[2].buffer = &slot;    b[2].is_unsigned = 1;
    b[3].buffer_type = MYSQL_TYPE_LONGLONG; b[3].buffer = &item_id; b[3].is_unsigned = 1;

    bool ok = (mysql_stmt_bind_param(stmt, b) == 0 && mysql_stmt_execute(stmt) == 0);
    if (!ok)
        AsyncLogger::GetInstance().LogError(
            "인벤토리 삽입 실패: " + std::string(mysql_stmt_error(stmt)));

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return ok;
}

bool DBManager::SelectInventory(uint64_t characterId, std::vector<PKT_Inventory>& outList) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql =
        "SELECT character_id, tab_type, slot_index, item_instance_id "
        "FROM inventory WHERE character_id=?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    MYSQL_BIND in_b[1];
    memset(in_b, 0, sizeof(in_b));
    in_b[0].buffer_type = MYSQL_TYPE_LONGLONG;
    in_b[0].buffer      = &characterId;
    in_b[0].is_unsigned  = 1;
    mysql_stmt_bind_param(stmt, in_b);

    if (mysql_stmt_execute(stmt) || mysql_stmt_store_result(stmt)) {
        AsyncLogger::GetInstance().LogError(
            "인벤토리 조회 실패: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint64_t char_id = 0, item_id = 0;
    uint8_t  tab  = 0;
    uint32_t slot = 0;

    MYSQL_BIND out_b[4];
    memset(out_b, 0, sizeof(out_b));
    out_b[0].buffer_type = MYSQL_TYPE_LONGLONG; out_b[0].buffer = &char_id; out_b[0].is_unsigned = 1;
    out_b[1].buffer_type = MYSQL_TYPE_TINY;     out_b[1].buffer = &tab;     out_b[1].is_unsigned = 1;
    out_b[2].buffer_type = MYSQL_TYPE_LONG;     out_b[2].buffer = &slot;    out_b[2].is_unsigned = 1;
    out_b[3].buffer_type = MYSQL_TYPE_LONGLONG; out_b[3].buffer = &item_id; out_b[3].is_unsigned = 1;

    mysql_stmt_bind_result(stmt, out_b);

    while (true) {
        int ret = mysql_stmt_fetch(stmt);
        if (ret == MYSQL_NO_DATA) break;
        if (ret != 0 && ret != MYSQL_DATA_TRUNCATED) break;

        PKT_Inventory inv{};
        inv.character_id     = char_id;
        inv.tab_type         = static_cast<TabType>(tab);
        inv.slot_index       = slot;
        inv.item_instance_id = item_id;
        outList.push_back(inv);
    }

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return true;
}

// ---------------------------------------------------------------
// 거래소
// ---------------------------------------------------------------

bool DBManager::InsertAuction(const PKT_Auction& pkt) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql =
        "INSERT INTO auction "
        "(auction_id, seller_id, item_instance_id, price, trade_status, created_at, expired_at) "
        "VALUES (?, ?, ?, ?, ?, NOW(), ?)";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint64_t auction_id = pkt.auction_id;
    uint64_t seller_id  = pkt.seller_id;
    uint64_t item_id    = pkt.item_instance_id;
    uint32_t price      = pkt.price;
    uint8_t  status     = static_cast<uint8_t>(pkt.trade_status);
    unsigned long ea_len = (unsigned long)strlen(pkt.expired_at);

    MYSQL_BIND b[6];
    memset(b, 0, sizeof(b));
    b[0].buffer_type = MYSQL_TYPE_LONGLONG; b[0].buffer = &auction_id; b[0].is_unsigned = 1;
    b[1].buffer_type = MYSQL_TYPE_LONGLONG; b[1].buffer = &seller_id;  b[1].is_unsigned = 1;
    b[2].buffer_type = MYSQL_TYPE_LONGLONG; b[2].buffer = &item_id;    b[2].is_unsigned = 1;
    b[3].buffer_type = MYSQL_TYPE_LONG;     b[3].buffer = &price;      b[3].is_unsigned = 1;
    b[4].buffer_type = MYSQL_TYPE_TINY;     b[4].buffer = &status;     b[4].is_unsigned = 1;
    b[5].buffer_type = MYSQL_TYPE_STRING;   b[5].buffer = (void*)pkt.expired_at;
    b[5].buffer_length = sizeof(pkt.expired_at); b[5].length = &ea_len;

    bool ok = (mysql_stmt_bind_param(stmt, b) == 0 && mysql_stmt_execute(stmt) == 0);
    if (!ok)
        AsyncLogger::GetInstance().LogError(
            "거래소 등록 실패: " + std::string(mysql_stmt_error(stmt)));

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return ok;
}

bool DBManager::UpdateAuctionStatus(uint64_t auctionId, TradeStatus status) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql = "UPDATE auction SET trade_status=? WHERE auction_id=?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint8_t  s   = static_cast<uint8_t>(status);

    MYSQL_BIND b[2];
    memset(b, 0, sizeof(b));
    b[0].buffer_type = MYSQL_TYPE_TINY;     b[0].buffer = &s;         b[0].is_unsigned = 1;
    b[1].buffer_type = MYSQL_TYPE_LONGLONG; b[1].buffer = &auctionId; b[1].is_unsigned = 1;

    bool ok = (mysql_stmt_bind_param(stmt, b) == 0 && mysql_stmt_execute(stmt) == 0);
    if (!ok)
        AsyncLogger::GetInstance().LogError(
            "거래소 상태 업데이트 실패: " + std::string(mysql_stmt_error(stmt)));

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return ok;
}

bool DBManager::SelectAuction(uint64_t auctionId, PKT_Auction& outPkt) {
    MYSQL* conn = GetConnection();
    if (!conn) return false;

    const char* sql =
        "SELECT auction_id, seller_id, item_instance_id, price, trade_status, "
        "created_at, expired_at "
        "FROM auction WHERE auction_id=?";

    MYSQL_STMT* stmt = mysql_stmt_init(conn);
    if (!stmt || mysql_stmt_prepare(stmt, sql, (unsigned long)strlen(sql))) {
        if (stmt) mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    MYSQL_BIND in_b[1];
    memset(in_b, 0, sizeof(in_b));
    in_b[0].buffer_type = MYSQL_TYPE_LONGLONG;
    in_b[0].buffer      = &auctionId;
    in_b[0].is_unsigned  = 1;
    mysql_stmt_bind_param(stmt, in_b);

    if (mysql_stmt_execute(stmt) || mysql_stmt_store_result(stmt)) {
        AsyncLogger::GetInstance().LogError(
            "거래소 조회 실패: " + std::string(mysql_stmt_error(stmt)));
        mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    uint64_t auction_id = 0, seller_id = 0, item_id = 0;
    uint32_t price  = 0;
    uint8_t  status = 0;
    char     created_at[20] = {};
    char     expired_at[20] = {};
    unsigned long ca_len = 0, ea_len = 0;

    MYSQL_BIND out_b[7];
    memset(out_b, 0, sizeof(out_b));
    out_b[0].buffer_type = MYSQL_TYPE_LONGLONG; out_b[0].buffer = &auction_id; out_b[0].is_unsigned = 1;
    out_b[1].buffer_type = MYSQL_TYPE_LONGLONG; out_b[1].buffer = &seller_id;  out_b[1].is_unsigned = 1;
    out_b[2].buffer_type = MYSQL_TYPE_LONGLONG; out_b[2].buffer = &item_id;    out_b[2].is_unsigned = 1;
    out_b[3].buffer_type = MYSQL_TYPE_LONG;     out_b[3].buffer = &price;      out_b[3].is_unsigned = 1;
    out_b[4].buffer_type = MYSQL_TYPE_TINY;     out_b[4].buffer = &status;     out_b[4].is_unsigned = 1;
    out_b[5].buffer_type = MYSQL_TYPE_STRING;   out_b[5].buffer = created_at;
    out_b[5].buffer_length = sizeof(created_at); out_b[5].length = &ca_len;
    out_b[6].buffer_type = MYSQL_TYPE_STRING;   out_b[6].buffer = expired_at;
    out_b[6].buffer_length = sizeof(expired_at); out_b[6].length = &ea_len;

    mysql_stmt_bind_result(stmt, out_b);

    int ret = mysql_stmt_fetch(stmt);
    if (ret != 0 && ret != MYSQL_DATA_TRUNCATED) {
        mysql_stmt_close(stmt);
        ReturnConnection(conn);
        return false;
    }

    outPkt.auction_id       = auction_id;
    outPkt.seller_id        = seller_id;
    outPkt.item_instance_id = item_id;
    outPkt.price            = price;
    outPkt.trade_status     = static_cast<TradeStatus>(status);
    strncpy_s(outPkt.created_at, created_at, _TRUNCATE);
    strncpy_s(outPkt.expired_at, expired_at, _TRUNCATE);

    mysql_stmt_close(stmt);
    ReturnConnection(conn);
    return true;
}

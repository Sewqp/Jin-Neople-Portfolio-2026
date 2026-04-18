#include "RedisManager.h"
#include "AsyncLogger.h"

RedisManager& RedisManager::GetInstance() {
    static RedisManager instance;
    return instance;
}

RedisManager::RedisManager() : m_context(nullptr) {}

RedisManager::~RedisManager() {
    // [연결 해제]
    if (m_context != nullptr) {
        redisFree(m_context);
    }
}

void RedisManager::Init(const std::string& host, int port) {
    m_context = redisConnect(host.c_str(), port);
    if (m_context == nullptr || m_context->err) {
        AsyncLogger::GetInstance().LogError("Redis 연결 실패");
        return;
    }
    AsyncLogger::GetInstance().Log("Redis 연결 완료");
}

std::string RedisManager::MakeCharacterStatKey(uint64_t characterId) const {
    return "character:stat:" + std::to_string(characterId);
}

std::string RedisManager::MakeSessionKey(uint64_t sessionId) const {
    return "session:" + std::to_string(sessionId);
}

bool RedisManager::SetCharacterStat(uint64_t characterId, const PKT_CharacterStat& stat) {
    std::lock_guard<std::mutex> lock(m_lock);
    auto key = MakeCharacterStatKey(characterId);
    size_t size = sizeof(PKT_CharacterStat);
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(m_context, "SET %s %b EX %d",
            key.c_str(), &stat, size, EXPIRE_SECONDS));
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        AsyncLogger::GetInstance().LogError("캐릭터 스탯 저장 실패");
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool RedisManager::GetCharacterStat(uint64_t characterId, PKT_CharacterStat& outStat) {
    std::lock_guard<std::mutex> lock(m_lock); // [멀티스레드 안전]
    auto key = MakeCharacterStatKey(characterId);
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(m_context, "GET %s", key.c_str()));
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        AsyncLogger::GetInstance().Log("캐릭터 스탯 캐시 없음");
        freeReplyObject(reply);
        return false;
    }
    memcpy(&outStat, reply->str, sizeof(PKT_CharacterStat));
    freeReplyObject(reply);
    return true;
}

bool RedisManager::DeleteCharacterStat(uint64_t characterId) {
    std::lock_guard<std::mutex> lock(m_lock); // [멀티스레드 안전]
    auto key = MakeCharacterStatKey(characterId);
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(m_context, "DEL %s", key.c_str()));
    freeReplyObject(reply);
    return true;
}

bool RedisManager::SetSession(uint64_t sessionId, uint64_t characterId) {
    std::lock_guard<std::mutex> lock(m_lock);
    auto key   = MakeSessionKey(sessionId);
    auto value = std::to_string(characterId);
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(m_context, "SET %s %s EX %d",
            key.c_str(), value.c_str(), EXPIRE_SECONDS));
    if (reply == nullptr || reply->type == REDIS_REPLY_ERROR) {
        AsyncLogger::GetInstance().LogError("세션 저장 실패");
        freeReplyObject(reply);
        return false;
    }
    freeReplyObject(reply);
    return true;
}

bool RedisManager::GetSession(uint64_t sessionId, uint64_t& outCharacterId) {
    std::lock_guard<std::mutex> lock(m_lock); // [멀티스레드 안전]
    auto key = MakeSessionKey(sessionId);
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(m_context, "GET %s", key.c_str()));
    if (reply == nullptr || reply->type == REDIS_REPLY_NIL) {
        AsyncLogger::GetInstance().Log("세션 캐시 없음");
        freeReplyObject(reply);
        return false;
    }
    outCharacterId = std::stoull(reply->str);
    freeReplyObject(reply);
    return true;
}

bool RedisManager::DeleteSession(uint64_t sessionId) {
    std::lock_guard<std::mutex> lock(m_lock); // [멀티스레드 안전]
    auto key = MakeSessionKey(sessionId);
    redisReply* reply = reinterpret_cast<redisReply*>(
        redisCommand(m_context, "DEL %s", key.c_str()));
    freeReplyObject(reply);
    return true;
}
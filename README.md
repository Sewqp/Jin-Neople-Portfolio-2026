# Jin-Neople-Portfolio-2026

## 사용 AI 모델
- **Local LLM**: Meta LLaMA 3.1 8B Instruct → Qwen 2.5 Coder 14B (업그레이드)
- **플랫폼**: LM Studio
- **운용 방식**: 외부 서버 전송 없는 완전 로컬 환경. 코드 유출 없음.

---

## 의존성 (Dependencies)
- Windows IOCP (ws2_32, mswsock)
- MySQL Connector/C++ 8.x (`mysqlcppconn.lib`)
- Redis (hiredis) — 예정
- Node.js (Express, mysql2, ioredis) — 예정

---

## 커밋 히스토리

### First commit
**2026-03-17**
- MySQL 기반 DB 스키마 및 구조 추가

### UPDATE
**2026-03-18**
- LLM 시스템 프롬프트 추가
- RAG 벡터 파일 (db_schema.txt) 추가

### UPDATE
**2026-03-22**
- IOCP 네트워크 엔진 구축 시작
- Session.h / Session.cpp 구현 (PostSend, RegisterSend, SendCompleted)
- IocpCore.h / IocpCore.cpp 구현 (워커 스레드 풀, GQCS 루프)
- SessionManager.h / SessionManager.cpp 구현 (싱글톤, shared_mutex)
- Packet.h 구현 (enum class + #pragma pack(1) DTO 구조체)

### UPDATE
**2026-03-23**
- AsyncLogger.h / AsyncLogger.cpp 구현 (비동기 로거, AI 에러 전송)
- Session 생성자 / 소멸자 / PostRecv 구현 완료
- RingBuffer.h / RingBuffer.cpp 구현 (패킷 조립용 링 버퍼)

### UPDATE
**2026-04-11**
- Acceptor.h / Acceptor.cpp 구현 (AcceptEx 기반 비동기 Accept)
- IocpCore.h 수정 (SetAcceptor, m_acceptor 추가)
- IO_TYPE에 ACCEPT 추가

### UPDATE
**2026-04-12**
- PacketHandler.h / PacketHandler.cpp 구현 (패킷 ID별 디스패처)
- DBManager.h / DBManager.cpp 구현 (MySQL 커넥션 풀 기반 CRUD)
- MySQL Connector/C++ 연동 완료
- LLM 모델 Qwen 2.5 Coder 14B로 업그레이드

---

## 프로젝트 구조

```
C++ 게임 서버 (IOCP)
├── Packet.h                ✅ 완성
├── AsyncLogger.h/.cpp      ✅ 완성
├── IocpCore.h/.cpp         🔄 WorkerThreadMain 완성 중
├── Session.h/.cpp          ✅ 완성
├── SessionManager.h/.cpp   ✅ 완성
├── RingBuffer.h/.cpp       ✅ 완성
├── Acceptor.h/.cpp         ✅ 완성
├── PacketHandler.h/.cpp    ✅ 완성
├── DBManager.h/.cpp        ✅ 완성
├── RedisManager.h/.cpp     ❌ 미구현
├── SyncWorker.h/.cpp       ❌ 미구현
├── Room.h/.cpp             ❌ 미구현
├── RoomManager.h/.cpp      ❌ 미구현
└── main.cpp                ❌ 미구현

Node.js 웹 서버
├── server.js               ❌ 미구현
├── db.js                   ❌ 미구현
├── redis.js                ❌ 미구현
└── routes/
    ├── character.js        ❌ 미구현
    ├── auction.js          ❌ 미구현
    ├── admin.js            ❌ 미구현
    └── ranking.js          ❌ 미구현
```

**진행률**
```
C++       : 9 / 14 파일 완성 (64%)
Node.js   : 0 / 7  파일 완성 (0%)
전체      : 9 / 21 파일 완성 (43%)
```

---

## AI 파이프라인 구조

```
[Qwen 2.5 Coder 14B — 로컬 폐쇄망]
        ↓
system_prom.txt (코딩 규칙 강제)
        +
vector/db_schema.txt (DB 컨텍스트)
        ↓
C++ / Node.js 코드 생성
        ↓
코드 리뷰 & 버그 수정
        ↓
프로젝트 반영
```

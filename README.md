# RPG GAME SERVER PORTFOLIO 2026

## 사용 AI 모델
- **Local LLM**: Meta LLaMA 3.1 8B Instruct → Qwen 2.5 Coder 14B (업그레이드)
- **플랫폼**: LM Studio
- **운용 방식**: 외부 서버 전송 없는 완전 로컬 환경. 코드 유출 없음.

---

## 의존성 (Dependencies)
- Windows IOCP (ws2_32, mswsock)
- MySQL Connector/C++ 8.x (`mysqlcppconn.lib`)
- Redis (hiredis)
- Node.js (Express, mysql2, ioredis, winston)

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

### UPDATE
**2026-04-18**
- RedisManager.h/.cpp 구현 (hiredis 기반 Redis 캐시)
- SyncWorker.h/.cpp 구현 (Redis → MySQL 주기적 동기화)
- Redis(hiredis) 연동 완료

### UPDATE
**2026-04-19**
- Room.h/.cpp 구현 (던전 인스턴스)
- RoomManager.h/.cpp 구현 (룸 풀 관리)
- main.cpp 구현 (서버 진입점)
- IocpCore WorkerThreadMain() 완성 (RECV/SEND/ACCEPT 분기)
- C++ 코드 100% 완성

### UPDATE
**2026-04-29**
- node_server/ 폴더 구성 시작
- server.js / db.js / redis.js / constants.js 구현
- routes/character.js 구현 (캐릭터 조회 + 인벤토리)
- routes/auction.js 구현 (거래소 목록/상세)

### UPDATE
**2026-05-06**
- routes/admin.js 구현 (서버 상태 + 로그 조회)
- routes/ranking.js 구현 (레벨/경매 랭킹)
- Node.js 웹 서버 100% 완성

### UPDATE
**2026-05-07**
- README 및 계획서 업데이트

### UPDATE
**2026-05-26**
- DBManager: MySQL X DevAPI → MySQL C API(libmysql) 전환 (안정성 개선)
- AsyncLogger: 콘솔 출력 추가, AI 에러 전송 구조 완성
- Packethandler.cpp 인코딩 수정 (한글 로그 출력 정상화)
- Node.js: logger.js 모듈 분리 (server.js에서 로거 책임 분리)
- DB 더미 데이터 추가 (dummy_data.sql)
- 프로젝트 구조 재편 및 빌드 아티팩트 정리
- PacketHandler DB 연동 구현 (InsertCharacter / UpdateCharacterStat / InsertInventory / InsertAuction 호출)
- Packet.h 주석 인코딩 수정 (CP949 → UTF-8)
- PacketHandler char 배열 안전 처리 (strnlen 적용)

---

## 프로젝트 구조

```
C++ 게임 서버 (IOCP)
├── Packet.h                ✅ 완성
├── AsyncLogger.h/.cpp      ✅ 완성
├── IocpCore.h/.cpp         ✅ 완성
├── Session.h/.cpp          ✅ 완성
├── SessionManager.h/.cpp   ✅ 완성
├── RingBuffer.h/.cpp       ✅ 완성
├── Acceptor.h/.cpp         ✅ 완성
├── PacketHandler.h/.cpp    ✅ 완성
├── DBManager.h/.cpp        ✅ 완성
├── RedisManager.h/.cpp     ✅ 완성
├── SyncWorker.h/.cpp       ✅ 완성
├── Room.h/.cpp             ✅ 완성
├── RoomManager.h/.cpp      ✅ 완성
└── main.cpp                ✅ 완성

Node.js 웹 서버
├── server.js               ✅ 완성
├── db.js                   ✅ 완성
├── redis.js                ✅ 완성
├── constants.js            ✅ 완성
└── routes/
    ├── character.js        ✅ 완성
    ├── auction.js          ✅ 완성
    ├── admin.js            ✅ 완성
    └── ranking.js          ✅ 완성
```

**진행률**
```
C++       : 14 / 14 파일 완성 (100%)
Node.js   :  8 /  8 파일 완성 (100%)
전체      : 22 / 22 파일 완성 (100%)
```

---

## 다음 단계 (테스트 및 최적화)
- 더미 클라이언트 제작 (C++ 기반 스트레스 테스트)
- 10,000명 동시 접속 테스트 및 결과 분석
- Discord 봇 연동 (에러 로그 실시간 알림)
- 성능 최적화 (메모리 풀 등)

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

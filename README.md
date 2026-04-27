# IOCP_CHAT_SERVER

Windows IOCP(Input/Output Completion Port) 기반의 고성능 멀티스레드 채팅 서버입니다.
로비/채팅방 분리 구조와 MySQL 기반 사용자 인증을 지원합니다.

<br>

## 🎯 개발 목표

C++와 IOCP를 활용한 확장 가능한 비동기 채팅 서버 구현
- 다수의 동시 접속자 처리
- 로비 ↔ 채팅방 구조의 다중 채팅 공간
- 회원가입 / 로그인을 통한 계정 기반 서비스

<br>

## 🚀 기술 스택

| 분류 | 기술 |
|------|------|
| **언어 / 표준** | C++17 |
| **플랫폼** | Windows |
| **핵심 기술** | IOCP, Winsock2, 멀티스레딩 |
| **동기화** | SRWLock (Slim Reader/Writer Lock), `std::atomic` |
| **데이터베이스** | MySQL |

<br>

## ✨ 주요 기능

| 기능 | 설명 |
|------|------|
| **회원가입 / 로그인** | MySQL 기반 계정 관리. `loginId`(로그인용) / `nickname`(표시용) 분리 |
| **로비 채팅** | 로그인한 모든 접속자에게 메시지 브로드캐스팅 |
| **귓속말** | 닉네임 기반 1:1 개인 메시지 |
| **채팅방 생성 / 조회** | 방 생성, 페이지 단위 방 목록 조회 |
| **채팅방 입장 / 퇴장** | 정원 검증, 입장/퇴장 자동 알림 |
| **채팅방 채팅** | 같은 방 인원에게만 메시지 전파 |
| **세션 관리** | 객체 풀 기반 클라이언트 연결 추적 및 재사용 |

<br>

## 📐 아키텍처

### 클래스 구조

```
IOCPServer
├── SessionManager
│   └── ClientSession       (세션 풀)
├── RoomManager
│   └── RoomSession         (방 풀)
├── PacketHandler           (패킷 라우팅)
└── DbManager               (MySQL 연결 / 쿼리)
```

| 클래스 | 역할 |
|--------|------|
| **IOCPServer** | IOCP I/O 처리, Accept/Worker 스레드 관리 |
| **SessionManager** | 클라이언트 세션 생명주기 / 닉네임 인덱스 관리 |
| **ClientSession** | 개별 클라이언트 상태 + Send Queue |
| **RoomManager** | 채팅방 풀 관리, 방 생성/조회/입퇴장 라우팅 |
| **RoomSession** | 단일 방 내 유저 목록, 방 단위 브로드캐스트 |
| **PacketHandler** | 헤더 기반 패킷 분기, 인증 가드, 비즈니스 로직 호출 |
| **DbManager** | MySQL 연결, 회원가입 / 로그인 쿼리 처리 |

<br>

## 🔧 구현 세부사항

### 1. 패킷 설계 및 처리

#### 📌 역할 기반 패킷 분리 (Req / Res / Noti)

요청-응답-알림(Notify)의 3단 구조로 책임을 명확히 분리했습니다.

#### 📌 구조화된 패킷 체계

```cpp
#pragma pack(push, 1)
struct PacketHeader {
    PacketType type;
    uint16_t   size;
};

template<typename T>
struct PacketBase : PacketHeader {
    PacketBase(PacketType type) : PacketHeader(type) {
        this->size = static_cast<uint16_t>(sizeof(T));
    }
};
#pragma pack(pop)
```

- `PacketBase<T>` CRTP 패턴으로 **컴파일 타임에 패킷 크기 자동 설정**
- `#pragma pack(1)`으로 네트워크 직렬화 시 패딩 제거

#### 📌 TCP 스트리밍 문제 해결

TCP는 데이터 경계를 보장하지 않기 때문에 **헤더 검증 → 패킷 크기 확인 → 완성된 패킷만 핸들러로 전달**하는 단계적 검증 로직을 구현했습니다.

**링버퍼(Ring Buffer) 구현**

기존 선형 버퍼의 `memmove()` O(N) 복사 연산을 직접 구현한 원형 큐 기반 링버퍼로 개선하여 O(1) 포인터 이동으로 최적화했습니다.
```cpp
// ❌ 기존 방식: 패킷 소비 시 전체 데이터 이동
memmove(buffer, buffer + len, dataSize - len);  // O(N)

// ✅ 개선 방식: Head/Tail 포인터만 이동
void Consume(size_t len) {
    tail = (tail + len) % capacity;  // O(1)
    dataSize -= len;
}
```

- **원형 큐 구조**: Head/Tail 포인터로 읽기/쓰기 위치 관리
- **경계 처리**: 버퍼 끝에서 처음으로 순환 시 2단계 복사로 연속성 보장

<br>

### 2. 인증 시스템 (회원가입 / 로그인)

#### 📌 MySQL 기반 계정 관리

- `loginId` UNIQUE 제약으로 중복 가입 방지
- **Prepared Statement** 사용으로 SQL Injection 방어
- 로그인용 `loginId`와 표시용 `nickname` 분리 → 향후 닉네임 변경 기능 확장 용이

#### 📌 세션 기반 상태 관리

```cpp
enum class SessionState { CONNECTED, AUTHENTICATED, DISCONNECTING, IDLE };
enum class UserState    { LOBBY, IN_ROOM };
```

- `SessionState`로 네트워크/인증 단계, `UserState`로 위치 관리
- `PacketHandler`의 **인증 가드**로 미인증 세션의 채팅/방 패킷을 일괄 차단

#### 📌 객체 풀 기반 세션 관리

- 사전 할당된 세션 풀로 동적 할당 오버헤드 제거
- 연결 해제 시 상태 초기화 후 풀에 반환하여 재사용

<br>

### 3. 채팅방 (Room) 시스템

#### 📌 Object Pool + Stack 기반 ID 재사용

```cpp
class RoomManager {
    vector<unique_ptr<RoomSession>> mRoomContainer; // 사전 할당된 방 풀
    stack<int>                      mRoomIndexes;   // 사용 가능한 방 ID
    map<uint16_t, RoomSession*>     mRoomById;      // 활성 방 빠른 조회
};
```

- 서버 시작 시 방을 미리 할당 → 런타임 동적 할당 0
- 빈 방 ID를 stack에서 O(1)로 꺼내 사용, 방 종료 시 ID 반환
- 활성 방은 `map<roomId, RoomSession*>`로 별도 인덱싱하여 빠른 조회

#### 📌 페이지네이션 기반 방 목록 조회

- 방 개수가 늘어나도 **응답 패킷 크기를 고정 상한 이하로 유지**하는 구조적 설계
- 페이지당 `MAX_ROOM_PAGE_COUNT`(10)개로 제한 → `RoomListResPacket` 크기를 컴파일 타임에 결정
- `MAX_PACKET_SIZE` 초과로 인한 연결 종료를 원천 차단

#### 📌 그 외

- **입퇴장 자동 알림** 및 **빈 방 자동 정리**

<br>

### 4. 멀티스레드 동기화

#### 📌 RAII SRWLock

```cpp
{
    SRWLockGuard lock(&mSrwLock);          // exclusive
    // 크리티컬 섹션
}

{
    SRWLockGuard lock(&mSrwLock, false);   // shared (read-only)
    // 읽기 전용 작업
}
```

- 커스텀 RAII 래퍼로 자동 잠금 해제 → 데드락 방지
- 읽기/쓰기 락 분리로 조회 작업의 동시성 향상

<br>

### 5. 브로드캐스트 / 귓속말

- **로비 브로드캐스트**: `LOBBY` 상태의 인증된 세션에게만 전파, `sessionId` 비교로 송신자 에코 방지
- **방 브로드캐스트**: `RoomSession`의 유저 리스트만 순회 → 다른 방/로비로의 메시지 누수 차단
- **귓속말**: 닉네임 → 세션 Hash Table로 O(1) 조회, 수신자 상태 검증 후 전송

<br>

### 6. 비동기 Send 순서 보장 (Send Queue)

#### 📌 문제 상황

IOCP의 비동기 특성상 여러 스레드에서 동시에 `WSASend()`를 호출하면 패킷 도착 순서가 뒤바뀔 수 있습니다.
- 패킷 A, B, C 순서로 전송 시도 → B, A, C 순서로 도착 가능
- 채팅 메시지 순서가 뒤바뀌는 치명적 문제 발생

#### 📌 해결: 세션별 Send Queue

```cpp
class ClientSession {
    std::queue<vector<char>> mSendQueue;
    bool                     mIsSending;
    SRWLOCK                  mSendLock;
};
```

- **FIFO 큐**: 전송 순서 보장
- **`IsSending` 플래그**: 중복 `WSASend()` 호출 방지, 완료 통지 시 다음 패킷 자동 전송
- **`SendLock`**: 여러 워커 스레드의 동시 큐 접근 보호

→ 메시지 순서 보장 + 경쟁 상태(race condition) 방지

<br>

## 🐛 트러블슈팅 & 회고

### ① 세션 풀 + 이동 생성자 컴파일 에러

#### 🐞 문제
세션 풀을 위해 `vector<ClientSession>`을 고정 크기로 사용하려 했으나 컴파일 에러 발생.

원인은 `ClientSession`의 멤버 중 **이동(move)이 불가능한 타입**이 포함되어 있었기 때문:
- `std::atomic<SessionState>`, `std::atomic<UserState>` → 표준이 이동 금지
- `SRWLOCK` → Windows 커널이 주소로 락 상태를 추적 (이동 시 미정의 동작)
- `const uint32_t mPoolIndex` → const 멤버는 대입 자체가 불가

> C++ 규칙: 멤버 중 하나라도 이동 불가면, 컴파일러는 그 클래스의 이동 생성자를 자동으로 `delete` 처리한다.
> 따라서 `ClientSession` 자체가 이동 불가능한 타입이 되었고, `vector`의 재할당 의존 연산들이 모두 컴파일 에러를 일으킴.

#### 🔧 해결
`vector<std::unique_ptr<ClientSession>>` 으로 전환.

- vector가 다루는 건 **8바이트 포인터**, 실제 객체는 힙의 고정 주소에 머무름
- vector 재할당이 일어나도 포인터만 이동, ClientSession 객체는 손대지 않음
- `WSARecv()`에 넘긴 OVERLAPPED 주소를 커널이 보관하는 IOCP 모델에서도 안전

추가로 `ClientSession`의 복사/이동 생성자·대입 연산자를 모두 `delete`로 명시 → 실수로 옮기는 것을 컴파일 타임에 원천 차단.

#### 💡 배운 점
- `atomic`, `mutex`/`SRWLOCK`처럼 **메모리 주소가 정체성의 일부인 객체**는 절대 이동되면 안 된다
- IOCP처럼 **커널이 객체 주소를 들고 있는 비동기 모델**에서는 객체의 주소가 객체의 생명 주기 내내 고정돼야 한다
- 무거운 객체를 STL 컨테이너에 직접 담기 전에, 컨테이너의 재할당 동작이 해당 타입에 안전한지 검증해야 한다

<br>

### ② 동시 종료로 인한 유령 세션(Phantom Session) 버그

#### 🐞 문제
세션 종료가 여러 경로에서 발생할 수 있는 구조였음:
- `WorkerThread`에서 `WSARecv`/`WSASend` 실패 감지 시
- `PacketHandler`에서 잘못된 패킷 감지 시 (`return false`)
- 클라이언트가 명시적으로 연결을 끊었을 때

여러 워커 스레드가 **같은 세션을 동시에 종료 처리**하면서 다음과 같은 상황이 발생:
- 동일 세션이 풀에 **두 번 반환**되어 다른 클라이언트와 충돌
- 한 스레드가 정리 중인 세션을 다른 스레드가 사용 → use-after-clear
- `UnregisterSession`은 호출됐지만 소켓이 살아있는 **유령 세션**이 풀에 잔존

기존 종료 처리는 `mStateLock`(별도 SRWLock) + 일반 `SessionState` 변수의 조합이었고, **lock → check → set** 패턴으로 동기화했지만, 여러 종료 경로가 분산돼 있어 누락/중복 호출이 생기는 구조적 문제가 있었음.

#### 🔧 해결

**1) 종료 경로를 `IOCPServer::DisconnectSession()` 단일 진입점으로 통합**
- `PacketHandler`는 더 이상 직접 종료하지 않고 `return false`만 반환
- 실제 종료 작업은 `WorkerThread`가 이 신호를 받아 `DisconnectSession()` 한 곳에서만 수행
- 종료 시 `UserState::IN_ROOM`이면 `LeaveRoom()` 호출 → `UnregisterSession()` 순서 보장

**2) 상태 전이를 CAS(Compare-And-Swap)로 변경**

```cpp
// Before: 락 + 일반 변수
SessionState mState;
SRWLOCK      mStateLock;

bool TryDisconnect() {
    SRWLockGuard lock(&mStateLock);
    if (mState == DISCONNECTING || mState == IDLE) return false;
    mState = DISCONNECTING;
    return true;
}
```

```cpp
// After: atomic + CAS
atomic<SessionState> mState;

bool TryDisconnect() {
    SessionState expected = SessionState::CONNECTED;
    if (mState.compare_exchange_strong(expected, SessionState::DISCONNECTING))
        return true;

    expected = SessionState::AUTHENTICATED;
    if (mState.compare_exchange_strong(expected, SessionState::DISCONNECTING))
        return true;

    return false;
}
```

- **여러 스레드가 동시에 `TryDisconnect()`를 호출해도 정확히 하나만 `true`** 를 반환 → 종료 처리는 단 한 번만 실행
- 별도 락이 필요 없어 `mStateLock` 자체를 제거할 수 있었음

#### 💡 배운 점
- "단순한 boolean 플래그 + 락" 패턴은 **상태 전이 + 정리 작업이 결합된 시나리오**에서는 race window가 생기기 쉽다. **CAS 한 줄이 락보다 강력하고 단순**한 경우가 많다.
- 자원 정리는 **단일 진입점**으로 모아야 한다. 분산된 정리 로직은 늘 누락/중복을 만든다.
- IOCP처럼 여러 워커가 동일 객체에 동시 접근하는 모델에서, 객체의 **lifecycle 상태 머신**은 atomic하게 다뤄야 안전하다.

<br>

### ③ 중첩 락 구조의 데드락 위험과 단순화

#### 🐞 문제
초기 구현에서는 `RoomManager`와 `RoomSession`이 **각자 SRWLock을 보유**하는 구조였음.

```
RoomManager::JoinRoom()
  └─ lock(RoomManager.mSrwLock)         ← 락 ①
       └─ RoomSession::AddUser()
            └─ lock(RoomSession.mSrwLock) ← 락 ②  (중첩!)
```

문제점:
- **두 단계 락**이 일상적으로 중첩됨 → 락 획득 순서 위반 시 데드락 위험
- `LeaveRoom`, `BroadCast` 등 다른 경로에서 락 획득 순서가 달라지면 즉시 데드락
- 락이 어디서 잠기는지 함수 시그니처만으로는 추적이 어려움 → 유지보수성 ↓
- 임시방편으로 `BroadCastUnsafe()` 같은 "락 안 잡는 내부용" 함수를 만들어야 했고, 호출자 책임으로 동기화를 떠넘기는 안티패턴 발생

#### 🔧 해결

**`RoomSession`의 자체 락을 제거하고, 모든 동기화를 `RoomManager`의 단일 락으로 일원화.**

- `RoomSession`의 모든 메서드(`JoinUser`, `LeaveUser`, `BroadCast` 등)는 **호출 시점에 이미 `RoomManager`의 락이 잡혀 있다는 것을 전제**로 동작
- `RoomSession`은 `RoomManager`를 통해서만 접근 가능하도록 격리 → 외부에서 직접 호출 경로 차단
- 보너스: 부자연스러웠던 `BroadCastUnsafe()` 같은 함수가 사라지고, `BroadCast()` 하나로 통합

```cpp
ErrorCode RoomManager::JoinRoom(ClientSession* session, uint16_t roomId) {
    SRWLockGuard lock(&mSrwLock);          // 단일 락
    auto room = FindRoomById(roomId);
    if (room == nullptr) return ErrorCode::ROOM_NOT_FOUND;
    return room->JoinUser(session);        // RoomSession은 락 없음, 안전 보장됨
}
```

#### 💡 배운 점
- **중첩 락은 정당화될 때만 도입해야 한다.** 두 객체의 락이 항상 같은 순서로만 획득된다면, 사실상 하나의 락으로 합치는 게 정답.
- 동시성 제어는 가능한 한 **상위 한 곳에서**. 하위 객체에 락을 두면 호출 경로마다 락 정책을 별도로 관리해야 하는 부담이 생긴다.
- 락의 **소유권 경계**가 명확해야 한다. "이 함수는 호출자가 락을 잡고 부른다"는 암묵적 계약은 코드를 보지 않으면 알 수 없는 위험. 따라서 접근 경로 자체를 `RoomManager`로 강제하여 계약을 컴파일 타임에 보장.

<br>

## 📝 향후 개발 계획

- 패킷 압축 및 암호화 (TLS)
- 비밀번호 해싱 강화
- 하트비트 / 타임아웃 처리
- 채팅 로그 영속화 (방별 메시지 히스토리)
- 부하 테스트 및 성능 측정

<br>

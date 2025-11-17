# IOCP_CHAT_SERVER

Windows IOCP(Input/Output Completion Port) 기반의 고성능 멀티스레드 채팅 서버입니다.

<br>

## 🎯 개발 목표

C++와 IOCP를 활용한 확장 가능한 비동기 채팅 서버 구현

<br>

## 🚀 기술 스택

| 분류 | 기술 |
|------|------|
| **언어** | C++ |
| **플랫폼** | Windows |
| **핵심 기술** | IOCP, Winsock2, 멀티스레딩 |
| **동기화** | SRWLock (Slim Reader/Writer Lock) |

<br>

## ✨ 주요 기능

| 기능 | 설명 |
|------|------|
| **로그인 시스템** | 사용자 인증 및 세션 관리 |
| **전체 채팅** | 모든 접속자에게 메시지 브로드캐스팅 |
| **귓속말** | 특정 사용자에게 개인 메시지 전송 |
| **세션 관리** | 효율적인 클라이언트 연결 추적 및 관리 |

<br>

## 📐 아키텍처

### 클래스 구조

```
IOCPServer
├── SessionManager
│   └── ClientSession (세션 풀)
└── PacketHandler
```

| 클래스 | 역할 |
|--------|------|
| **IOCPServer** | IOCP 기반 비동기 I/O 처리 및 워커 스레드 관리 |
| **SessionManager** | 클라이언트 세션 생명주기 관리 및 동기화 |
| **ClientSession** | 개별 클라이언트 상태 및 소켓 정보 관리 |
| **PacketHandler** | 패킷 파싱 및 비즈니스 로직 처리 |

<br>

## 🔧 구현 세부사항

### 1. 패킷 설계 및 처리

#### 📌 역할 기반 패킷 분리

요청-응답 구조로 책임을 명확히 분리했습니다.

- **클라이언트 요청**: `LoginReqPacket`, `BroadCastReqPacket`, `WhisperReqPacket`
- **서버 응답**: `LoginResPacket`, `BroadCastResPacket`, `WhisperResPacket`

#### 📌 구조화된 패킷 체계

```cpp
struct PacketHeader {
    uint16_t size;        // 전체 패킷 크기
    PacketType type;      // 패킷 타입 식별자
};
```

`PacketType` enum을 통해 패킷을 체계적으로 분류하고, 헤더를 통해 데이터 무결성을 보장합니다.

#### 📌 TCP 스트리밍 문제 해결
TCP는 데이터 경계를 보장하지 않기 때문에 다음과 같은 **3단계 검증 로직**을 구현했습니다.
1. **헤더 크기 검증**: 최소 헤더 크기 확인
2. **전체 패킷 크기 확인**: 헤더에 명시된 크기만큼 데이터 수신 여부 확인
3. **완전한 패킷 처리**: 완성된 패킷만 `ProcessPacket()`으로 전달

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

**핵심 구현**
- **원형 큐 구조**: Head/Tail 포인터로 읽기/쓰기 위치 관리
- **경계 처리**: 버퍼 끝에서 처음으로 순환 시 2단계 복사로 연속성 보장
- **성능 개선**: 시간 복잡도 O(N) → O(1), 불필요한 메모리 복사 제거
<br>

### 2. 로그인 시스템

#### 📌 세션 기반 상태 관리

```cpp
enum class SessionState {
    CONNECTED,      // 소켓 연결됨
    AUTHENTICATED,  // 로그인 완료
    DISCONNECTING   // 연결 종료 중
};
```

- `ClientSession` 구조체로 각 클라이언트의 독립적인 상태 관리
- 고유한 `sessionId`를 통한 클라이언트 식별
- **Hash Table 기반 검색**: username → session 매핑으로 빠른 사용자 조회

#### 📌 효율적인 메모리 관리 (객체 풀 패턴)

- **사전 할당된 세션 풀**: 동적 할당 오버헤드 제거
- **세션 재사용**: `GetEmptySession()`으로 유휴 세션 탐색
- **자동 반환**: 연결 해제 시 세션 초기화 후 풀에 반환

<br>

### 3. 멀티스레드 세션 관리

#### 📌 스레드 안전성 보장

- **커스텀 RAII SRWLock**: RAII 패턴을 적용한 Slim Reader/Writer Lock 구현
- **스코프 기반 잠금**: 자동 잠금 해제로 데드락 방지
- **동기화된 브로드캐스트**: 여러 워커 스레드에서 안전한 메시지 전파

```cpp
// 예시: RAII 기반 락 사용
{
    ScopedSRWLock lock(sessionLock);
    // 크리티컬 섹션
} // 자동 unlock
```

<br>

### 4. 브로드캐스트 메시지

#### 📌 선택적 메시지 전파

- 송신자를 제외한 모든 `AUTHENTICATED` 상태의 세션에게 메시지 전달
- `sessionId` 비교를 통한 에코(echo) 방지
- 효율적인 순회: 활성 세션만 필터링하여 전송

<br>

### 5. 귓속말 기능

#### 📌 1:1 개인 메시지

- **Hash Table 조회**: 수신자 username으로 빠른 세션 검색
- **상태 검증**: 수신자가 로그인 상태인지 확인 후 전송
- **에러 처리**: 존재하지 않거나 오프라인 사용자에 대한 응답 처리

<br>

### 6. 비동기 Send 순서 보장 (Send Queue)

#### 📌문제 상황
IOCP의 비동기 특성상 여러 스레드에서 동시에 `WSASend()`를 호출하면:
- 패킷 A, B, C 순서로 전송 시도 → B, A, C 순서로 도착 가능
- 채팅 메시지 순서가 뒤바뀌는 치명적 문제 발생

#### 📌 해결 방법: Send Queue 패턴
```cpp
// 각 세션마다 독립적인 전송 큐 유지
class ClientSession {
    std::queue<vector<char>> mSendQueue;
    bool mIsSending;
    SRWLOCK mSendLock;
};

```
#### 📌 동작 원리: Send Queue 패턴

- **패킷 큐잉**: 전송 요청이 들어오면 sendQueue에 추가
- **전송 상태 확인**:
전송 중이 아니면 → 즉시 WSASend() 호출
전송 중이면 → 큐에만 추가 (대기)

- **완료 처리**: IOCP 완료 통지 받으면 큐에서 제거 후 다음 패킷 전송
- **동기화**: sendLock으로 여러 워커 스레드의 큐 접근 보호

#### ✅ 효과
- **SendQueue FIFO 구조**: 전송 순서 보장
- **IsSending**: 중복 전송 방지 플래그
- **SendLock**: 여러 스레드의 동시 큐 접근 방지
- **IOCP Send Completion**: 자동으로 다음 패킷 처리

메시지 순서 완벽 보장 + 경쟁 상태(race condition) 방지
<br>

## 📝 향후 개발 계획

- Room 기능 구현
- 패킷 압축 및 암호화
- 데이터베이스 연동
- 하트비트 기능 

<br>

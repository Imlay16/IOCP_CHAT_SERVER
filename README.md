# IOCP_CHAT_SERVER

Windows IOCP 기반 고성능 멀티스레드 채팅 서버

## 🎯 개발 목표
C++와 IOCP를 활용한 확장 가능한 비동기 채팅 서버 구현

## ✨ 주요 기능
- 로그인 시스템
- 전체 채팅 (브로드캐스트)
- 귓속말 (1:1 메시지)
- 세션 관리

## 📐 아키텍처

```
IOCPServer
├── SessionManager
│   └── ClientSession (세션 풀)
└── PacketHandler
```

## 🔧 핵심 구현

### 1. 패킷 설계

**요청-응답 구조**
- 클라이언트 요청: `LoginReqPacket`, `BroadCastReqPacket`, `WhisperReqPacket`
- 서버 응답: `LoginResPacket`, `BroadCastResPacket`, `WhisperResPacket`

**TCP 스트리밍 문제 해결**

TCP는 데이터 경계를 보장하지 않으므로 3단계 검증 로직 구현:
1. 헤더 크기 검증
2. 전체 패킷 크기 확인
3. 완전한 패킷만 처리

불완전한 패킷은 `accumulatedSize`로 버퍼에 누적 후 다음 데이터와 결합하여 처리합니다.

### 2. 로그인 시스템

**세션 상태 관리**
```cpp
enum class SessionState {
    CONNECTED,      // 소켓 연결됨
    AUTHENTICATED,  // 로그인 완료
    DISCONNECTING   // 연결 종료 중
};
```

**객체 풀 패턴**
- 사전 할당된 세션 풀로 동적 할당 오버헤드 제거
- Hash Table로 username → session 빠른 검색

### 3. 멀티스레드 관리

**RAII 기반 동기화**
```cpp
{
    ScopedSRWLock lock(sessionLock);
    // 크리티컬 섹션
} // 자동 unlock
```

커스텀 SRWLock으로 데드락 방지 및 안전한 브로드캐스트 보장

### 4. 브로드캐스트

송신자를 제외한 모든 인증된 세션에 메시지 전달

### 5. 귓속말

Hash Table로 수신자를 빠르게 검색하여 1:1 메시지 전송

## 🚀 기술 스택
- **언어**: C++
- **플랫폼**: Windows
- **핵심 기술**: IOCP, Winsock2, 멀티스레딩
- **동기화**: SRWLock

## 📝 향후 계획
- [ ] Room/Channel 기능
- [ ] 메시지 암호화
- [ ] 데이터베이스 연동
- [ ] 재연결 로직
- [ ] 로그 시스템

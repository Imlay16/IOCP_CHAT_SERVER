## IOCP_CHAT_SERVER
Windows IOCP(Input/Output Completion Port) 기반의 고성능 멀티스레드 채팅 서버입니다.

## 🎯 개발 목표
C++와 IOCP를 활용한 확장 가능한 비동기 채팅 서버 구현

## ✨ 주요 기능
로그인 시스템: 사용자 인증 및 세션 관리
전체 채팅: 모든 접속자에게 메시지 브로드캐스팅
귓속말: 특정 사용자에게 개인 메시지 전송
세션 관리: 효율적인 클라이언트 연결 추적 및 관리

## 📐 아키텍처
클래스 구조
IOCPServer
├── SessionManager
│   └── ClientSession (세션 풀)
└── PacketHandler

IOCPServer: IOCP 기반 비동기 I/O 처리 및 워커 스레드 관리
SessionManager: 클라이언트 세션 생명주기 관리 및 동기화
ClientSession: 개별 클라이언트 상태 및 소켓 정보 관리
PacketHandler: 패킷 파싱 및 비즈니스 로직 처리

## 🔧 구현 세부사항
### 1. 패킷 설계
역할 기반 패킷 분리
요청-응답 구조로 책임을 명확히 분리했습니다.

클라이언트 요청: LoginReqPacket, BroadCastReqPacket, WhisperReqPacket
서버 응답: LoginResPacket, BroadCastResPacket, WhisperResPacket

구조화된 패킷 체계
cppstruct PacketHeader {
    uint16_t size;        // 전체 패킷 크기
    PacketType type;      // 패킷 타입 식별자
};
PacketType enum을 통해 패킷을 체계적으로 분류하고, 헤더를 통해 데이터 무결성을 보장합니다.
TCP 스트리밍 문제 해결
TCP는 데이터 경계를 보장하지 않기 때문에 다음과 같은 3단계 검증 로직을 구현했습니다.

헤더 크기 검증: 최소 헤더 크기 확인
전체 패킷 크기 확인: 헤더에 명시된 크기만큼 데이터 수신 여부 확인
완전한 패킷 처리: 완성된 패킷만 ProcessPacket()으로 전달

cpp// 불완전한 패킷은 accumulatedSize를 통해 버퍼에 누적
// 다음 수신 데이터와 결합하여 완전한 패킷 구성
패킷 처리 완료 후 memmove()를 사용하여 버퍼 내 잔여 데이터를 앞으로 이동시켜 다음 패킷을 효율적으로 처리합니다.

### 2. 로그인 시스템
세션 기반 상태 관리
cppenum class SessionState {
    CONNECTED,      // 소켓 연결됨
    AUTHENTICATED,  // 로그인 완료
    DISCONNECTING   // 연결 종료 중
};

ClientSession 구조체로 각 클라이언트의 독립적인 상태 관리
고유한 sessionId를 통한 클라이언트 식별
Hash Table 기반 검색: username → session 매핑으로 빠른 사용자 조회

효율적인 메모리 관리 (객체 풀 패턴)

사전 할당된 세션 풀: 동적 할당 오버헤드 제거
세션 재사용: GetEmptySession()으로 유휴 세션 탐색
자동 반환: 연결 해제 시 세션 초기화 후 풀에 반환

### 3. 멀티스레드 세션 관리
스레드 안전성 보장

커스텀 RAII SRWLock: RAII 패턴을 적용한 Slim Reader/Writer Lock 구현
스코프 기반 잠금: 자동 잠금 해제로 데드락 방지
동기화된 브로드캐스트: 여러 워커 스레드에서 안전한 메시지 전파

cpp// 예시: RAII 기반 락 사용
{
    ScopedSRWLock lock(sessionLock);
    // 크리티컬 섹션
} // 자동 unlock

### 4. 브로드캐스트 메시지
선택적 메시지 전파

송신자를 제외한 모든 AUTHENTICATED 상태의 세션에게 메시지 전달
sessionId 비교를 통한 에코(echo) 방지
효율적인 순회: 활성 세션만 필터링하여 전송

### 5. 귓속말 기능
1:1 개인 메시지

Hash Table 조회: 수신자 username으로 빠른 세션 검색
상태 검증: 수신자가 로그인 상태인지 확인 후 전송
에러 처리: 존재하지 않거나 오프라인 사용자에 대한 응답 처리

## 🚀 기술 스택

언어: C++
플랫폼: Windows
핵심 기술: IOCP, Winsock2, 멀티스레딩
동기화: SRWLock (Slim Reader/Writer Lock)

## 📝 향후 개발 계획

1. Room 기능 구현
2. 메시지 암호화
3. 데이터베이스 연동 (사용자 정보 영구 저장)
4. 재연결 로직 강화
5. 로그 시스템 추가

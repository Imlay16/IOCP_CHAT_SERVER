# IOCP_CHAT_SERVER

🎯개발 목표
C++, IOCP를 이용한 채팅 프로그램 구현

✒️ 구현 기능
1. 패킷 설계를 통한 패킷처리 기능 구현
2. 세션을 이용한 클라이언트 관리
3. 메시지 브로드캐스팅 기능

✒️ 구현 예정 기능
1. 로그인 기능 (HiRedis)
2. 채팅 방 기능 구현
3. 1:1 채팅 구현

## 1. 패킷 설계를 통한 패킷 처리 기능 구현

### 패킷 타입 기반 구조화된 통신
PacketType enum을 통해 요청/응답 패킷을 체계적으로 분류
PacketHeader 구조체로 패킷 크기와 타입을 명시하여 데이터 무결성 확보

### TCP 스트리밍 특성 해결
TCP의 데이터 경계 모호성을 해결하기 위해 accumulatedSize를 활용한 점진적 패킷 수신
ProcessPacket() 함수에서 헤더 크기 검증 → 전체 패킷 크기 확인 → 완전한 패킷 수신 시에만 처리하는 3단계 검증 로직
불완전한 패킷은 버퍼에 누적하여 다음 수신 데이터와 결합 처리

## 2. 세션을 이용한 클라이언트 관리

### 세션 기반 상태 관리
ClientSession 구조체로 클라이언트별 독립적인 상태 관리
SessionState enum으로 연결 생명주기 추적 (CONNECTED → AUTHENTICATED → DISCONNECTING)
고유한 sessionId 할당으로 클라이언트 식별 및 추적

### 효율적인 메모리 관리
사전 할당된 세션 풀(m_clientSessions)로 동적 할당 오버헤드 제거
GetEmptyClientInfo()로 재사용 가능한 세션 탐색
연결 해제 시 세션 초기화 후 풀에 반환하는 객체 풀 패턴 구현

### 비동기 I/O 구조
각 세션마다 독립적인 OverlappedEx 구조체로 송수신 작업 분리
IOCP와 연동하여 세션별 비동기 작업 처리

## 3. 메시지 브로드 캐스팅 기능

### 스레드 안전 브로드캐스트
CRITICAL_SECTION을 활용한 동기화로 멀티스레드 환경에서 안전한 브로드캐스트 보장
BroadCastPacket() 함수에서 임계 영역 진입/해제로 세션 목록 순회 중 데이터 레이스 방지

### 선택적 메시지 전파
송신자를 제외한 모든 활성 세션에게 메시지 전달
sessionId 비교로 에코 방지 구현

### 패킷 변환 처리
클라이언트 요청(BroadCastReqPacket) → 서버 응답(BroadCastResPacket) 변환
사용자명과 메시지를 결합한 완전한 채팅 메시지 구성



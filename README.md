# IOCP_CHAT_SERVER

🎯개발 목표
C++, IOCP를 이용한 채팅 프로그램 구현

✒️ 구현 기능
1. 패킷 설계
2. 로그인 기능
3. 세션을 이용한 클라이언트 관리
4. 귓속말 기능 
5. 메시지 브로드캐스팅 기능
6. Room 기능

## 클래스 정리도

IOCPServer
ClientSession
SessionManager
PacketHandler

## 1. 패킷 설계

### 역할에 따른 패킷 분리
클라이언트 요청(BroadCastReqPacket) → 서버 응답(BroadCastResPacket) 변환
요청 응답 구조의 역할이 분리된 패킷 설계 
(Req: 클라이언트, Res: 서버)

### 패킷 타입 기반 구조화된 통신
PacketType enum을 통해 요청/응답 패킷을 체계적으로 분류
PacketHeader 구조체로 패킷 크기와 타입을 명시하여 데이터 무결성 확보

### TCP 스트리밍 특성 해결
TCP의 데이터 경계 모호성을 해결하기 위해 accumulatedSize를 활용한 점진적 패킷 수신
ProcessPacket() 함수에서 헤더 크기 검증 → 전체 패킷 크기 확인 → 완전한 패킷 수신 시에만 처리하는 3단계 검증 로직
불완전한 패킷은 버퍼에 누적하여 다음 수신 데이터와 결합 처리

하나의 패킷을 처리 완료하면 수신 버퍼 안에 남아 있는 데이터를 확인하고 데이터가 존재할시, memmove함수를 통해 다음 패킷을 첫 위치로 당겨서 다시금 패킷을 처리

## 2. 로그인 기능

### 세션 기반 상태 관리
ClientSession 구조체로 클라이언트별 독립적인 상태 관리
SessionState enum으로 연결 생명주기 추적 (CONNECTED → AUTHENTICATED → DISCONNECTING)
고유한 sessionId 할당으로 클라이언트 식별 및 추적
HashTable을 이용해 클라이언트 username -> session 찾기 구현

### 효율적인 메모리 관리
사전 할당된 세션 풀로 동적 할당 오버헤드 제거
GetEmptySession()로 재사용 가능한 세션 탐색
연결 해제 시 세션 초기화 후 풀에 반환하는 객체 풀 패턴 구현

## 3. 세션을 이용한 클라이언트 관리

### 멀티 쓰레드 기반
RAII 패턴을 이용한 SRWLock 클래스를 직접 구현해 활용한 동기화로 멀티스레드 환경에서 안전한 브로드캐스트 보장

### 선택적 메시지 전파
송신자를 제외한 모든 활성 세션에게 메시지 전달
sessionId 비교로 에코 방지 구현

## 4. 귓속말 기능 

## 5. 메시지 브로드캐스팅 기능

## 6. Room 기능



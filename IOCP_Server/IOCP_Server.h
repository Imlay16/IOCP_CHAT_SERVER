#pragma once
#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include <WS2tcpip.h>

#include <thread>
#include <vector>
#include <unordered_map>

#include <string>
#include <cstring>

#include <iostream>
#include "Packet.h"

#define MAX_SOCKBUF 2048
#define MAX_WORKERTHREAD 4

using namespace std;

enum class SessionState {
	CONNECTED,
	AUTHENTICATED,
	DISCONNECTING
};

enum class IOOperation
{
	RECV,
	SEND
};

struct OverlappedEx
{
	WSAOVERLAPPED wsaOverlapped;
	WSABUF wsaBuf;
	IOOperation operation;
};

struct ClientSession
{
	UINT32 sessionId;
	SOCKET clientSocket;

	string username;

	SessionState state;

	OverlappedEx recvOverlappedEx;
	OverlappedEx sendOverlappedEx;
	DWORD accumulatedSize;

	char recvBuf[MAX_SOCKBUF];
	char sendBuf[MAX_SOCKBUF];

	ClientSession()
	{
		ZeroMemory(&recvOverlappedEx, sizeof(OverlappedEx));
		ZeroMemory(&sendOverlappedEx, sizeof(OverlappedEx));		

		ZeroMemory(recvBuf, sizeof(recvBuf));  
		ZeroMemory(sendBuf, sizeof(sendBuf));  

		clientSocket = INVALID_SOCKET;

		accumulatedSize = 0;
		sessionId = 0;
		state = SessionState::DISCONNECTING;
	}
};

class IOCP_SERVER
{
private:
	// 멤버 변수
	UINT32 sessionIdCounter = 1;
	vector<ClientSession> mClientSessions;
	SOCKET mListenSocket;
	int mClientCnt;
	vector<thread> mIOWorkerThreads;
	thread mAccepterThread;
	HANDLE mIOCPHandle;
	bool mIsWorkerRun = true;
	bool mIsAccepterRun = true;

	unordered_map<string, UINT32> mSessionIdByName;
	unordered_map<UINT32, ClientSession*> mSessionById;

	SRWLOCK mSrwLock;
	CRITICAL_SECTION mBroadcastCS;

	/// <summary>
	/// 고유한 클라이언트 세션 아이디 생성
	/// </summary>
	/// <returns>생성된 세션 아이디 반환</returns>
	UINT32 generateSessionId() {
		return sessionIdCounter++;
	}

	/// <summary>
	/// 클라이언트 세션 관리 공간을 미리 확보
	/// </summary>
	/// <param name="maxClientCount">최대 클라이언트 세션 개수 설정</param>
	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; i++)
		{
			mClientSessions.emplace_back();
		}
	}

	/// <summary>
	/// 워커 쓰레드 생성
	/// </summary>
	/// <returns></returns>
	bool CreateWorkerThread()
	{
		for (int i = 0; i < MAX_WORKERTHREAD; i++)
		{
			mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
		}

		return true;
	}

	/// <summary>
	/// 클라이언트 Accept 쓰레드 생성
	/// </summary>
	/// <returns></returns>
	bool CreateAccepterThread()
	{
		mAccepterThread = thread([this]() { AccepterThread(); });
		return true;
	}

	ClientSession* GetEmptyClientInfo()
	{
		for (auto& client : mClientSessions)
		{
			if (client.clientSocket == INVALID_SOCKET)
			{
				return &client;
			}
		}
		return nullptr;
	}

	ClientSession* FindSessionByName(const string& name)
	{
		AcquireSRWLockShared(&mSrwLock);

		auto nameIt = mSessionIdByName.find(name);
		if (nameIt == mSessionIdByName.end())
		{
			ReleaseSRWLockShared(&mSrwLock);
			return nullptr;
		}

		UINT32 sessionId = nameIt->second;
		auto sessionIt = mSessionById.find(sessionId);

		ClientSession* result = (sessionIt != mSessionById.end()) ? sessionIt->second : nullptr;

		ReleaseSRWLockShared(&mSrwLock);

		if (result && result->clientSocket == INVALID_SOCKET)
		{
			return nullptr;
		}

		return result;
	}

	/// <summary>
	/// IOCP 핸들과 클라이언트 소켓을 연결
	/// </summary>
	/// <param name="pClientInfo"></param>
	/// <returns></returns>
	bool BindIOCompletionPort(ClientSession* pClientInfo)
	{
		auto hIOCP = CreateIoCompletionPort((HANDLE)pClientInfo->clientSocket, mIOCPHandle, (ULONG_PTR)pClientInfo, NULL);

		if (hIOCP != NULL)
		{
			return true;
		}
		else
		{
			cout << "CreateIoCompletionPort Error() " << GetLastError() << endl;
			return false;
		}
	}

	// 패킷 처리 함수들
	void SendLoginPacket(ClientSession* pClientSession, PacketHeader* packet)
	{
		
	}

	void SendRoomChatPacket(ClientSession* pClientSession)
	{

	}

	void SendRoomCreatePacket(ClientSession* pClientSession)
	{

	}

	void SendRoomJoinPacket(ClientSession* pClientSession)
	{

	}

	void SendRoomListPacket(ClientSession* pClientSession)
	{

	}

	/// <summary>
	/// 패킷 보내기
	/// </summary>
	/// <param name="pClientSession"></param>
	/// <param name="packet"></param>
	/// <returns></returns>
	bool SendPacket(ClientSession* pClientSession, PacketHeader* packet)
	{
		DWORD sendBytes;
		memcpy(pClientSession->sendBuf, (char*)packet, packet->GetSize());

		ZeroMemory(&pClientSession->sendOverlappedEx.wsaOverlapped, sizeof(WSAOVERLAPPED));

		pClientSession->sendOverlappedEx.operation = IOOperation::SEND;
		pClientSession->sendOverlappedEx.wsaBuf.buf = pClientSession->sendBuf;
		pClientSession->sendOverlappedEx.wsaBuf.len = packet->GetSize();

		int ret = WSASend(pClientSession->clientSocket,
						  &(pClientSession->sendOverlappedEx.wsaBuf),
						  1,
						  &sendBytes,
						  0,
						  (LPWSAOVERLAPPED)&(pClientSession->sendOverlappedEx),
						  NULL);
						  
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			cout << "WSASend Error() " << WSAGetLastError() << endl;
			return false;
		}
		return true;
	}

	/// <summary>
	/// 패킷 받기
	/// </summary>
	/// <param name="pClientSession"></param>
	/// <returns></returns>
	bool ReceivePacket(ClientSession* pClientSession)
	{
		DWORD recvBytes = 0;
		DWORD flag = 0;

		if (pClientSession->accumulatedSize >= MAX_SOCKBUF) {
			cout << "Buffer overflow detected!" << endl;
			// 연결 종료 또는 에러 처리
			return false;
		}
		
		ZeroMemory(&pClientSession->recvOverlappedEx.wsaOverlapped, sizeof(WSAOVERLAPPED));

		pClientSession->recvOverlappedEx.operation = IOOperation::RECV;
		pClientSession->recvOverlappedEx.wsaBuf.buf = pClientSession->recvBuf + pClientSession->accumulatedSize;
		pClientSession->recvOverlappedEx.wsaBuf.len = MAX_SOCKBUF - pClientSession->accumulatedSize;

		int ret = WSARecv(pClientSession->clientSocket,
			&(pClientSession->recvOverlappedEx.wsaBuf),
			1,
			&recvBytes,
			&flag,
			(LPWSAOVERLAPPED) & (pClientSession->recvOverlappedEx),
			NULL);
		
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			cout << "WSARecv Error() " << WSAGetLastError() << endl;
			return false;
		}

		return true;
	}
	
	/// <summary>
	/// 패킷 브로드캐스팅
	/// </summary>
	/// <param name="pClientSession"></param>
	/// <param name="packet"></param>
	void BroadCastPacket(ClientSession* pClientSession, PacketHeader* packet)
	{
		EnterCriticalSection(&mBroadcastCS);

		for (auto& session : mClientSessions)
		{
			if (session.clientSocket != INVALID_SOCKET)
			{
				if (session.sessionId == pClientSession->sessionId)
					continue;

				SendPacket(&session, packet);
			}
		}

		LeaveCriticalSection(&mBroadcastCS);
	}

	/// <summary>
	/// 로그인 패킷 처리
	/// </summary>
	/// <param name="pClientSession"></param>
	/// <param name="recvPacket"></param>
	void HandleLogin(ClientSession* pClientSession, PacketHeader* recvPacket)
	{
		if (recvPacket->GetSize() != sizeof(LoginReqPacket))
		{
			cout << "Login Packet Size Error!" << endl;
			return;
		}

		LoginReqPacket* packet = (LoginReqPacket*)recvPacket;

		bool isValid = CheckUserCredentials(packet->userId, packet->password);
		LoginResPacket resPacket;

		if (isValid)
		{
			AcquireSRWLockExclusive(&mSrwLock);

			pClientSession->state = SessionState::AUTHENTICATED;
			pClientSession->username = packet->username;	

			// 이 두 코드를 RegisterClient() 함수로?
			mSessionIdByName.insert_or_assign(pClientSession->username, pClientSession->sessionId);
			mSessionById.insert_or_assign(pClientSession->sessionId, pClientSession);

			ReleaseSRWLockExclusive(&mSrwLock);

			resPacket.result = ErrorCode::SUCCESS;
			cout << "[Login Success] " << packet->username << endl;
		}
		else
		{
			resPacket.result = ErrorCode::AUTH_FAILED;
			cout << "[Login Failed] " << packet->userId << endl;
		}		
	
		// + UserJoinNotify로 모든 연결된 클라이언트들에게 유저 조인 메세지 브로드캐스트 하기
		SendPacket(pClientSession, &resPacket);
	}

	bool CheckUserCredentials(const char* userId, const char* userPw)
	{
		// hash table 체크
		// unordered_map을 이용.
		// ["userId"] == userPw를 확인하여 true or false 리턴

		return true;
	}

	/// <summary>
	/// 브로드캐스트 패킷 처리
	/// </summary>
	/// <param name="pClientSession"></param>
	/// <param name="recvPacket"></param>
	void HandleBroadCast(ClientSession* pClientSession, PacketHeader* recvPacket)
	{
		if (recvPacket->GetSize() != sizeof(BroadCastReqPacket)) {
			cout << "BroadCast Packet Size Error!" << endl;
			return;
		}
		
		BroadCastReqPacket* packet = (BroadCastReqPacket*)recvPacket;

		BroadCastResPacket resPacket;
		resPacket.SetUser(pClientSession->username.c_str());
		resPacket.SetMessage(packet->message);

		BroadCastPacket(pClientSession, (PacketHeader*)&resPacket);
	}

	/// <summary>
	/// 귓속말 패킷 처리
	/// </summary>
	/// <param name="pClientSession"></param>
	/// <param name="dataSize"></param>
	void HandleWhispher(ClientSession* pClientSession, PacketHeader* recvPacket)
	{
		if (recvPacket->GetSize() != sizeof(WhispherChatReqPacket)) {
			cout << "Whispher Packet Size Error!" << endl;
			return;
		}
		
		WhispherChatReqPacket* packet = (WhispherChatReqPacket*)recvPacket;

		string receiver = packet->GetReceiver();
		
		ClientSession* targetClient = FindSessionByName(receiver);
		if (targetClient != nullptr)
		{
			WhispherChatResPacket resPacket;
			resPacket.SetResult(ErrorCode::SUCCESS);
			resPacket.SetMessage(pClientSession->username.c_str(), packet->GetMessageW());
			SendPacket(targetClient, &resPacket);
		}
		else
		{
			WhispherChatResPacket resPacket;
			resPacket.SetResult(ErrorCode::USER_NOT_FOUND);
			snprintf(resPacket.message, sizeof(resPacket.message),
				"User '%s' not found", packet->receiver);

			SendPacket(pClientSession, &resPacket);
		}
	}

	void ProcessPacket(ClientSession* pClientSession, DWORD dataSize)
	{
		pClientSession->accumulatedSize += dataSize;

		if (pClientSession->accumulatedSize < sizeof(PacketHeader)) {
			cout << "Packet Header Receiving... (" << pClientSession->accumulatedSize
				<< "/" << sizeof(PacketHeader) << ")" << endl;
			return;
		}

		PacketHeader* header = (PacketHeader*)pClientSession->recvBuf;
		DWORD expectedSize = header->GetSize();

		if (pClientSession->accumulatedSize < expectedSize) {
			cout << "Packet Body Receiving..." << endl;
			return;
		}

		switch (header->GetType())
		{
		case PacketType::LOGIN_REQUEST:
			HandleLogin(pClientSession, header);
			break;

		case PacketType::BROADCAST_REQUEST:
		case PacketType::ROOM_CHAT_REQUEST:
		case PacketType::ROOM_LIST_REQUEST:
		case PacketType::WHISPER_REQUEST:
			// 인증된 사용자만 처리
			if (pClientSession->state == SessionState::AUTHENTICATED) {
				switch (header->GetType()) {
				case PacketType::BROADCAST_REQUEST:
					HandleBroadCast(pClientSession, header);
					break;
				case PacketType::ROOM_CHAT_REQUEST:
					// HandleRoomChat(pClientSession, header);
					break;
				case PacketType::ROOM_LIST_REQUEST:
					// HandleRoomList(pClientSession, header);
					break;
				case PacketType::WHISPER_REQUEST:
					HandleWhispher(pClientSession, header);
					break;
				}
			}
			else {
				cout << "[Client " << pClientSession->sessionId << "] Not authenticated. Packet ignored." << endl;
				// 또는 에러 응답 패킷 전송
			}
			break;

		default:
			cout << "Unknown Packet Type" << endl;
			break;
		}

		pClientSession->accumulatedSize = 0;
		cout << "[DEBUG] Packet processing complete. Buffer reset." << endl;
	}

	void WorkerThread()
	{
		DWORD bSuccess = TRUE;
		DWORD dwIoSize;

		ClientSession* pClientSession = nullptr;
		LPOVERLAPPED lpOverlapped = NULL;

		while (mIsWorkerRun)
		{
			bSuccess = GetQueuedCompletionStatus(mIOCPHandle,
				&dwIoSize,
				(PULONG_PTR)&pClientSession,
				&lpOverlapped,
				INFINITE);

			if (bSuccess == FALSE || dwIoSize == 0)
			{
				if (pClientSession && pClientSession->clientSocket != INVALID_SOCKET)
				{
					cout << "Client Disconnected. Session ID: " << pClientSession->sessionId << endl;
					CloseSession(pClientSession);
				}

				continue;
			}

			auto pOverlappedEx = (OverlappedEx*)lpOverlapped;

			if (pOverlappedEx->operation == IOOperation::RECV)
			{				
				ProcessPacket(pClientSession, dwIoSize);

				// 다시 Recv 시작
				ReceivePacket(pClientSession);
			}
			else if (pOverlappedEx->operation == IOOperation::SEND)
			{
				cout << "Send completed for session: " << pClientSession->sessionId << endl;
			}
		}
	}

	void AccepterThread()
	{
		SOCKADDR_IN clntAdr;
		int clntLen = sizeof(clntAdr);

		while (mIsAccepterRun)
		{
			ClientSession* pClientSession = GetEmptyClientInfo();
			if (pClientSession == NULL)
			{
				cout << "Client Full() " << endl;
				return;
			}

			pClientSession->clientSocket = accept(mListenSocket, (SOCKADDR*)&clntAdr, &clntLen);
			if (pClientSession->clientSocket == INVALID_SOCKET)
			{
				continue;
			}

			// 세션 ID 생성
			pClientSession->sessionId = generateSessionId();
			pClientSession->state = SessionState::CONNECTED;		

			bool ret = BindIOCompletionPort(pClientSession);
			if (ret == false)
			{
				return;
			}

			ret = ReceivePacket(pClientSession);
			if (ret == false)
			{
				return;
			}

			char clientIP[32] = { 0, };
			inet_ntop(AF_INET, &(clntAdr.sin_addr), clientIP, 32 - 1);
			cout << "Session ID: " << pClientSession->sessionId << " Client Connected: IP" << clientIP << " SOCKET" << pClientSession->clientSocket << endl;

			++mClientCnt;
		}
	}

	void CloseSession(ClientSession* pClientSession)
	{
		shutdown(pClientSession->clientSocket, SD_BOTH);
		closesocket(pClientSession->clientSocket);
		pClientSession->state = SessionState::DISCONNECTING;
		pClientSession->sessionId = 0;
		pClientSession->accumulatedSize = 0;  
		pClientSession->clientSocket = INVALID_SOCKET;
		--mClientCnt;
	}

public:
	bool InitSocket()
	{
		WSADATA wsaData;
		if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		{
			cout << "WSAStartup() Error" << WSAGetLastError() << endl;
			return false;
		}

		mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (mListenSocket == INVALID_SOCKET)
		{
			cout << "WSASocket() Error" << WSAGetLastError() << endl;
			return false;
		}

		return true;
	}

	bool BindandListen(int nBindPort)
	{
		SOCKADDR_IN servAdr;

		memset(&servAdr, 0, sizeof(servAdr));
		servAdr.sin_family = AF_INET;
		servAdr.sin_addr.s_addr = htonl(INADDR_ANY);
		servAdr.sin_port = htons(nBindPort);

		if (bind(mListenSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) != 0)
		{
			cout << "bind() Error" << GetLastError() << endl;
			return false;
		}

		if (listen(mListenSocket, 5) != 0)
		{
			cout << "listen() Error" << GetLastError() << endl;
			return false;
		}

		return true;
	}

	bool StartServer(const UINT32 maxClientCount)
	{
		CreateClient(maxClientCount);

		mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, 0, 0, MAX_WORKERTHREAD);

		CreateWorkerThread();
		CreateAccepterThread();

		return true;
	}

	IOCP_SERVER()
	{
		mClientCnt = 0;          
		InitializeCriticalSection(&mBroadcastCS);
		InitializeSRWLock(&mSrwLock);
	}

	~IOCP_SERVER()
	{
		DeleteCriticalSection(&mBroadcastCS);
		WSACleanup();
	}

	void DestroyThread()
	{
		mIsWorkerRun = false;
		CloseHandle(mIOCPHandle);

		for (auto& th : mIOWorkerThreads)
		{
			if (th.joinable())
			{
				th.join();
			}
		}

		mIsAccepterRun = false;
		closesocket(mListenSocket);

		if (mAccepterThread.joinable())
		{
			mAccepterThread.join();
		}
	}
};
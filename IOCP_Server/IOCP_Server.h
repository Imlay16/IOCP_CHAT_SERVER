#pragma once
#pragma comment(lib, "ws2_32")
#include <WinSock2.h>
#include <WS2tcpip.h>

#include <thread>
#include <vector>

#include <iostream>
#include "Packet.h"

#define MAX_SOCKBUF 1024
#define MAX_WORKERTHREAD 4

using namespace std;

enum class SessionState {
	CONNECTING,
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

	char recvBuf[MAX_SOCKBUF];
	char sendBuf[MAX_SOCKBUF];

	ClientSession()
	{
		ZeroMemory(&recvOverlappedEx, sizeof(OverlappedEx));
		ZeroMemory(&sendOverlappedEx, sizeof(OverlappedEx));

		clientSocket = INVALID_SOCKET;
	}
};

class IOCP_SERVER
{
private:

	// 멤버 변수
	UINT32 sessionIdCounter = 1;
	vector<ClientSession> m_clientSessions;
	SOCKET mListenSocket;
	int mClientCnt;
	vector<thread> mIOWorkerThreads;
	thread mAccepterThread;
	HANDLE mIOCPHandle;
	bool mIsWorkerRun = true;
	bool mIsAccepterRun = true;

	// Accept 스레드에서만 호출
	UINT32 generateSessionId() {
		return sessionIdCounter++;
	}

	void CreateClient(const UINT32 maxClientCount)
	{
		for (UINT32 i = 0; i < maxClientCount; i++)
		{
			m_clientSessions.emplace_back();
		}
	}

	bool CreateWorkerThread()
	{
		for (int i = 0; i < MAX_WORKERTHREAD; i++)
		{
			mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
		}

		return true;
	}

	bool CreateAccepterThread()
	{
		mAccepterThread = thread([this]() { AccepterThread(); });
		return true;
	}

	ClientSession* GetEmptyClientInfo()
	{
		for (auto& client : m_clientSessions)
		{
			if (client.clientSocket == INVALID_SOCKET)
			{
				return &client;
			}
		}
		return nullptr;
	}

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

	bool BindRecv(ClientSession* pClientSession)
	{
		DWORD recvBytes = 0;
		DWORD flag = 0;

		ZeroMemory(&pClientSession->recvOverlappedEx.wsaOverlapped, sizeof(WSAOVERLAPPED));

		pClientSession->recvOverlappedEx.operation = IOOperation::RECV;
		pClientSession->recvOverlappedEx.wsaBuf.buf = pClientSession->recvBuf;
		pClientSession->recvOverlappedEx.wsaBuf.len = MAX_SOCKBUF;

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

	bool SendMsg(ClientSession* pClientSession, const char* pMsg, int nLen)
	{
		DWORD sendBytes;
		
		memcpy(pClientSession->sendBuf, pMsg, nLen);

		pClientSession->sendOverlappedEx.operation = IOOperation::SEND;
		pClientSession->sendOverlappedEx.wsaBuf.buf = pClientSession->sendBuf;
		pClientSession->sendOverlappedEx.wsaBuf.len = nLen;

		int ret = WSASend(pClientSession->clientSocket,
			&(pClientSession->sendOverlappedEx.wsaBuf),
			1,
			&sendBytes,
			0,
			(LPWSAOVERLAPPED) & (pClientSession->sendOverlappedEx),
			NULL);

		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
		{
			cout << "WSASend Error() " << WSAGetLastError() << endl;
			return false;
		}

		return true;
	}

	void UserJoinNotify(ClientSession* pClientSession)
	{
		string joinMsg;

		joinMsg = pClientSession->username + " has joined.";

		BroadCastMsg(pClientSession, joinMsg.c_str(), joinMsg.length());
	}

	void BroadCastMsg(ClientSession* pClientSession, const char* pMsg, int nLen)
	{
		for (auto& session : m_clientSessions)
		{
			if (session.clientSocket != INVALID_SOCKET && session.sessionId != pClientSession->sessionId)
			{
				SendMsg(&session, pMsg, nLen);
			}
		}
	}

	void ProcessMessage(ClientSession* pClientSession, const char* pMsg, int nLen)
	{
		if (pClientSession->state == SessionState::AUTHENTICATED)
		{
			pClientSession->username = string(pMsg, nLen);			
			pClientSession->state = SessionState::CONNECTED;
			UserJoinNotify(pClientSession);
		}
		else
		{
			string chatMsg = "[" + pClientSession->username + "]:" + string(pMsg, nLen);
			BroadCastMsg(pClientSession, chatMsg.c_str(), chatMsg.length());
		}
	}

	void ProcessPacket(ClientSession* pClientSession, PacketHeader* packet)
	{

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
					cout << "클라이언트 연결 종료. 세션 ID: " << pClientSession->sessionId << endl;
					CloseSession(pClientSession);
				}

				continue;
			}

			auto pOverlappedEx = (OverlappedEx*)lpOverlapped;

			if (pOverlappedEx->operation == IOOperation::RECV)
			{
				ProcessMessage(pClientSession, pClientSession->recvBuf, dwIoSize);

				// 다시 Recv 시작
				BindRecv(pClientSession);

				// 이제 여기에 ProcessPacket함수를 넣으면 됨. 
				// 그리고 패킷 처리하게!
			}
			else if (pOverlappedEx->operation == IOOperation::SEND)
			{
				// SEND


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
				// 여기서 continue; 아니면 mIsAccepterRun을 false?
				// return문으로 빠져나오는군.. 왜냐하면 클라이언트가 허용량이 꽉 찼기 때문!
			}

			pClientSession->clientSocket = accept(mListenSocket, (SOCKADDR*)&clntAdr, &clntLen);
			if (pClientSession->clientSocket == INVALID_SOCKET)
			{
				continue;
			}

			// 세션 ID 생성
			pClientSession->sessionId = generateSessionId();
			pClientSession->state = SessionState::AUTHENTICATED;

			bool ret = BindIOCompletionPort(pClientSession);
			if (ret == false)
			{
				return;
			}

			ret = BindRecv(pClientSession);
			if (ret == false)
			{
				return;
			}

			char clientIP[32] = { 0, };
			inet_ntop(AF_INET, &(clntAdr.sin_addr), clientIP, 32 - 1);
			cout << "세션 ID: " << pClientSession->sessionId << " 클라이언트 접속: IP" << clientIP << " SOCKET" << pClientSession->clientSocket << endl;

			++mClientCnt;
		}
	}

	void CloseSession(ClientSession* pClientSession)
	{
		shutdown(pClientSession->clientSocket, SD_BOTH);
		closesocket(pClientSession->clientSocket);
		pClientSession->state = SessionState::DISCONNECTING;
		pClientSession->sessionId = 0;
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
	}

	~IOCP_SERVER()
	{
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
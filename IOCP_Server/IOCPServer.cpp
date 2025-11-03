#include "IOCPServer.h"

IOCPServer::IOCPServer() 
	: mListenSocket(INVALID_SOCKET)
	, mIOCPHandle(nullptr)
	, mSessionManager(nullptr)
	, mSessionIdCounter(1)
	, mIsWorkerRun(true)
	, mIsAccepterRun(true)
{
}

IOCPServer::~IOCPServer()
{
	DestroyThread();

	delete mSessionManager;
	mSessionManager = nullptr;

	WSACleanup();
}

bool IOCPServer::InitSocket()
{
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "[IOCPServer] WSAStartup Error: " << WSAGetLastError() << endl;
		return false;
	}

	mListenSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (mListenSocket == INVALID_SOCKET)
	{
		cout << "[IOCPServer] WSASocket Error: " << WSAGetLastError() << endl;
		return false;
	}

	cout << "[IOCPServer] Socket initialized" << endl;
	return true;
}

bool IOCPServer::BindAndListen(int bindPort)
{
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(bindPort);

	if (bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) != 0)
	{
		cout << "[IOCPServer] bind Error: " << GetLastError() << endl;
		return false;
	}

	if (listen(mListenSocket, SOMAXCONN) != 0)
	{
		cout << "[IOCPServer] listen Error: " << GetLastError() << endl;
		return false;
	}

	cout << "[IOCPServer] Server listening on port " << bindPort << endl;
	return true;
}

bool IOCPServer::StartServer(UINT32 maxClientCount)
{
	mSessionManager = new SessionManager(maxClientCount);

	mIOCPHandle = CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, MAX_WORKERTHREAD);
	if (mIOCPHandle == nullptr)
	{
		cout << "[IOCPServer] CreateIoCompletionPort Error" << endl;
		return false;
	}

	for (int i = 0; i < MAX_WORKERTHREAD; i++)
	{
		mIOWorkerThreads.emplace_back([this]() { WorkerThread(); });
	}

	mAccepterThread = thread([this]() { AccepterThread(); });

	cout << "[IOCPServer] Server startew with " << MAX_WORKERTHREAD << " worker threads" << endl;
	return true;
}

void IOCPServer::DestroyThread()
{
	mIsWorkerRun = false;

	if (mIOCPHandle != nullptr)
	{
		CloseHandle(mIOCPHandle);
		mIOCPHandle = nullptr;
	}

	for (auto& thread : mIOWorkerThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	mIsAccepterRun = false;
	if (mListenSocket != INVALID_SOCKET)
	{
		closesocket(mListenSocket);
	}

	if (mAccepterThread.joinable()) {
		mAccepterThread.join();
	}

	cout << "[IOCPServer] All threads destroyed" << endl;
}

bool IOCPServer::BindIOCompletionPort(ClientSession* session)
{
	HANDLE handle = CreateIoCompletionPort(
		(HANDLE)session->GetSocket(),
		mIOCPHandle,
		(ULONG_PTR)session,
		0
	);

	if (handle == nullptr)
	{
		cout << "[IOCPServer] BindIOCompletionPort Error: " << GetLastError() << endl;
		return false;
	}

	return true;
}

void IOCPServer::WorkerThread()
{
	DWORD transferred = 0;
	ClientSession* session = nullptr;
	LPOVERLAPPED overlapped = nullptr;

	while (mIsWorkerRun)
	{
		BOOL success = GetQueuedCompletionStatus(
			mIOCPHandle,
			&transferred,
			(PULONG_PTR)&session,
			&overlapped,
			INFINITE
		);

		if (!success || transferred == 0)
		{
			if (session && session->IsValid())
			{
				cout << "[IOCPServer] Client disconnected. Session ID: " << session->GetSessionId() << endl;
				mSessionManager->UnregisterSession(session);
				session->Reset();
			}
			continue;
		}

		auto overlappedEx = (OverlappedEx*)overlapped;

		if (overlappedEx->operation == IOOperation::RECV)
		{
			session->GetRecvBuffer().Write(session->GetTempRecvBuf(), transferred);

			PacketHandler::ProcessPacket(session, mSessionManager);
			
			if (!session->RegisterRecv())
			{
				cout << "[IOCP Server] RegisterRecv failed! Session: " << session->GetSessionId() << endl;
				mSessionManager->UnregisterSession(session);
				session->Reset();
			}			
		}
		else if (overlappedEx->operation == IOOperation::SEND)
		{
			session->OnSendCompleted();
		}
	}
}

void IOCPServer::AccepterThread()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);

	while (mIsAccepterRun)
	{
		ClientSession* session = mSessionManager->GetEmptySession();
		if (session == nullptr)
		{
			cout << "[IOCPServer] Client pool is full" << endl;			
			continue;
		}

		SOCKET clientSocket = accept(mListenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
		{
			continue;
		}

		UINT32 sessionId = GenerateSessionId();
		session->Initialize(clientSocket, sessionId);

		if (!BindIOCompletionPort(session))
		{
			session->Reset();
			continue;
		}

		if (!session->RegisterRecv())
		{
			session->Reset();
			continue;
		}

		char clientIP[32] = { 0 };
		inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, sizeof(clientIP) - 1);
		cout << "[IOCPServer] Client connected - Session ID: " << sessionId
			<< ", IP: " << clientIP
			<< ", Socket: " << clientSocket << std::endl;
	}
}
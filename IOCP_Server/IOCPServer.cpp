#include "IOCPServer.h"

IOCPServer::IOCPServer()
	: mListenSocket(INVALID_SOCKET)
	, mIOCPHandle(nullptr)
	, mSessionManager(nullptr)
	, mRoomManager(nullptr)
	, mDbManager(nullptr)
	, mSessionIdCounter(1)
	, mIsAcceptRun(true)
{
	mPacketHandler = new PacketHandler();
}

IOCPServer::~IOCPServer()
{
	delete mSessionManager;
	mSessionManager = nullptr;

	delete mRoomManager;
	mRoomManager = nullptr;

	delete mPacketHandler;
	mPacketHandler = nullptr;

	delete mDbManager;
	mDbManager = nullptr;
	
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

	if (::bind(mListenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) != 0)
	{
		cout << "[IOCPServer] bind Error: " << GetLastError() << endl;
		return false;
	}

	if (::listen(mListenSocket, SOMAXCONN) != 0)
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
	mRoomManager = new RoomManager(MAX_ROOM_COUNT);
	mDbManager = new DbManager();

	if (!mDbManager->Init("localhost", 33060, "root", "1234", "chat"))
	{
		cout << "[IOCPServer] DbManager Init failed" << endl;
		return false;
	}

	if (!mDbManager->CreateTables())
	{
		cout << "[IOCPServer] DbManager CreateTables failed" << endl;
		return false;
	}

	if (!mDbManager->ClearTable())
	{
		cout << "[IOCPServer] DbManager ClearTable failed" << endl;
		return false;
	}

	mPacketHandler->SetSessionManager(mSessionManager);
	mPacketHandler->SetRoomManager(mRoomManager);
	mPacketHandler->SetDbManager(mDbManager);

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

	mAcceptThread = thread([this]() { AcceptThread(); });

	cout << "[IOCPServer] Server startew with " << MAX_WORKERTHREAD << " worker threads" << endl;
	return true;
}

void IOCPServer::StopServer()
{
	mIsAcceptRun = false;
	closesocket(mListenSocket);
	if (mAcceptThread.joinable())
		mAcceptThread.join();

	for (int i = 0; i < mIOWorkerThreads.size(); ++i)
	{
		PostQueuedCompletionStatus(mIOCPHandle, 0, 0, nullptr);
	}

	for (auto& thread : mIOWorkerThreads)
	{
		if (thread.joinable())
		{
			thread.join();
		}
	}

	mSessionManager->CloseAllSessions();

	if (mIOCPHandle != nullptr)
	{
		CloseHandle(mIOCPHandle);
		mIOCPHandle = nullptr;
	}

	cout << "[IOCPServer] Stop Server..." << endl;
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

void IOCPServer::DisconnectSession(ClientSession* session)
{
	if (session->GetUserState() == UserState::IN_ROOM)
		mRoomManager->LeaveRoom(session);

	mSessionManager->UnregisterSession(session);
}

void IOCPServer::WorkerThread()
{
	DWORD transferred = 0;
	ClientSession* session = nullptr;
	LPOVERLAPPED overlapped = nullptr;

	while (true)
	{
		BOOL success = GetQueuedCompletionStatus(
			mIOCPHandle,
			&transferred,
			(PULONG_PTR)&session,
			&overlapped,
			INFINITE
		);

		if (overlapped == nullptr)
			break;

		if (!success || transferred == 0)
		{
			DWORD err = GetLastError();

			if (!success &&
				err != ERROR_OPERATION_ABORTED &&
				err != ERROR_CONNECTION_ABORTED &&
				err != ERROR_NETNAME_DELETED)
			{
				cout << "[IOCP Server] Abnormal Disconnect (Error: " << err << ")\n";
			}

			if (session->TryDisconnect())
				DisconnectSession(session);

			continue;
		}

		auto overlappedEx = (OverlappedEx*)overlapped;

		if (overlappedEx->operation == IOOperation::RECV)
		{
			session->GetRecvBuffer().Write(session->GetTempRecvBuf(), transferred);
			session->UpdateActivity();

			bool packetOk = mPacketHandler->ProcessPacket(session);

			if (!packetOk || !session->RegisterRecv())
			{
				if (session->TryDisconnect())
					DisconnectSession(session);
			}
		}
		else if (overlappedEx->operation == IOOperation::SEND)
		{
			session->OnSendCompleted();
		}
	}
}

void IOCPServer::AcceptThread()
{
	SOCKADDR_IN clientAddr;
	int addrLen = sizeof(clientAddr);

	while (mIsAcceptRun)
	{
		SOCKET clientSocket = ::accept(mListenSocket, (SOCKADDR*)&clientAddr, &addrLen);
		if (clientSocket == INVALID_SOCKET)
			continue;

		ClientSession* session = mSessionManager->GetEmptySession();
		if (session == nullptr)
		{
			cout << "[IOCPServer] Client pool is full." << endl;
			::closesocket(clientSocket);
			continue;
		}

		uint32_t sessionId = GenerateSessionId();
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
	}
}
#include "TestClient.h"
#include <iostream>
#include <WS2tcpip.h>

TestClient::TestClient(int id, const string& name, const char* serverIP, int serverPort)
	: mId(id)
	, mName(name)
	, mServerIP(serverIP)
	, mServerPort(serverPort)
	, mRecvThread(nullptr)
	, mIsRunning(false)
	, mIsAuthenticated(false)
{
}

TestClient::~TestClient()
{
	Disconnect();
}

bool TestClient::Connect()
{
	mSocket = socket(PF_INET, SOCK_STREAM, 0);
	if (mSocket == INVALID_SOCKET)
	{
		cout << "[" << mName << "] Failed to create socket" << endl;
		return false;
	}
	
	SOCKADDR_IN serverAddr;
	ZeroMemory(&serverAddr, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, mServerIP, &serverAddr.sin_addr);
	serverAddr.sin_port = htons(mServerPort);

	if (connect(mSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
	{
		cout << "[" << mName << "] Connect failed" << endl;
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
		return false;
	}

	mIsRunning = true;

	mRecvThread = (HANDLE)_beginthreadex(nullptr, 0, RecvThreadFunc, this, 0, nullptr);

	cout << "[" << mName << "] Connected to server" << endl;
	return true;
}

void TestClient::Disconnect()
{
	mIsRunning = false;

	if (mSocket != INVALID_SOCKET)
	{
		shutdown(mSocket, SD_BOTH);
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}

	if (mRecvThread)
	{
		WaitForSingleObject(mRecvThread, INFINITE);
		CloseHandle(mRecvThread);
		mRecvThread = nullptr;
	}
}

bool TestClient::Login()
{
	LoginReqPacket loginPacket;
	loginPacket.SetLoginInfo(mName.c_str(), "testpw", mName.c_str());

	int sendBytes = send(mSocket, (char*)&loginPacket, loginPacket.size, 0);
	if (sendBytes == SOCKET_ERROR)
	{
		cout << "[" << mName << "] Login send error: " << WSAGetLastError() << endl;
		return false;
	}

	// 로그인 응답 대기 (최대 3초)
	for (int i = 0; i < 300 && !mIsAuthenticated && mIsRunning; i++)
	{
		Sleep(10);
	}

	return mIsAuthenticated;
}

bool TestClient::SendBroadcast(const string& message)
{
	if (!mIsAuthenticated)
	{
		cout << "[" << mName << "] Not authenticated" << endl;
		return false;
	}

	BroadcastReqPacket packet;
	packet.SetMessage(message.c_str());

	int sendBytes = send(mSocket, (char*)&packet, packet.size, 0);
	if (sendBytes == SOCKET_ERROR)
	{
		cout << "[" << mName << "] Broadcast send error: " << WSAGetLastError() << endl;
		return false;
	}

	return true;
}

bool TestClient::SendWhisper(const string& targetUser, const string& message)
{
	if (!mIsAuthenticated)
	{
		cout << "[" << mName << "] Not authenticated" << endl;
		return false;
	}

	WhisperChatReqPacket packet;
	packet.SetWhisper(targetUser.c_str(), message.c_str());

	int sendBytes = send(mSocket, (char*)&packet, packet.size, 0);
	if (sendBytes == SOCKET_ERROR)
	{
		cout << "[" << mName << "] Whisper send error: " << WSAGetLastError() << endl;
		return false;
	}

	return true;
}

unsigned WINAPI TestClient::RecvThreadFunc(void* arg)
{
	TestClient* client = (TestClient*)arg;
	client->RecvLoop();
	return 0;
}

void TestClient::RecvLoop()
{
	int totalRecv = 0;

	while (mIsRunning)
	{
		totalRecv = 0;
		while (totalRecv < sizeof(PacketHeader))
		{
			int recvSize = recv(mSocket, mRecvBuffer + totalRecv, sizeof(PacketHeader) - totalRecv, 0);
			if (recvSize <= 0)
			{
				if (recvSize == 0)
				{
					cout << "[" << mName << "] Server disconnected" << endl;
				}

				mIsRunning = false;
				return;
			}
			totalRecv += recvSize;
		}

		PacketHeader* header = (PacketHeader*)mRecvBuffer;
		int remainingSize = header->size - sizeof(PacketHeader);

		if (remainingSize > 0)
		{
			while (totalRecv < header->size)
			{
				int recvSize = recv(mSocket, mRecvBuffer + totalRecv, header->size - totalRecv, 0);
				if (recvSize <= 0)
				{
					mIsRunning = false;
					return;
				}
				totalRecv += recvSize;
			}
		}

		ProcessPacket(header);
	}
}

void TestClient::ProcessPacket(PacketHeader* packet)
{
	switch (packet->GetType())
	{
	case PacketType::LOGIN_RESPONSE:
		HandleLoginResponse((LoginResPacket*)packet);
		break;
	case PacketType::BROADCAST_RESPONSE:
		HandleBroadcastResponse((BroadcastResPacket*)packet);
		break;
	case PacketType::WHISPER_RESPONSE:
		HandleWhisperResponse((WhisperChatResPacket*)packet);
		break;

	default:
		cout << "[" << mName << "] Unknown packet type" << endl;
		break;
	}
}

void TestClient::HandleLoginResponse(LoginResPacket* packet)
{
	if (packet->result == ErrorCode::SUCCESS)
	{
		mIsAuthenticated = true;
		cout << "[" << mName << "] Login successful" << endl;
	}
	else
	{
		cout << "[" << mName << "] Login failed" << endl;
		mIsRunning = false;
	}
}

void TestClient::HandleBroadcastResponse(BroadcastResPacket* packet)
{
	mReceivedBroadcastCount++;
	cout << "[BROADCAST][" << packet->user << "]: " << packet->message << endl;
}

void TestClient::HandleWhisperResponse(WhisperChatResPacket* packet)
{
	if (packet->result == ErrorCode::SUCCESS)
	{
		cout << "[WHISPER][" << packet->sender << " -> " << mName << "]: " << packet->message << endl;
 	}
	else
	{
		cout << "[" << mName << "] Whisper failed: " << packet->message << endl;
	}
}


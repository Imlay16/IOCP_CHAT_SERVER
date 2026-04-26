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
	, mRegisterResponseArrived(false)
	, mRegisterResult(ErrorCode::SERVER_ERROR)
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

bool TestClient::SendAll(SOCKET sock, const char* data, int totalSize)
{
	int sent = 0;

	while (sent < totalSize)
	{
		int ret = send(sock, data + sent, totalSize - sent, 0);
		if (ret == SOCKET_ERROR || ret == 0)
		{
			return false;
		}

		sent += ret;
	}

	return true;
}

bool TestClient::Register(int num)
{
	mRegisterResponseArrived = false;
	mRegisterResult = ErrorCode::SERVER_ERROR;

	string id = "LoginId" + to_string(num);
	string nickname = "bot_" + to_string(num);

	RegisterReqPacket packet;
	packet.SetRegisterInfo(id.c_str(), "testpw", nickname.c_str());

	if (!SendAll(mSocket, (const char*)&packet, packet.size))
	{
		cout << "[Client " << num << "] Register send error: " << WSAGetLastError() << endl;
		return false;
	}

	for (int i = 0; i < 300 && !mRegisterResponseArrived && mIsRunning; i++)
	{
		Sleep(10);
	}

	if (!mRegisterResponseArrived)
	{
		cout << "[Client " << num << "] Register response timeout" << endl;
		return false;
	}

	return (mRegisterResult == ErrorCode::SUCCESS ||
		mRegisterResult == ErrorCode::ID_ALREADY_EXISTS);
}

bool TestClient::Login(int num)
{
	mIsAuthenticated = false;

	string id = "LoginId" + to_string(num);

	LoginReqPacket packet;
	packet.SetLoginInfo(id.c_str(), "testpw");

	if (!SendAll(mSocket, (const char*)&packet, packet.size))
	{
		cout << "[Client " << num << "] Login send error" << endl;
		return false;
	}

	for (int i = 0; i < 300 && !mIsAuthenticated && mIsRunning; i++)
	{
		Sleep(10);
	}

	return mIsAuthenticated;
}

bool TestClient::SendLobbyChat(const string& message)
{
	if (!mIsAuthenticated)
		return false;

	LobbyChatReqPacket packet;
	packet.SetMessage(message);

	return SendAll(mSocket, (const char*)&packet, packet.size);
}

bool TestClient::SendWhisper(int targetId, const string& message)
{
	if (!mIsAuthenticated)
		return false;

	string nickname = "bot_" + to_string(targetId);

	WhisperChatReqPacket packet;
	packet.SetWhisper(nickname.c_str(), message.c_str());

	return SendAll(mSocket, (const char*)&packet, packet.size);
}


bool TestClient::CreateRoom(const string& name, uint16_t maxUser)
{
	if (!mIsAuthenticated)
		return false;

	mCreateRoomArrived = false;
	mCreateRoomResult = ErrorCode::SERVER_ERROR;

	CreateRoomReqPacket packet;
	packet.SetRoomInfo(name, maxUser);

	if (!SendAll(mSocket, (const char*)&packet, packet.size))
		return false;

	for (int i = 0; i < 300 && !mCreateRoomArrived && mIsRunning; i++)
		Sleep(10);

	if (!mCreateRoomArrived)
	{
		cout << "[" << mName << "] CreateRoom response timeout" << endl;
		return false;
	}

	return mCreateRoomResult == ErrorCode::SUCCESS;
}

bool TestClient::RequestRoomList(uint16_t page)
{
	if (!mIsAuthenticated)
		return false;

	mRoomListArrived = false;
	mRoomListResult  = ErrorCode::SERVER_ERROR;
	mRoomListCount   = 0;

	RoomListReqPacket packet;
	packet.SetPage(page);

	if (!SendAll(mSocket, (const char*)&packet, packet.size))
		return false;

	for (int i = 0; i < 300 && !mRoomListArrived && mIsRunning; i++)
		Sleep(10);

	if (!mRoomListArrived)
	{
		cout << "[" << mName << "] RoomList response timeout" << endl;
		return false;
	}

	return mRoomListResult == ErrorCode::SUCCESS;
}

bool TestClient::JoinRoom(uint16_t roomId)
{
	if (!mIsAuthenticated)
		return false;

	mJoinRoomArrived = false;
	mJoinRoomResult = ErrorCode::SERVER_ERROR;

	JoinRoomReqPacket packet;
	packet.JoinRoom(roomId);

	if (!SendAll(mSocket, (const char*)&packet, packet.size))
		return false;

	for (int i = 0; i < 300 && !mJoinRoomArrived && mIsRunning; i++)
		Sleep(10);

	if (!mJoinRoomArrived)
	{
		cout << "[" << mName << "] JoinRoom response timeout" << endl;
		return false;
	}

	return mJoinRoomResult == ErrorCode::SUCCESS;
}

bool TestClient::LeaveRoom()
{
	if (!mIsAuthenticated)
		return false;

	mLeaveRoomArrived = false;
	mLeaveRoomResult = ErrorCode::SERVER_ERROR;

	LeaveRoomReqPacket packet;

	if (!SendAll(mSocket, (const char*)&packet, packet.size))
		return false;

	for (int i = 0; i < 300 && !mLeaveRoomArrived && mIsRunning; i++)
		Sleep(10);

	if (!mLeaveRoomArrived)
	{
		cout << "[" << mName << "] LeaveRoom response timeout" << endl;
		return false;
	}

	return mLeaveRoomResult == ErrorCode::SUCCESS;
}

bool TestClient::SendRoomChat(const string& message)
{
	if (!mIsAuthenticated || mCurrentRoomId == INVALID_ROOM_ID)
		return false;

	RoomChatReqPacket packet;
	packet.SetMessage(message);

	return SendAll(mSocket, (const char*)&packet, packet.size);
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
	case PacketType::REGISTER_RESPONSE:
		HandleRegisterResponse((RegisterResPacket*)packet);
		break;
	case PacketType::LOGIN_RESPONSE:
		HandleLoginResponse((LoginResPacket*)packet);
		break;
	case PacketType::LOBBY_CHAT_RESPONSE:
		HandleLobbyChatResponse((LobbyChatResPacket*)packet);
		break;
	case PacketType::LOBBY_CHAT_NOTIFY:
		HandleLobbyChatNoti((LobbyChatNotiPacket*)packet);
		break;
	case PacketType::WHISPER_RESPONSE:
		HandleWhisperResponse((WhisperChatResPacket*)packet);
		break;
	case PacketType::CREATE_ROOM_RESPONSE:
		HandleCreateRoomResponse((CreateRoomResPacket*)packet);
		break;
	case PacketType::ROOM_LIST_RESPONSE:
		HandleRoomListResponse((RoomListResPacket*)packet);
		break;
	case PacketType::JOIN_ROOM_RESPONSE:
		HandleJoinRoomResponse((JoinRoomResPacket*)packet);
		break;
	case PacketType::LEAVE_ROOM_RESPONSE:
		HandleLeaveRoomResponse((LeaveRoomResPacket*)packet);
		break;
	case PacketType::ROOM_CHAT_RESPONSE:
		HandleRoomChatResponse((RoomChatResPacket*)packet);
		break;
	case PacketType::ROOM_CHAT_NOTIFY:
		HandleRoomChatNoti((RoomChatNotiPacket*)packet);
		break;
	default:
		break;
	}
}

void TestClient::HandleRegisterResponse(RegisterResPacket* packet)
{
	mRegisterResult = packet->result;
	mRegisterResponseArrived = true;
}

void TestClient::HandleLoginResponse(LoginResPacket* packet)
{
	if (packet->result == ErrorCode::SUCCESS)
	{
		mIsAuthenticated = true;
		mName = packet->nickname;
	}
	else
	{
		mIsRunning = false;
	}
}

void TestClient::HandleLobbyChatResponse(LobbyChatResPacket* packet)
{
	// 응답 수신 (필요시 카운트)
}

void TestClient::HandleLobbyChatNoti(LobbyChatNotiPacket* packet)
{
	mReceivedLobbyChatCount++;
}

void TestClient::HandleWhisperResponse(WhisperChatResPacket* packet)
{
	if (packet->result == ErrorCode::SUCCESS)
	{
		mReceivedWhisperCount++;
	}
}


void TestClient::HandleCreateRoomResponse(CreateRoomResPacket* packet)
{
	mCreateRoomResult = packet->result;
	if (packet->result == ErrorCode::SUCCESS)
	{
		mCreatedRoomInfo = packet->room;
		mCurrentRoomId = packet->room.roomId;
	}
	mCreateRoomArrived = true;
}

void TestClient::HandleRoomListResponse(RoomListResPacket* packet)
{
	mRoomListResult = packet->result;
	if (packet->result == ErrorCode::SUCCESS)
	{
		mRoomListCount = packet->roomCount;
		memcpy_s(mRoomList, sizeof(mRoomList), packet->rooms, sizeof(RoomInfo) * packet->roomCount);
	}
	mRoomListArrived = true;
}

void TestClient::HandleJoinRoomResponse(JoinRoomResPacket* packet)
{
	mJoinRoomResult = packet->result;
	if (packet->result == ErrorCode::SUCCESS)
	{
		mCurrentRoomId = packet->room.roomId;
	}
	mJoinRoomArrived = true;
}

void TestClient::HandleLeaveRoomResponse(LeaveRoomResPacket* packet)
{
	mLeaveRoomResult = packet->result;
	if (packet->result == ErrorCode::SUCCESS)
	{
		mCurrentRoomId = INVALID_ROOM_ID;
	}
	mLeaveRoomArrived = true;
}

void TestClient::HandleRoomChatResponse(RoomChatResPacket* packet)
{
	// 응답 수신
}

void TestClient::HandleRoomChatNoti(RoomChatNotiPacket* packet)
{
	mReceivedRoomChatCount++;
}

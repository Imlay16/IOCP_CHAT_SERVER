#pragma once
#include <WinSock2.h>
#include <string>
#include <atomic>
#include <process.h>
#include "../Common/Packet.h"

#define MAX_SOCKBUF 2048

using namespace std;

class TestClient
{
public:
	TestClient(int id, const string& name, const char* serverIP, int serverPort);
	~TestClient();

	bool Connect();
	void Disconnect();
	bool SendAll(SOCKET sock, const char* data, int totalSize);
	bool Register(int num);
	bool Login(int num);
	bool SendLobbyChat(const string& message);
	bool SendWhisper(int targetId, const string& message);
	// Room
	bool CreateRoom(const string& name, uint16_t maxUser);
	bool RequestRoomList(uint16_t page = 0);
	bool JoinRoom(uint16_t roomId);
	bool LeaveRoom();
	bool SendRoomChat(const string& message);

	bool IsAuthenticated() const { return mIsAuthenticated; }
	bool IsRunning() const { return mIsRunning; }
	const string& GetName() const { return mName; }
	int GetId() const { return mId; }

	int GetReceivedLobbyChatCount() const { return mReceivedLobbyChatCount; }
	void ResetLobbyChatCount() { mReceivedLobbyChatCount = 0; }

	int GetReceivedWhisperCount() const { return mReceivedWhisperCount; }
	void ResetWhisperCount() { mReceivedWhisperCount = 0; }

	void ResetAllCounts() { mReceivedLobbyChatCount = 0; mReceivedWhisperCount = 0; }

	// Room getters
	uint16_t GetCurrentRoomId() const { return mCurrentRoomId; }
	RoomInfo GetCreatedRoomInfo() const { return mCreatedRoomInfo; }
	uint16_t GetRoomListCount() const { return mRoomListCount; }
	RoomInfo GetRoomListEntry(int idx) const { return mRoomList[idx]; }
	int GetReceivedRoomChatCount() const { return mReceivedRoomChatCount; }
	void ResetRoomChatCount() { mReceivedRoomChatCount = 0; }

private:
	static unsigned WINAPI RecvThreadFunc(void* arg);
	void RecvLoop();
	void ProcessPacket(PacketHeader* packet);

	void HandleRegisterResponse(RegisterResPacket* packet);
	void HandleLoginResponse(LoginResPacket* packet);
	void HandleLobbyChatResponse(LobbyChatResPacket* packet);
	void HandleLobbyChatNoti(LobbyChatNotiPacket* packet);
	void HandleWhisperResponse(WhisperChatResPacket* packet);
	// Room handlers
	void HandleCreateRoomResponse(CreateRoomResPacket* packet);
	void HandleRoomListResponse(RoomListResPacket* packet);
	void HandleJoinRoomResponse(JoinRoomResPacket* packet);
	void HandleLeaveRoomResponse(LeaveRoomResPacket* packet);
	void HandleRoomChatResponse(RoomChatResPacket* packet);
	void HandleRoomChatNoti(RoomChatNotiPacket* packet);

private:
	int mId;
	string mName;
	SOCKET mSocket;
	const char* mServerIP;
	int mServerPort;
	HANDLE mRecvThread;

	atomic<bool> mIsRunning;
	atomic<bool> mIsAuthenticated;
	atomic<bool> mRegisterResponseArrived;
	atomic<ErrorCode> mRegisterResult;

	char mRecvBuffer[MAX_SOCKBUF];
	atomic<int> mReceivedLobbyChatCount{ 0 };
	atomic<int> mReceivedWhisperCount{ 0 };

	// Room state
	atomic<uint16_t> mCurrentRoomId{ INVALID_ROOM_ID };

	atomic<bool> mCreateRoomArrived{ false };
	atomic<ErrorCode> mCreateRoomResult{ ErrorCode::SERVER_ERROR };
	RoomInfo mCreatedRoomInfo;

	atomic<bool>      mRoomListArrived{ false };
	atomic<ErrorCode> mRoomListResult{ ErrorCode::SERVER_ERROR };
	uint16_t          mRoomListCount{ 0 };
	RoomInfo          mRoomList[MAX_ROOM_PAGE_COUNT];

	atomic<bool> mJoinRoomArrived{ false };
	atomic<ErrorCode> mJoinRoomResult{ ErrorCode::SERVER_ERROR };

	atomic<bool> mLeaveRoomArrived{ false };
	atomic<ErrorCode> mLeaveRoomResult{ ErrorCode::SERVER_ERROR };

	atomic<int> mReceivedRoomChatCount{ 0 };
};

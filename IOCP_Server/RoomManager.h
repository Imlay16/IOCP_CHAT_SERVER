#pragma once
#include <vector>
#include <stack>
#include <unordered_map>
#include "RoomSession.h"

class RoomManager
{
public:
	RoomManager(uint32_t maxRoomCount);
	~RoomManager() = default;

	RoomSession* GetEmptyRoom();
	RoomSession* FindRoomById(uint16_t roomId);
	uint16_t CreateRoomSession(ClientSession* session, uint8_t maxUserCount);
	bool JoinRoom(ClientSession* session, uint16_t roomId);
	void LeaveRoom(ClientSession* session, uint16_t roomId);
	void RemoveRoomSession(RoomSession* room);

private:
	static constexpr int PAGECOUNT = 10;
	static constexpr UINT16 INVALID_ROOM_ID = 0xFFFF;

	vector<RoomSession> mRoomContainer;
	stack<int> mRoomIndexes;

	unordered_map<uint16_t, RoomSession*> mRoomById;

	int mCurrentPage;
	int mActiveRoomCount;

	SRWLOCK mSrwLock;
};


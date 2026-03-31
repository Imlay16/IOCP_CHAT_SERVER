#pragma once
#include <vector>
#include <stack>
#include <map>
#include <optional>
#include "RoomSession.h"
#include "..\Common\Packet.h"

class RoomManager
{
public:
	RoomManager(uint32_t maxRoomCount);
	~RoomManager() = default;

	std::optional<vector<RoomInfo>> GetRoomListByPage(uint16_t page);
	std::optional<RoomInfo> CreateRoomSession(ClientSession* session, uint16_t maxUserCount);
	bool JoinRoom(ClientSession* session, uint16_t roomId);
	void LeaveRoom(ClientSession* session, uint16_t roomId);
	
private:
	RoomSession* FindRoomById(uint16_t roomId);
	void RemoveRoomSession(RoomSession* room);
	RoomSession* GetEmptyRoom();

private:
	static constexpr int PAGECOUNT = 10;
	static constexpr uint16_t INVALID_ROOM_ID = 0xFFFF;

	vector<RoomSession> mRoomContainer;
	stack<int> mRoomIndexes;

	map<uint16_t, RoomSession*> mRoomById;

	int mActiveRoomCount;

	SRWLOCK mSrwLock;
};


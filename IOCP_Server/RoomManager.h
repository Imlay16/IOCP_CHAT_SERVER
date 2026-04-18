#pragma once
#include <vector>
#include <stack>
#include <map>
#include <optional>
#include <memory>
#include "RoomSession.h"
#include "..\Common\Packet.h"

class RoomManager
{
public:
	RoomManager(uint32_t maxRoomCount);
	~RoomManager() = default;

	RoomManager(const RoomManager&) = delete;
	RoomManager& operator=(const RoomManager&) = delete;
	RoomManager(RoomManager&&) = delete;
	RoomManager& operator=(RoomManager&&) = delete;

	std::optional<vector<RoomInfo>> GetRoomListByPage(uint16_t page);
	std::optional<RoomInfo> CreateRoomSession(ClientSession* session, std::string_view roomName, uint16_t maxUserCount);
	std::pair<ErrorCode, RoomSession*> JoinRoom(ClientSession* session, uint16_t roomId);
	ErrorCode LeaveRoom(ClientSession* session);
	ErrorCode RoomChat(ClientSession* session, const char* message);
	
private:
	RoomSession* FindRoomById(uint16_t roomId);
	void RemoveRoomSession(RoomSession* room);
	RoomSession* GetEmptyRoom();

private:
	static constexpr int PAGECOUNT = 10;

	vector<std::unique_ptr<RoomSession>> mRoomContainer;
	stack<int> mRoomIndexes;

	map<uint16_t, RoomSession*> mRoomById;

	int mActiveRoomCount;

	SRWLOCK mSrwLock;
};


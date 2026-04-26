#pragma once
#include <vector>
#include "ClientSession.h"
#include "SRWLockGuard.h"
#include "../Common/Packet.h"

using namespace std;

enum class RoomState {
	IDLE,     // 비활성
	ACTIVE,   // 활성
	CLOSING,  // 종료 중
};

class RoomSession
{
private:
	bool HasSession(ClientSession* session);

	void JoinNotify(ClientSession* joinSession);
	void LeaveNotify(ClientSession* leaveSession);

public:
	explicit RoomSession(uint16_t roomId);
	~RoomSession() = default;

	RoomSession(const RoomSession&) = delete;
	RoomSession& operator=(const RoomSession&) = delete;
	RoomSession(RoomSession&&) = delete;
	RoomSession& operator=(RoomSession&&) = delete;

	void Init(uint16_t maxUserCount);

	void SetRoomName(const string& name) { mName = name; }
	void SetMaxUserCount(uint16_t maxUserCount) { mMaxUserCount = maxUserCount; }

	uint16_t GetRoomId() const { return mRoomId; }
	const string& GetRoomName() const { return mName; }
	uint16_t GetMaxUserCount() const { return mMaxUserCount; }
	uint16_t GetCurrentUserCount() const { return mUsers.size(); }

	ErrorCode JoinUser(ClientSession* session);
	bool LeaveUser(ClientSession* session);

	bool IsFull() const { return mUsers.size() == mMaxUserCount; }
	bool IsEmpty() const { return mUsers.size() == 0; }

	RoomInfo ToRoomInfo() const;
	int FillUserList(UserInfo* outList, int maxCount) const;

	void BroadCast(const char* data, int length);

	void Clear();
private:
	uint16_t mRoomId;
	string mName;

	uint16_t mCurPage;
	uint16_t mMaxUserCount;

	vector<ClientSession*> mUsers;

	atomic<RoomState> mRoomState;
};


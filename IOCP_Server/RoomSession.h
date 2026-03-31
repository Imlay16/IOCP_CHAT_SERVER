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
	void BroadCastUnsafe(ClientSession* excludeSession, const char* data, int length);

public:
	RoomSession();
	~RoomSession() = default;

	void Init(uint16_t maxUserCount);

	void SetRoomId(uint16_t id) { mRoomId = id; }
	void SetRoomName(const string& name) { mName = name; }

	void SetMaxUserCount(uint16_t maxUserCount) { mMaxUserCount = maxUserCount; }

	uint16_t GetRoomId() const { return mRoomId; }
	const string& GetRoomName() const { return mName; }
	uint16_t GetMaxUserCount() const { return mMaxUserCount; }
	uint16_t GetCurrentUserCount() const { return mUsers.size(); }

	bool IsEmpty() const { return mUsers.size() < mMaxUserCount; }

	bool AddUser(ClientSession* session);
	bool RemoveUser(ClientSession* session);
	void Clear();

	void BroadCast(ClientSession* excludeSession, const char* data, int length);
	void JoinNotify(ClientSession* joinSession);
	void LeaveNotify(ClientSession* leaveSession);

private:
	uint16_t mRoomId;
	string mName;

	uint16_t mCurPage;

	uint16_t mMaxUserCount;

	RoomSession* mHost;
	vector<ClientSession*> mUsers;

	atomic<RoomState> mRoomState;

	SRWLOCK mSrwLock;
};


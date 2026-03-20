#pragma once
#include <vector>
#include "ClientSession.h"
#include "SRWLockGuard.h"

using namespace std;

class RoomSession
{
private:
	uint16_t mRoomId;
	string mName;

	uint16_t mCurPage;

	uint8_t mMaxUserCount;
	uint8_t mCurUserCount;

	RoomSession* mHost;
	vector<ClientSession*> mUsers;

	SRWLOCK mSrwLock;
public:
	RoomSession(uint8_t maxUserCount);
	~RoomSession();
	void BroadCast(ClientSession* excludeSession, const char* data, int length);
};


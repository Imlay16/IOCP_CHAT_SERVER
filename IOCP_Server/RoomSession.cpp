#include "RoomSession.h"

RoomSession::RoomSession(uint8_t maxUserCount)
	: mMaxUserCount(maxUserCount)
	, mRoomId(0)
	, mCurPage(1)
	, mCurUserCount(1)
	, mHost(nullptr)
{
	mUsers.reserve(mMaxUserCount);
}

void RoomSession::BroadCast(ClientSession* excludeSession, const char* data, int length)
{
	SRWLockGuard lock(&mSrwLock);

	for (auto* session : mUsers)
	{
		if (session->IsValid() && session != excludeSession)
		{
			session->SendPacket(data, length);
		}
	}
}
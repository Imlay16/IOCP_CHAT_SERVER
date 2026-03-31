#include "RoomSession.h"

RoomSession::RoomSession()
	: mMaxUserCount(2)
	, mRoomId(0)
	, mCurPage(0)
	, mHost(nullptr)
	, mRoomState(RoomState::IDLE)
{
	mUsers.reserve(mMaxUserCount);
}

void RoomSession::Init(uint16_t maxUserCount)
{
	mMaxUserCount = maxUserCount;
	mUsers.reserve(maxUserCount);
	mRoomState = RoomState::ACTIVE;
}

void RoomSession::Clear()
{
	mMaxUserCount = 2;
	mUsers.clear();
	mRoomState = RoomState::IDLE;
}

bool RoomSession::AddUser(ClientSession* session)
{
	if (mRoomState != RoomState::ACTIVE)
		return false;

	if (mUsers.size() >= mMaxUserCount)
		return false;

	mUsers.push_back(session);

	JoinNotify(session);

	return true;
}

bool RoomSession::RemoveUser(ClientSession* session)
{
	auto it = std::find(mUsers.begin(), mUsers.end(), session);
	if (it != mUsers.end()) {
		std::iter_swap(it, mUsers.end() - 1);
		mUsers.pop_back();

		LeaveNotify(session);
	}

	if (mUsers.empty())
		mRoomState = RoomState::CLOSING;

	return mUsers.empty();
}

void RoomSession::BroadCast(ClientSession* excludeSession, const char* data, int length)
{
	SRWLockGuard lock(&mSrwLock);
	BroadCastUnsafe(excludeSession, data, length);
}

void RoomSession::JoinNotify(ClientSession* joinSession)
{
	string joinMsg(joinSession->GetUsername() + " Joined the room.");
	BroadCastUnsafe(joinSession, joinMsg.c_str(), joinMsg.size());
}

void RoomSession::LeaveNotify(ClientSession* leaveSession)
{
	string leaveMsg(leaveSession->GetUsername() + " left the room.");
	BroadCastUnsafe(leaveSession, leaveMsg.c_str(), leaveMsg.size());
}

void RoomSession::BroadCastUnsafe(ClientSession* excludeSession, const char* data, int length)
{
	for (auto* session : mUsers)
	{
		if (session->IsValid() && session != excludeSession)
		{
			session->SendPacket(data, length);
		}
	}
}
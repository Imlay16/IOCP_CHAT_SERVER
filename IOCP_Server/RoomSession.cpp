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

ErrorCode RoomSession::JoinUser(ClientSession* session)
{
	if (mRoomState != RoomState::ACTIVE)
		return ErrorCode::INVALID_ROOM_REQUEST;

	if (IsFull())
		return ErrorCode::ROOM_FULL;

	if (HasSession(session))
		return ErrorCode::ALREADY_IN_ROOM;

	mUsers.push_back(session);
	JoinNotify(session);

	return ErrorCode::SUCCESS;
}

bool RoomSession::LeaveUser(ClientSession* session)
{
	auto it = std::find(mUsers.begin(), mUsers.end(), session);
	if (it == mUsers.end())
		return false;

	std::iter_swap(it, mUsers.end() - 1);
	mUsers.pop_back();
	LeaveNotify(session);

	if (mUsers.empty())
		mRoomState = RoomState::CLOSING;
	
	return mUsers.empty();
}

bool RoomSession::HasSession(ClientSession* session)
{
	for (auto* u : mUsers)
	{
		if (u == session)
			return true;
	}
	return false;
}

void RoomSession::JoinNotify(ClientSession* joinSession)
{
	string joinMsg(joinSession->GetUsername() + " Joined the room.");
	BroadCast(joinSession, joinMsg.c_str(), joinMsg.size());
}

void RoomSession::LeaveNotify(ClientSession* leaveSession)
{
	string leaveMsg(leaveSession->GetUsername() + " left the room.");
	BroadCast(leaveSession, leaveMsg.c_str(), leaveMsg.size());
}

void RoomSession::BroadCast(ClientSession* excludeSession, const char* data, int length)
{
	for (auto* session : mUsers)
	{
		if (session->IsValid() && session != excludeSession)
		{
			session->SendPacket(data, length);
		}
	}
}
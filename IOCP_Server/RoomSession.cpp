#include "RoomSession.h"

RoomSession::RoomSession(uint16_t roomId)
	: mMaxUserCount(2)
	, mRoomId(roomId)
	, mCurPage(0)
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

	session->SetUserState(UserState::IN_ROOM);
	session->SetRoomId(mRoomId);

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

	session->SetUserState(UserState::LOBBY);
	session->SetRoomId(INVALID_ROOM_ID);

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
	SystemNotiPacket notiPacket;
	string joinMsg = joinSession->GetUsername() + " Joined the room.";
	strncpy_s(notiPacket.message, sizeof(notiPacket.message), joinMsg.c_str(), _TRUNCATE);
	BroadCast((char*)&notiPacket, sizeof(notiPacket));
}

void RoomSession::LeaveNotify(ClientSession* leaveSession)
{
	SystemNotiPacket notiPacket;
	string leaveMsg = leaveSession->GetUsername() + " left the room.";
	strncpy_s(notiPacket.message, sizeof(notiPacket.message), leaveMsg.c_str(), _TRUNCATE);
	BroadCast((char*)&notiPacket, sizeof(notiPacket));
}

void RoomSession::BroadCast(const char* data, int length)
{
	for (auto* session : mUsers)
	{
		if (session->IsValid())
		{
			session->SendPacket(data, length);
		}
	}
}

RoomInfo RoomSession::ToRoomInfo() const
{
	return RoomInfo(mRoomId, mName, mMaxUserCount, (uint16_t)mUsers.size());
}

int RoomSession::FillUserList(UserInfo* outList, int maxCount) const
{
	int count = 0;
	for (auto* session : mUsers)
	{
		if (count >= maxCount) break;
		outList[count++] = session->ToUserInfo();
	}
	return count;
}
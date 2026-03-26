#include "RoomManager.h"

RoomManager::RoomManager(uint32_t maxRoomCount) : mActiveRoomCount(0)
{
	InitializeSRWLock(&mSrwLock);

	mRoomContainer.resize(maxRoomCount);  
	for (uint32_t i = 0; i < maxRoomCount; ++i)
	{
		mRoomIndexes.push(i);
	}
}

RoomSession* RoomManager::GetEmptyRoom()
{
	RoomSession* room = nullptr;

	SRWLockGuard lock(&mSrwLock);

	if (!mRoomIndexes.empty())
	{
		int idx = mRoomIndexes.top();
		mRoomIndexes.pop();

		room = &mRoomContainer[idx];
		room->SetRoomId(idx);
	}

	return room;
}

RoomSession* RoomManager::FindRoomById(uint16_t roomId)
{
	SRWLockGuard lock(&mSrwLock, false);

	auto it = mRoomById.find(roomId);
	RoomSession* result = (it != mRoomById.end()) ? it->second : nullptr;

	return result;
}

UINT16 RoomManager::CreateRoomSession(ClientSession* session, uint8_t maxUserCount)
{
	RoomSession* room = GetEmptyRoom();
	if (room == nullptr) return INVALID_ROOM_ID;

	room->Init(maxUserCount);
	room->AddUser(session);

	{
		SRWLockGuard lock(&mSrwLock, true);
		mRoomById[room->GetRoomId()] = room;
		mActiveRoomCount++;
		mCurrentPage = (mActiveRoomCount - 1) / PAGECOUNT;
	}

	return room->GetRoomId();
}

bool RoomManager::JoinRoom(ClientSession* session, uint16_t roomId)
{
	auto room = FindRoomById(roomId);

	if (room == nullptr) 
		return false;

	if (room->GetRemainUserCount() == 0) 
		return false;

	room->AddUser(session);
	return true;
}

void RoomManager::LeaveRoom(ClientSession* session, uint16_t roomId)
{
	auto room = FindRoomById(roomId);

	if (room == nullptr)
		return;

	if (room->RemoveUser(session))
	{
		RemoveRoomSession(room);
		mActiveRoomCount--;
		mCurrentPage = (mActiveRoomCount + PAGECOUNT - 1) / PAGECOUNT;
	}	
}

void RoomManager::RemoveRoomSession(RoomSession* room)
{
	SRWLockGuard lock(&mSrwLock);

	mRoomById.erase(room->GetRoomId());
	room->Clear();

	int index = room - &mRoomContainer[0];
	mRoomIndexes.push(index);

	mActiveRoomCount--;
}
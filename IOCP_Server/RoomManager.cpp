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
	auto it = mRoomById.find(roomId);
	RoomSession* result = (it != mRoomById.end()) ? it->second : nullptr;

	return result;
}

std::optional<RoomInfo> RoomManager::CreateRoomSession(ClientSession* session, uint16_t maxUserCount)
{
	SRWLockGuard lock(&mSrwLock);

	RoomSession* room = GetEmptyRoom();
	if (room == nullptr) return std::nullopt;

	room->Init(maxUserCount);
	room->AddUser(session);

	mRoomById[room->GetRoomId()] = room;
	mActiveRoomCount++;

	return RoomInfo{ room->GetRoomId(), room->GetRoomName(), maxUserCount, 1 };
}

std::optional<vector<RoomInfo>> RoomManager::GetRoomListByPage(uint16_t page)
{
	SRWLockGuard lock(&mSrwLock, false); 

	vector<RoomInfo> list;
	list.reserve(MAX_ROOM_PAGE_COUNT);

	// 페이지가 존재하는지 확인
	uint16_t totalPage = mRoomById.size() / MAX_ROOM_PAGE_COUNT;
	if (mRoomById.size() % MAX_ROOM_PAGE_COUNT != 0)
		totalPage++;

	// 페이지가 없는 경우 
	if (page >= totalPage)
		return std::nullopt;

	uint16_t startIdx = page * MAX_ROOM_PAGE_COUNT;
	uint16_t idx = 0;

	for (auto& [_, room] : mRoomById)
	{
		if (idx < startIdx) { idx++; continue; }
		if (idx >= startIdx + MAX_ROOM_PAGE_COUNT) break;

		list.push_back(RoomInfo{ room->GetRoomId(),
								 room->GetRoomName(),
								 room->GetMaxUserCount(),
								 room->GetCurrentUserCount() });
		idx++;
	}
	return list;
}

bool RoomManager::JoinRoom(ClientSession* session, uint16_t roomId)
{
	SRWLockGuard lock(&mSrwLock);

	auto room = FindRoomById(roomId);

	if (room == nullptr) 
		return false;

	if (!room->IsEmpty())
		return false;

	room->AddUser(session);
	return true;
}

void RoomManager::LeaveRoom(ClientSession* session, uint16_t roomId)
{
	SRWLockGuard lock(&mSrwLock);

	auto room = FindRoomById(roomId);

	if (room == nullptr)
		return;

	if (room->RemoveUser(session))
	{
		RemoveRoomSession(room);
	}	
}

void RoomManager::RemoveRoomSession(RoomSession* room)
{
	mRoomById.erase(room->GetRoomId());
	room->Clear();

	int index = room - &mRoomContainer[0];
	mRoomIndexes.push(index);

	mActiveRoomCount--;
}
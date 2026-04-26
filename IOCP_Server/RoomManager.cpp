#include "RoomManager.h"

RoomManager::RoomManager(uint32_t maxRoomCount) : mActiveRoomCount(0)
{
	InitializeSRWLock(&mSrwLock);

	mRoomContainer.reserve(maxRoomCount);
	for (uint32_t i = 0; i < maxRoomCount; ++i)
	{
		mRoomContainer.emplace_back(std::make_unique<RoomSession>(i));
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

		room = mRoomContainer[idx].get();
	}

	return room;
}

RoomSession* RoomManager::FindRoomById(uint16_t roomId)
{
	auto it = mRoomById.find(roomId);
	RoomSession* result = (it != mRoomById.end()) ? it->second : nullptr;

	return result;
}

std::optional<RoomInfo> RoomManager::CreateRoomSession(ClientSession* session, std::string_view roomName, uint16_t maxUserCount)
{
	SRWLockGuard lock(&mSrwLock);

	RoomSession* room = GetEmptyRoom();
	if (room == nullptr) return std::nullopt;

	room->Init(maxUserCount);
	room->SetRoomName(string(roomName));
	room->JoinUser(session);

	mRoomById[room->GetRoomId()] = room;
	mActiveRoomCount++;

	return RoomInfo{ room->GetRoomId(), room->GetRoomName(), maxUserCount, 1 };
}

std::optional<vector<RoomInfo>> RoomManager::GetRoomListByPage(uint16_t page)
{
	SRWLockGuard lock(&mSrwLock, false);

	vector<RoomInfo> list;
	list.reserve(MAX_ROOM_PAGE_COUNT);

	if (mRoomById.empty())
		return list;

	uint16_t totalPage = static_cast<uint16_t>(mRoomById.size() / MAX_ROOM_PAGE_COUNT);
	if (mRoomById.size() % MAX_ROOM_PAGE_COUNT != 0)
		totalPage++;

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

std::pair<ErrorCode, RoomSession*> RoomManager::JoinRoom(ClientSession* session, uint16_t roomId)
{
	SRWLockGuard lock(&mSrwLock);
	auto room = FindRoomById(roomId);
	if (room == nullptr)
		return { ErrorCode::ROOM_NOT_FOUND, nullptr };

	ErrorCode result = room->JoinUser(session);

	return { result, result == ErrorCode::SUCCESS ? room : nullptr };
}

ErrorCode RoomManager::LeaveRoom(ClientSession* session)
{
	if (session->GetUserState() != UserState::IN_ROOM)
		return ErrorCode::INVALID_STATE;

	SRWLockGuard lock(&mSrwLock);

	auto room = FindRoomById(session->GetRoomId());
	if (room == nullptr)
		return ErrorCode::ROOM_NOT_FOUND;

	room->LeaveUser(session);

	if (room->IsEmpty())
		RemoveRoomSession(room);

	return ErrorCode::SUCCESS;
}

void RoomManager::RemoveRoomSession(RoomSession* room)
{
	mRoomById.erase(room->GetRoomId());
	room->Clear();

	mRoomIndexes.push(room->GetRoomId());

	mActiveRoomCount--;
}

ErrorCode RoomManager::RoomChat(ClientSession* session, const char* message)
{
	if (session->GetUserState() != UserState::IN_ROOM)
		return ErrorCode::INVALID_STATE;

	SRWLockGuard lock(&mSrwLock);

	auto room = FindRoomById(session->GetRoomId());
	if (room == nullptr)
		return ErrorCode::ROOM_NOT_FOUND;

	RoomChatNotiPacket notiPacket;
	strncpy_s(notiPacket.user, sizeof(notiPacket.user), session->GetUsername().c_str(), _TRUNCATE);
	strncpy_s(notiPacket.message, sizeof(notiPacket.message), message, _TRUNCATE);

	room->BroadCast((char*)&notiPacket, sizeof(notiPacket));
	return ErrorCode::SUCCESS;
}
#include "SessionManager.h"
#include <iostream>

SessionManager::SessionManager(UINT32 maxSessionCount) : mActiveSessionCount(0)
{
	InitializeSRWLock(&mSrwLock);

	mSessionContainer.reserve(maxSessionCount);

	for (UINT32 i = 0; i < maxSessionCount; ++i)
	{
		mSessionContainer.emplace_back(std::make_unique<ClientSession>(i));
		mSessionIndexes.push(i);
	}	
}

ClientSession* SessionManager::GetEmptySession()
{
	SRWLockGuard lock(&mSrwLock);

	ClientSession* client = nullptr;

	if (!mSessionIndexes.empty())
	{
		int idx = mSessionIndexes.top();
		mSessionIndexes.pop();

		client = mSessionContainer[idx].get();	 
	}
	
	return client;
}

ClientSession* SessionManager::FindSessionByLoginId(const string& loginId)
{
	SRWLockGuard lock(&mSrwLock, false);

	auto nameIt = mSessionIdByLoginId.find(loginId);
	if (nameIt == mSessionIdByLoginId.end())
	{
		return nullptr;
	}

	UINT32 sessionId = nameIt->second;
	auto sessionIt = mSessionById.find(sessionId);
	ClientSession* result = (sessionIt != mSessionById.end()) ? sessionIt->second : nullptr;

	return result;
}

ClientSession* SessionManager::FindSessionById(UINT32 sessionId)
{
	SRWLockGuard lock(&mSrwLock, false);

	auto it = mSessionById.find(sessionId);
	ClientSession* result = (it != mSessionById.end()) ? it->second : nullptr;

	return result;
}

ClientSession* SessionManager::FindSessionByUsername(const string& username)
{
	auto it = mSessionByUsername.find(username);
	if (it == mSessionByUsername.end())
		return nullptr;
	return it->second;
}

void SessionManager::RegisterSession(ClientSession* session)
{
	SRWLockGuard lock(&mSrwLock);

	mSessionIdByLoginId[session->GetLoginId()] = session->GetSessionId();
	mSessionById[session->GetSessionId()] = session;
	mSessionByUsername[session->GetUsername()] = session;
	mActiveSessionCount++;
}

void SessionManager::UnregisterSession(ClientSession* session)
{
	SRWLockGuard lock(&mSrwLock);

	mSessionIdByLoginId.erase(session->GetLoginId());
	mSessionById.erase(session->GetSessionId());
	mSessionByUsername.erase(session->GetUsername());
	mSessionIndexes.push(session->GetPoolIndex());
	mActiveSessionCount--;

	session->Reset();
}

ErrorCode SessionManager::LobbyChat(ClientSession* session, const char* message)
{
	if (session->GetUserState() != UserState::LOBBY)
		return ErrorCode::INVALID_STATE;

	LobbyChatNotiPacket notiPacket;
	strncpy_s(notiPacket.user, sizeof(notiPacket.user), session->GetUsername().c_str(), _TRUNCATE);
	strncpy_s(notiPacket.message, sizeof(notiPacket.message), message, _TRUNCATE);

	BroadcastToLobby((char*)&notiPacket, sizeof(notiPacket));
	return ErrorCode::SUCCESS;
}

ErrorCode SessionManager::WhisperChat(ClientSession* sender, const char* targetName, const char* message)
{
	auto* target = FindSessionByUsername(targetName);
	if (target == nullptr)
		return ErrorCode::USER_NOT_FOUND;

	WhisperChatNotiPacket notiPacket;
	strncpy_s(notiPacket.sender, sizeof(notiPacket.sender), sender->GetUsername().c_str(), _TRUNCATE);
	strncpy_s(notiPacket.message, sizeof(notiPacket.message), message, _TRUNCATE);

	target->SendPacket((char*)&notiPacket, sizeof(notiPacket));
	return ErrorCode::SUCCESS;
}

void SessionManager::BroadcastAll(const char* data, int length)
{
	SRWLockGuard lock(&mSrwLock, false);

	for (auto& session : mSessionContainer)
	{
		if (session->IsValid())
		{
			session->SendPacket(data, length);
		}
	}
}

void SessionManager::BroadcastToLobby(const char* data, int length)
{
	SRWLockGuard lock(&mSrwLock, false);
	for (auto& session : mSessionContainer)
	{
		if (session->IsValid() && session->GetUserState() == UserState::LOBBY)
			session->SendPacket(data, length);
	}
}

void SessionManager::CloseAllSessions()
{
	for (auto& session : mSessionContainer)
	{
		if (session->GetState() != SessionState::IDLE)
		{
			closesocket(session->GetSocket());
		}
	}
}

void SessionManager::SystemNotify(const char* message)
{
	SystemNotiPacket notiPacket;
	strncpy_s(notiPacket.message, sizeof(notiPacket.message), message, _TRUNCATE);
	BroadcastAll((char*)&notiPacket, sizeof(notiPacket));
}

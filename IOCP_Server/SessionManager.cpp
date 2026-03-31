#include "SessionManager.h"
#include <iostream>

SessionManager::SessionManager(UINT32 maxSessionCount) : mActiveSessionCount(0)
{
	InitializeSRWLock(&mSrwLock);

	mSessionContainer.reserve(maxSessionCount);

	for (UINT32 i = 0; i < maxSessionCount; ++i)
	{
		mSessionContainer.emplace_back();
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

		client = &mSessionContainer[idx];	 
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

void SessionManager::RegisterSession(ClientSession* session)
{
	SRWLockGuard lock(&mSrwLock);

	mSessionIdByLoginId[session->GetLoginId()] = session->GetSessionId();
	mSessionById[session->GetSessionId()] = session;
	mActiveSessionCount++;
}

void SessionManager::UnregisterSession(ClientSession* session)
{
	SRWLockGuard lock(&mSrwLock);

	mSessionIdByLoginId.erase(session->GetUsername());
	mSessionById.erase(session->GetSessionId());
	mSessionIndexes.push(session - &mSessionContainer[0]);
	mActiveSessionCount--;

	session->Reset();
}

void SessionManager::BroadcastPacket(ClientSession* excludeSession, const char* data, int length)
{
	SRWLockGuard lock(&mSrwLock, false);

	for (auto& session : mSessionContainer)
	{
		if (session.IsValid() && &session != excludeSession)
		{
			session.SendPacket(data, length);
		}
	}
}

void SessionManager::CloseAllSessions()
{
	for (auto& session : mSessionContainer)
	{
		if (session.GetState() != SessionState::IDLE)
		{
			closesocket(session.GetSocket());
		}
	}
}

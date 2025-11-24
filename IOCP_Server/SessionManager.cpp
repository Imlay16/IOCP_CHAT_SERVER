#include "SessionManager.h"
#include "SRWLockGuard.h"
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

SessionManager::~SessionManager() { }

ClientSession* SessionManager::GetEmptySession()
{
	SRWLockGuard lock(&mSrwLock, true);

	ClientSession* client = nullptr;

	if (!mSessionIndexes.empty())
	{
		int idx = mSessionIndexes.top();
		mSessionIndexes.pop();

		client = &mSessionContainer[idx];	 
	}
	
	return client;
}

ClientSession* SessionManager::FindSessionByName(const string& name)
{
	SRWLockGuard lock(&mSrwLock, false);

	auto nameIt = mSessionIdByName.find(name);
	if (nameIt == mSessionIdByName.end())
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

	mSessionIdByName[session->GetUsername()] = session->GetSessionId();
	mSessionById[session->GetSessionId()] = session;
	mActiveSessionCount++;
}

void SessionManager::UnregisterSession(ClientSession* session)
{
	SRWLockGuard lock(&mSrwLock);

	// ClientSession ÃÊ±âÈ­
	session->Reset();
	mSessionIndexes.push(session - &mSessionContainer[0]);

	mSessionIdByName.erase(session->GetUsername());
	mSessionById.erase(session->GetSessionId());
	
	mActiveSessionCount--;
}

void SessionManager::BroadcastPacket(ClientSession* excludeSession, const char* data, int length)
{
	SRWLockGuard lock(&mSrwLock);

	for (auto& session : mSessionContainer)
	{
		if (session.IsValid() && &session != excludeSession)
		{
			session.SendPacket(data, length);
		}
	}
}

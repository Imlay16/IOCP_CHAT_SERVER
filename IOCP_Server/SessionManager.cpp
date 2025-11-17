#include "SessionManager.h"
#include "SRWLockGuard.h"
#include <iostream>

SessionManager::SessionManager(UINT32 maxSessionCount) : mActiveSessionCount(0)
{
	InitializeSRWLock(&mSrwLock);

	mSessions.reserve(maxSessionCount);
	for (UINT32 i = 0; i < maxSessionCount; i++)
	{
		mSessions.emplace_back();
	}
}

SessionManager::~SessionManager() { }

// GetEmptySession 찾는 것 O(1)로 하는 법은?

ClientSession* SessionManager::GetEmptySession()
{
	for (auto& session : mSessions)
	{
		if (!session.IsValid())
		{
			return &session;
		}
	}
	return nullptr;
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

	mSessionIdByName.erase(session->GetUsername());
	mSessionById.erase(session->GetSessionId());
	mActiveSessionCount--;
}

void SessionManager::BroadcastPacket(ClientSession* excludeSession, const char* data, int length)
{
	SRWLockGuard lock(&mSrwLock);

	for (auto& session : mSessions)
	{
		if (session.IsValid() && &session != excludeSession)
		{
			session.SendPacket(data, length);
		}
	}
}

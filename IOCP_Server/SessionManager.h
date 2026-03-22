#pragma once
#include "ClientSession.h"
#include "SRWLockGuard.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <Windows.h>


class SessionManager
{
public:
	SessionManager(UINT32 maxSessionCount);
	~SessionManager();

	ClientSession* GetEmptySession();
	ClientSession* FindSessionByLoginId(const string& loginId);
	ClientSession* FindSessionById(UINT32 sessionId);

	void RegisterSession(ClientSession* session);
	void UnregisterSession(ClientSession* session);

	void BroadcastPacket(ClientSession* excludeSession, const char* data, int length);
	void CloseAllSessions();

	int GetActiveSessionCount() const { return mActiveSessionCount; }

private:
	vector<ClientSession> mSessionContainer;
	stack<int> mSessionIndexes;

	// vector<ClientSession> mSessions;

	unordered_map<string, UINT32> mSessionIdByLoginId;
	unordered_map<UINT32, ClientSession*> mSessionById;

	atomic<int> mActiveSessionCount;	
	SRWLOCK mSrwLock;
};


#pragma once
#include "ClientSession.h"
#include "SRWLockGuard.h"
#include <vector>
#include <stack>
#include <unordered_map>
#include <Windows.h>
#include <memory>
#include "../Common/Packet.h"

class SessionManager
{
public:
	SessionManager(UINT32 maxSessionCount);
	~SessionManager() = default;

	ClientSession* GetEmptySession();
	ClientSession* FindSessionByLoginId(const string& loginId);
	ClientSession* FindSessionByUsername(const string& username);
	ClientSession* FindSessionById(UINT32 sessionId);

	void RegisterSession(ClientSession* session);
	void UnregisterSession(ClientSession* session);

	ErrorCode LobbyChat(ClientSession* session, const char* message);
	ErrorCode WhisperChat(ClientSession* sender, const char* targetName, const char* message);

	void BroadcastAll(const char* data, int length);
	void BroadcastToLobby(const char* data, int length);

	void CloseAllSessions();

	int GetActiveSessionCount() const { return mActiveSessionCount; }

	void SystemNotify(const char* message);

private:
	vector<std::unique_ptr<ClientSession>> mSessionContainer;
	stack<int> mSessionIndexes;

	unordered_map<string, UINT32> mSessionIdByLoginId;
	unordered_map<string, ClientSession*> mSessionByUsername;
	unordered_map<UINT32, ClientSession*> mSessionById;

	atomic<int> mActiveSessionCount;	
	SRWLOCK mSrwLock;
};


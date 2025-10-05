#pragma once
#include <WinSock2.h>
#include <string>
#include <queue>
#include <vector>

#define MAX_SOCKBUF 2048

using namespace std;

enum class SessionState
{
	CONNECTD,
	AUTHENTICATED,
	DISCONNECTING
};

enum class IOOperation
{
	RECV,
	SEND
};

struct OverlappedEx
{
	WSAOVERLAPPED wsaOverlapped;
	WSABUF wsaBuf;
	IOOperation operation;
};


class ClientSession
{
public:
	ClientSession();
	~ClientSession();

	void Initialize(SOCKET socket, UINT32 sessionId);
	void Reset();

	bool SendPacekt(const char* data, int length);
	bool RegisterRecv();
	void OnSendCompleted();

	// Getter
	UINT32 GetSessionId() const { return mSessionId; }
	SOCKET GetSocket() const { return mSocket; }
	SessionState GetState() const { return mState; }	
	const string& GetUsername() const { return mUsername; }
	char* GetRecvBuffer() { return mRecvBuf; }
	DWORD GetAccumulatedSize() const { return mAccumulatedSize; }

	// Setter
	void SetState(SessionState state) { mState = state; }
	void SetUsername(const string& username) { mUsername = username; }
	void AddAccumulatedSize(DWORD size) { mAccumulatedSize += size; }
	void SetAccumulatedSize(DWORD size) { mAccumulatedSize = size; }
	void ResetAccumulatedSize() { mAccumulatedSize = 0; }

	bool IsValid() const { return mSocket != INVALID_SOCKET; }
	bool IsAuthenticated() const { return mState == SessionState::AUTHENTICATED; }

private:
	void ProcessSend();

private:

	// Session 
	UINT32 mSessionId;
	SOCKET mSocket;
	string mUsername;
	SessionState mState;

	// Recv
	OverlappedEx mRecvOverlappedEx;
	char mRecvBuf[MAX_SOCKBUF];
	DWORD mAccumulatedSize;

	// Send
	OverlappedEx mSendOverlappedEx;
	char mSendBuf[MAX_SOCKBUF];
	queue<vector<char>> mSendQueue;
	bool mIsSending;
	SRWLOCK mSendLock;
};


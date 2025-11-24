#pragma once
#include <WinSock2.h>
#include <string>
#include <queue>
#include <vector>
#include "RingBuffer.h"

#define MAX_SOCKBUF 4096

using namespace std;

enum class SessionState
{
	CONNECTED,
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

	ClientSession(ClientSession&&) noexcept = default;
	ClientSession& operator=(ClientSession&&) noexcept = default;

	ClientSession(const ClientSession&) = delete;
	ClientSession& operator=(const ClientSession&) = delete;

	void Initialize(SOCKET socket, UINT32 sessionId);
	void Reset();

	bool SendPacket(const char* data, int length);
	bool RegisterRecv();
	void OnSendCompleted();

	// Getter
	UINT32 GetSessionId() const { return mSessionId; }
	SOCKET GetSocket() const { return mSocket; }
	SessionState GetState() const { return mState; }	
	const string& GetUsername() const { return mUsername; }

	char* GetTempRecvBuf() { return mTempRecvBuf; }
	RingBuffer& GetRecvBuffer() { return mRecvBuffer; }

	// Setter
	void SetState(SessionState state) { mState = state; }
	void SetUsername(const string& username) { mUsername = username; }

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
	RingBuffer mRecvBuffer;
	char mTempRecvBuf[MAX_SOCKBUF];

	// Send
	OverlappedEx mSendOverlappedEx;
	char mSendBuf[MAX_SOCKBUF];
	queue<vector<char>> mSendQueue;
	bool mIsSending;
	SRWLOCK mSendLock;

	DWORD mLastRecvTime;
};


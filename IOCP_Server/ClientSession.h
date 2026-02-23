#pragma once
#include <WinSock2.h>
#include <string>
#include <queue>
#include <vector>
#include <chrono>
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

	void UpdateActivity() { mLastActivityTime = chrono::steady_clock::now(); }
	int GetInactiveSeconds() const
	{
		auto now = chrono::steady_clock::now();
		auto elapsed = chrono::duration_cast<std::chrono::seconds>(now - mLastActivityTime);
		return static_cast<int>(elapsed.count());
	}

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

	chrono::steady_clock::time_point mLastActivityTime;
};


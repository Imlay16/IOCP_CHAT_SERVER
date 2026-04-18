#pragma once
#include <WinSock2.h>
#include <string>
#include <queue>
#include <vector>
#include <chrono>
#include <atomic>
#include "RingBuffer.h"
#include "../Common/Common.h"

#define MAX_SOCKBUF 4096

using namespace std;

enum class SessionState
{
	CONNECTED,
	AUTHENTICATED,
	DISCONNECTING,
	IDLE
};

enum class UserState
{
	LOBBY,
	IN_ROOM
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
	explicit ClientSession(uint32_t poolIndex);
	~ClientSession() = default;

	ClientSession(const ClientSession&) = delete;
	ClientSession& operator=(const ClientSession&) = delete;
	ClientSession(ClientSession&&) = delete;
	ClientSession& operator=(ClientSession&&) = delete;

	void Initialize(SOCKET socket, uint32_t sessionId);
	void Reset();

	bool SendPacket(const char* data, int length);
	bool RegisterRecv();
	void OnSendCompleted();

	bool TryDisconnect();

	// Getter
	uint32_t GetSessionId() const { return mSessionId; }
	uint32_t GetPoolIndex() const { return mPoolIndex; }
	SOCKET GetSocket() const { return mSocket; }
	SessionState GetState() const { return mState; }	
	const string& GetUsername() const { return mNickname; }
	UserState GetUserState() const { return mUserState; }
	const string& GetLoginId() const { return mLoginId; }
	uint16_t GetRoomId() const { return mRoomId; }

	char* GetTempRecvBuf() { return mTempRecvBuf; }
	RingBuffer& GetRecvBuffer() { return mRecvBuffer; }

	// Setter
	void SetSessionId(uint32_t id) { mSessionId = id; }
	void SetState(SessionState state) { mState = state; }
	void SetUserState(UserState userState) { mUserState = userState; }
	void SetUsername(const string& name) { mNickname = name; }
	void SetLoginId(const string& id) { mLoginId = id; }
	void SetRoomId(uint16_t roomId) { mRoomId = roomId; }

	bool IsValid() const { return mSocket != INVALID_SOCKET; }
	bool IsAuthenticated() const { return mState == SessionState::AUTHENTICATED; }

	void UpdateActivity() { mLastActivityTime = chrono::steady_clock::now(); }
	int GetInactiveSeconds() const
	{
		auto now = chrono::steady_clock::now();
		auto elapsed = chrono::duration_cast<std::chrono::seconds>(now - mLastActivityTime);
		return static_cast<int>(elapsed.count());
	}

	UserInfo ToUserInfo() const;

private:
	void ProcessSend();

private:

	// Session 
	uint32_t mSessionId;
	const uint32_t mPoolIndex;
	string mLoginId;
	SOCKET mSocket;
	string mNickname;
	atomic<SessionState> mState;
	atomic<UserState> mUserState;
	uint16_t mRoomId = 0;

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
	SRWLOCK mStateLock;

	chrono::steady_clock::time_point mLastActivityTime;
};


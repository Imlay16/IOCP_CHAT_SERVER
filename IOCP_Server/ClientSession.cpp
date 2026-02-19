#include "ClientSession.h"
#include "SRWLockGuard.h"
#include <iostream>

ClientSession::ClientSession()
	: mSessionId(0)
	, mSocket(INVALID_SOCKET)
	, mState(SessionState::DISCONNECTING)
	, mRecvBuffer(MAX_SOCKBUF * 2)
	, mIsSending(false)
{
	ZeroMemory(&mRecvOverlappedEx, sizeof(OverlappedEx));
	ZeroMemory(&mSendOverlappedEx, sizeof(OverlappedEx));
	ZeroMemory(mTempRecvBuf, sizeof(mTempRecvBuf));
	ZeroMemory(mSendBuf, sizeof(mSendBuf));

	InitializeSRWLock(&mSendLock);
}

ClientSession::~ClientSession()
{
	Reset();
}

void ClientSession::Initialize(SOCKET socket, UINT32 sessionId)
{
	mSocket = socket;
	mSessionId = sessionId;
	mState = SessionState::CONNECTED;
	mIsSending = false;
	mUsername.clear();

	while (!mSendQueue.empty())
	{
		mSendQueue.pop();
	}

	ZeroMemory(&mRecvOverlappedEx, sizeof(OverlappedEx));
	ZeroMemory(&mSendOverlappedEx, sizeof(OverlappedEx));
	ZeroMemory(mTempRecvBuf, sizeof(mTempRecvBuf));
	ZeroMemory(mSendBuf, sizeof(mSendBuf));
}

void ClientSession::Reset()
{
	SRWLockGuard lock(&mSendLock);

	if (mSocket != INVALID_SOCKET)
	{
		shutdown(mSocket, SD_BOTH);
		closesocket(mSocket);
		mSocket = INVALID_SOCKET;
	}		

	mSessionId = 0;
	mState = SessionState::DISCONNECTING;
	mUsername.clear();
	mToken.clear();

	while (!mSendQueue.empty())
	{
		mSendQueue.pop();
	}

	mIsSending = false;
}

bool ClientSession::SendPacket(const char* data, int length)
{
	if (mSocket == INVALID_SOCKET)
	{
		return false;
	}

	SRWLockGuard lock(&mSendLock);

	vector<char> packet(data, data + length);
	mSendQueue.push(move(packet));

	if (!mIsSending)
	{
		ProcessSend();
	}

	return true;
}

void ClientSession::ProcessSend()
{
	if (mSendQueue.empty())
	{
		mIsSending = false;
		return;
	}

	mIsSending = true;

	const auto& packet = mSendQueue.front();

	if (packet.size() > MAX_SOCKBUF) {
		std::cout << "[ClientSession] Packet too large: " << packet.size() << std::endl;
		mSendQueue.pop();
		ProcessSend();
		return;
	}

	memcpy(mSendBuf, packet.data(), packet.size());

	ZeroMemory(&mSendOverlappedEx.wsaOverlapped, sizeof(WSAOVERLAPPED));
	mSendOverlappedEx.operation = IOOperation::SEND;
	mSendOverlappedEx.wsaBuf.buf = mSendBuf;
	mSendOverlappedEx.wsaBuf.len = static_cast<ULONG>(packet.size());

	int ret = WSASend(mSocket,
		&mSendOverlappedEx.wsaBuf,
		1,
		nullptr,
		0,
		(LPWSAOVERLAPPED)&mSendOverlappedEx,
		NULL);

	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		cout << "[ClientSession] WSASend Error: " << WSAGetLastError() << endl;
		mIsSending = false;
		mSendQueue.pop();
		ProcessSend();
	}
}

void ClientSession::OnSendCompleted()
{
	SRWLockGuard lock(&mSendLock);

	if (!mSendQueue.empty())
	{
		mSendQueue.pop();
	}

	ProcessSend();
}

bool ClientSession::RegisterRecv()
{
	if (mSocket == INVALID_SOCKET)
	{
		return false;
	}

	if (mRecvBuffer.IsFull())
	{
		cout << "[ClientSession] Buffer overflow detected!" << endl;
		return false;
	}

	DWORD recvBytes = 0;
	DWORD flag = 0;

	ZeroMemory(&mRecvOverlappedEx.wsaOverlapped, sizeof(WSAOVERLAPPED));
	mRecvOverlappedEx.operation = IOOperation::RECV;
	mRecvOverlappedEx.wsaBuf.buf = mTempRecvBuf; 
	mRecvOverlappedEx.wsaBuf.len = MAX_SOCKBUF; 

	int ret = WSARecv(mSocket,
		&mRecvOverlappedEx.wsaBuf,
		1,
		&recvBytes,
		&flag,
		(LPWSAOVERLAPPED)&mRecvOverlappedEx,
		NULL);

	if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING)
	{
		cout << "[ClientSession] WSARecv Error: " << WSAGetLastError() << endl;
		return false;
	}

	return true;
}
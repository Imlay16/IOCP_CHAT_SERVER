#pragma once
#include <WinSock2.h>
#include <string>
#include <atomic>
#include <process.h>
#include "../Common/Packet.h"

#define MAX_SOCKBUF 2048

using namespace std;

class TestClient
{
public:
	TestClient(int id, const string& name, const char* serverIP, int serverPort);
	~TestClient();

	bool Connect();
	void Disconnect();

	bool SendAll(SOCKET sock, const char* data, int totalSize);
	bool Register(int num);
	bool Login(int num);
	bool SendBroadcast(const string& message);
	bool SendWhisper(int targetId, const string& message);
	bool SendHeartbeat();

	bool IsAuthenticated() const { return mIsAuthenticated; }
	bool IsRunning() const { return mIsRunning; }
	const string& GetName() const { return mName; }
	int GetId() const { return mId; }

	int GetReceivedBroadcastCount() const { return mReceivedBroadcastCount; }

private:
	static unsigned WINAPI RecvThreadFunc(void* arg);
	void RecvLoop();
	void ProcessPacket(PacketHeader* packet);

	void HandleRegisterResponse(RegisterResPacket* packet);
	void HandleLoginResponse(LoginResPacket* packet);
	void HandleBroadcastResponse(BroadcastResPacket* packet);
	void HandleWhisperResponse(WhisperChatResPacket* packet);
	void HandleHeartbeat();

private:
	int mId;
	string mName;
	SOCKET mSocket;
	const char* mServerIP;
	int mServerPort;

	HANDLE mRecvThread;
	atomic<bool> mIsRunning;
	atomic<bool> mIsAuthenticated;

	atomic<bool> mRegisterResponseArrived;
	atomic<ErrorCode> mRegisterResult;

	char mRecvBuffer[MAX_SOCKBUF];

	int mReceivedBroadcastCount = 0;
};


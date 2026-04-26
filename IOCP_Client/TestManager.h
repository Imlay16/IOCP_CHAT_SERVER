#pragma once
#include "TestClient.h"
#include <vector>
#include <memory>
#include <chrono>
#include <string>
#include <fstream>

class TestManager
{
public:
	TestManager(const char* serverIP, int serverPort);
	~TestManager();

	void Connect(int numClients);

	void RunBroadcastTest(int messagesPerClient);
	void RunWhisperTest();
	void RunMixedTest();
	void RunPerformanceTest();
	void RoomTest();

private:
	void CreateClients();
	void ConnectAllClients();
	void LoginAllClients();
	void RegisterAllClients();
	void DisconnectAllClients();

	void WaitForSeconds(int seconds);
	bool WaitForLobbyChat(int expected, int timeoutSec);
	bool WaitForWhisper(int expected, int timeoutSec);
	bool WaitForRoomChat(const vector<TestClient*>& roomClients, int expected, int timeoutSec);
	void SaveResultCSV(const string& filename, const string& header, const string& row);

private:
	const char* mServerIP;
	int mServerPort;
	vector<unique_ptr<TestClient>> mClients;
	int mNumClients = 0;
};
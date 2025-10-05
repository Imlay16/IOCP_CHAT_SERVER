#pragma once
#include "TestClient.h"
#include <vector>
#include <memory>

class TestManager
{
public:
	TestManager(const char* serverIP, int serverPort);
	~TestManager();

	void RunBroadcastTest(int numClients, int messagesPerClient);
	void RunWhisperTest(int numClients);
	void RunMixedTest(int numClients);

private:
	void CreateClients(int numClients);
	void ConnectAllClients();
	void LoginAllClients();
	void DisconnectAllClients();
	void WaitForSeconds(int seconds);

private:
	const char* mServerIP;
	int mServerPort;
	vector<unique_ptr<TestClient>> mClients;
};


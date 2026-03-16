#pragma once
#include "TestClient.h"
#include <vector>
#include <memory>

class TestManager
{
public:
	TestManager(const char* serverIP, int serverPort);
	~TestManager();

	void Connect(int numClients);
	void RunBroadcastTest(int messagesPerClient);
	void RunWhisperTest();
	void RunMixedTest();

private:
	void CreateClients();
	void ConnectAllClients();
	void LoginAllClients();
	void RegisterAllClients();
	void DisconnectAllClients();
	void WaitForSeconds(int seconds);

private:
	const char* mServerIP;
	int mServerPort;
	vector<unique_ptr<TestClient>> mClients;

	int mNumClients;	
};


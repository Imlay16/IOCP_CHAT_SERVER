#include "TestManager.h"
#include <iostream>
#include <thread>

TestManager::TestManager(const char* serverIP, int serverPort)
	: mServerIP(serverIP)
	, mServerPort(serverPort)
{
}

TestManager::~TestManager()
{
	DisconnectAllClients();
}

void TestManager::CreateClients(int numClients)
{
	mClients.clear();

	for (int i = 0; i < numClients; i++)
	{
		string name = "TestUser" + to_string(i + 1);
		auto client = make_unique<TestClient>(i, name, mServerIP, mServerPort);
		mClients.push_back(move(client));
	}

	cout << "Created " << numClients << " test clients" << endl;
}

void TestManager::ConnectAllClients()
{
	cout << "Connecting all clients..." << endl;

	for (auto& client : mClients)
	{
		if (!client->Connect())
		{
			cout << "Failed to connect: " << client->GetName() << endl;
		}
		Sleep(10);
	}

	cout << "All clients connected" << endl;
}

void TestManager::LoginAllClients()
{
	cout << "Logging in all clients..." << endl;

	for (auto& client : mClients)
	{
		if (!client->Login())
		{
			cout << "Failed to login: " << client->GetName() << endl;
		}
		Sleep(10);
	}

	cout << "All Clients logged in" << endl;
}

void TestManager::DisconnectAllClients()
{
	cout << "Disconnecting all clients..." << endl;

	for (auto& client : mClients)
	{
		client->Disconnect();
	}

	mClients.clear();
	cout << "All clients disconnected" << endl;
}

void TestManager::WaitForSeconds(int seconds)
{
	cout << "Waiting " << seconds << " seconds..." << endl;
	Sleep(seconds * 1000);
}

void TestManager::RunBroadcastTest(int numClients, int messagesPerClient)
{
	cout << "\n========================================" << endl;
	cout << "BROADCAST TEST" << endl;
	cout << "Clients: " << numClients << ", Messages: " << messagesPerClient << endl;
	cout << "========================================\n" << endl;

	CreateClients(numClients);
	ConnectAllClients();
	LoginAllClients();

	WaitForSeconds(1);

	cout << "\nStarting broadcast test..." << endl;

	for (int msg = 0; msg < messagesPerClient; msg++)
	{
		for (auto& client : mClients)
		{
			string message = "Broadcast message " + to_string(msg + 1);
			client->SendBroadcast(message);
			Sleep(50);
		}
	}

	cout << "\nBroadcast test completed. Waiting for responses..." << endl;
	WaitForSeconds(3);

	cout << "\n=== BROADCAST STATISTICS ===" << endl;
	int totalReceived = 0;
	for (auto& client : mClients)
	{
		int count = client->GetReceivedBroadcastCount();
		cout << client->GetName() << ": " << count << " messages" << endl;
		totalReceived += count;
	}
	cout << "Total messages received: " << totalReceived << endl;
	cout << "Expected: " << (mClients.size() * messagesPerClient * (mClients.size() - 1)) << endl;

	DisconnectAllClients();

	cout << "\n========================================" << endl;
	cout << "BROADCAST TEST FINISHED" << endl;
	cout << "========================================\n" << endl;
}

void TestManager::RunWhisperTest(int numClients)
{
	cout << "\n========================================" << endl;
	cout << "WHISPER TEST" << endl;
	cout << "Clients: " << numClients << endl;
	cout << "========================================\n" << endl;

	CreateClients(numClients);
	ConnectAllClients();
	LoginAllClients();

	WaitForSeconds(1);

	cout << "\nStarting whisper test..." << endl;

	for (size_t i = 0; i < mClients.size(); i++)
	{
		size_t targetIdx = (i + 1) % mClients.size();

		string message = "Whisper from " + mClients[i]->GetName();
		mClients[i]->SendWhisper(mClients[targetIdx]->GetName(), message);

		cout << "[" << mClients[i]->GetName() << "] -> [" << mClients[targetIdx]->GetName() << "]" << endl;

		Sleep(100);
	}

	cout << "\nTesting whisper to non-existent user..." << endl;
	mClients[0]->SendWhisper("NonExistentUser", "NonExistent Message");

	cout << "\nWhisper test completed. Waiting for responses..." << endl;
	WaitForSeconds(3);

	DisconnectAllClients();

	cout << "\n========================================" << endl;
	cout << "WHISPER TEST FINISHED" << endl;
	cout << "========================================\n" << endl;	
}

void TestManager::RunMixedTest(int numClients)
{
	cout << "\n========================================" << endl;
	cout << "MIXED TEST (Broadcast + Whisper)" << endl;
	cout << "Clients: " << numClients << endl;
	cout << "========================================\n" << endl;

	CreateClients(numClients);
	ConnectAllClients();
	LoginAllClients();

	WaitForSeconds(1);

	cout << "\nStarting mixed test..." << endl;

	for (int i = 0; i < 5; i++)
	{
		mClients[0]->SendBroadcast("Broadcast " + to_string(i));
		Sleep(100);

		if (mClients.size() >= 2)
		{
			mClients[1]->SendWhisper(mClients[0]->GetName(), "Whisper " + to_string(i));
			Sleep(100);
		}
	}

	cout << "\nMixed test completed. Waiting for responses..." << endl;
	WaitForSeconds(3);

	DisconnectAllClients();

	cout << "\n========================================" << endl;
	cout << "MIXED TEST FINISHED" << endl;
	cout << "========================================\n" << endl;
}
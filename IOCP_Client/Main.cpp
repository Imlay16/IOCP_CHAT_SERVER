#include <iostream>
#include <WinSock2.h>
#include "TestManager.h"
#pragma comment(lib, "ws2_32.lib")

void PrintMenu()
{
	cout << "\n========================================" << endl;
	cout << "IOCP Chat Server Test Menu" << endl;
	cout << "========================================" << endl;
	cout << "1. Broadcast Test" << endl;
	cout << "2. Whisper Test" << endl;
	cout << "3. Mixed Test" << endl;
	cout << "4. Performance Test" << endl;
	cout << "5. Room Test" << endl;
	cout << "6. Exit" << endl;
	cout << "========================================" << endl;
	cout << "Select: ";
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		cout << "Usage: " << argv[0] << " <IP> <port>" << endl;
		return 1;
	}

	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		cout << "WSAStartup failed" << endl;
		return 1;
	}

	TestManager testManager(argv[1], atoi(argv[2]));

	int initialClients;
	cout << "Enter number of clients: ";
	cin >> initialClients;
	testManager.Connect(initialClients);

	while (true)
	{
		PrintMenu();

		int choice;
		cin >> choice;

		switch (choice)
		{
		case 1:
		{
			int msgCount;
			cout << "Messages per client: ";
			cin >> msgCount;
			testManager.RunBroadcastTest(msgCount);
			break;
		}
		case 2:
			testManager.RunWhisperTest();
			break;
		case 3:
			testManager.RunMixedTest();
			break;
		case 4:
			testManager.RunPerformanceTest();
			break;
		case 5:
			testManager.RoomTest();
			break;
		case 6:
			cout << "Exiting..." << endl;
			WSACleanup();
			return 0;
		default:
			cout << "Invalid choice" << endl;
			break;
		}
	}

	WSACleanup();
	return 0;
}
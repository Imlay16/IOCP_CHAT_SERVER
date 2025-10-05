#include <iostream>
#include <WinSock2.h>
#include "TestManager.h"

#pragma comment(lib, "ws2_32.lib")

void PrintMenu() {
	cout << "\n========================================" << endl;
	cout << "IOCP Chat Server Test Menu" << endl;
	cout << "========================================" << endl;
	cout << "1. Broadcast Test (10 clients, 3 messages each)" << endl;
	cout << "2. Whisper Test (5 clients)" << endl;
	cout << "3. Mixed Test (5 clients)" << endl;
	cout << "4. Custom Broadcast Test" << endl;
	cout << "5. Exit" << endl;
	cout << "========================================" << endl;
	cout << "Select: ";
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <IP> <port>" << endl;
        return 1;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "WSAStartup failed" << endl;
        return 1;
    }

    TestManager testManager(argv[1], atoi(argv[2]));

    while (true) {
        PrintMenu();

        int choice;
        cin >> choice;

        switch (choice) {
        case 1:
            testManager.RunBroadcastTest(10, 3);
            break;

        case 2:
            testManager.RunWhisperTest(5);
            break;

        case 3:
            testManager.RunMixedTest(5);
            break;

        case 4: {
            int numClients, numMessages;
            cout << "Number of clients: ";
            cin >> numClients;
            cout << "Messages per client: ";
            cin >> numMessages;
            testManager.RunBroadcastTest(numClients, numMessages);
            break;
        }

        case 5:
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
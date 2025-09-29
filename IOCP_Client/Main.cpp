#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h> 
#include <iostream>
#include <vector>
#include <string>
#include "../Common/Packet.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define USER_NAME 32
#define BUF_SIZE 2048
#define NUM_CLIENTS 50  // 클라이언트 수
#define NUM_MESSAGES 2   // 클라이언트당 메시지 수

vector<std::string> TEST_USERNAMES(NUM_CLIENTS);

struct ClientData
{
	int id;
	string name;
	SOCKET socket;
	HANDLE sendThread;
	HANDLE recvThread;
	bool isAuthenticated;
	const char* serverIP;
	int serverPort;
	atomic<bool> isRunning;
};

void ProcessPacket(ClientData* client, PacketHeader* packet);
void HandleBroadCastPacket(ClientData* client, BroadCastResPacket* packet);
void HandleLoginPacket(ClientData* client, LoginResPacket* packet);
void HandleWhispherPacket(ClientData* client, WhispherChatResPacket* packet);
void ErrorHandling(const char* msg);

unsigned WINAPI ClientThread(void* arg);
unsigned WINAPI SendPacket(void* arg);
unsigned WINAPI RecvPacket(void* arg);

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hSock;
	SOCKADDR_IN servAdr;
	HANDLE hSndThread, hRcvThread;	

	if (argc != 3) {
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");

	printf("=================================\n");
	printf("IOCP Chat Server Test\n");
	printf("Clients: %d, Messages per client: %d\n", NUM_CLIENTS, NUM_MESSAGES);
	printf("=================================\n\n");

	HANDLE threads[NUM_CLIENTS];
	ClientData clients[NUM_CLIENTS];

	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		TEST_USERNAMES[i] = "TestUser" + to_string(i + 1);
	}

	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		clients[i].id = i;
		clients[i].socket = INVALID_SOCKET;
		clients[i].name = TEST_USERNAMES[i];
		clients[i].serverIP = argv[1];
		clients[i].serverPort = atoi(argv[2]);
		clients[i].isAuthenticated = false;
		clients[i].isRunning = true;

		threads[i] = (HANDLE)_beginthreadex(NULL, 0, ClientThread, &clients[i], 0, NULL);
	}

	printf("All clients started. Waiting for completion...\n\n");

	WaitForMultipleObjects(NUM_CLIENTS, threads, TRUE, INFINITE);

	for (int i = 0; i < NUM_CLIENTS; i++)
	{
		CloseHandle(threads[i]);
	}	
	
	printf("\n=================================\n");
	printf("Test completed!\n");
	printf("=================================\n");

	// 귓속말 TEST 진행
	 	

	WSACleanup();
	return 0;
}

unsigned WINAPI ClientThread(void* arg)
{
	ClientData* client = (ClientData*)arg;

	client->socket = socket(PF_INET, SOCK_STREAM, 0);
	if (client->socket == INVALID_SOCKET)
	{
		printf("[%s] Failed to create socket\n", client->name.c_str());
		return 0;
	}

	SOCKADDR_IN servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, client->serverIP, &servAddr.sin_addr);
	servAddr.sin_port = htons(client->serverPort);

	if (connect(client->socket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
	{
		printf("[%s] Connect failed!\n", client->name.c_str());
		closesocket(client->socket);
		return 0;
	}

	printf("[%s] Connected to server!\n", client->name.c_str());

	HANDLE sendThread = (HANDLE)_beginthreadex(NULL, 0, SendPacket, client, 0, NULL);
	HANDLE recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvPacket, client, 0, NULL);

	HANDLE threads[2] = { sendThread, recvThread };
	WaitForMultipleObjects(2, threads, TRUE, INFINITE);

	CloseHandle(sendThread);
	CloseHandle(recvThread);
	closesocket(client->socket);

	printf("[%s] Disconnected\n", client->name.c_str());

	return 0;
}

unsigned WINAPI SendPacket(void* arg)
{
	ClientData* client = (ClientData*)arg;
	char message[256];
	int messageCount = 0;

	// 여기에서 LoginPacket을 보내고, 받은 후에 client의 username 설정하기

	LoginReqPacket loginPacket;
	loginPacket.SetLoginInfo(
		client->name.c_str(),  // userId
		"testpw",              // password  
		client->name.c_str()   // username 
	);

	printf("[%s] Attempting login\n",
		client->name.c_str());

	int sendBytes = send(client->socket, (char*)&loginPacket, loginPacket.size, 0);
	if (sendBytes == SOCKET_ERROR)
	{
		printf("[%s] Login Send Error: %d\n", TEST_USERNAMES[client->id].c_str(), WSAGetLastError());
		client->isRunning = false;
		return 0;
	}
	
	// 로그인 응답 대기
	while (client->isRunning && !client->isAuthenticated)
	{
		Sleep(10);
	}

	printf("[%s] Login Successful!\n", TEST_USERNAMES[client->id].c_str());

	while (client->isRunning && messageCount < NUM_MESSAGES)
	{
		sprintf_s(message, sizeof(message), "Message %d Test message from client!", messageCount + 1);

		BroadCastReqPacket packet;
		packet.SetMessage(message);

		int sentBytes = send(client->socket, (char*)&packet, packet.size, 0);

		if (sentBytes == SOCKET_ERROR)
		{
			printf("[%s] Send error: %d\n", client->name.c_str(), WSAGetLastError());
			client->isRunning = false;
			break;
		}
		else if (sentBytes != packet.size) {
			printf("[%s] Partial send: sent %d, expected %d\n",
				client->name, sentBytes, packet.size);
		}
		else {
			printf("[%s] Sent message %d/%d\n",
				client->name.c_str(), messageCount + 1, NUM_MESSAGES);
		}

		messageCount++;
		Sleep(200);
	}

	printf("[%s] Finished sending %d messages\n", client->name.c_str(), messageCount);

	Sleep(5000);

	// 이제 테스트 종료
	client->isRunning = false;
	shutdown(client->socket, SD_BOTH);

	return 0;
}

unsigned WINAPI RecvPacket(void* arg)
{
	ClientData* client = (ClientData*)arg;

	char recvBuffer[BUF_SIZE];
	int recvSize;
	int totalRecv;
	int messageCount = 0;

	while (client->isRunning)
	{
		totalRecv = 0;

		while (totalRecv < sizeof(PacketHeader))
		{
			recvSize = recv(client->socket, recvBuffer + totalRecv, sizeof(PacketHeader) - totalRecv, 0);
			if (recvSize <= 0)
			{
				if (recvSize == 0)
				{
					printf("[%s] Server disconnected\n", client->name.c_str());
				}
				else if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					printf("[%s] Recv error: %d\n", client->name.c_str(), WSAGetLastError());
				}
				client->isRunning = false;
				return 0;
			}

			totalRecv += recvSize;
		}

		PacketHeader* header = (PacketHeader*)recvBuffer;
		int remainingSize = header->size - sizeof(PacketHeader);

		if (remainingSize > 0)
		{
			while (totalRecv < header->size)
			{
				recvSize = recv(client->socket, recvBuffer + totalRecv, header->size - totalRecv, 0);
				if (recvSize <= 0)
				{
					if (recvSize == 0)
					{
						printf("[%s] Connection closed\n", client->name.c_str());
						return 0;
					}
					else {
						printf("[%s] Recv error: %d\n", client->name.c_str(), WSAGetLastError());
					}

					client->isRunning = false;
					return 0;
				}

				totalRecv += recvSize;
			}
		}

		ProcessPacket(client, header);
	}
}

void ProcessPacket(ClientData* client, PacketHeader* packet)
{
	switch (packet->type)
	{
	case PacketType::LOGIN_RESPONSE:
		HandleLoginPacket(client, (LoginResPacket*)packet);
		break;

	case PacketType::BROADCAST_RESPONSE:
		HandleBroadCastPacket(client, (BroadCastResPacket*)packet);
		break;

	case PacketType::WHISPHER_RESPONSE:
		HandleWhispherPacket(client, (WhispherChatResPacket*)packet);
		break;
	}
}

void HandleLoginPacket(ClientData* client, LoginResPacket* packet)
{
	if (packet->result == ErrorCode::SUCCESS)
	{
		printf("[%s] Login Successful!\n", client->name.c_str());
		client->isAuthenticated = true;
	}
	else
	{
		printf("[%s] Login Failed: %d\n", client->name.c_str(), packet->result);
		client->isRunning = false; // 로그인 실패 시 클라이언트 종료?
	}
}

void HandleWhispherPacket(ClientData* client, WhispherChatResPacket* packet)
{

}

void HandleBroadCastPacket(ClientData* client, BroadCastResPacket* packet)
{
	printf("[%s]: %s\n", packet->user, packet->message);
}

void ErrorHandling(const char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

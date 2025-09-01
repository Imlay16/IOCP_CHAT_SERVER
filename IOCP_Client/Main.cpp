#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h> 
#include <iostream>
#include <vector>
#include "../Common/Packet.h"

using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define USER_NAME 32
#define BUF_SIZE 2048
#define NUM_CLIENTS 50  // 클라이언트 수
#define NUM_MESSAGES 2   // 클라이언트당 메시지 수

void ProcessPacket(PacketHeader* packet);
void HandleBroadCastPacket(BroadCastResPacket* packet);
void ErrorHandling(const char* msg);
unsigned WINAPI ClientThread(void* arg);
unsigned WINAPI SendPacket(void* arg);
unsigned WINAPI RecvPacket(void* arg);

struct ClientData
{
	int clientId;
	SOCKET socket;
	HANDLE sendThread;
	HANDLE recvThread;
	const char* serverIP;
	int serverPort;
	atomic<bool> isRunning;

	ClientData()
	{
		socket = INVALID_SOCKET;
	}
};

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
		clients[i].clientId = i;
		clients[i].serverIP = argv[1];
		clients[i].serverPort = atoi(argv[2]);
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

	WSACleanup();
	return 0;
}

unsigned WINAPI ClientThread(void* arg)
{
	ClientData* client = (ClientData*)arg;

	client->socket = socket(PF_INET, SOCK_STREAM, 0);
	if (client->socket == INVALID_SOCKET)
	{
		printf("[Client %d] Failed to create socket\n", client->clientId);
		return 0;
	}

	SOCKADDR_IN servAddr;
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, client->serverIP, &servAddr.sin_addr);
	servAddr.sin_port = htons(client->serverPort);

	if (connect(client->socket, (SOCKADDR*)&servAddr, sizeof(servAddr)) == SOCKET_ERROR)
	{
		printf("[Client %d] Connect failed!\n", client->clientId);
		closesocket(client->socket);
		return 0;
	}

	printf("[Client %d] Connected to server!\n", client->clientId);

	HANDLE sendThread = (HANDLE)_beginthreadex(NULL, 0, SendPacket, client, 0, NULL);
	HANDLE recvThread = (HANDLE)_beginthreadex(NULL, 0, RecvPacket, client, 0, NULL);

	HANDLE threads[2] = { sendThread, recvThread };
	WaitForMultipleObjects(2, threads, TRUE, INFINITE);

	CloseHandle(sendThread);
	CloseHandle(recvThread);
	closesocket(client->socket);

	printf("[Client %d] Disconnected\n", client->clientId);

	return 0;
}

unsigned WINAPI SendPacket(void* arg)
{
	ClientData* client = (ClientData*)arg;
	char message[256];
	int messageCount = 0;
	
	while (client->isRunning && messageCount < NUM_MESSAGES)
	{
		sprintf_s(message, sizeof(message), "Client %d - Message %d: Test message from client!", client->clientId, messageCount + 1);

		BroadCastReqPacket packet;
		packet.SetMessage(message);

		int sentBytes = send(client->socket, (char*)&packet, packet.size, 0);

		if (sentBytes == SOCKET_ERROR)
		{
			printf("[Client %d] Send error: %d\n", client->clientId, WSAGetLastError());

			client->isRunning = false;
			break;
		}
		else if (sentBytes != packet.size) {
			printf("[Client %d] Partial send: sent %d, expected %d\n",
				client->clientId, sentBytes, packet.size);
		}
		else {
			printf("[Client %d] Sent message %d/%d\n",
				client->clientId, messageCount + 1, NUM_MESSAGES);
		}

		messageCount++;
		Sleep(200);
	}

	printf("[Client %d] Finished sending %d messages\n", client->clientId, messageCount);

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
					printf("[Client %d] Server disconnected\n", client->clientId);
				}
				else if (WSAGetLastError() != WSAEWOULDBLOCK)
				{
					printf("[Client %d] Recv error: %d\n", client->clientId, WSAGetLastError());
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
						printf("[Client %d] Connection closed\n", client->clientId);
						return 0;
					}
					else {
						printf("[Client %d] Recv error: %d\n", client->clientId, WSAGetLastError());
					}

					client->isRunning = false;
					return 0;
				}

				totalRecv += recvSize;
			}
		}

		ProcessPacket(header);
	}
}

void ProcessPacket(PacketHeader* packet)
{
	switch (packet->type)
	{
	case PacketType::BROADCAST_RESPONSE:
		HandleBroadCastPacket((BroadCastResPacket*)packet);
		break;
	}
}

void HandleBroadCastPacket(BroadCastResPacket* packet)
{
	printf("[%s]: %s\n", packet->user, packet->message);
}

void ErrorHandling(const char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

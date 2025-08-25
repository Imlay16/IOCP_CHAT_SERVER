#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <process.h> 
#include <iostream>
#include "../Common/Packet.h"


using namespace std;

#pragma comment(lib, "ws2_32.lib")

#define BUF_SIZE 2048
#define USER_NAME 32

unsigned WINAPI SendPacket(void* arg);
unsigned WINAPI RecvPacket(void* arg);
void ProcessPacket(PacketHeader* packet);
void HandleBroadCastPacket(BroadCastResPacket* packet);
void ErrorHandling(const char* msg);

volatile bool g_bRunning = true;
SOCKET g_hSocket;

char msg[BUF_SIZE];

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
	g_hSocket = socket(PF_INET, SOCK_STREAM, 0);

	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family = AF_INET;
	if (InetPtonA(AF_INET, argv[1], &servAdr.sin_addr) <= 0) {
		ErrorHandling("Invalid IP address format");
	}
	servAdr.sin_port = htons(atoi(argv[2]));

	if (connect(g_hSocket, (SOCKADDR*)&servAdr, sizeof(servAdr)) == SOCKET_ERROR)
		ErrorHandling("connect() error");

	hSndThread =
		(HANDLE)_beginthreadex(NULL, 0, SendPacket, (void*)&g_hSocket, 0, NULL);
	hRcvThread =
		(HANDLE)_beginthreadex(NULL, 0, RecvPacket, (void*)&g_hSocket, 0, NULL);
	WaitForSingleObject(hSndThread, INFINITE);
	WaitForSingleObject(hRcvThread, INFINITE);
	closesocket(g_hSocket);
	WSACleanup();
	return 0;
}

// 클라이언트에서 패킷 전송 시 확인
unsigned WINAPI SendPacket(void* arg)
{
	SOCKET hSock = *((SOCKET*)arg);
	while (1)
	{
		fputs("ME: ", stdout);
		fgets(msg, BUF_SIZE, stdin);
		if (!strcmp(msg, "q\n") || !strcmp(msg, "Q\n"))
		{
			printf("Shutting down client...\n");
			g_bRunning = false;

			shutdown(hSock, SD_BOTH);
			break;
			//closesocket(hSock);
			//exit(0);
		}

		// 개행 문자 제거
		msg[strcspn(msg, "\n")] = 0;

		BroadCastReqPacket packet;
		packet.SetMessage(msg);

		// 디버깅: 전송 패킷 정보
		//printf("[CLIENT] Sending packet - Type: %d, Size: %d\n",
		//	(int)packet.type, packet.size);

		int sentBytes = send(hSock, (char*)&packet, packet.size, 0);
		if (sentBytes != packet.size) {
			printf("[ERROR] Failed to send complete packet. Sent: %d, Expected: %d\n",
				sentBytes, packet.size);
		}
	}
	return 0;
}

unsigned WINAPI RecvPacket(void* arg)   // read thread main
{
	int hSock = *((SOCKET*)arg);
	char recvBuffer[BUF_SIZE];
	int recvSize;
	int totalRecv = 0;

	while (1)
	{
		totalRecv = 0;
		while (totalRecv < sizeof(PacketHeader))
		{
			recvSize = recv(hSock, recvBuffer + totalRecv, sizeof(PacketHeader) - totalRecv, 0);

			if (recvSize <= 0)
			{
				if (recvSize == 0)
					printf("Server disconnected\n");
				else
					printf("recv() error: %d\n", WSAGetLastError());

				g_bRunning = false;
				return 0;

				//closesocket(hSock);
				//exit(0);
			}
			totalRecv += recvSize;
		}

		if (!g_bRunning) break;

		PacketHeader* header = (PacketHeader*)recvBuffer;

		int remainingSize = header->size - sizeof(PacketHeader);

		if (remainingSize > 0)
		{
			while (totalRecv < header->size)
			{
				recvSize = recv(hSock, recvBuffer + totalRecv, header->size - totalRecv, 0);
				if (recvSize <= 0)
				{
					if (recvSize == 0)
						printf("Server disconnected\n");
					else
						printf("recv() error: %d\n", WSAGetLastError());
					//closesocket(hSock);
					//exit(0);
					g_bRunning = false;
					return 0;
				}
				totalRecv += recvSize;
			}
		}

		if (g_bRunning)
		{
			ProcessPacket((PacketHeader*)recvBuffer);			
		}		
	}

	return 0;
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
	printf("[%s]: ", packet->user);
	printf("%s\n", packet->message);
}

void ErrorHandling(const char* msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}

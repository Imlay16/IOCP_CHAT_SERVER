#pragma once
#pragma comment(lib, "ws2_32")

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <vector>
#include <iostream>

#include "ClientSession.h"
#include "SessionManager.h"
#include "PacketHandler.h"
#include "DbManager.h"

#define MAX_WORKERTHREAD 4
#define KEEP_ALIVE_TIMEOUT 60

using namespace std;

class IOCPServer
{
public:
    IOCPServer();
    ~IOCPServer();

    bool InitSocket();
    bool BindAndListen(int bindPort);
    bool StartServer(UINT32 maxClientCount);
    void StopServer();
    void DestroyThread();

private:
    SOCKET mListenSocket;
    HANDLE mIOCPHandle;
    vector<thread> mIOWorkerThreads;
    thread mAcceptThread;

    SessionManager* mSessionManager;
    RoomManager* mRoomManager;
    PacketHandler* mPacketHandler;
    DbManager* mDbManager;

    UINT32 mSessionIdCounter;
    atomic<bool> mIsAcceptRun;

    void WorkerThread();
    void AcceptThread();

    bool BindIOCompletionPort(ClientSession* session);
    UINT32 GenerateSessionId() { return mSessionIdCounter++; }
};


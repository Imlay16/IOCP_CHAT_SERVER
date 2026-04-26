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
    void DisconnectSession(ClientSession* session);

    bool BindIOCompletionPort(ClientSession* session);
    UINT32 GenerateSessionId() { return mSessionIdCounter++; }
};


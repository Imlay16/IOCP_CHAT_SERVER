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
    void DestroyThread();

private:
    SOCKET mListenSocket;
    HANDLE mIOCPHandle;
    vector<thread> mIOWorkerThreads;
    thread mAcceptThread;

    SessionManager* mSessionManager;

    UINT32 mSessionIdCounter;
    bool mIsWorkerRun;
    bool mIsAcceptRun;

    void WorkerThread();
    void AcceptThread();

    bool BindIOCompletionPort(ClientSession* session);
    UINT32 GenerateSessionId() { return mSessionIdCounter++; }
};


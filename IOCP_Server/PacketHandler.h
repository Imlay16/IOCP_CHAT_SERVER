#pragma once

#include "ClientSession.h"
#include "SessionManager.h"
#include "DbManager.h"
#include "../Common/Packet.h"

using namespace std;

class ClientSession;
class IOCPServer;

class PacketHandler
{
public:
	PacketHandler() { }

	void ProcessPacket(ClientSession* session);
	
	void SetSessionManager(SessionManager* sessionManager);
	void SetDbManager(DbManager* dbManager);

private:
	void HandleLogin(ClientSession* session, PacketHeader* packet);
	void HandleBroadcast(ClientSession* session, PacketHeader* packet); 
	void HandleWhisper(ClientSession* session, PacketHeader* packet); 

	void HandleRegister(ClientSession* session, PacketHeader* packet);

	ErrorCode ConvertDbResultToErrorCode(DbResult result);
private:
	SessionManager* mSessionManager = nullptr;
	DbManager* mDbManager = nullptr;

	// static void SendLoginResponse(ClientSession* session, ErrorCode result);
	// static void HandleRoomChat(ClientSession* session, RoomChatReqPacket* packet);
	// static void HandleRoomList(ClientSession* session, RoomListReqPacket* packet);
	// static void HandleRoomCreate(ClientSession* session, CreateRoomReqPacket* packet);
	// static void HandleRoomJoin(ClientSession* session, JoinRoomReqPacket* packet);
};


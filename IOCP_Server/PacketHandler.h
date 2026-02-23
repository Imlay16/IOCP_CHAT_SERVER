#pragma once

#include "ClientSession.h"
#include "SessionManager.h"
#include "../Common/Packet.h"

using namespace std;

class ClientSession;
class IOCPServer;

class PacketHandler
{
public:
	PacketHandler() { }

	void ProcessPacket(ClientSession* session, SessionManager* sessionManager);

private:
	void HandleLogin(ClientSession* session, PacketHeader* packet, SessionManager* sessionManager);
	void HandleBroadcast(ClientSession* session, PacketHeader* packet, SessionManager* sessionManager); 
	void HandleWhisper(ClientSession* session, PacketHeader* packet, SessionManager* sessionManager); 

	void HandleRegister(ClientSession* session, PacketHeader* packet, SessionManager* sessionManager);

	// static void SendLoginResponse(ClientSession* session, ErrorCode result);
	// static void HandleRoomChat(ClientSession* session, RoomChatReqPacket* packet);
	// static void HandleRoomList(ClientSession* session, RoomListReqPacket* packet);
	// static void HandleRoomCreate(ClientSession* session, CreateRoomReqPacket* packet);
	// static void HandleRoomJoin(ClientSession* session, JoinRoomReqPacket* packet);
};


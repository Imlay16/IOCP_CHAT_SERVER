#pragma once

#include "ClientSession.h"
#include "SessionManager.h"
#include "../Common/Packet.h"

using namespace std;

struct ClientSession;
class IOCPServer;

class PacketHandler
{
public:
	static void ProcessPacket(ClientSession* session, DWORD dataSize, SessionManager* sessionManager);

private:
	static void HandleLogin(ClientSession* session, PacketHeader* packet, SessionManager* sessionManager);
	static void HandleBroadcast(ClientSession* session, PacketHeader* packet, SessionManager* sessionManager);
	static void HandleWhispher(ClientSession* session, PacketHeader* packet, SessionManager* sessionManager);

	static bool CheckUserCredentials(const char* userId, const char* userPw);

	// static void SendLoginResponse(ClientSession* session, ErrorCode result);
	// static void HandleRoomChat(ClientSession* session, RoomChatReqPacket* packet);
	// static void HandleRoomList(ClientSession* session, RoomListReqPacket* packet);
	// static void HandleRoomCreate(ClientSession* session, CreateRoomReqPacket* packet);
	// static void HandleRoomJoin(ClientSession* session, JoinRoomReqPacket* packet);
};


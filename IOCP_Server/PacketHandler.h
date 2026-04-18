#pragma once

#include "ClientSession.h"
#include "SessionManager.h"
#include "RoomManager.h"
#include "DbManager.h"
#include "../Common/Packet.h"

using namespace std;

class ClientSession;
class IOCPServer;

class PacketHandler
{
public:
	PacketHandler() = default;
	~PacketHandler() = default;

	bool ProcessPacket(ClientSession* session);
	
	void SetSessionManager(SessionManager* sessionManager);
	void SetRoomManager(RoomManager* roomManager);
	void SetDbManager(DbManager* dbManager);

private:
	void HandleLogin(ClientSession* session, PacketHeader* header);
	void HandleLobbyChat(ClientSession* session, PacketHeader* header);
	void HandleWhisper(ClientSession* session, PacketHeader* header);
	void HandleRegister(ClientSession* session, PacketHeader* header);
	void HandleCreateRoom(ClientSession* session, PacketHeader* header);
	void HandleRoomList(ClientSession* session, PacketHeader* header);
	void HandleJoinRoom(ClientSession* session, PacketHeader* header);
	void HandleLeaveRoom(ClientSession* session, PacketHeader* header);
	void HandleRoomChat(ClientSession* session, PacketHeader* header);

	ErrorCode ConvertDbResultToErrorCode(DbResult result);
private:
	SessionManager* mSessionManager = nullptr;
	RoomManager* mRoomManager = nullptr;
	DbManager* mDbManager = nullptr;
};


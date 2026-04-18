#pragma once

#include "PacketHandler.h"
#include <iostream>
#include <random>

using namespace std;

bool PacketHandler::ProcessPacket(ClientSession* session)
{
	RingBuffer& recvBuffer = session->GetRecvBuffer();

	while (true)
	{
		PacketHeader header;                         

		if (!recvBuffer.Peek((char*)&header, sizeof(PacketHeader)))
		{
			// cout << "[PacketHandler] Waiting for header... (has " << recvBuffer.GetDataSize() << " bytes)" << endl;
			break;
		}

		DWORD packetSize = header.GetSize();

		if (recvBuffer.GetDataSize() < packetSize)
		{
			// cout << "[PacketHandler] Waiting for body... (need " << packetSize << ", has" << recvBuffer.GetDataSize() << ")" << endl;
			break;
		}

		if (packetSize > MAX_PACKET_SIZE)
		{		
			//mSessionManager->UnregisterSession(session);
			return false;
		}

		vector<char> packetBuffer(packetSize);
		recvBuffer.Peek(packetBuffer.data(), packetSize);

		PacketHeader* fullHeader = (PacketHeader*)packetBuffer.data();

		switch (header.GetType())
		{
		case PacketType::REGISTER_REQUEST:
			HandleRegister(session, fullHeader);
			break;
		case PacketType::LOGIN_REQUEST:
			HandleLogin(session, fullHeader);
			break;

		case PacketType::LOBBY_CHAT_REQUEST:
		case PacketType::WHISPER_REQUEST:
		case PacketType::CREATE_ROOM_REQUEST:
		case PacketType::ROOM_LIST_REQUEST:
		case PacketType::JOIN_ROOM_REQUEST:
		case PacketType::LEAVE_ROOM_REQUEST:
		case PacketType::ROOM_CHAT_REQUEST:
			if (session->IsAuthenticated())
			{
				switch (fullHeader->GetType())
				{
				case PacketType::LOBBY_CHAT_REQUEST:  HandleLobbyChat(session, fullHeader);  break;
				case PacketType::WHISPER_REQUEST:     HandleWhisper(session, fullHeader);     break;
				case PacketType::CREATE_ROOM_REQUEST: HandleCreateRoom(session, fullHeader);  break;
				case PacketType::ROOM_LIST_REQUEST:   HandleRoomList(session, fullHeader);    break;
				case PacketType::JOIN_ROOM_REQUEST:   HandleJoinRoom(session, fullHeader);    break;
				case PacketType::LEAVE_ROOM_REQUEST:  HandleLeaveRoom(session, fullHeader);   break;
				case PacketType::ROOM_CHAT_REQUEST:   HandleRoomChat(session, fullHeader);    break;
				default: break;
				}
			}
			else
			{
				cout << "[PacketHandler] Session " << session->GetSessionId() << " not authenticated. Packet ignored." << endl;
			}
			break;

		default:
			cout << "[PacketHandler] Unknown packet type" << endl;
			break;
		}

		recvBuffer.Consume(packetSize);

		// cout << "[PacketHandler] Packet processed. Remaining: " << recvBuffer.GetDataSize() << " bytes" << endl;
	}
	return true;
}

void PacketHandler::SetSessionManager(SessionManager* sessionManager)
{
	mSessionManager = sessionManager;
}

void PacketHandler::SetRoomManager(RoomManager* roomManager)
{
	mRoomManager = roomManager;
}

void PacketHandler::SetDbManager(DbManager* dbManager)
{
	mDbManager = dbManager;
}

ErrorCode PacketHandler::ConvertDbResultToErrorCode(DbResult result)
{
	switch (result)
	{
	case DbResult::OK: return ErrorCode::SUCCESS;
	case DbResult::DUPLICATE_ID: return ErrorCode::ID_ALREADY_EXISTS;
	case DbResult::USER_NOT_FOUND: return ErrorCode::USER_NOT_FOUND;
	case DbResult::WRONG_PASSWORD: return ErrorCode::WRONG_PASSWORD;
	default: return ErrorCode::SERVER_ERROR;
	}
}

void PacketHandler::HandleRegister(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(RegisterReqPacket))
	{
		cout << "[PacketHandler] Register packet size error" << endl;
		return;
	}

	RegisterReqPacket* packet = (RegisterReqPacket*)header;
	RegisterResPacket resPacket;	

	string loginId(packet->loginId, strnlen_s(packet->loginId, sizeof(packet->loginId)));
	string password(packet->password, strnlen_s(packet->password, sizeof(packet->password)));
	string nickname(packet->nickname, strnlen_s(packet->nickname, sizeof(packet->nickname)));


	// string passwordHash = Hash(password);
	string passwordHash = password;

	DbResult dbResult = mDbManager->RegisterUser(loginId, passwordHash, nickname);
	
	resPacket.result = ConvertDbResultToErrorCode(dbResult);

	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleLogin(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(LoginReqPacket))
	{
		cout << "[PacketHandler] Login packet size error" << endl;
		return;
	}

	LoginReqPacket* packet = (LoginReqPacket*)header;
	LoginResPacket resPacket;

	if (session->GetState() == SessionState::AUTHENTICATED)
	{
		resPacket.result = ErrorCode::ALREADY_LOGGED_IN;
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
		return;
	}

	string loginId(packet->loginId, strnlen_s(packet->loginId, sizeof(packet->loginId)));
	string password(packet->password, strnlen_s(packet->password, sizeof(packet->password)));

	if (mSessionManager->FindSessionByLoginId(loginId) != nullptr)
	{
		resPacket.result = ErrorCode::ALREADY_LOGGED_IN;
		cout << "[PacketHandler] Login failed - Already logged in: " << loginId << endl;
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
		return;
	}

	string passwordHash = password;

	UserRow user{};
	DbResult dbResult = mDbManager->LoginUser(loginId, passwordHash, user);

	if (dbResult != DbResult::OK)
	{
		resPacket.result = ConvertDbResultToErrorCode(dbResult);
		cout << "[PacketHandler] Login failed: " << loginId
			<< ", result = " << static_cast<UINT16>(resPacket.result) << endl;
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
		return;
	}

	session->SetState(SessionState::AUTHENTICATED);
	session->SetLoginId(user.loginId);
	session->SetUsername(user.nickname);

	mSessionManager->RegisterSession(session);

	resPacket.result = ErrorCode::SUCCESS;
	strcpy_s(resPacket.nickname, sizeof(resPacket.nickname), user.nickname.c_str());

	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleLobbyChat(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(LobbyChatReqPacket))
	{
		cout << "[PacketHandler] Broadcast packet size error" << endl;
		return;
	}

	auto* packet = reinterpret_cast<LobbyChatReqPacket*>(header);

	LobbyChatResPacket resPacket;
	resPacket.result = mSessionManager->LobbyChat(session, packet->message);
	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleWhisper(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(WhisperChatReqPacket))
	{
		cout << "[PacketHandler] Whisper packet size error" << endl;
		return;
	}

	auto* packet = reinterpret_cast<WhisperChatReqPacket*>(header);

	WhisperChatResPacket resPacket;
	resPacket.result = mSessionManager->WhisperChat(session, packet->receiver, packet->message);
	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleCreateRoom(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(CreateRoomReqPacket))
	{
		cout << "[PacketHandler] CreateRoom packet size error" << endl;
		return;
	}

	CreateRoomReqPacket* packet = (CreateRoomReqPacket*)header;

	CreateRoomResPacket resPacket;

	auto result = mRoomManager->CreateRoomSession(session, packet->roomName, packet->maxUser);
	if (result.has_value())
	{
		resPacket.room = result.value();
		resPacket.result = ErrorCode::SUCCESS;
	}
	else
	{
		resPacket.result = ErrorCode::ROOM_CREATION_FAIL;
	}

	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleRoomList(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(RoomListReqPacket))
	{
		cout << "[PacketHandler] RoomList packet size error" << endl;
		return;
	}

	RoomListReqPacket* packet = (RoomListReqPacket*)header;

	RoomListResPacket resPacket;

	auto result = mRoomManager->GetRoomListByPage(packet->page);

	if (result.has_value())
	{
		resPacket.result = ErrorCode::SUCCESS;
		resPacket.roomCount = static_cast<uint16_t>(result->size());
		memcpy_s(resPacket.rooms, sizeof(resPacket.rooms), result->data(), result->size() * sizeof(RoomInfo));
	}
	else
	{
		resPacket.result = ErrorCode::INVALID_ROOM_REQUEST;
	}

	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleJoinRoom(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(JoinRoomReqPacket))
	{
		cout << "[PacketHandler] JoinRoom packet size error" << endl;
		return;
	}

	auto* packet = reinterpret_cast<JoinRoomReqPacket*>(header);

	JoinRoomResPacket resPacket;

	auto [result, room] = mRoomManager->JoinRoom(session, packet->roomId);

	resPacket.result = result;

	if (result == ErrorCode::SUCCESS)
	{
		resPacket.room = room->ToRoomInfo();
		room->FillUserList(resPacket.users, MAX_ROOM_USER);
	}

	session->SendPacket((char*)&resPacket, sizeof(JoinRoomResPacket));
}

void PacketHandler::HandleLeaveRoom(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(LeaveRoomReqPacket))
	{
		cout << "[PacketHandler] LeaveRoom packet size error" << endl;
		return;
	}

	auto* packet = reinterpret_cast<LeaveRoomReqPacket*>(header);

	auto result = mRoomManager->LeaveRoom(session);

	LeaveRoomResPacket resPacket;

	resPacket.result = result;
	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleRoomChat(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(RoomChatReqPacket))
	{
		cout << "[PacketHandler] RoomChat packet size error" << endl;
		return;
	}

	if (session->GetUserState() != UserState::IN_ROOM)
		return;

	RoomChatResPacket resPacket;

	auto* packet = reinterpret_cast<RoomChatReqPacket*>(header);
	resPacket.result = mRoomManager->RoomChat(session, packet->message);

	session->SendPacket((char*)&resPacket, sizeof(RoomChatResPacket));
}
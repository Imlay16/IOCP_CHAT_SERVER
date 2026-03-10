#pragma once

#include "PacketHandler.h"
#include <iostream>
#include <random>

using namespace std;

void PacketHandler::ProcessPacket(ClientSession* session)
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

		// 만약 패킷이 최대 크기를 넘는다면,
		if (packetSize > MAX_PACKET_SIZE)
		{
			// 세션 정리하고 반납하기
			mSessionManager->UnregisterSession(session);
			break;
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

		case PacketType::BROADCAST_REQUEST:
		case PacketType::WHISPER_REQUEST:
			if (session->IsAuthenticated())
			{
				if (fullHeader->GetType() == PacketType::BROADCAST_REQUEST)
				{
					HandleBroadcast(session, fullHeader);
				}
				else
				{
					HandleWhisper(session, fullHeader);
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

		cout << "[PacketHandler] Packet processed. Remaining: " << recvBuffer.GetDataSize() << " bytes" << endl;
	}
}

void PacketHandler::SetSessionManager(SessionManager* sessionManager)
{
	mSessionManager = sessionManager;
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

	// MySql Connector를 이용해서 회원 등록. 아이디 중복 가입 금지

	//string loginId(packet->loginId);
	//string password(packet->password);
	//string nickname(packet->nickname);

	string loginId(packet->loginId, strnlen_s(packet->loginId, sizeof(packet->loginId)));
	string password(packet->password, strnlen_s(packet->password, sizeof(packet->password)));
	string nickname(packet->nickname, strnlen_s(packet->nickname, sizeof(packet->nickname)));


	// Hash 적용
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

	cout << "[PacketHandler] Login success: " << loginId
		<< ", nickname = " << user.nickname << endl;

	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleBroadcast(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(BroadcastReqPacket))
	{
		cout << "[PacketHandler] Broadcast packet size error" << endl;
		return;
	}

	BroadcastReqPacket* packet = (BroadcastReqPacket*)header;

	BroadcastResPacket resPacket;
	resPacket.SetUser(session->GetUsername().c_str());
	resPacket.SetMessage(packet->message);

	mSessionManager->BroadcastPacket(session, (char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleWhisper(ClientSession* session, PacketHeader* header)
{
	if (header->GetSize() != sizeof(WhisperChatReqPacket))
	{
		cout << "[PacketHandler] Whisper packet size error" << endl;
		return;
	}

	WhisperChatReqPacket* packet = (WhisperChatReqPacket*)header;
	string receiver = packet->GetReceiver();

	ClientSession* targetSession = mSessionManager->FindSessionByLoginId(receiver);

	WhisperChatResPacket resPacket;

	if (targetSession != nullptr)
	{
		resPacket.result = ErrorCode::SUCCESS;
		resPacket.SetMessage(session->GetUsername().c_str(), packet->GetMsg());
		targetSession->SendPacket((char*)&resPacket, sizeof(resPacket));
	}
	else
	{
		resPacket.result = ErrorCode::USER_NOT_FOUND;
		snprintf(resPacket.message, sizeof(resPacket.message),
			"User '%s' not found", packet->receiver);
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
	}
}
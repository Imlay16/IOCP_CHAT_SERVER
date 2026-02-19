#include "PacketHandler.h"
#include <iostream>
#include <random>

using namespace std;

void PacketHandler::ProcessPacket(ClientSession* session, SessionManager* sessionManager)
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
			sessionManager->UnregisterSession(session);
			break;
		}

		vector<char> packetBuffer(packetSize);
		recvBuffer.Peek(packetBuffer.data(), packetSize);

		PacketHeader* fullHeader = (PacketHeader*)packetBuffer.data();

		switch (header.GetType())
		{
		case PacketType::LOGIN_REQUEST:
			HandleLogin(session, fullHeader, sessionManager);
			break;

		case PacketType::BROADCAST_REQUEST:
		case PacketType::WHISPER_REQUEST:
			if (session->IsAuthenticated())
			{
				if (fullHeader->GetType() == PacketType::BROADCAST_REQUEST)
				{
					HandleBroadcast(session, fullHeader, sessionManager);
				}
				else
				{
					HandleWhisper(session, fullHeader, sessionManager);
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

string GenerateToken()
{
	static const char charset[] = "0123456789abcdef";
	string token;
	token.reserve(32);

	random_device rd;
	mt19937 gen(rd());
	uniform_int_distribution<> dis(0, 15);

	for (int i = 0; i < 32; ++i)
	{
		token += charset[dis(gen)];
	}

	return token;
}

void PacketHandler::HandleLogin(ClientSession* session, PacketHeader* header, SessionManager* sessionManager)
{
	if (header->GetSize() != sizeof(LoginReqPacket))
	{
		cout << "[PacketHandler] Login packet size error" << endl;
		return;
	}

	LoginReqPacket* packet = (LoginReqPacket*)header;

	std::string storedPassword = mRedis->GetUserPassword(packet->userId);

	LoginResPacket resPacket;

	if (storedPassword.empty())
	{
		resPacket.result = ErrorCode::USER_NOT_FOUND;
		cout << "[PacketHandler] Login failed - User not found: " << packet->userId << endl;
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
		return;
	}

	if (storedPassword != packet->password)
	{
		resPacket.result = ErrorCode::WRONG_PASSWORD;
		cout << "[PacketHandler] Login failed - Wrong password: " << packet->userId << endl;
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
		return;
	}

	if (sessionManager->FindSessionByName(packet->username) != nullptr)
	{
		resPacket.result = ErrorCode::ALREADY_LOGGED_IN;
		cout << "[PacketHandler] Login failed - Already logged in: " << packet->username << endl;
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
		return;
	}

	string token = GenerateToken();

	if (!mRedis->CreateSession(token, packet->userId, 1800))
	{
		resPacket.result = ErrorCode::SERVER_ERROR;
		cout << "[PacketHandler] Login failed - Session creation error" << endl;
		session->SendPacket((char*)&resPacket, sizeof(resPacket));
		return;
	}

	mRedis->SetOnlineStatus(packet->username, session->GetSessionId());

	session->SetState(SessionState::AUTHENTICATED);
	session->SetUsername(packet->username);
	session->SetToken(token);

	sessionManager->RegisterSession(session);

	resPacket.result = ErrorCode::SUCCESS;
	cout << "[PacketHandler] Login success: " << packet->username << " (token: " << token.substr(0, 8) << "...)" << endl;

	session->SendPacket((char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleBroadcast(ClientSession* session, PacketHeader* header, SessionManager* sessionManager)
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

	sessionManager->BroadcastPacket(session, (char*)&resPacket, sizeof(resPacket));
}

void PacketHandler::HandleWhisper(ClientSession* session, PacketHeader* header, SessionManager* sessionManager)
{
	if (header->GetSize() != sizeof(WhisperChatReqPacket))
	{
		cout << "[PacketHandler] Whisper packet size error" << endl;
		return;
	}

	WhisperChatReqPacket* packet = (WhisperChatReqPacket*)header;
	string receiver = packet->GetReceiver();

	ClientSession* targetSession = sessionManager->FindSessionByName(receiver);

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
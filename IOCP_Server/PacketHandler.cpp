#include "PacketHandler.h"
#include <iostream>

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

void PacketHandler::HandleLogin(ClientSession* session, PacketHeader* header, SessionManager* sessionManager)
{
	if (header->GetSize() != sizeof(LoginReqPacket))
	{
		cout << "[PacketHandler] Login packet size error" << endl;
		return;
	}

	LoginReqPacket* packet = (LoginReqPacket*)header;
	bool isValid = CheckUserCredentials(packet->userId, packet->password);

	LoginResPacket resPacket;

	if (isValid)
	{
		session->SetState(SessionState::AUTHENTICATED);
		session->SetUsername(packet->username);
		sessionManager->RegisterSession(session);

		resPacket.result = ErrorCode::SUCCESS;
		cout << "[PacketHandler] Login success: " << packet->username << endl;
	}
	else
	{
		resPacket.result = ErrorCode::AUTH_FAILED;
		cout << "[PacketHandler] Login failed: " << packet->userId << endl;
	}

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

bool PacketHandler::CheckUserCredentials(const char* userId, const char* password)
{
	return true;
}
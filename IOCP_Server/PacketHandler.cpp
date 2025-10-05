#include "PacketHandler.h"
#include <iostream>

using namespace std;

void PacketHandler::ProcessPacket(ClientSession* session, DWORD dataSize, SessionManager* sessionManager)
{
	session->AddAccumulatedSize(dataSize);

	while (true)
	{
		DWORD currentAccumulatedSize = session->GetAccumulatedSize();

		cout << "[ProcessPacket] Loop - Accumulated: " << currentAccumulatedSize << endl;

		if (currentAccumulatedSize < sizeof(PacketHeader))
		{
			cout << "[PacketHandler] Waiting for header... (" << currentAccumulatedSize << "/" << sizeof(PacketHeader) << ")" << endl;
			break;
		}

		PacketHeader* header = (PacketHeader*)session->GetRecvBuffer();
		DWORD packetSize = header->GetSize();

		if (currentAccumulatedSize < packetSize)
		{
			cout << "[PacketHandler] Waiting for body..." << endl;
			break;
		}

		switch (header->GetType())
		{
		case PacketType::LOGIN_REQUEST:
			HandleLogin(session, header, sessionManager);
			break;

		case PacketType::BROADCAST_REQUEST:
		case PacketType::WHISPER_REQUEST:
			if (session->IsAuthenticated())
			{
				if (header->GetType() == PacketType::BROADCAST_REQUEST)
				{
					HandleBroadcast(session, header, sessionManager);
				}
				else
				{
					HandleWhispher(session, header, sessionManager);
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

		DWORD remainingSize = currentAccumulatedSize - packetSize;
		if (remainingSize > 0)
		{
			memmove(session->GetRecvBuffer(),
					session->GetRecvBuffer() + packetSize,
					remainingSize);
		}
		
		session->SetAccumulatedSize(remainingSize);
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

	session->SendPacekt((char*)&resPacket, sizeof(resPacket));
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

void PacketHandler::HandleWhispher(ClientSession* session, PacketHeader* header, SessionManager* sessionManager)
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
		resPacket.SetMessage(session->GetUsername().c_str(), packet->GetMessageW());
		targetSession->SendPacekt((char*)&resPacket, sizeof(resPacket));
	}
	else
	{
		resPacket.result = ErrorCode::USER_NOT_FOUND;
		snprintf(resPacket.message, sizeof(resPacket.message),
			"User '%s' not found", packet->receiver);
		session->SendPacekt((char*)&resPacket, sizeof(resPacket));
	}
}

bool PacketHandler::CheckUserCredentials(const char* userId, const char* password)
{
	return true;
}
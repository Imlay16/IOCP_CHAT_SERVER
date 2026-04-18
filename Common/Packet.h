#pragma once
#include <Windows.h>
#include <cstdint>
#include <string_view>
#include "..\Common\Common.h"

#pragma pack(push, 1)
struct PacketHeader
{
	PacketType type;
	uint16_t size;

	PacketHeader() : type(PacketType::NONE), size(0)
	{
	}

	PacketHeader(PacketType packetType) : type(packetType), size(0)
	{
	}

	void SetSize(uint16_t packetSize) { size = packetSize; }
	uint16_t GetSize() const { return size; }
	PacketType GetType() const { return type; }
};

template<typename T>
struct PacketBase : PacketHeader
{
	PacketBase(PacketType type) : PacketHeader(type)
	{
		this->size = static_cast<uint16_t>(sizeof(T));
	}
};

struct RegisterReqPacket : PacketBase<RegisterReqPacket>
{
	char loginId[MAX_USER_ID + 1];
	char password[MAX_USER_PW + 1];
	char nickname[MAX_USER_NAME + 1];

	RegisterReqPacket() : PacketBase(PacketType::REGISTER_REQUEST)
	{
		memset(loginId, 0, sizeof(loginId));
		memset(password, 0, sizeof(password));
		memset(nickname, 0, sizeof(nickname));
	}

	void SetRegisterInfo(const char* id, const char* pw, const char* name)
	{
		if (!id || !pw || !name)
			return;

		strncpy_s(loginId, sizeof(loginId), id, _TRUNCATE);
		strncpy_s(password, sizeof(password), pw, _TRUNCATE);
		strncpy_s(nickname, sizeof(nickname), name, _TRUNCATE);
	}
};

struct RegisterResPacket : PacketBase<RegisterResPacket>
{
	ErrorCode result;

	RegisterResPacket() : PacketBase(PacketType::REGISTER_RESPONSE) { }
};

struct LoginReqPacket : PacketBase<LoginReqPacket>
{
	char loginId[MAX_USER_ID + 1];
	char password[MAX_USER_PW + 1];

	LoginReqPacket() : PacketBase(PacketType::LOGIN_REQUEST)
	{
		memset(loginId, 0, sizeof(loginId));
		memset(password, 0, sizeof(password));
	}

	void SetLoginInfo(const char* id, const char* pw) {
		if (!id || !pw) 
			return;

		strncpy_s(loginId, sizeof(loginId), id, _TRUNCATE);
		strncpy_s(password, sizeof(password), pw, _TRUNCATE);
	}
};

struct LoginResPacket : PacketBase<LoginResPacket>
{
	ErrorCode result;
	char nickname[MAX_USER_NAME + 1];
	LoginResPacket() : PacketBase(PacketType::LOGIN_RESPONSE)
	{
		memset(nickname, 0, sizeof(nickname));
	}
};

struct LobbyChatReqPacket : PacketBase<LobbyChatReqPacket>
{
	char message[MAX_CHAT_SIZE + 1];

	LobbyChatReqPacket() : PacketBase(PacketType::LOBBY_CHAT_REQUEST)
	{
		memset(message, 0, sizeof(message));
	}

	void SetMessage(std::string_view msg)
	{
		if (msg.empty()) 
			return;

		strncpy_s(message, sizeof(message), msg.data(), _TRUNCATE);
	}
};

struct LobbyChatResPacket : PacketBase<LobbyChatResPacket>
{
	ErrorCode result;

	LobbyChatResPacket() : PacketBase(PacketType::LOBBY_CHAT_RESPONSE) { }
};

struct LobbyChatNotiPacket : PacketBase<LobbyChatNotiPacket>
{
	char user[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	LobbyChatNotiPacket() : PacketBase(PacketType::LOBBY_CHAT_NOTIFY)
	{
		memset(user, 0, sizeof(user));
		memset(message, 0, sizeof(message));
	}

	void SetUser(std::string_view name)
	{
		if (name.empty())
			return;

		strncpy_s(user, sizeof(user), name.data(), _TRUNCATE);
	}

	void SetMessage(std::string_view msg)
	{
		if (msg.empty())
			return;

		strncpy_s(message, sizeof(message), msg.data(), _TRUNCATE);
	}
};

struct WhisperChatReqPacket : PacketBase<WhisperChatReqPacket>
{
	char receiver[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	WhisperChatReqPacket() : PacketBase(PacketType::WHISPER_REQUEST)
	{
		memset(receiver, 0, sizeof(receiver));
		memset(message, 0, sizeof(message));
	}

	void SetWhisper(std::string_view receiverName, std::string_view msg)
	{
		if (receiverName.empty() || msg.empty()) 
			return;

		strncpy_s(receiver, sizeof(receiver), receiverName.data(), _TRUNCATE);
		strncpy_s(message, sizeof(message), msg.data(), _TRUNCATE);
	}

	const char* GetReceiver() const { return receiver; }
	const char* GetMsg() const { return message; }
};

struct WhisperChatResPacket : PacketBase<WhisperChatResPacket>
{
	ErrorCode result;

	WhisperChatResPacket() : PacketBase(PacketType::WHISPER_RESPONSE) { }
};

struct WhisperChatNotiPacket : PacketBase<WhisperChatNotiPacket>
{
	char sender[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	WhisperChatNotiPacket() : PacketBase(PacketType::WHISPER_NOTIFY)
	{
		memset(sender, 0, sizeof(sender));
		memset(message, 0, sizeof(message));
	}

	void SetMessage(std::string_view user, std::string_view msg)
	{
		if (user.empty() || msg.empty())
			return;

		strncpy_s(sender, sizeof(sender), user.data(), _TRUNCATE);
		strncpy_s(message, sizeof(message), msg.data(), _TRUNCATE);
	}

	const char* GetSender() { return sender; }
};

struct UserJoinNotifyPacket : PacketBase<UserJoinNotifyPacket>
{
	char user[MAX_USER_NAME + 1];
	UserJoinNotifyPacket() : PacketBase(PacketType::USER_JOIN_NOTIFY)
	{
		memset(user, 0, sizeof(user));
	}

	void SetUser(std::string_view name)
	{
		if (name.empty())
			return;

		strncpy_s(user, MAX_USER_NAME + 1, name.data(), _TRUNCATE);
	}
};

struct UserLeaveNotifyPacket : PacketBase<UserLeaveNotifyPacket>
{
	char user[MAX_USER_NAME + 1];
	UserLeaveNotifyPacket() : PacketBase(PacketType::USER_LEAVE_NOTIFY)
	{
		memset(user, 0, sizeof(user));
	}

	void SetUser(std::string_view name)
	{		
		if (name.empty())
			return;

		strncpy_s(user, sizeof(user), name.data(), _TRUNCATE);
	}
};

struct CreateRoomReqPacket : PacketBase<CreateRoomReqPacket>
{
	char roomName[MAX_ROOM_NAME + 1];
	uint16_t maxUser;

	CreateRoomReqPacket() : PacketBase(PacketType::CREATE_ROOM_REQUEST), maxUser(0)
	{
		memset(this->roomName, 0, sizeof(this->roomName));
	}

	void SetRoomInfo(std::string_view roomName, uint16_t maxUser)
	{
		if (roomName.empty())
			return;

		this->maxUser = maxUser;
		strncpy_s(this->roomName, sizeof(this->roomName), roomName.data(), _TRUNCATE);
	}
};

struct CreateRoomResPacket : PacketBase<CreateRoomResPacket>
{	
	ErrorCode result;
	RoomInfo room;

	CreateRoomResPacket() : PacketBase(PacketType::CREATE_ROOM_RESPONSE)
	{
	}

	void SetRoomInfo(uint16_t roomId, std::string_view roomName, uint16_t maxUserCount, uint16_t curUserCount)
	{
		if (roomName.empty()) 
			return;

		room.roomId = roomId;
		room.maxUserCount = maxUserCount;
		room.curUserCount = curUserCount;

		strncpy_s(room.roomName, sizeof(room.roomName), roomName.data(), _TRUNCATE);
	}
};

struct JoinRoomReqPacket : PacketBase<JoinRoomReqPacket>
{
	uint16_t roomId;

	JoinRoomReqPacket() : PacketBase(PacketType::JOIN_ROOM_REQUEST), roomId(0) {}

	void JoinRoom(uint16_t id)
	{
		roomId = id;
	}
};

struct JoinRoomResPacket : PacketBase<JoinRoomResPacket>
{
	ErrorCode result;
	RoomInfo room;
	UserInfo users[MAX_ROOM_USER];

	JoinRoomResPacket() : PacketBase(PacketType::JOIN_ROOM_RESPONSE) {}
};

struct LeaveRoomReqPacket : PacketBase<LeaveRoomReqPacket>
{
	LeaveRoomReqPacket() : PacketBase(PacketType::LEAVE_ROOM_REQUEST) {}
};

struct LeaveRoomResPacket : PacketBase<LeaveRoomResPacket>
{
	ErrorCode result;

	LeaveRoomResPacket() : PacketBase(PacketType::LEAVE_ROOM_RESPONSE) {}
};

struct RoomListReqPacket : PacketBase<RoomListReqPacket>
{
	uint16_t page;

	RoomListReqPacket() : PacketBase(PacketType::ROOM_LIST_REQUEST), page(0)
	{
	}

	void SetPage(uint16_t pageNumber)
	{
		page = pageNumber;
	}
};

struct RoomListResPacket : PacketBase<RoomListResPacket>
{
	ErrorCode result;
	uint16_t roomCount;
	RoomInfo rooms[MAX_ROOM_PAGE_COUNT];

	RoomListResPacket() : PacketBase(PacketType::ROOM_LIST_RESPONSE),
		result(ErrorCode::SUCCESS),
		roomCount(0)
	{
	}
};

struct RoomChatReqPacket : PacketBase<RoomChatReqPacket>
{
	char message[MAX_CHAT_SIZE + 1];

	RoomChatReqPacket() : PacketBase(PacketType::ROOM_CHAT_REQUEST)
	{
		memset(message, 0, sizeof(message));
	}

	void SetMessage(std::string_view msg)
	{
		if (msg.empty()) 
			return;

		strncpy_s(message, sizeof(message), msg.data(), _TRUNCATE);
	}
};

struct RoomChatResPacket : PacketBase<RoomChatResPacket>
{
	ErrorCode result;

	RoomChatResPacket() : PacketBase(PacketType::ROOM_CHAT_RESPONSE) { }
};

struct RoomChatNotiPacket : PacketBase<RoomChatNotiPacket>
{
	char user[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	RoomChatNotiPacket() : PacketBase(PacketType::ROOM_CHAT_NOTIFY)
	{
		memset(user, 0, sizeof(user));
		memset(message, 0, sizeof(message));
	}

	void SetMessage(std::string_view name, std::string_view msg)
	{
		if (name.empty() || msg.empty())
			return;

		strncpy_s(user, sizeof(user), name.data(), _TRUNCATE);
		strncpy_s(message, sizeof(message), msg.data(), _TRUNCATE);
	}
};

struct SystemNotiPacket : PacketBase<SystemNotiPacket>
{
	char message[MAX_CHAT_SIZE + 1];
	SystemNotiPacket() : PacketBase(PacketType::SYSTEM_NOTIFY)
	{
		memset(message, 0, sizeof(message));
	}
};

#pragma pack(pop)
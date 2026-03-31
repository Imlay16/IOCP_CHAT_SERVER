#pragma once
#include <Windows.h>
#include <cstdint>
#include <string_view>

constexpr uint16_t MAX_PACKET_SIZE = 2048;
constexpr uint16_t MAX_CHAT_SIZE = 1024;
constexpr uint8_t MAX_USER_ID = 32;
constexpr uint8_t MAX_USER_PW = 64;
constexpr uint8_t MAX_USER_NAME = 32;
constexpr uint8_t MAX_ROOM_NAME = 32;
constexpr uint8_t MAX_ROOM_PAGE_COUNT = 10;

enum class PacketType : uint16_t
{
	REGISTER_REQUEST = 1001,
	REGISTER_RESPONSE = 1002,

	LOGIN_REQUEST = 2001,
	LOGIN_RESPONSE = 2002,

	BROADCAST_REQUEST = 3001,
	BROADCAST_RESPONSE = 3002,
	ROOM_CHAT_REQUEST = 3003,
	ROOM_CHAT_RESPONSE = 3004,
	WHISPER_REQUEST = 3005,
	WHISPER_RESPONSE = 3006,

	ROOM_LIST_REQUEST = 4001,
	ROOM_LIST_RESPONSE = 4002,

	CREATE_ROOM_REQUEST = 4003,
	CREATE_ROOM_RESPONSE = 4004,

	JOIN_ROOM_REQUEST = 4005,
	JOIN_ROOM_RESPONSE = 4006,

	LEAVE_ROOM_REQUEST = 4007,
	LEAVE_ROOM_RESPONSE = 4008,

	USER_JOIN_NOTIFY = 5001,
	USER_LEAVE_NOTIFY = 5002,

	HEART_BEAT = 6000,

	NONE = 0,
};

enum class ErrorCode : uint16_t
{
	SUCCESS = 0,

	// Packet
	INVALID_PACKET = 1001,

	// Auth
	AUTH_FAILED = 1101,
	ALREADY_LOGGED_IN = 1102,

	// Login
	USER_NOT_FOUND = 1201,
	WRONG_PASSWORD = 1202,
	LOGIN_USER_ALREADY = 1203,
	LOGIN_USER_FULL = 1204,

	// Register
	ID_ALREADY_EXISTS = 1301,
	ID_INVALID = 1302,
	PW_INVALID = 1303,

	// Room
	ROOM_NOT_FOUND = 2001,
	ROOM_FULL = 2002,
	ALREADY_IN_ROOM = 2003,
	PERMISSION_DENIED = 2004,
	INVALID_ROOM_REQUEST = 2005,
	ROOM_CREATION_FAIL = 2006,

	// Server
	SERVER_ERROR = 9999
};


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

struct BroadcastReqPacket : PacketBase<BroadcastReqPacket>
{
	char message[MAX_CHAT_SIZE + 1];

	BroadcastReqPacket() : PacketBase(PacketType::BROADCAST_REQUEST)
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

struct BroadcastResPacket : PacketBase<BroadcastResPacket>
{
	char user[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	BroadcastResPacket() : PacketBase(PacketType::BROADCAST_RESPONSE)
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
	char sender[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	WhisperChatResPacket() : PacketBase(PacketType::WHISPER_RESPONSE)
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

	void SetResult(ErrorCode error) { result = error; }
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

struct RoomInfo
{
	uint16_t roomId;
	char roomName[MAX_ROOM_NAME + 1];
	uint16_t maxUserCount;
	uint16_t curUserCount;

	RoomInfo() : roomId(0), maxUserCount(0), curUserCount(0)
	{
		memset(roomName, 0, sizeof(roomName));
	}

	RoomInfo(uint16_t id, std::string_view name, uint16_t max, uint16_t cur)
		: roomId(id), maxUserCount(max), curUserCount(cur)
	{
		memset(roomName, 0, sizeof(roomName)); 
		strncpy_s(roomName, sizeof(roomName), name.data(), _TRUNCATE);
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
	char user[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	RoomChatResPacket() : PacketBase(PacketType::ROOM_CHAT_RESPONSE)
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


struct HeartbeatPacket : PacketBase<HeartbeatPacket>
{
	HeartbeatPacket() : PacketBase(PacketType::HEART_BEAT) { }	
};

#pragma pack(pop)
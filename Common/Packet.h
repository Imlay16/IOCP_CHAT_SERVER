#pragma once
#include <Windows.h>
#include <cstdint>

const UINT16 MAX_PACKET_SIZE = 2048;
const UINT16 MAX_CHAT_SIZE = 1024;
const UINT8 MAX_USER_ID = 32;
const UINT8 MAX_USER_PW = 64;
const UINT8 MAX_USER_NAME = 32;
const UINT8 MAX_ROOM_NAME = 32;
const UINT8 MAX_ROOM_PAGE_COUNT = 10;

enum class PacketType : UINT16
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

enum class ErrorCode : UINT16
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

	// Server
	SERVER_ERROR = 9999
};


#pragma pack(push, 1)
struct PacketHeader
{
	PacketType type;
	UINT16 size;

	PacketHeader() : type(PacketType::NONE), size(0)
	{
	}

	PacketHeader(PacketType packetType) : type(packetType), size(0)
	{
	}

	void SetSize(UINT16 packetSize) { size = packetSize; }
	UINT16 GetSize() const { return size; }
	PacketType GetType() const { return type; }
};

template<typename T>
struct PacketBase : PacketHeader
{
	PacketBase(PacketType type) : PacketHeader(type)
	{
		this->size = static_cast<UINT16>(sizeof(T));
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

	void SetMessage(const char* msg)
	{
		if (!msg || msg[0] == '\0') 
			return;

		strncpy_s(message, sizeof(message), msg, _TRUNCATE);
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

	void SetUser(const char* name)
	{
		if (!name) 
			return;

		strncpy_s(user, sizeof(user), name, _TRUNCATE);
	}

	void SetMessage(const char* msg)
	{
		if (!msg) 
			return;

		strncpy_s(message, sizeof(message), msg, _TRUNCATE);
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

	void SetWhisper(const char* receiverName, const char* msg)
	{
		if (!receiverName|| !msg) 
			return;

		strncpy_s(receiver, sizeof(receiver), receiverName, _TRUNCATE);
		strncpy_s(message, sizeof(message), msg, _TRUNCATE);
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

	void SetMessage(const char* user, const char* msg)
	{
		if (!user || !msg) 
			return;

		strncpy_s(sender, sizeof(sender), user, _TRUNCATE);
		strncpy_s(message, sizeof(message), msg, _TRUNCATE);
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

	void SetUser(const char* name)
	{
		if (!name)
			return;

		strncpy_s(user, MAX_USER_NAME + 1, name, _TRUNCATE);
	}
};

struct UserLeaveNotifyPacket : PacketBase<UserLeaveNotifyPacket>
{
	char user[MAX_USER_NAME + 1];
	UserLeaveNotifyPacket() : PacketBase(PacketType::USER_LEAVE_NOTIFY)
	{
		memset(user, 0, sizeof(user));
	}

	void SetUser(const char* name)
	{		
		if (!name)
			return;

		strncpy_s(user, sizeof(user), name, _TRUNCATE);
	}
};

struct RoomInfo
{
	UINT16 roomId;
	char roomName[MAX_ROOM_NAME + 1];
	UINT8 maxUserCount;
	UINT8 curUserCount;

	RoomInfo() : roomId(0), maxUserCount(0), curUserCount(0)
	{
		memset(roomName, 0, sizeof(roomName));
	}
};

struct CreateRoomReqPacket : PacketBase<CreateRoomReqPacket>
{
	char roomName[MAX_ROOM_NAME + 1];
	UINT8 maxUser;

	CreateRoomReqPacket() : PacketBase(PacketType::CREATE_ROOM_REQUEST), maxUser(0)
	{
		memset(this->roomName, 0, sizeof(this->roomName));
	}

	void SetRoomInfo(const char* roomName, UINT8 maxUser)
	{
		if (!roomName)
			return;

		this->maxUser = maxUser;
		strncpy_s(this->roomName, sizeof(this->roomName), roomName, _TRUNCATE);
	}
};

struct CreateRoomResPacket : PacketBase<CreateRoomResPacket>
{	
	ErrorCode result;
	RoomInfo room;

	CreateRoomResPacket() : PacketBase(PacketType::CREATE_ROOM_RESPONSE)
	{
	}

	void SetRoomInfo(UINT16 roomId, const char* roomName, UINT8 maxUserCount, UINT8 curUserCount)
	{
		if (!roomName) 
			return;

		room.roomId = roomId;
		room.maxUserCount = maxUserCount;
		room.curUserCount = curUserCount;

		strncpy_s(room.roomName, sizeof(room.roomName), roomName, _TRUNCATE);
	}
};

struct JoinRoomReqPacket : PacketBase<JoinRoomReqPacket>
{
	UINT16 roomId;

	JoinRoomReqPacket() : PacketBase(PacketType::JOIN_ROOM_REQUEST), roomId(0) {}

	void JoinRoom(UINT16 id)
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
	UINT16 page;

	RoomListReqPacket() : PacketBase(PacketType::ROOM_LIST_REQUEST), page(0)
	{
	}

	void SetPage(UINT16 pageNumber)
	{
		page = pageNumber;
	}
};

struct RoomListResPacket : PacketBase<RoomListResPacket>
{
	ErrorCode result;
	UINT16 curPage;
	UINT16 totalRoomCount;
	UINT8 roomCount;
	RoomInfo rooms[MAX_ROOM_PAGE_COUNT];

	RoomListResPacket() : PacketBase(PacketType::ROOM_LIST_RESPONSE),
		result(ErrorCode::SUCCESS),
		curPage(0),
		totalRoomCount(0),
		roomCount(0)
	{
	}

	void SetPage(UINT16 pageNumber)
	{
		curPage = pageNumber;
	}
};

struct RoomChatReqPacket : PacketBase<RoomChatReqPacket>
{
	char message[MAX_CHAT_SIZE + 1];

	RoomChatReqPacket() : PacketBase(PacketType::ROOM_CHAT_REQUEST)
	{
		memset(message, 0, sizeof(message));
	}

	void SetMessage(const char* msg)
	{
		if (!msg) 
			return;

		strncpy_s(message, sizeof(message), msg, _TRUNCATE);
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

	void SetMessage(const char* name, const char* msg)
	{
		if (!name || !msg) 
			return;

		strncpy_s(user, sizeof(user), name, _TRUNCATE);
		strncpy_s(message, sizeof(message), msg, _TRUNCATE);
	}
};


struct HeartbeatPacket : PacketBase<HeartbeatPacket>
{
	HeartbeatPacket() : PacketBase(PacketType::HEART_BEAT) { }	
};

#pragma pack(pop)
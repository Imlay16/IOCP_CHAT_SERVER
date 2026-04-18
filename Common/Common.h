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
constexpr uint16_t MAX_ROOM_USER = 32;
constexpr uint8_t MAX_ROOM_PAGE_COUNT = 10;
constexpr uint16_t MAX_ROOM_COUNT = 100;
constexpr uint16_t INVALID_ROOM_ID = UINT16_MAX;

enum class PacketType : uint16_t
{
	REGISTER_REQUEST = 1001,
	REGISTER_RESPONSE = 1002,

	LOGIN_REQUEST = 2001,
	LOGIN_RESPONSE = 2002,

	LOBBY_CHAT_REQUEST = 3001,
	LOBBY_CHAT_RESPONSE = 3002,
	LOBBY_CHAT_NOTIFY = 3003,

	ROOM_CHAT_REQUEST = 3004,
	ROOM_CHAT_RESPONSE = 3005,
	ROOM_CHAT_NOTIFY = 3006,

	WHISPER_REQUEST = 3007,
	WHISPER_RESPONSE = 3008,
	WHISPER_NOTIFY = 3009,

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

	SYSTEM_NOTIFY = 7000,

	NONE = 0,
};

enum class ErrorCode : uint16_t
{
	SUCCESS = 0,

	// Packet
	INVALID_PACKET = 1001,

	// Auth (State)
	AUTH_FAILED = 1101,
	ALREADY_LOGGED_IN = 1102,
	INVALID_STATE = 1103, 
	
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

struct UserInfo
{
	uint16_t userId;
	char nickname[MAX_USER_NAME + 1];

	UserInfo() : userId(0)
	{
		memset(nickname, 0, sizeof(nickname));
	}

	UserInfo(uint16_t id, std::string_view name)
		: userId(id)
	{
		memset(nickname, 0, sizeof(nickname));
		strncpy_s(nickname, sizeof(nickname), name.data(), _TRUNCATE);
	}
};

#pragma pack(pop)
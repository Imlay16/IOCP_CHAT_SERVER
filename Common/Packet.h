#pragma once

#include <Windows.h>

const UINT16 MAX_CHAT_SIZE = 1024;
const UINT8 MAX_USER_ID = 32;
const UINT8 MAX_USER_PW = 32;
const UINT8 MAX_ROOM_PW = 32;
const UINT8 MAX_ROOM_NUM = 32;
const UINT8 MAX_USER_NAME = 32;

enum class PacketType : UINT16
{
	LOGIN_REQUEST = 2001,
	LOGIN_RESPONSE = 2002,

	CHAT_MSG = 3000, 

	BROADCAST_REQUEST = 3001,
	BROADCAST_RESPONSE = 3002,
	ROOM_CHAT_REQUEST = 3003,
	ROOM_CHAT_RESPONSE = 3004,
	WHISPER = 3005,

	ROOM_LIST_REQUEST = 4001,
	ROOM_LIST_RESPONSE = 4002,
	ROOM_CREATE_REQUEST = 4003,
	ROOM_CREATE_RESPONSE = 4004,
	ROOM_JOIN_REQUEST = 4005,
	ROOM_JOIN_RESPONSE = 4006,
	ROOM_LEAVE_REQUEST = 4007,
	ROOM_LEAVE_RESPONSE = 4008,

	USER_JOIN_NOTIFY = 5001,
	USER_LEAVE_NOTIFY = 5002,
};

enum class ErrorCode : UINT16
{
	SUCCESS = 0,
	INVALID_PACKET = 1001,
	AUTH_FAILED = 1002,

	LOGIN_USER_ALREADY = 1003,
	LOGIN_USER_FULL = 1004,

	USER_NOT_FOUND = 1005,
	ROOM_NOT_FOUND = 1006,
	ROOM_FULL = 1007,
	ALREADY_IN_ROOM = 1008,
	PERMISSION_DENIED = 1009,

	SERVER_ERROR = 9999
};


#pragma pack(push, 1)
struct PacketHeader
{
	PacketType type;
	UINT16 size;

	PacketHeader(PacketType packetType) : type(packetType), size(0) {  }
	PacketHeader(PacketType packetType, UINT16 packetSize) : type(packetType), size(packetSize) { }
	void SetSize(UINT16 packetSize) { size = packetSize; }
};

struct LoginReqPacket : PacketHeader
{
	char userId[MAX_USER_ID + 1];
	char password[MAX_USER_PW + 1];

	LoginReqPacket() : PacketHeader(PacketType::LOGIN_REQUEST)
	{
		memset(userId, 0, sizeof(userId));
		memset(password, 0, sizeof(password));
		SetSize(sizeof(*this));
	}

	void SetLoginInfo(const char* user, const char* pw)
	{
		// 로그인에 필요한 정보를 설정
		strcpy_s(userId, MAX_USER_ID + 1, user);
		strcpy_s(password, MAX_USER_PW + 1, password);
	}
};

struct LoginResPacket : PacketHeader
{
	ErrorCode result;
	std::string username;
	
	LoginResPacket() : PacketHeader(PacketType::LOGIN_RESPONSE)
	{
		SetSize(sizeof(*this));
	}
};

struct BroadCastReqPacket : PacketHeader
{
	char message[MAX_CHAT_SIZE + 1];

	BroadCastReqPacket() : PacketHeader(PacketType::BROADCAST_REQUEST)
	{
		memset(message, 0, sizeof(message));
		SetSize(sizeof(*this));
	}

	void SetMessage(const char* msg)
	{
		if (!msg) return;
		strcpy_s(message, MAX_CHAT_SIZE + 1, msg);
	}
};

struct BroadCastResPacket : PacketHeader
{
	// Res할 땐, username 필요! 누가 보냈는지를 명시해야함
	char user[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	BroadCastResPacket() : PacketHeader(PacketType::BROADCAST_RESPONSE)
	{
		memset(user, 0, sizeof(user));
		memset(message, 0, sizeof(message));
		SetSize(sizeof(*this));
	}

	void SetMessage(const char*name, const char* msg)
	{
		if (!name || !msg) return;
		strcpy_s(user, sizeof(user), name);
		strcpy_s(message, sizeof(message), msg);
	}
};

struct RoomChatReqPacket : PacketHeader
{
	char message[MAX_CHAT_SIZE + 1];

	RoomChatReqPacket() : PacketHeader(PacketType::ROOM_CHAT_REQUEST)
	{
		memset(message, 0, sizeof(message));
		SetSize(sizeof(*this));
	}

	void SetMessage(const char* msg)
	{
		if (!msg) return;

		strcpy_s(message, sizeof(message), msg);
		SetSize(sizeof(*this));
	}
};

struct RoomChatResPacket : PacketHeader
{
	char user[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	RoomChatResPacket() : PacketHeader(PacketType::ROOM_CHAT_RESPONSE)
	{
		memset(user, 0, sizeof(user));
		memset(message, 0, sizeof(message));
	}
	
	void SetMessage(const char* name, const char* msg)
	{
		if (!name || !msg) return;

		strcpy_s(user, sizeof(user), name);
		strcpy_s(message, sizeof(message), msg);
		SetSize(sizeof(*this));
	}
};

struct WhisperChatPacket : PacketHeader
{
	char targetUser[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	WhisperChatPacket() : PacketHeader(PacketType::WHISPER)
	{
		memset(targetUser, 0, sizeof(targetUser));
		memset(message, 0, sizeof(message));
	}

	void SetMessage(const char* user, const char* msg)
	{
		if (!user || !msg) return;

		strcpy_s(targetUser, sizeof(targetUser), user);
		strcpy_s(message, sizeof(message), msg);
		SetSize(sizeof(*this));
	}
};

struct UserJoinNotifyPacket : PacketHeader
{
	char user[MAX_USER_NAME + 1];
	UserJoinNotifyPacket() : PacketHeader(PacketType::USER_JOIN_NOTIFY, sizeof(*this))
	{
		memset(user, 0, sizeof(user));
		SetSize(sizeof(*this));
	}

	void SetUser(const char* name)
	{
		strcpy_s(user, MAX_USER_NAME + 1, name);
	}
};

struct UserLeaveNotifyPacket : PacketHeader
{
	char user[MAX_USER_NAME + 1];
	UserLeaveNotifyPacket() : PacketHeader(PacketType::USER_LEAVE_NOTIFY, sizeof(*this))
	{
		memset(user, 0, sizeof(user));
		SetSize(sizeof(*this));
	}

	void SetUser(const char* name)
	{
		strcpy_s(user, MAX_USER_NAME + 1, name);
	}
};

struct CreateRoomReqPacket : PacketHeader
{
	UINT8 roomNum;
	UINT8 maxUser;
	bool isPrivate;
	char roomPassword[MAX_ROOM_PW + 1];
	
	// 방 설정 정보
	// 최대 인원
	// 비밀번호 유무
	// 비밀번호
	// 만약 비밀방이 아니면, PW를 입력X
	// -> 패킷을 나누어야 하나? CreatePublicRoom / CreatePrivateRoom 이렇게..?

	CreateRoomReqPacket() : PacketHeader(PacketType::ROOM_CREATE_REQUEST) { }

	void CreateRoom(UINT8 roomNum, UINT8 maxUser, bool isPrivate, const char* pw)
	{
		this->roomNum = roomNum;
		this->maxUser = maxUser;
		this->isPrivate = isPrivate;
		strcpy_s(roomPassword, MAX_ROOM_PW + 1, pw);
	}
};

struct CreateRoomResPacket : PacketHeader
{
	ErrorCode result;

	CreateRoomResPacket() : PacketHeader(PacketType::ROOM_CREATE_RESPONSE, sizeof(*this)) { }
};

struct RoomJoinReqPacket : PacketHeader
{
	UINT8 roomNum;

	RoomJoinReqPacket() : PacketHeader(PacketType::ROOM_JOIN_REQUEST) { }

	void SetRoom(UINT8 roomNum)
	{
		this->roomNum = roomNum;
		SetSize(sizeof(*this));
	}
};

struct RoomJoinResPacket : PacketHeader
{
	ErrorCode result;
	char user[MAX_USER_NAME + 1];

	RoomJoinResPacket() : PacketHeader(PacketType::ROOM_JOIN_RESPONSE, sizeof(*this)) { }
};

struct RoomLeaveReqPacket : PacketHeader
{
	RoomLeaveReqPacket() : PacketHeader(PacketType::ROOM_LEAVE_REQUEST) { }
};

struct RoomLeaveResPacket : PacketHeader
{
	ErrorCode result;
	char user[MAX_USER_NAME + 1];

	RoomLeaveResPacket() : PacketHeader(PacketType::ROOM_LEAVE_RESPONSE, sizeof(*this)) { }
};

struct RoomListReqPacket : PacketHeader
{
	RoomListReqPacket() : PacketHeader(PacketType::ROOM_LIST_REQUEST) { }

	void SetRoom(UINT8 room)
	{
		SetSize(sizeof(*this));
	}
};


// 고치기!!!
// ROOM_LIST를 보내줘야하기 때문에, 
// 패킷 내용을 바꾸어야함!!!! 

struct RoomListResPacket : PacketHeader
{
	ErrorCode result;
	// UINT8 roomNum;

	// 방 리스트 
	// 방 정보들 (CURRENT_USER_COUNT / FULL COUNT)
	// 비밀번호 있는 방인지 없는 방인지 등등
	// 방 List 

	RoomListResPacket() : PacketHeader(PacketType::ROOM_LIST_RESPONSE) { }

	void SetRoom(UINT8 room)
	{
		// roomNum = room;
		SetSize(sizeof(*this));
	}
};
#pragma pack(pop)
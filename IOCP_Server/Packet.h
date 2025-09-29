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

	BROADCAST_REQUEST = 3001,
	BROADCAST_RESPONSE = 3002,
	ROOM_CHAT_REQUEST = 3003,
	ROOM_CHAT_RESPONSE = 3004,
	WHISPER_REQUEST = 3005,
	WHISPER_RESPONSE = 3006,

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
private:
	PacketType type;
	UINT16 size;

public:
	PacketHeader(PacketType packetType) : type(packetType), size(0) {  }
	PacketHeader(PacketType packetType, UINT16 packetSize) : type(packetType), size(packetSize) { }
	void SetSize(UINT16 packetSize) { size = packetSize; }
	UINT16 GetSize() { return size; }
	PacketType GetType() { return type; }
};

struct LoginReqPacket : PacketHeader
{
	char userId[MAX_USER_ID + 1];
	char password[MAX_USER_PW + 1];
	char username[MAX_USER_NAME + 1];

	LoginReqPacket() : PacketHeader(PacketType::LOGIN_REQUEST)
	{
		memset(userId, 0, sizeof(userId));
		memset(password, 0, sizeof(password));
		memset(username, 0, sizeof(username));
		SetSize(sizeof(*this));
	}

	void SetLoginInfo(const char* user, const char* pw, const char* name)
	{
		// 로그인에 필요한 정보를 설정
		strcpy_s(userId, MAX_USER_ID + 1, user);
		strcpy_s(password, MAX_USER_PW + 1, pw);
		strcpy_s(username, MAX_USER_NAME + 1, name);
	}
};

struct LoginResPacket : PacketHeader
{
	ErrorCode result;
	
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

	void SetUser(const char* name)
	{
		if (!user) return;
		strcpy_s(user, sizeof(user), name);
	}

	void SetMessage(const char* msg)
	{
		if (!msg) return;		
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

struct WhispherChatReqPacket : PacketHeader
{
	ErrorCode result;
	char receiver[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	WhispherChatReqPacket() : PacketHeader(PacketType::WHISPER_REQUEST)
	{
		memset(receiver, 0, sizeof(receiver));
		memset(message, 0, sizeof(message));
	}

	void SetMessage(const char* user, const char* msg)
	{
		if (!user || !msg) return;

		strcpy_s(receiver, sizeof(receiver), user);
		strcpy_s(message, sizeof(message), msg);
		SetSize(sizeof(*this));
	}
	
	void SetResult(ErrorCode error) { result = error; }
	const char* GetReceiver() { return receiver; }
	const char* GetMessage() { return message; }
};

struct WhispherChatResPacket : PacketHeader
{
	ErrorCode result;
	char sender[MAX_USER_NAME + 1];
	char message[MAX_CHAT_SIZE + 1];

	WhispherChatResPacket() : PacketHeader(PacketType::WHISPER_RESPONSE)
	{
		memset(sender, 0, sizeof(sender));
		memset(message, 0, sizeof(message));
	}

	void SetMessage(const char* user, const char* msg)
	{
		if (!user || !msg) return;

		strcpy_s(sender, sizeof(sender), user);
		strcpy_s(message, sizeof(message), msg);
		SetSize(sizeof(*this));
	}

	void SetResult(ErrorCode error) { result = error; }
	const char* GetSender() { return sender; }
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

struct RoomListResPacket : PacketHeader
{
	ErrorCode result;
	// UINT8 roomNum;

	// 방 리스트 (ROOM NUMBER 리스트)
	// 방 정보들 (CURRENT_USER_COUNT / FULL COUNT)
	// 비밀번호 있는 방인지 없는 방인지 등등

	RoomListResPacket() : PacketHeader(PacketType::ROOM_LIST_RESPONSE) { }

	void SetRoom(UINT8 room)
	{
		// roomNum = room;
		SetSize(sizeof(*this));
	}
};
#pragma pack(pop)
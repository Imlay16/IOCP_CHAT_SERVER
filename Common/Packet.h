#pragma once

#include <Windows.h>

const UINT16 MAX_PACKET_SIZE = 2048;
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

	HEART_BEAT_REQUEST = 6001,
	HEART_BEAT_RESPONSE = 6002,

	NONE = 0,
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

	PacketHeader() : type(PacketType::NONE), size(0)
	{
	}

	PacketHeader(PacketType packetType) : type(packetType), size(sizeof(0))
	{
		// 생성 시 자동으로 크기 설정
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

struct LoginReqPacket : PacketBase<LoginReqPacket>
{
	char userId[MAX_USER_ID + 1];
	char password[MAX_USER_PW + 1];
	char username[MAX_USER_NAME + 1];

	LoginReqPacket() : PacketBase(PacketType::LOGIN_REQUEST)
	{
		memset(userId, 0, sizeof(userId));
		memset(password, 0, sizeof(password));
		memset(username, 0, sizeof(username));
	}

	void SetLoginInfo(const char* user, const char* pw, const char* name)
	{
		// 로그인에 필요한 정보를 설정
		strcpy_s(userId, MAX_USER_ID + 1, user);
		strcpy_s(password, MAX_USER_PW + 1, pw);
		strcpy_s(username, MAX_USER_NAME + 1, name);
	}
};

struct LoginResPacket : PacketBase<LoginResPacket>
{
	ErrorCode result;

	LoginResPacket() : PacketBase(PacketType::LOGIN_RESPONSE)
	{

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
		if (!msg) return;
		strcpy_s(message, MAX_CHAT_SIZE + 1, msg);
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
		if (!name) return;
		strcpy_s(user, sizeof(user), name);
	}

	void SetMessage(const char* msg)
	{
		if (!msg) return;
		strcpy_s(message, sizeof(message), msg);
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
		if (!msg) return;
		strcpy_s(message, sizeof(message), msg);
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
		if (!name || !msg) return;

		strcpy_s(user, sizeof(user), name);
		strcpy_s(message, sizeof(message), msg);
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
		if (!receiverName || !msg) return;
		strcpy_s(receiver, sizeof(receiver), receiverName);
		strcpy_s(message, sizeof(message), msg);
	}

	// void SetResult(ErrorCode error) { result = error; }
	const char* GetReceiver() { return receiver; }
	const char* GetMsg() { return message; }
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
		if (!user || !msg) return;

		strcpy_s(sender, sizeof(sender), user);
		strcpy_s(message, sizeof(message), msg);
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
		strcpy_s(user, MAX_USER_NAME + 1, name);
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
		strcpy_s(user, MAX_USER_NAME + 1, name);
	}
};

struct CreateRoomReqPacket : PacketBase<CreateRoomReqPacket>
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

	CreateRoomReqPacket() : PacketBase(PacketType::ROOM_CREATE_REQUEST) {}

	void CreateRoom(UINT8 roomNum, UINT8 maxUser, bool isPrivate, const char* pw)
	{
		this->roomNum = roomNum;
		this->maxUser = maxUser;
		this->isPrivate = isPrivate;
		strcpy_s(roomPassword, MAX_ROOM_PW + 1, pw);
	}
};

struct CreateRoomResPacket : PacketBase<CreateRoomResPacket>
{
	ErrorCode result;

	CreateRoomResPacket() : PacketBase(PacketType::ROOM_CREATE_RESPONSE)
	{

	}
};

struct JoinRoomReqPacket : PacketBase<JoinRoomReqPacket>
{
	UINT8 roomNum;

	JoinRoomReqPacket() : PacketBase(PacketType::ROOM_JOIN_REQUEST) {}

	void SetRoom(UINT8 roomNum)
	{
		this->roomNum = roomNum;
	}
};

struct JoinRoomResPacket : PacketBase<JoinRoomResPacket>
{
	ErrorCode result;
	char user[MAX_USER_NAME + 1];

	JoinRoomResPacket() : PacketBase(PacketType::ROOM_JOIN_RESPONSE) {}
};

struct RoomLeaveReqPacket : PacketBase<RoomLeaveReqPacket>
{
	RoomLeaveReqPacket() : PacketBase(PacketType::ROOM_LEAVE_REQUEST) {}
};

struct RoomLeaveResPacket : PacketBase<RoomLeaveResPacket>
{
	ErrorCode result;
	char user[MAX_USER_NAME + 1];

	RoomLeaveResPacket() : PacketBase(PacketType::ROOM_LEAVE_RESPONSE)
	{

	}
};

struct RoomListReqPacket : PacketBase<RoomListReqPacket>
{
	RoomListReqPacket() : PacketBase(PacketType::ROOM_LIST_REQUEST)
	{

	}

	void SetRoom(UINT8 room)
	{
		
	}
};

struct RoomListResPacket : PacketBase<RoomListResPacket>
{
	ErrorCode result;
	// UINT8 roomNum;

	// 방 리스트 (ROOM NUMBER 리스트)
	// 방 정보들 (CURRENT_USER_COUNT / FULL COUNT)
	// 비밀번호 있는 방인지 없는 방인지 등등

	RoomListResPacket() : PacketBase(PacketType::ROOM_LIST_RESPONSE)
	{

	}

	void SetRoom(UINT8 room)
	{
		// roomNum = room;
	}
};
#pragma pack(pop)
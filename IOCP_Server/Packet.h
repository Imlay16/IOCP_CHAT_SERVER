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
	USER_NOT_FOUND = 1003,
	ROOM_NOT_FOUND = 1004,
	ROOM_FULL = 1005,
	ALREADY_IN_ROOM = 1006,
	PERMISSION_DENIED = 1007,
	SERVER_ERROR = 9999
};


#pragma pack(push, 1)
struct PacketHeader
{
	PacketType type;
	UINT16 size;

	PacketHeader(PacketType packetType) : type(packetType), size(0) { }
	PacketHeader(PacketType packetType, UINT16 packetSize) : type(packetType), size(packetSize) { }
	void SetSize(UINT16 packetSize) { size = packetSize; }
};

struct LoginReqPacket
{
	PacketHeader header;
	char userId[MAX_USER_ID + 1];
	char password[MAX_USER_PW + 1];

	LoginReqPacket() : header(PacketType::LOGIN_REQUEST)
	{
		memset(userId, 0, sizeof(userId));
		memset(password, 0, sizeof(password));
	}

	void SetLoginInfo(const char* user, const char* pw)
	{
		// 로그인에 필요한 정보를 설정
		strcpy_s(userId, MAX_USER_ID + 1, user);
		strcpy_s(password, MAX_USER_PW + 1, password);
		header.SetSize(sizeof(*this));
	}
};

struct LoginResPacket
{
	PacketHeader header;
	ErrorCode result;
	
	LoginResPacket() : header(PacketType::LOGIN_RESPONSE)
	{
		header.SetSize(sizeof(*this));
	}
};

struct BaseChatPacket
{
	PacketHeader header;
	UINT16 messageLen;
	char message[MAX_CHAT_SIZE + 1];

	BaseChatPacket(PacketType type) : header(type)
	{
		memset(message, 0, sizeof(message));
		messageLen = 0;
	}
};

struct BroadCastReqPacket : BaseChatPacket
{
	// username 필드 없음 (세션에서 관리)
	BroadCastReqPacket() : BaseChatPacket(PacketType::BROADCAST_REQUEST) { }

	void SetMessage(const char* msg) {
		strcpy_s(message, sizeof(message), msg);
		messageLen = strlen(msg);
		header.SetSize(sizeof(PacketHeader)
			+ sizeof(messageLen) + messageLen);
	}
};

struct BroadCastResPacket : BaseChatPacket
{
	// Res할 땐, username 필요! 누가 보냈는지를 명시해야함
	char user[MAX_USER_NAME + 1];

	BroadCastResPacket() : BaseChatPacket(PacketType::BROADCAST_RESPONSE)
	{
		memset(user, 0, sizeof(user));
	}

	void SetMessage(const char*name, const char* msg)
	{
		memset(user, 0, sizeof(user));
		strcpy_s(user, sizeof(user), name);

		memset(message, 0, sizeof(message));
		strcpy_s(message, sizeof(message), msg);
		messageLen = strlen(msg);
		header.SetSize(sizeof(PacketHeader)
			+ sizeof(user)
			+ sizeof(messageLen) + messageLen);
	}
};

struct RoomChatReqPacket : BaseChatPacket
{
	RoomChatReqPacket() : BaseChatPacket(PacketType::ROOM_CHAT_REQUEST) { }

	void SetMessage(const char* msg)
	{
		memset(message, 0, sizeof(message));
		strcpy_s(message, sizeof(message), msg);
		messageLen = strlen(msg);
		header.SetSize(sizeof(PacketHeader)
			+ sizeof(messageLen) + messageLen);
	}
};

struct RoomChatResPacket : BaseChatPacket
{
	char user[MAX_USER_NAME + 1];

	RoomChatResPacket() : BaseChatPacket(PacketType::ROOM_CHAT_RESPONSE)
	{
		memset(user, 0, sizeof(user));
	}
	
	void SetMessage(const char* name, const char* msg)
	{
		memset(user, 0, sizeof(user));
		strcpy_s(user, sizeof(user), name);

		memset(message, 0, sizeof(message));
		strcpy_s(message, sizeof(message), msg);
		messageLen = strlen(msg);
		header.SetSize(sizeof(PacketHeader)
			+ sizeof(user)
			+ sizeof(messageLen) + messageLen);
	}
};

struct WhisperChatPacket : BaseChatPacket
{
	char targetUser[MAX_USER_NAME + 1];

	WhisperChatPacket() : BaseChatPacket(PacketType::WHISPER)
	{
		memset(targetUser, 0, sizeof(targetUser));
	}

	void SetMessage(const char* user, const char* msg)
	{
		memset(targetUser, 0, sizeof(targetUser));
		strcpy_s(targetUser, sizeof(targetUser), user);
		memset(message, 0, sizeof(message));
		strcpy_s(message, sizeof(message), msg);
		messageLen = strlen(msg);
		header.SetSize(sizeof(PacketHeader) 
			+ sizeof(targetUser) 
			+ sizeof(messageLen) + messageLen);
	}
};

struct UserJoinNotifyPacket
{
	PacketHeader header;
	char user[MAX_USER_NAME + 1];
	UserJoinNotifyPacket() : header(PacketType::USER_JOIN_NOTIFY, sizeof(*this)) { } 
};

struct UserLeaveNotifyPacket
{
	PacketHeader header;
	char user[MAX_USER_NAME + 1];
	UserLeaveNotifyPacket() : header(PacketType::USER_LEAVE_NOTIFY, sizeof(*this)) { } 
};

struct CreateRoomReqPacket
{
	PacketHeader header;
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

	CreateRoomReqPacket() : header(PacketType::ROOM_CREATE_REQUEST) { }

	void CreateRoom(UINT8 roomNum, UINT8 maxUser, bool isPrivate, const char* pw)
	{
		this->roomNum = roomNum;
		this->maxUser = maxUser;
		this->isPrivate = isPrivate;
		strcpy_s(roomPassword, MAX_ROOM_PW + 1, pw);
	}
};

struct CreateRoomResPacket
{
	PacketHeader header;
	ErrorCode result;

	CreateRoomResPacket() : header(PacketType::ROOM_CREATE_RESPONSE, sizeof(*this)) { }
};

struct RoomJoinReqPacket
{
	PacketHeader header;
	UINT8 roomNum;

	RoomJoinReqPacket() : header(PacketType::ROOM_JOIN_REQUEST) { }

	void SetRoom(UINT8 roomNum)
	{
		this->roomNum = roomNum;
		header.SetSize(sizeof(*this));
	}
};

struct RoomJoinResPacket
{
	PacketHeader header;
	ErrorCode result;
	char user[MAX_USER_NAME + 1];

	RoomJoinResPacket() : header(PacketType::ROOM_JOIN_RESPONSE, sizeof(*this)) { }
};

struct RoomLeaveReqPacket
{
	PacketHeader header;
	RoomLeaveReqPacket() : header(PacketType::ROOM_LEAVE_REQUEST) { }
};

struct RoomLeaveResPacket
{
	PacketHeader header;
	ErrorCode result;
	char user[MAX_USER_NAME + 1];

	RoomLeaveResPacket() : header(PacketType::ROOM_LEAVE_RESPONSE, sizeof(*this)) { }
};

struct RoomListReqPacket
{
	PacketHeader header;

	RoomListReqPacket() : header(PacketType::ROOM_LIST_REQUEST) { }

	void SetRoom(UINT8 room)
	{
		header.SetSize(sizeof(*this));
	}
};


// 고치기!!!
// ROOM_LIST를 보내줘야하기 때문에, 
// 패킷 내용을 바꾸어야함!!!! 

struct RoomListResPacket
{
	PacketHeader header;
	ErrorCode result;
	// UINT8 roomNum;

	// 방 리스트 
	// 방 정보들 (CURRENT_USER_COUNT / FULL COUNT)
	// 비밀번호 있는 방인지 없는 방인지 등등
	// 방 List 

	RoomListResPacket() : header(PacketType::ROOM_LIST_RESPONSE) { }

	void SetRoom(UINT8 room)
	{
		// roomNum = room;
		header.SetSize(sizeof(*this));
	}
};
#pragma pack(pop)
#include <hiredis.h>
#include <winsock2.h>
#include <string>
#include <iostream>

class RedisManager
{
private:
	redisContext* context;
	bool connected;

public:
	RedisManager() : context(nullptr), connected(false) { }

	~RedisManager()
	{
		if (context != nullptr)
		{
			redisFree(context);
			context = nullptr;
		}
	}

	bool Init(const char* ip, int port, timeval timeout)
	{
		context = redisConnectWithTimeout(ip, port, timeout);

		if (context == nullptr || context->err)
		{
			if (context)
			{
				std::cerr << "Redis 연결 실패" << context->errstr << std::endl;
				redisFree(context);
				context = nullptr;
			}
			else
			{
				std::cerr << "Redis context 할당 실패" << std::endl;
			}
			connected = false;
			return false;
		}
		
		std::cout << "Redis 연결 성공" << std::endl;
		connected = true;
		return true;
	}

	bool IsConnected() const
	{
		return connected && context != nullptr;
	}

	bool RegisterUser(const std::string& username, const std::string& passwordHash)
	{
		if (!IsConnected()) return false;

		redisReply* reply = (redisReply*)redisCommand(context, "HSET user:%s password %s", username.c_str(), passwordHash.c_str());

		if (reply == nullptr)
		{
			std::cerr << "Redis 명령 실패" << std::endl;
			return false;
		}

		bool success = (reply->type != REDIS_REPLY_ERROR);
		freeReplyObject(reply);
		return success;
	}

	std::string GetUserPassword(const std::string& username)
	{
		if (!IsConnected()) return "";

		redisReply* reply = (redisReply*)redisCommand(context, "HGET user:%s password", username.c_str());

		if (reply == nullptr || reply->type == REDIS_REPLY_NIL)
		{
			if (reply) freeReplyObject(reply);
			return "";
		}

		std::string password = reply->str;
		freeReplyObject(reply);
		return password;
	}

	bool CreateSession(const std::string& token, const std::string& username, int ttl = 1800)
	{
		if (!IsConnected()) return false;

		redisReply* reply = (redisReply*)redisCommand(context, "SETEX session:%s %d %s", token.c_str(), ttl, username.c_str());

		if (reply == nullptr)
		{
			return false;
		}

		bool success = (reply->type == REDIS_REPLY_STATUS);
		freeReplyObject(reply);
		return success;
	}

	std::string validateSession(const std::string& token)
	{
		if (!IsConnected()) return "";

		redisReply* reply = (redisReply*)redisCommand(context, "GET session:%s", token.c_str());

		if (reply == nullptr || reply->type == REDIS_REPLY_NIL)
		{
			if (reply) freeReplyObject(reply);
			return "";
		}

		std::string username = reply->str;
		freeReplyObject(reply);
		return username;
	}

	bool DeleteSession(const std::string& token)
	{
		if (!IsConnected()) return false;

		redisReply* reply = (redisReply*)redisCommand(context, "DEL session:%s", token.c_str());

		if (reply == nullptr)
		{
			return false;
		}

		bool success = (reply->integer > 0);
		freeReplyObject(reply);
		return success;
	}

	bool SetOnlineStatus(const std::string& username, int socketId)
	{
		if (!IsConnected()) return false;

		redisReply* reply = (redisReply*)redisCommand(context, "HSET online:%s socket_id %d login_time %lld", username.c_str(), socketId, time(nullptr));

		if (reply == nullptr)
		{
			return false;
		}

		bool success = (reply->type != REDIS_REPLY_ERROR);
		freeReplyObject(reply);
		return success;
	}

	bool RemoveOnlineStatus(const std::string& username)
	{
		if (!IsConnected()) return false;
	
		redisReply* reply = (redisReply*)redisCommand(context, "DEL online:%s", username.c_str());

		if (reply == nullptr)
		{
			return false;
		}

		bool success = (reply->integer > 0);
		freeReplyObject(reply);
		return success;
	}
};


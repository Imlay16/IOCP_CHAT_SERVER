#pragma once
#include <mysqlx/xdevapi.h>
#include <string>
#include <mutex>

using namespace std;

enum class DbResult
{
	OK,

	DUPLICATE_ID,
	USER_NOT_FOUND,
	WRONG_PASSWORD,

	CONNECTION_ERROR,
	QUERY_ERROR
};

struct UserRow
{
	long long id = 0;
	string loginId;
	string passwordHash;
	string nickname;
};

class DbManager
{
public:
	DbManager() = default;
	~DbManager() = default;

	DbManager(const DbManager&) = delete;
	DbManager& operator=(const DbManager&) = delete;

	bool CreateTables();

	bool Init(const string& host,
			  int port,
			  const string& user,
			  const string& password,
			  const string& schema);
	
	bool IsConnected() const;

	DbResult RegisterUser(const string& loginId,
						   const string& passwordHash,
						   const string& nickname);

	DbResult LoginUser(const string& loginId,
					const string& passwordHash,
					UserRow& outUser);

private:
	DbResult GetUserByLoginId(const string& loginId, UserRow& outUser);
	
private:
	unique_ptr<mysqlx::Session> mSession;
	string mSchema;
	mutex mDbMutex;
};


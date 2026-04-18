#include "DbManager.h"

#include <iostream>

using namespace std;

bool DbManager::CreateTables()
{
    try
    {
        if (!mSession)
        {
            cout << "[DB] CreateTables Error: session is null" << endl;
            return false;
        }

        mSession->sql(R"(
        CREATE TABLE IF NOT EXISTS users (
            user_id INT AUTO_INCREMENT PRIMARY KEY,
            login_id VARCHAR(32) NOT NULL UNIQUE,
            password_hash VARCHAR(255) NOT NULL,
            nickname VARCHAR(32) NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
        )").execute();

        return true;
    }
    catch (const mysqlx::Error& e)
    {
        cout << "[DB] CreateTables Error: " << e.what() << endl;
        return false;
    }
    return true;
}

bool DbManager::ClearTable()
{
    try
    {
        if (!mSession)
        {
            cout << "[DB] ClearTable Error: session is null" << endl;
            return false;
        }

        mSession->sql("DELETE FROM users").execute();
        return true;
    }
    catch (const mysqlx::Error& e)
    {
        cout << "[DB] ClearTable Error: " << e.what() << endl;
        return false;
    }
}

bool DbManager::Init(const string& host,
					 int port,
					 const string& user,
					 const string& password,
					 const string& schema)
{
	mSchema = schema;

    try
    {
        mSession = make_unique<mysqlx::Session>(host, port, user, password);

        // 스키마 선택
        mSession->sql("USE " + mSchema).execute();

        // 연결 테스트
        mSession->sql("SELECT 1").execute();

        return true;
    }
    catch (const mysqlx::Error& e)
    {
        std::cout << "[DB] Init mysqlx::Error: " << e.what() << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cout << "[DB] Init std::exception: " << e.what() << std::endl;
    }

    return false;
}

bool DbManager::IsConnected() const
{
    if (mSession == nullptr)
        return false;

    try
    {
        mSession->sql("SELECT 1").execute();
        return true;
    }
    catch (...)
    {
        return false;
    }
}

DbResult DbManager::RegisterUser(const string& loginId,
                                 const string& passwordHash,
                                 const string& nickname)
{
    if (!IsConnected())
        return DbResult::CONNECTION_ERROR;

    try
    {
        auto res = mSession->sql("SELECT 1 FROM users WHERE login_id=? LIMIT 1").bind(loginId).execute();

        auto row = res.fetchOne();
        if (row)
            return DbResult::DUPLICATE_ID;

        mSession->sql(
            "INSERT INTO users(login_id, password_hash, nickname) VALUES(?,?,?)"
        ).bind(loginId, passwordHash, nickname).execute();

        return DbResult::OK;
    }
    catch (const mysqlx::Error& e)
    {
        cout << "[DB] RegisterUser mysqlx::Error: " << e.what() << endl;
        return DbResult::QUERY_ERROR;
    }
    catch (...)
    {
        cout << "[DB] RegisterUser unknown error" << endl;
        return DbResult::QUERY_ERROR;
    }
}

DbResult DbManager::LoginUser(const string& loginId,
                          const string& inputPasswordHash,
                          UserRow& outUser)
{
    if (!IsConnected())
        return DbResult::CONNECTION_ERROR;

    UserRow user{};
    DbResult result = GetUserByLoginId(loginId, user);

    if (result != DbResult::OK)
        return result;

    if (user.passwordHash != inputPasswordHash)
        return DbResult::WRONG_PASSWORD;

    outUser = move(user);
    return DbResult::OK;
}

DbResult DbManager::GetUserByLoginId(const string& loginId,
                                     UserRow& outUser)
{
    if (!IsConnected())
        return DbResult::CONNECTION_ERROR;

    lock_guard<mutex> lock(mDbMutex);

    try
    {
        auto res = mSession->sql(R"(
            SELECT user_id, login_id, password_hash, nickname
            FROM users
            WHERE login_id=? LIMIT 1
        )"
        ).bind(loginId).execute();

        auto row = res.fetchOne();

        if (!row)
            return DbResult::USER_NOT_FOUND;

        outUser.id = static_cast<long long>(row[0]);
        outUser.loginId = string(row[1]);
        outUser.passwordHash = string(row[2]);
        outUser.nickname = string(row[3]);

        return DbResult::OK;
    }
    catch (const mysqlx::Error& e)
    {
        cout << "[DB] GetUserByLoginId mysqlx::Error: " << e.what() << endl;
        return DbResult::QUERY_ERROR;
    }
    catch (...)
    {
        cout << "[DB] GetUserByLoginId unknown error" << endl;
        return DbResult::QUERY_ERROR;
    }        
}
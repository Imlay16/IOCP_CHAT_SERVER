
#include "IOCPServer.h"

int main(int argc, char* argv[])
{
	const UINT16 SERVER_PORT = 11021;
	const UINT16 MAX_CLIENT = 100;

	RedisManager redis;
	timeval timeout = { 2, 0 };

	if (!redis.Init("127.0.0.1", 6379, timeout))
	{
		std::cerr << "Redis Connection failed" << std::endl;
		return -1;
	}

	cout << "Redis connected successfully" << endl;

	redis.RegisterUser("TestUser1", "testpw");
	redis.RegisterUser("TestUser2", "testpw");
	redis.RegisterUser("TestUser3", "testpw");
	redis.RegisterUser("TestUser4", "testpw");
	redis.RegisterUser("TestUser5", "testpw");

	cout << "Test accounts registered (password: testpw)" << endl;

	IOCPServer server(&redis);

	//소켓을 초기화
	server.InitSocket();

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	server.BindAndListen(SERVER_PORT);

	server.StartServer(MAX_CLIENT);

	printf("Press q or Q to quit\n");
	while (true)
	{
		string inputCmd;
		getline(cin, inputCmd);

		if (inputCmd == "q" || inputCmd == "Q")
		{
			break;
		}
	}

	return 0;
}
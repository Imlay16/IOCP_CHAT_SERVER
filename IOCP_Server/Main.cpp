
#include "IOCP_Server.h"
#include <string> 

int main(int argc, char* argv[])
{
	const UINT16 SERVER_PORT = 11021;
	const UINT16 MAX_CLIENT = 100;

	IOCP_SERVER server;

	//소켓을 초기화
	server.InitSocket();

	//소켓과 서버 주소를 연결하고 등록 시킨다.
	server.BindandListen(SERVER_PORT);

	server.StartServer(MAX_CLIENT);

	printf("아무 키나 누를 때까지 대기합니다\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "quit")
		{
			break;
		}
	}

	server.DestroyThread();
	return 0;
}
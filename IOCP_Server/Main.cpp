
#include "IOCPServer.h"

int main(int argc, char* argv[])
{
	const UINT16 SERVER_PORT = 11021;
	const UINT16 MAX_CLIENT = 100;

	IOCPServer server;

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
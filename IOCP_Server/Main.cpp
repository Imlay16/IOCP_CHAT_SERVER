
#include "IOCP_Server.h"
#include <string> 

int main(int argc, char* argv[])
{
	const UINT16 SERVER_PORT = 11021;
	const UINT16 MAX_CLIENT = 100;

	IOCP_SERVER server;

	//������ �ʱ�ȭ
	server.InitSocket();

	//���ϰ� ���� �ּҸ� �����ϰ� ��� ��Ų��.
	server.BindandListen(SERVER_PORT);

	server.StartServer(MAX_CLIENT);

	printf("Press q or Q to quit\n");
	while (true)
	{
		std::string inputCmd;
		std::getline(std::cin, inputCmd);

		if (inputCmd == "q" || inputCmd == "Q")
		{
			break;
		}
	}

	server.DestroyThread();
	return 0;
}
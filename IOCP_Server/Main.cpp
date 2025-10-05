
#include "IOCPServer.h"

int main(int argc, char* argv[])
{
	const UINT16 SERVER_PORT = 11021;
	const UINT16 MAX_CLIENT = 100;

	IOCPServer server;

	//������ �ʱ�ȭ
	server.InitSocket();

	//���ϰ� ���� �ּҸ� �����ϰ� ��� ��Ų��.
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
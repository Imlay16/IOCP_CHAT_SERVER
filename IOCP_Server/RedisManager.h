#include <hiredis.h>
#include <winsock2.h>

class RedisManager
{
private:
	

public:
	void init(const char* ip, int port, timeval timeout)
	{
		redisContext* context = redisConnectWithTimeout(ip, port, timeout);
		

	}
};


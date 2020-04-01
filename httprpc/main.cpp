#include "libhttp/httpserver.h"
#include <unistd.h>

int main(int argc,char*argv[])
{
	InitHTTPServer();
	StartHTTPServer();
	while(true)
	{
		sleep(1);
	}
	return true;
}

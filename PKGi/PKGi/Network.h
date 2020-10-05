#include <stdint.h>
#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>
#include <orbis/VideoOut.h>
#include <orbis/GnmDriver.h>

#include <proto-include.h>

#ifndef NETWORK_H
#define NETWORK_H

class Network
{
private:
	int poolID;
	int sslID;
	int httpID;
public:
	Network();
	void* GetRequest(const char* url, size_t* len);
};

#endif
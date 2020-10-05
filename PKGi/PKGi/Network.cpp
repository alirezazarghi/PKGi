#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <orbis/Net.h>
#include <orbis/Http.h>
#include <orbis/Ssl.h>

#include "App.h"
#include "Network.h"

Network::Network() {
	char poolName[] = "PKGiNetPool";
	poolID = sceNetPoolCreate(poolName, 4096, 0);
	sslID = sceSslInit(SSL_POOLSIZE * 10);
	httpID = sceHttpInit(poolID, sslID, LIBHTTP_POOLSIZE);
	printf("Network initialized\n");
}

void* Network::GetRequest(const char* url, size_t* len) {
	char templateName[] = "PKGiNetTmpl";

	char user_agent[50];
	snprintf(user_agent, 50, "PKGi/%i", APP_VER);

	int tmplId = sceHttpCreateTemplate(httpID, templateName, 1, 0);
	if (tmplId < 0) {
		printf("Network: Bad template id (0x%08x)\n", tmplId);
		return nullptr;
	}

	// Disable HTTPS Certificate check
	sceHttpsDisableOption(tmplId, 0x01);

	int connId = sceHttpCreateConnectionWithURL(tmplId, url, 1);
	if (connId < 0) {
		sceHttpDeleteTemplate(tmplId);
		printf("Network: Bad connection id (0x%08x)\n", connId);
		return nullptr;
	}

	int reqId = sceHttpCreateRequestWithURL(connId, ORBIS_METHOD_GET, url, 0);
	if (reqId < 0) {
		sceHttpDeleteTemplate(tmplId);
		sceHttpDeleteConnection(connId);
		printf("Network: Bad request id (0x%08x)\n", reqId);
		return nullptr;
	}

	int ret = sceHttpSendRequest(reqId, nullptr, 0);
	if (ret < 0) {
		printf("Network: Request send error (0x%08x)\n", ret);
		sceHttpDeleteRequest(reqId);
		sceHttpDeleteConnection(connId);
		sceHttpDeleteTemplate(tmplId);
		return nullptr;
	}

	int result;
	size_t lenght;
	ret = sceHttpGetResponseContentLength(reqId, &result, &lenght);
	if (ret < 0 || result != 0) {
		printf("Network: Content lenght error (ret: 0x%08x result: %i)\n", ret, result);
		sceHttpDeleteRequest(reqId);
		sceHttpDeleteConnection(connId);
		sceHttpDeleteTemplate(tmplId);
		return nullptr;
	}

	void* reponse = malloc(lenght);

	sceHttpReadData(reqId, reponse, (unsigned int)lenght);

	sceHttpDeleteRequest(reqId);
	sceHttpDeleteConnection(connId);
	sceHttpDeleteTemplate(tmplId);

	*len = lenght;

	return reponse;
}
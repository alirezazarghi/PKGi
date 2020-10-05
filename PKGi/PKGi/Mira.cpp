#include "Mira.h"
#include "Syscall.h"

#include "fcntl.h"
#include <orbis/libkernel.h>

Mira::Mira() {
	mira_dev = sceKernelOpen("/dev/mira", 0, O_RDWR);
	if (mira_dev < 0) {
		printf("ERROR: Mira device is not available (%i)\n", mira_dev);
	}
}

void Mira::MountInSandbox(const char* path, int permission) {
	MiraMountInSandbox arguments;
	strncpy(arguments.Path, path, _MAX_PATH);
	arguments.Permissions = (int32_t)permission;

	syscall(54, MIRA_MOUNT_IN_SANDBOX, &arguments);
}

void Mira::ChangeAuthID(SceAuthenticationId_t authid) {
	MiraThreadCredentials params;
	params.State = GSState::Get;
	syscall(54, MIRA_GET_PROC_THREAD_CREDENTIALS, &params);

	params.State = GSState::Set;
	params.SceAuthId = authid;
	syscall(54, MIRA_GET_PROC_THREAD_CREDENTIALS, &params);
}

bool Mira::isAvailable() {
	if (mira_dev < 0)
		return false;

	return true;
}

Mira::~Mira() {
	sceKernelClose(mira_dev);
}


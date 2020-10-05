#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "Utility.h"

// Copy file
int Utility::CopyFile(char* source, char* destination) {
	printf("Copying %s to %s ...\n", source, destination);

	int fd_src = sceKernelOpen(source, 0x0000, 0777);
	int fd_dst = sceKernelOpen(destination, 0x0001 | 0x0200 | 0x0400, 0777);
	if (fd_src < 0 || fd_dst < 0)
		return 1;

	int data_size = sceKernelLseek(fd_src, 0, SEEK_END);
	sceKernelLseek(fd_src, 0, SEEK_SET);

	void* data = NULL;

	sceKernelMmap(0, data_size, PROT_READ, MAP_PRIVATE, fd_src, 0, &data);
	sceKernelWrite(fd_dst, data, data_size);
	sceKernelMunmap(data, data_size);

	sceKernelClose(fd_dst);
	sceKernelClose(fd_src);

	return 0;
}
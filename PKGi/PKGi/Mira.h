#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "sparse.h"

#ifndef MIRA_H
#define MIRA_H

// Needed for IOCTL Calculation
#define _IOC_NRBITS        8
#define _IOC_TYPEBITS        8
#define _IOC_SIZEBITS        14
#define _IOC_DIRBITS        2
#define _IOC_NRMASK        ((1 << _IOC_NRBITS)-1)
#define _IOC_TYPEMASK        ((1 << _IOC_TYPEBITS)-1)
#define _IOC_SIZEMASK        ((1 << _IOC_SIZEBITS)-1)
#define _IOC_DIRMASK        ((1 << _IOC_DIRBITS)-1)
#define _IOC_NRSHIFT        0
#define _IOC_TYPESHIFT        (_IOC_NRSHIFT+_IOC_NRBITS)
#define _IOC_SIZESHIFT        (_IOC_TYPESHIFT+_IOC_TYPEBITS)
#define _IOC_DIRSHIFT        (_IOC_SIZESHIFT+_IOC_SIZEBITS)
#define _IOC_NONE        0U
#define _IOC_WRITE        1U
#define _IOC_READ        2U
#define IOC_IN                (_IOC_WRITE << _IOC_DIRSHIFT)
#define IOC_OUT                (_IOC_READ << _IOC_DIRSHIFT)
#define IOC_INOUT        ((_IOC_WRITE|_IOC_READ) << _IOC_DIRSHIFT)
#define IOCSIZE_MASK        (_IOC_SIZEMASK << _IOC_SIZESHIFT)
#define IOCSIZE_SHIFT        (_IOC_SIZESHIFT)

#define _IOC(dir,type,nr,size) \
        (((dir)  << _IOC_DIRSHIFT) | \
         ((type) << _IOC_TYPESHIFT) | \
         ((nr)   << _IOC_NRSHIFT) | \
         ((size) << _IOC_SIZESHIFT))

#define COMM_LEN 19 + 13
#define NAME_LEN 19 + 17
#define _MAX_PATH 1024

typedef enum SceAuthenticationId_t : uint64_t
{
	SceVdecProxy = 0x3800000000000003ULL,
	SceVencProxy = 0x3800000000000004ULL,
	Orbis_audiod = 0x3800000000000005ULL,
	Coredump = 0x3800000000000006ULL,
	SceSysCore = 0x3800000000000007ULL,
	Orbis_setip = 0x3800000000000008ULL,
	GnmCompositor = 0x3800000000000009ULL,
	SceShellUI = 0x380000000000000fULL, // NPXS20001
	SceShellCore = 0x3800000000000010ULL,
	NPXS20103 = 0x3800000000000011ULL,
	NPXS21000 = 0x3800000000000012ULL,
	OrbisSWU = 0x3801000000000024ULL,
	Decid = 0x3800000000010003,
} SceAuthenticationId;

typedef enum SceCapabilities_t : uint64_t
{
	Max = 0xFFFFFFFFFFFFFFFFULL,
} SceCapabilites;

typedef enum class _State : uint32_t
{
	Get,
	Set,
	COUNT
} GSState;


// "safe" way in order to modify kernel ucred externally
typedef struct _MiraThreadCredentials {
	typedef enum class _MiraThreadCredentialsPrison : uint32_t
	{
		// Non-root prison
		Default,

		// Switch prison to root vnode
		Root,

		// Total options count
		COUNT
	} MiraThreadCredentialsPrison;

	// Is this a get or set operation
	GSState State;

	// Process ID to modify
	int32_t ProcessId;

	// Threaad ID to modify, or -1 for (all threads in process, USE WITH CAUTION)
	int32_t ThreadId;

	// Everything below is Get/Set
	int32_t EffectiveUserId;
	int32_t RealUserId;
	int32_t SavedUserId;
	int32_t NumGroups;
	int32_t RealGroupId;
	int32_t SavedGroupId;
	MiraThreadCredentialsPrison Prison;
	SceAuthenticationId SceAuthId;
	SceCapabilites Capabilities[4];
	uint64_t Attributes[4];
} MiraThreadCredentials;

#define MIRA_IOCTL_BASE 'M'

typedef struct _MiraMountInSandbox
{
	int32_t Permissions;
	char Path[_MAX_PATH];
} MiraMountInSandbox;

#define MIRA_MOUNT_IN_SANDBOX _IOC(IOC_IN, MIRA_IOCTL_BASE, 4, sizeof(MiraMountInSandbox))
#define MIRA_GET_PROC_THREAD_CREDENTIALS _IOC(IOC_INOUT, MIRA_IOCTL_BASE, 1, sizeof(MiraThreadCredentials))

class Mira {
private:
	int mira_dev;
public:
	Mira();
	~Mira();
	bool isAvailable();
	void MountInSandbox(const char* path, int permission);
	void ChangeAuthID(SceAuthenticationId_t authid);
};

#endif
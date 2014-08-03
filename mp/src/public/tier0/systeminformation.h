//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#ifndef SYSTEMINFORMATION_H
#define SYSTEMINFORMATION_H

#ifdef _WIN32
	#pragma once
#endif

#ifndef PLATFORM_INTERFACE
	#define PLATFORM_INTERFACE
#endif

//
//	Defines a possible outcome of a system call
//
enum SYSTEM_CALL_RESULT_t
{
	SYSCALL_SUCCESS		= 0,	// System call succeeded
	SYSCALL_FAILED		= 1,	// System call failed
	SYSCALL_NOPROC		= 2,	// Failed to find required system procedure
	SYSCALL_NODLL		= 3,	// Failed to find or load required system module
	SYSCALL_UNSUPPORTED	= 4,	// System call unsupported on the OS
};


//
//	Information about paged pool memory
//
struct PAGED_POOL_INFO_t
{
	unsigned long numPagesUsed;		// Number of Paged Pool pages used
	unsigned long numPagesFree;		// Number of Paged Pool pages free
};

//
//	Plat_GetMemPageSize
//		Returns the size of a memory page in kilobytes.
//
PLATFORM_INTERFACE unsigned long Plat_GetMemPageSize();

//
//	Plat_GetPagedPoolInfo
//		Fills in the paged pool info structure if successful.
//
PLATFORM_INTERFACE SYSTEM_CALL_RESULT_t Plat_GetPagedPoolInfo( PAGED_POOL_INFO_t *pPPI );



#endif // #ifndef SYSTEMINFORMATION_H

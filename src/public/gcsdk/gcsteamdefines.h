//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a bunch of stuff that would be defined in Steam, but
//			isn't in Source.
//
//=============================================================================

#ifndef GCSTEAMDEFINES_H
#define GCSTEAMDEFINES_H
#ifdef _WIN32
#pragma once
#endif

#include "tier0/memalloc.h"

// steam defines some things that games don't
#ifndef STEAM
#define PvAlloc(x) malloc(x)
#define PvRealloc(x, y) realloc(x, y)
#define FreePv(x) free(x)

// auto-lock class for read-write locks
template< class T >
class CRWLockAutoWrite
{
	T &m_RWLock;
public:
	CRWLockAutoWrite( T &RWLock ) : m_RWLock( RWLock )
	{
		m_RWLock.LockForWrite();
	}

	~CRWLockAutoWrite()
	{
		m_RWLock.UnlockWrite();
	}
};

#define AUTO_LOCK_WRITE( mutex ) CRWLockAutoWrite<CThreadRWLock> UNIQUE_ID( mutex )
#define AUTO_LOCK_SPIN_WRITE( mutex ) CRWLockAutoWrite<CThreadSpinRWLock> UNIQUE_ID( mutex )

inline void *MemAlloc_AllocAligned( size_t size, size_t align, bool bCanFail ) { return MemAlloc_AllocAligned( size, align ); }
inline void MemAlloc_FreeAligned( void *pMemBlock, bool bOperatorNew ) { MemAlloc_FreeAligned( pMemBlock ); }

#endif


#endif // GCSTEAMDEFINES_H

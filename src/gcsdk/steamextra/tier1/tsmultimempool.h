//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#ifndef TSMULTIMEMPOOL_H
#define TSMULTIMEMPOOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlmap.h"
#include "tier1/mempool.h"
#include "tier1/tsmempool.h"

//-----------------------------------------------------------------------------
// Purpose: A container of a range of mem pool sizes (for network buffers for example) 
//			and a raw alloc capability (for sizes greater than any contained mem pool).
//-----------------------------------------------------------------------------
class CThreadSafeMultiMemoryPool
{
public:
	struct MemPoolConfig_t
	{
		uint32 m_cubBlockSize;
		uint32 m_cubDefaultPoolSize;
	};

	CThreadSafeMultiMemoryPool( const MemPoolConfig_t *pnBlock, int cnMemPoolConfig, int nGrowMode = UTLMEMORYPOOL_GROW_FAST );
	~CThreadSafeMultiMemoryPool();

	// Allocate a block of at least nAllocSize bytes
	void*		Alloc( uint32 cubAlloc );
	// Free a previously alloc'd block
	void		Free(void *pvMem);
	// ReAllocate a previously allocated block to a new size
	void*		ReAlloc( void *pvMem, uint32 cubAlloc );
	
	// Frees everything
	void		Clear();

	// alloc size for this bit of alloc'd memory
	int CubAllocSize( void *pvMem );
	// prints details about our contained memory
	void PrintStats();

	// total number of alloc'd elements
	int Count();
	
	// Return the total size in MB allocated for this pool
	int CMBPoolSize() const;
	// Return the amount of memory in use
	int CMBPoolSizeInUse() const;

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

private:
	struct MemPoolRecord_t
	{
		CThreadSafeMemoryPool *m_pMemPool;
		uint32 m_nBlockSize;
	};

	CUtlVector<MemPoolRecord_t> m_VecMemPool; // stores our list of mem pools

	uint32 m_nBlockSizeMax;
	CUtlVector<MemPoolRecord_t *> m_VecMemPoolLookup; // quick lookup table of mempools

	struct RawAllocation_t
	{
		void *m_pvMem;
		uint32 m_nBlockSize;
	};
	CUtlMap<void *,RawAllocation_t,int> m_MapRawAllocation;  // stores our list of raw alloc'd mem
	CThreadFastMutex m_mutexRawAllocations;

	uint32 m_cubReallocedTotal;
};


#endif // TSMULTIMEMPOOL_H

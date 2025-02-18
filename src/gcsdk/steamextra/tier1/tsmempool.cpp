//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//=============================================================================


//#include "pch_vstdlib.h"

#include "stdafx.h"
#include "tier0/tslist.h"
#include "tier0/t0constants.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const uint k_cubBytesAllocatedToConsiderFreeingMemory = 5 * k_nMegabyte;
static const int k_cBlocksAllocatedToConsiderFreeingMemory = 10;

typedef TSLNodeBase_t FreeListItem_t;

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CThreadSafeMemoryPool::CThreadSafeMemoryPool( int blockSize, int numElements, int growMode )
{
	m_ptslistFreeBlocks = new CTSListBase;

	// round up to the nearest 8-byte boundary
	if ( blockSize % TSLIST_NODE_ALIGNMENT != 0 )
	{
		blockSize += TSLIST_NODE_ALIGNMENT - (blockSize % TSLIST_NODE_ALIGNMENT);
	}
	Assert( blockSize % TSLIST_NODE_ALIGNMENT == 0 );
	Assert( blockSize > sizeof(FreeListItem_t) );
	m_nGrowMode = growMode;
	m_cubBlockSize = blockSize;
	m_nGrowSize = numElements;
	m_cubAllocated = 0;
}


//-----------------------------------------------------------------------------
// Purpose: Frees the memory contained in the mempool
//-----------------------------------------------------------------------------
CThreadSafeMemoryPool::~CThreadSafeMemoryPool()
{
	AUTO_LOCK_SPIN_WRITE( m_threadRWLock );
	FOR_EACH_VEC( m_vecBlockSets, i )
	{
		_aligned_free( m_vecBlockSets[i].m_pubBlockSet );
	}

	delete m_ptslistFreeBlocks;
}


//-----------------------------------------------------------------------------
// Purpose: Frees everything
//-----------------------------------------------------------------------------
void CThreadSafeMemoryPool::Clear()
{
	AUTO_LOCK_SPIN_WRITE( m_threadRWLock );
	ClearNoLock();
}


//-----------------------------------------------------------------------------
// Purpose: Frees everything
//-----------------------------------------------------------------------------
void CThreadSafeMemoryPool::ClearNoLock()
{
	FOR_EACH_VEC( m_vecBlockSets, i )
	{
		_aligned_free( m_vecBlockSets[i].m_pubBlockSet );
	}
	m_ptslistFreeBlocks->Detach();
	m_cubAllocated = 0;
	m_cBlocksInUse = 0;
	m_vecBlockSets.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: Allocates a single block of memory from the pool.  
//-----------------------------------------------------------------------------
void *CThreadSafeMemoryPool::Alloc()
{
	return Alloc( m_cubBlockSize );
}


//-----------------------------------------------------------------------------
// Purpose: Allocates a single block of memory from the pool.  
//-----------------------------------------------------------------------------
void *CThreadSafeMemoryPool::Alloc( unsigned int amount )
{
	// loop attempting to get memory
	// there appears to be a case where memory corruption can get this into an infinite loop
	// normally 1 or 2 attempts are necessary to get a block, so if we hit 1000 we know something is wrong
	int cAttempts = 1000;
	while ( --cAttempts )
	{
		// pull first from the free list
		m_threadRWLock.LockForRead();
		FreeListItem_t *pFreeListItem = m_ptslistFreeBlocks->Pop();
		if ( pFreeListItem )
		{
			m_threadRWLock.UnlockRead();
			m_cBlocksInUse++;
			return (void *)pFreeListItem;
		}
		m_threadRWLock.UnlockRead();

		// no free items, add a new block

		// switch from a read lock to a write lock
		AUTO_LOCK_SPIN_WRITE( m_threadRWLock );

		// another thread may have allocated memory; try the free list again if so
		if ( m_ptslistFreeBlocks->Count() > 0 )
			continue;

		size_t cubBlob = m_nGrowSize * m_cubBlockSize;
		if ( m_nGrowMode == GROW_FAST )
		{
			cubBlob *= (m_vecBlockSets.Count() + 1);
		}

		// don't grow if we're told not to
		if ( m_nGrowMode == GROW_NONE && m_vecBlockSets.Count() == 1 )
			return NULL;

		// allocate, but we can fail
		byte *pBlobBase = (byte *)MemAlloc_AllocAligned( cubBlob, TSLIST_NODE_ALIGNMENT /*, (m_nGrowMode == GROW_TIL_YOU_CANT)*/ );
		if ( !pBlobBase )
			return NULL;

		byte *pBlobEnd = pBlobBase + cubBlob;
		// add all the items to the pool
		for ( byte *pBlob = pBlobBase; pBlob < pBlobEnd; pBlob += m_cubBlockSize )
		{
			m_ptslistFreeBlocks->Push( (FreeListItem_t *)pBlob );
		}

		m_cubAllocated += cubBlob;
		BlockSet_t blockSet = { pBlobBase, cubBlob };
		m_vecBlockSets.AddToTail( blockSet );
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Frees a block of memory
//-----------------------------------------------------------------------------
void CThreadSafeMemoryPool::Free( void *pMem )
{
	Free( pMem, m_cubBlockSize );
}


//-----------------------------------------------------------------------------
// Purpose: Frees a block of memory
//-----------------------------------------------------------------------------
void CThreadSafeMemoryPool::Free( void *pMem, int cubAlloc )
{
	m_threadRWLock.LockForRead();

	// push the item back onto the free list
	m_ptslistFreeBlocks->Push( (FreeListItem_t *)pMem );
	m_cBlocksInUse--;

	m_threadRWLock.UnlockRead();

	// if we're completely free, and have too much memory allocated, free some
	if ( m_cBlocksInUse == 0 
		&& m_cubAllocated >= k_cubBytesAllocatedToConsiderFreeingMemory 
		&& m_vecBlockSets.Count() >= k_cBlocksAllocatedToConsiderFreeingMemory )
	{
		AUTO_LOCK_SPIN_WRITE( m_threadRWLock );

		// check again nothing is in use
		if ( m_cBlocksInUse == 0 )
		{
			// free all the blocks
			ClearNoLock();
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: display
//-----------------------------------------------------------------------------
void CThreadSafeMemoryPool::PrintStats()
{
	AUTO_LOCK_SPIN_WRITE( m_threadRWLock );
	int cBlocksInUse = m_cBlocksInUse;
	Msg( "Block size: %-11s Alloc'd: %8d Num blobs: %5d (%s)\n", Q_pretifymem( m_cubBlockSize, 2, true ),
		 cBlocksInUse, m_vecBlockSets.Count(), Q_pretifymem( m_cubAllocated, 2, true ) );
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
size_t CThreadSafeMemoryPool::CubTotalSize()
{
	return m_cubAllocated;
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
size_t CThreadSafeMemoryPool::CubSizeInUse()
{
	return m_cBlocksInUse * m_cubBlockSize;
}


//-----------------------------------------------------------------------------
// Purpose: data accessor
//-----------------------------------------------------------------------------
int CThreadSafeMemoryPool::Count()
{
	return m_cBlocksInUse;
}


#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: Run a global validation pass on all of our data structures and memory
//			allocations.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
void CThreadSafeMemoryPool::Validate( CValidator &validator, const char *pchName )
{
	AUTO_LOCK_SPIN_WRITE( m_threadRWLock );
	VALIDATE_SCOPE();
	FOR_EACH_VEC( m_vecBlockSets, i )
	{
		validator.ClaimMemory( MemAlloc_Unalign( m_vecBlockSets[i].m_pubBlockSet ) );
	}
	ValidateObj( m_vecBlockSets );
	validator.ClaimMemory( MemAlloc_Unalign( m_ptslistFreeBlocks ) );
}
#endif // DBGFLAG_VALIDATE

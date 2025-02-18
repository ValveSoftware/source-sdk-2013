//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//

#include <stdafx.h>
#include "tier0/t0constants.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

static const int k_cubMemBlockPrefixSize = sizeof(uint32);

#define ALLOCSIZE_TO_LOOKUP( cubAlloc ) ( (cubAlloc - 1) >> 5 )
#define LOOKUP_TO_ALLOCSIZE( iLookup ) ( (iLookup << 5) + 1 )


//-----------------------------------------------------------------------------
// Purpose: constructor, the sizes in pMemPoolConfig must be in ascending order
//-----------------------------------------------------------------------------
CThreadSafeMultiMemoryPool::CThreadSafeMultiMemoryPool( const MemPoolConfig_t *pMemPoolConfig, int cnMemPoolConfig, int nGrowMode /*= GROW_FAST*/ )
{
	m_cubReallocedTotal = 0;
	m_MapRawAllocation.SetLessFunc( DefLessFunc( void * ) );

	for ( int iMemPoolConfig = 0; iMemPoolConfig < cnMemPoolConfig; iMemPoolConfig++ )
	{
		MemPoolRecord_t memPoolRecord;
		// verify that the mem pool sizes are in ascending order
		Assert( iMemPoolConfig == 0 || ( iMemPoolConfig > 0 && pMemPoolConfig[ iMemPoolConfig - 1 ].m_cubBlockSize < pMemPoolConfig[ iMemPoolConfig].m_cubBlockSize ) );
		AssertMsg( pMemPoolConfig[ iMemPoolConfig].m_cubBlockSize % 32 == 0, "Mempools sizes must be multiples of 32" );
		// add an int to the block size so we can note the alloc size
		memPoolRecord.m_pMemPool = new CThreadSafeMemoryPool( pMemPoolConfig[ iMemPoolConfig ].m_cubBlockSize + k_cubMemBlockPrefixSize, 
															pMemPoolConfig[ iMemPoolConfig ].m_cubDefaultPoolSize, nGrowMode );
		Assert( memPoolRecord.m_pMemPool );
		memPoolRecord.m_nBlockSize = pMemPoolConfig[ iMemPoolConfig ].m_cubBlockSize;
		m_VecMemPool.AddToTail( memPoolRecord );

		// update the largest blocksize
		m_nBlockSizeMax = MAX( m_nBlockSizeMax, memPoolRecord.m_nBlockSize );
	}

	// build the lookup table
	int nLookupMax = m_nBlockSizeMax >> 5;
	m_VecMemPoolLookup.AddMultipleToTail( nLookupMax );
	for ( int i = 0; i < nLookupMax; i++ )
	{
		uint32 cubAllocSize = LOOKUP_TO_ALLOCSIZE( i );
		for ( int iMemPool = 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
		{
			if ( m_VecMemPool[iMemPool].m_nBlockSize >= cubAllocSize )
			{
				m_VecMemPoolLookup[i] = &m_VecMemPool[iMemPool];
				break;
			}
		}
	}

#if defined(_DEBUG)
	// validate the lookup table
	for ( int i = 1; i < (int)m_nBlockSizeMax; i++ )
	{
		for ( int iMemPool = 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
		{
			if ( (int)m_VecMemPool[iMemPool].m_nBlockSize >= i )
			{
				AssertMsg( m_VecMemPoolLookup[ALLOCSIZE_TO_LOOKUP(i)] == &m_VecMemPool[iMemPool], "Invalid mempool block size, can't generate lookup table" );
				break;
			}
		}
	}
#endif // _DEBUG
}


//-----------------------------------------------------------------------------
// Purpose: destructor
//-----------------------------------------------------------------------------
CThreadSafeMultiMemoryPool::~CThreadSafeMultiMemoryPool()
{
	AUTO_LOCK( m_mutexRawAllocations );

	for ( int iMemPool = 0; iMemPool < m_VecMemPool.Count(); iMemPool ++ )
	{
		delete m_VecMemPool[iMemPool].m_pMemPool;
	}

	FOR_EACH_MAP_FAST( m_MapRawAllocation, iRawAllocation )
	{
		FreePv( m_MapRawAllocation[iRawAllocation].m_pvMem );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Allocates a block of memory at of least nAllocSize bytes
// Input  : nAllocSize - number of bytes to alloc
// Output : pointer to memory alloc'd, NULL on error
//-----------------------------------------------------------------------------
void *CThreadSafeMultiMemoryPool::Alloc( uint32 cubAllocSize )
{
	if ( cubAllocSize == 0 )
		return NULL;

	if ( cubAllocSize <= m_nBlockSizeMax )
	{
		MemPoolRecord_t *pMemPoolRecord = m_VecMemPoolLookup[ALLOCSIZE_TO_LOOKUP( cubAllocSize )];
		void *pvMem = pMemPoolRecord->m_pMemPool->Alloc( cubAllocSize + k_cubMemBlockPrefixSize );
		*(uint32 *)pvMem = cubAllocSize;
		return ( (char *)pvMem + k_cubMemBlockPrefixSize );
	}


	// can't fit in our mem pools, alloc it in our one off buffer
	RawAllocation_t rawAllocation;
	rawAllocation.m_nBlockSize = cubAllocSize;
	rawAllocation.m_pvMem = PvAlloc( cubAllocSize + k_cubMemBlockPrefixSize );
	if ( !rawAllocation.m_pvMem )
	{
		return NULL;
	}
	*(uint32 *)rawAllocation.m_pvMem = rawAllocation.m_nBlockSize;
	AUTO_LOCK( m_mutexRawAllocations );
	m_MapRawAllocation.Insert( rawAllocation.m_pvMem, rawAllocation );
	return ( (char *)rawAllocation.m_pvMem + k_cubMemBlockPrefixSize );
}


//-----------------------------------------------------------------------------
// Purpose: Free a previously alloc'd block
// Input  : pMem - memory to free
//-----------------------------------------------------------------------------
void CThreadSafeMultiMemoryPool::Free( void *pvMem )
{
	if ( !pvMem )
		return;

	uint32 cubAllocSize = *( (uint32 *)pvMem - 1 );

	if ( cubAllocSize <= m_nBlockSizeMax )
	{
		MemPoolRecord_t *pMemPoolRecord = m_VecMemPoolLookup[ALLOCSIZE_TO_LOOKUP( cubAllocSize )];
		pMemPoolRecord->m_pMemPool->Free( (char *)pvMem - k_cubMemBlockPrefixSize, cubAllocSize + k_cubMemBlockPrefixSize );
		return;
	}

	AUTO_LOCK( m_mutexRawAllocations );

	// must have been alloc'd from the raw heap, find it in map
	void *pvAllocedMem = (char *)pvMem - k_cubMemBlockPrefixSize;
	int iRawAllocation = m_MapRawAllocation.Find( pvAllocedMem );
	if ( m_MapRawAllocation.InvalidIndex() == iRawAllocation )
	{
		AssertMsg3( false, "CThreadSafeMultiMemoryPool::Free: raw allocation %p (original alloc: %p, %d bytes) not found in allocation map",
			pvMem, pvAllocedMem, cubAllocSize );
		return;

	}

	FreePv( m_MapRawAllocation[iRawAllocation].m_pvMem );
	m_MapRawAllocation.RemoveAt( iRawAllocation);
}


//-----------------------------------------------------------------------------
// Purpose: Return the size alloc'd for this block
// Input  : pMem - memory to report
// Output : size in bytes of this memory
//-----------------------------------------------------------------------------
int CThreadSafeMultiMemoryPool::CubAllocSize(void *pvMem)
{
	if ( !pvMem )
	{
		return -1;
	}

	return *(((uint32 *)pvMem) -1);
}
	

//-----------------------------------------------------------------------------
// Purpose: Frees all previously alloc'd memory
//-----------------------------------------------------------------------------
void CThreadSafeMultiMemoryPool::Clear()
{
	AUTO_LOCK( m_mutexRawAllocations );

	for ( int iMemPool = 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
	{
		m_VecMemPool[iMemPool].m_pMemPool->Clear();
	}

	FOR_EACH_MAP_FAST( m_MapRawAllocation, iRawAllocation )
	{
		FreePv( m_MapRawAllocation[iRawAllocation].m_pvMem );
	}
	m_MapRawAllocation.RemoveAll();
}


//-----------------------------------------------------------------------------
// Purpose: print to the console info about our storage
//-----------------------------------------------------------------------------
void CThreadSafeMultiMemoryPool::PrintStats()
{
	for ( int iMemPool= 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
	{
		m_VecMemPool[iMemPool].m_pMemPool->PrintStats();
	}
	int cubRawBytesAllocd = 0;
	AUTO_LOCK( m_mutexRawAllocations );
	FOR_EACH_MAP_FAST( m_MapRawAllocation, iRawAllocation )
	{
		cubRawBytesAllocd += m_MapRawAllocation[iRawAllocation].m_nBlockSize;
	}
	Msg( "Raw bytes alloc'd: %s\n", Q_pretifymem( cubRawBytesAllocd, 2, true ) );
	Msg( "Cumulative bytes re-alloced: %s\n", Q_pretifymem( m_cubReallocedTotal, 2, true ) );
}


//-----------------------------------------------------------------------------
// Purpose: return the total mem alloced by this pool in MB
//-----------------------------------------------------------------------------
int CThreadSafeMultiMemoryPool::CMBPoolSize() const
{
	uint64 cubRawBytesAllocd = 0;
	for ( int iMemPool= 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
	{
		cubRawBytesAllocd += ( m_VecMemPool[iMemPool].m_pMemPool->CubTotalSize() );
	}
	AUTO_LOCK( m_mutexRawAllocations );
	FOR_EACH_MAP_FAST( m_MapRawAllocation, iRawAllocation )
	{
		cubRawBytesAllocd += m_MapRawAllocation[iRawAllocation].m_nBlockSize;
	}

	return ( cubRawBytesAllocd / k_nMegabyte );
}

//-----------------------------------------------------------------------------
// Purpose: return the total mem alloced by this pool in MB
//-----------------------------------------------------------------------------
int CThreadSafeMultiMemoryPool::CMBPoolSizeInUse() const
{
	uint64 cubRawBytesAllocd = 0;
	for ( int iMemPool= 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
	{
		cubRawBytesAllocd += ( m_VecMemPool[iMemPool].m_pMemPool->CubSizeInUse() );
	}
	AUTO_LOCK( m_mutexRawAllocations );
	FOR_EACH_MAP_FAST( m_MapRawAllocation, iRawAllocation )
	{
		cubRawBytesAllocd += m_MapRawAllocation[iRawAllocation].m_nBlockSize;
	}

	return ( cubRawBytesAllocd / k_nMegabyte );
}


//-----------------------------------------------------------------------------
// Purpose: return number of mempool blocks alloc'd
//-----------------------------------------------------------------------------
int CThreadSafeMultiMemoryPool::Count()
{
	int cCount = 0;
	for ( int iMemPool = 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
	{
		cCount += m_VecMemPool[iMemPool].m_pMemPool->Count();
	}
	return cCount;
}


//-----------------------------------------------------------------------------
// Purpose: reallocate an existing block of memory to a new size (and copy the data
//	Input:	pvMem - a pointer to the existing memory
//			cubAlloc - number of bytes to alloc
//  Output: returns a pointer to the memory allocated (NULL on error)
//-----------------------------------------------------------------------------
void *CThreadSafeMultiMemoryPool::ReAlloc( void *pvMem, uint32 cubAlloc )
{
	uint32 cubOldAlloc = CubAllocSize(pvMem);
	if ( pvMem && cubAlloc <= cubOldAlloc  )
		return pvMem;

	if ( cubOldAlloc > m_nBlockSizeMax )
	{
		AUTO_LOCK( m_mutexRawAllocations );
		// okay, must have been alloc'd from the raw heap, search for it
		void *pvAllocedMem = (char *)pvMem - k_cubMemBlockPrefixSize;
		int iRawAllocation = m_MapRawAllocation.Find( pvAllocedMem );
		if ( m_MapRawAllocation.InvalidIndex() == iRawAllocation )
		{
			AssertMsg3( false, "CThreadSafeMultiMemoryPool::ReAlloc: raw allocation %p (original alloc: %p, %d bytes) not found in allocation map",
				pvMem, pvAllocedMem, cubOldAlloc );
			return NULL;
		}

		// realloc the memory
		void *pvNewMem = PvRealloc( pvAllocedMem, cubAlloc + k_cubMemBlockPrefixSize );
		if ( !pvNewMem )
		{
			m_MapRawAllocation.RemoveAt( iRawAllocation );
			return NULL;
		}

		// update our tracking
		*(uint32 *)pvNewMem = cubAlloc;
		if ( pvAllocedMem == pvNewMem )
		{
			// if pointer is the same, use the same map entry with the same key (the pointer given to caller)
			m_MapRawAllocation[iRawAllocation].m_pvMem = pvNewMem;
			m_MapRawAllocation[iRawAllocation].m_nBlockSize = cubAlloc;
		}
		else
		{
			// if pointer changed, need to remove the old entry and re-insert with new key
			m_MapRawAllocation.RemoveAt( iRawAllocation );
			RawAllocation_t rawAllocation;
			rawAllocation.m_pvMem = pvNewMem;
			rawAllocation.m_nBlockSize = cubAlloc;
			m_MapRawAllocation.Insert( rawAllocation.m_pvMem, rawAllocation );
		}
		return ( (char *)pvNewMem + k_cubMemBlockPrefixSize );
	}
	else
	{
		// see if we can stay in the same block
		MemPoolRecord_t *pMemPoolRecord = m_VecMemPoolLookup[ALLOCSIZE_TO_LOOKUP( cubOldAlloc )];
		if ( cubAlloc <= pMemPoolRecord->m_nBlockSize )
		{
			// re-assign the size
			*((uint32 *)pvMem - 1) = cubAlloc;
			return pvMem;
		}

 		void *pvNewMem = Alloc( cubAlloc );
		if ( !pvNewMem )
		{
			return NULL;
		}
		m_cubReallocedTotal += cubOldAlloc;
		Q_memcpy( pvNewMem, pvMem, cubOldAlloc );
		Free( pvMem ); // now free the old memory buffer we had
		return pvNewMem;
	}
}


#ifdef DBGFLAG_VALIDATE
//-----------------------------------------------------------------------------
// Purpose: Ensure that all of our internal structures are consistent, and
//			account for all memory that we've allocated.
// Input:	validator -		Our global validator object
//			pchName -		Our name (typically a member var in our container)
//-----------------------------------------------------------------------------
void CThreadSafeMultiMemoryPool::Validate( CValidator &validator, const char *pchName )
{
	validator.Push( "CThreadSafeMultiMemoryPool", this, pchName );

	ValidateObj( m_VecMemPool );
	for( int iMemPool = 0; iMemPool < m_VecMemPool.Count(); iMemPool++ )
	{
		validator.ClaimMemory_Aligned( m_VecMemPool[iMemPool].m_pMemPool );
		m_VecMemPool[iMemPool].m_pMemPool->Validate( validator, "m_VecMemPool[iMemPool].m_pMemPool" );
	}

	AUTO_LOCK( m_mutexRawAllocations );
	ValidateObj( m_MapRawAllocation );
	FOR_EACH_MAP_FAST( m_MapRawAllocation, iRawAllocation )
	{
		validator.ClaimMemory( m_MapRawAllocation[iRawAllocation].m_pvMem );
	}

	ValidateObj( m_VecMemPoolLookup );

	validator.Pop();
}
#endif // DBGFLAG_VALIDATE


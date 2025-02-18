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

#ifndef TSMEMPOOL_H
#define TSMEMPOOL_H

#ifdef _WIN32
#pragma once
#endif

#undef new

//-----------------------------------------------------------------------------
// Purpose: Optimized pool memory allocator
//-----------------------------------------------------------------------------
class CThreadSafeMemoryPool
{
public:
	enum
	{
		GROW_NONE=0,		// Don't allow new blobs.
		GROW_FAST=1,		// New blob size is numElements * (i+1)  (ie: the blocks it allocates get larger and larger each time it allocates one).
		GROW_SLOW=2,		// New blob size is numElements.
		GROW_TIL_YOU_CANT=3	// GROW_SLOW til alloc fails - then STOP and dont assert!
	};

	CThreadSafeMemoryPool( int blockSize, int numElements, int growMode = GROW_FAST );
	~CThreadSafeMemoryPool();

	void *Alloc();	// Allocate the element size you specified in the constructor.
	void *Alloc( unsigned int cubAlloc );
	void Free( void *pMem );
	void Free( void *pMem, int cubAlloc );
	
	// Frees everything
	void Clear();

	// display
	void PrintStats();
	size_t CubTotalSize();
	size_t CubSizeInUse();
	int Count();

	static void * operator new( size_t size )
	{
		CThreadSafeMemoryPool *pNode = (CThreadSafeMemoryPool *)MemAlloc_AllocAligned( size, 8, __FILE__, __LINE__ 
#ifdef STEAM
			, true // new operator
#endif
			);
		return pNode;
	}
	
	static void * operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )
	{
		CThreadSafeMemoryPool *pNode = (CThreadSafeMemoryPool *)MemAlloc_AllocAligned( size, 8, pFileName, nLine
#ifdef STEAM
			, true // new operator
#endif
		);
		return pNode;
	}
	
	static void operator delete( void *p)
	{
		MemAlloc_FreeAligned( p
#ifdef STEAM
			, true // new operator
#endif
			);
	}
	
	static void operator delete( void *p, int nBlockUse, const char *pFileName, int nLine )
	{
		MemAlloc_FreeAligned( p
#ifdef STEAM
			, true // new operator
#endif
			);
	}
		
#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, const char *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

private:
	// These ain't gonna work
	static void * operator new[] ( size_t size );
	static void operator delete [] ( void *p);

	// CThreadSpinRWLock needs 8 byte alignment to work but we new() CThreadSafeMemoryPool 
	// so simply place it at the start of the class to make sure it fits on the 8-byte boundary
	CThreadSpinRWLock m_threadRWLock;

	int m_nGrowMode;
	int m_cubBlockSize;
	int m_nGrowSize;

	void ClearNoLock();

	CInterlockedInt m_cBlocksInUse;
	size_t m_cubAllocated;

	struct BlockSet_t
	{
		byte *m_pubBlockSet;
		size_t m_cubAllocated;
	};
	CUtlVector<BlockSet_t> m_vecBlockSets;

	class CTSListBase *m_ptslistFreeBlocks;
};


//-----------------------------------------------------------------------------
// Wrapper macro to make an allocator that returns particular typed allocations
// and construction and destruction of objects.
//-----------------------------------------------------------------------------
template< class T >
class CThreadSafeClassMemoryPool : public CThreadSafeMemoryPool
{
public:
	CThreadSafeClassMemoryPool(int numElements, int growMode = GROW_FAST)	:
	  CThreadSafeMemoryPool( sizeof(T), numElements, growMode ) {}

	  T*		Alloc();
	  void	Free( T *pMem );
};


template< class T >
T* CThreadSafeClassMemoryPool<T>::Alloc()
{
	T *pRet = (T*)CThreadSafeMemoryPool::Alloc();
	if ( pRet )
	{
		Construct( pRet );
	}
	return pRet;
}


template< class T >
void CThreadSafeClassMemoryPool<T>::Free(T *pMem)
{
	if ( pMem )
	{
		Destruct( pMem );
	}

	CThreadSafeMemoryPool::Free( pMem );
}


#endif // TSMEMPOOL_H

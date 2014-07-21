//========= Copyright Valve Corporation, All rights reserved. ============//
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
//===========================================================================//

#ifndef MEMPOOL_H
#define MEMPOOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/memalloc.h"
#include "tier0/tslist.h"
#include "tier0/platform.h"
#include "tier1/utlvector.h"
#include "tier1/utlrbtree.h"

//-----------------------------------------------------------------------------
// Purpose: Optimized pool memory allocator
//-----------------------------------------------------------------------------

typedef void (*MemoryPoolReportFunc_t)( PRINTF_FORMAT_STRING char const* pMsg, ... );

// Ways a memory pool can grow when it needs to make a new blob:
enum MemoryPoolGrowType_t
{
	UTLMEMORYPOOL_GROW_NONE=0,		// Don't allow new blobs.
	UTLMEMORYPOOL_GROW_FAST=1,		// New blob size is numElements * (i+1)  (ie: the blocks it allocates
									// get larger and larger each time it allocates one).
	UTLMEMORYPOOL_GROW_SLOW=2		// New blob size is numElements.
};

class CUtlMemoryPool
{
public:
	// !KLUDGE! For legacy code support, import the global enum into this scope
	enum MemoryPoolGrowType_t
	{
		GROW_NONE=UTLMEMORYPOOL_GROW_NONE,
		GROW_FAST=UTLMEMORYPOOL_GROW_FAST,
		GROW_SLOW=UTLMEMORYPOOL_GROW_SLOW
	};

				CUtlMemoryPool( int blockSize, int numElements, int growMode = UTLMEMORYPOOL_GROW_FAST, const char *pszAllocOwner = NULL, int nAlignment = 0 );
				~CUtlMemoryPool();

	void*		Alloc();	// Allocate the element size you specified in the constructor.
	void*		Alloc( size_t amount );
	void*		AllocZero();	// Allocate the element size you specified in the constructor, zero the memory before construction
	void*		AllocZero( size_t amount );
	void		Free(void *pMem);
	
	// Frees everything
	void		Clear();

	// Error reporting... 
	static void SetErrorReportFunc( MemoryPoolReportFunc_t func );

	// returns number of allocated blocks
	int Count() { return m_BlocksAllocated; }
	int PeakCount() { return m_PeakAlloc; }

protected:
	class CBlob
	{
	public:
		CBlob	*m_pPrev, *m_pNext;
		int		m_NumBytes;		// Number of bytes in this blob.
		char	m_Data[1];
		char	m_Padding[3]; // to int align the struct
	};

	// Resets the pool
	void		Init();
	void		AddNewBlob();
	void		ReportLeaks();

	int			m_BlockSize;
	int			m_BlocksPerBlob;

	int			m_GrowMode;	// GROW_ enum.

	// Put m_BlocksAllocated in front of m_pHeadOfFreeList for better
	// packing on 64-bit where pointers are 8-byte aligned.
	int				m_BlocksAllocated;
	// FIXME: Change m_ppMemBlob into a growable array?
	void			*m_pHeadOfFreeList;
	int				m_PeakAlloc;
	unsigned short	m_nAlignment;
	unsigned short	m_NumBlobs;
	const char *	m_pszAllocOwner;
	// CBlob could be not a multiple of 4 bytes so stuff it at the end here to keep us otherwise aligned
	CBlob			m_BlobHead;

	static MemoryPoolReportFunc_t g_ReportFunc;
};


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CMemoryPoolMT : public CUtlMemoryPool
{
public:
	CMemoryPoolMT(int blockSize, int numElements, int growMode = UTLMEMORYPOOL_GROW_FAST, const char *pszAllocOwner = NULL) : CUtlMemoryPool( blockSize, numElements, growMode, pszAllocOwner) {}


	void*		Alloc()	{ AUTO_LOCK( m_mutex ); return CUtlMemoryPool::Alloc(); }
	void*		Alloc( size_t amount )	{ AUTO_LOCK( m_mutex ); return CUtlMemoryPool::Alloc( amount ); }
	void*		AllocZero()	{ AUTO_LOCK( m_mutex ); return CUtlMemoryPool::AllocZero(); }	
	void*		AllocZero( size_t amount )	{ AUTO_LOCK( m_mutex ); return CUtlMemoryPool::AllocZero( amount ); }
	void		Free(void *pMem) { AUTO_LOCK( m_mutex ); CUtlMemoryPool::Free( pMem ); }

	// Frees everything
	void		Clear() { AUTO_LOCK( m_mutex ); return CUtlMemoryPool::Clear(); }
private:
	CThreadFastMutex m_mutex; // @TODO: Rework to use tslist (toml 7/6/2007)
};


//-----------------------------------------------------------------------------
// Wrapper macro to make an allocator that returns particular typed allocations
// and construction and destruction of objects.
//-----------------------------------------------------------------------------
template< class T >
class CClassMemoryPool : public CUtlMemoryPool
{
public:
	CClassMemoryPool(int numElements, int growMode = GROW_FAST, int nAlignment = 0 ) :
		CUtlMemoryPool( sizeof(T), numElements, growMode, MEM_ALLOC_CLASSNAME(T), nAlignment ) {
			#ifdef PLATFORM_64BITS 
				COMPILE_TIME_ASSERT( sizeof(CUtlMemoryPool) == 64 );
			#else
				COMPILE_TIME_ASSERT( sizeof(CUtlMemoryPool) == 48 );
			#endif
		}

	T*		Alloc();
	T*		AllocZero();
	void	Free( T *pMem );

	void	Clear();
};


//-----------------------------------------------------------------------------
// Specialized pool for aligned data management (e.g., Xbox cubemaps)
//-----------------------------------------------------------------------------
template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, int COMPACT_THRESHOLD = 4 >
class CAlignedMemPool
{
	enum
	{
		BLOCK_SIZE = ALIGN_VALUE( ITEM_SIZE, ALIGNMENT ) > 8 ? ALIGN_VALUE( ITEM_SIZE, ALIGNMENT ) : 8
	};

public:
	CAlignedMemPool();

	void *Alloc();
	void Free( void *p );

	static int __cdecl CompareChunk( void * const *ppLeft, void * const *ppRight );
	void Compact();

	int NumTotal()			{ return m_Chunks.Count() * ( CHUNK_SIZE / BLOCK_SIZE ); }
	int NumAllocated()		{ return NumTotal() - m_nFree; }
	int NumFree()			{ return m_nFree; }

	int BytesTotal()		{ return NumTotal() * BLOCK_SIZE; }
	int BytesAllocated()	{ return NumAllocated() * BLOCK_SIZE; }
	int BytesFree()			{ return NumFree() * BLOCK_SIZE; }

	int ItemSize()			{ return ITEM_SIZE; }
	int BlockSize()			{ return BLOCK_SIZE; }
	int ChunkSize()			{ return CHUNK_SIZE; }

private:
	struct FreeBlock_t
	{
		FreeBlock_t *pNext;
		byte		reserved[ BLOCK_SIZE - sizeof( FreeBlock_t *) ];
	};

	CUtlVector<void *>	m_Chunks;		// Chunks are tracked outside blocks (unlike CUtlMemoryPool) to simplify alignment issues
	FreeBlock_t *		m_pFirstFree;
	int					m_nFree;
	CAllocator			m_Allocator;
	float				m_TimeLastCompact;
};

//-----------------------------------------------------------------------------
// Pool variant using standard allocation
//-----------------------------------------------------------------------------
template <typename T, int nInitialCount = 0, bool bDefCreateNewIfEmpty = true >
class CObjectPool
{
public:
	CObjectPool()
	{
		int i = nInitialCount;
		while ( i-- > 0 )
		{
			m_AvailableObjects.PushItem( new T );
		}
	}

	~CObjectPool()
	{
		Purge();
	}

	int NumAvailable()
	{
		return m_AvailableObjects.Count();
	}

	void Purge()
	{
		T *p;
		while ( m_AvailableObjects.PopItem( &p ) )
		{
			delete p;
		}
	}

	T *GetObject( bool bCreateNewIfEmpty = bDefCreateNewIfEmpty )
	{
		T *p;
		if ( !m_AvailableObjects.PopItem( &p )  )
		{
			p = ( bCreateNewIfEmpty ) ? new T : NULL;
		}
		return p;
	}

	void PutObject( T *p )
	{
		m_AvailableObjects.PushItem( p );
	}

private:
	CTSList<T *> m_AvailableObjects;
};

//-----------------------------------------------------------------------------


template< class T >
inline T* CClassMemoryPool<T>::Alloc()
{
	T *pRet;

	{
	MEM_ALLOC_CREDIT_(MEM_ALLOC_CLASSNAME(T));
	pRet = (T*)CUtlMemoryPool::Alloc();
	}

	if ( pRet )
	{
		Construct( pRet );
	}
	return pRet;
}

template< class T >
inline T* CClassMemoryPool<T>::AllocZero()
{
	T *pRet;

	{
	MEM_ALLOC_CREDIT_(MEM_ALLOC_CLASSNAME(T));
	pRet = (T*)CUtlMemoryPool::AllocZero();
	}

	if ( pRet )
	{
		Construct( pRet );
	}
	return pRet;
}

template< class T >
inline void CClassMemoryPool<T>::Free(T *pMem)
{
	if ( pMem )
	{
		Destruct( pMem );
	}

	CUtlMemoryPool::Free( pMem );
}

template< class T >
inline void CClassMemoryPool<T>::Clear()
{
	CUtlRBTree<void *> freeBlocks;
	SetDefLessFunc( freeBlocks );

	void *pCurFree = m_pHeadOfFreeList;
	while ( pCurFree != NULL )
	{
		freeBlocks.Insert( pCurFree );
		pCurFree = *((void**)pCurFree);
	}

	for( CBlob *pCur=m_BlobHead.m_pNext; pCur != &m_BlobHead; pCur=pCur->m_pNext )
	{
		T *p = (T *)pCur->m_Data;
		T *pLimit = (T *)(pCur->m_Data + pCur->m_NumBytes);
		while ( p < pLimit )
		{
			if ( freeBlocks.Find( p ) == freeBlocks.InvalidIndex() )
			{
				Destruct( p );
			}
			p++;
		}
	}

	CUtlMemoryPool::Clear();
}


//-----------------------------------------------------------------------------
// Macros that make it simple to make a class use a fixed-size allocator
// Put DECLARE_FIXEDSIZE_ALLOCATOR in the private section of a class,
// Put DEFINE_FIXEDSIZE_ALLOCATOR in the CPP file
//-----------------------------------------------------------------------------
#define DECLARE_FIXEDSIZE_ALLOCATOR( _class )									\
	public:																		\
		inline void* operator new( size_t size ) { MEM_ALLOC_CREDIT_(#_class " pool"); return s_Allocator.Alloc(size); }   \
		inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine ) { MEM_ALLOC_CREDIT_(#_class " pool"); return s_Allocator.Alloc(size); }   \
		inline void  operator delete( void* p ) { s_Allocator.Free(p); }		\
		inline void  operator delete( void* p, int nBlockUse, const char *pFileName, int nLine ) { s_Allocator.Free(p); }   \
	private:																		\
		static   CUtlMemoryPool   s_Allocator
    
#define DEFINE_FIXEDSIZE_ALLOCATOR( _class, _initsize, _grow )					\
	CUtlMemoryPool   _class::s_Allocator(sizeof(_class), _initsize, _grow, #_class " pool")

#define DEFINE_FIXEDSIZE_ALLOCATOR_ALIGNED( _class, _initsize, _grow, _alignment )		\
	CUtlMemoryPool   _class::s_Allocator(sizeof(_class), _initsize, _grow, #_class " pool", _alignment )

#define DECLARE_FIXEDSIZE_ALLOCATOR_MT( _class )									\
	public:																		\
	   inline void* operator new( size_t size ) { MEM_ALLOC_CREDIT_(#_class " pool"); return s_Allocator.Alloc(size); }   \
	   inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine ) { MEM_ALLOC_CREDIT_(#_class " pool"); return s_Allocator.Alloc(size); }   \
	   inline void  operator delete( void* p ) { s_Allocator.Free(p); }		\
	   inline void  operator delete( void* p, int nBlockUse, const char *pFileName, int nLine ) { s_Allocator.Free(p); }   \
	private:																		\
		static   CMemoryPoolMT   s_Allocator

#define DEFINE_FIXEDSIZE_ALLOCATOR_MT( _class, _initsize, _grow )					\
	CMemoryPoolMT   _class::s_Allocator(sizeof(_class), _initsize, _grow, #_class " pool")

//-----------------------------------------------------------------------------
// Macros that make it simple to make a class use a fixed-size allocator
// This version allows us to use a memory pool which is externally defined...
// Put DECLARE_FIXEDSIZE_ALLOCATOR_EXTERNAL in the private section of a class,
// Put DEFINE_FIXEDSIZE_ALLOCATOR_EXTERNAL in the CPP file
//-----------------------------------------------------------------------------

#define DECLARE_FIXEDSIZE_ALLOCATOR_EXTERNAL( _class )							\
   public:																		\
      inline void* operator new( size_t size )  { MEM_ALLOC_CREDIT_(#_class " pool"); return s_pAllocator->Alloc(size); }   \
      inline void* operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )  { MEM_ALLOC_CREDIT_(#_class " pool"); return s_pAllocator->Alloc(size); }   \
      inline void  operator delete( void* p )   { s_pAllocator->Free(p); }		\
   private:																		\
      static   CUtlMemoryPool*   s_pAllocator

#define DEFINE_FIXEDSIZE_ALLOCATOR_EXTERNAL( _class, _allocator )				\
   CUtlMemoryPool*   _class::s_pAllocator = _allocator


template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, int COMPACT_THRESHOLD >
inline CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, COMPACT_THRESHOLD>::CAlignedMemPool()
  : m_pFirstFree( 0 ),
	m_nFree( 0 ),
	m_TimeLastCompact( 0 )
{
	COMPILE_TIME_ASSERT( sizeof( FreeBlock_t ) >= BLOCK_SIZE );
	COMPILE_TIME_ASSERT( ALIGN_VALUE( sizeof( FreeBlock_t ), ALIGNMENT ) == sizeof( FreeBlock_t ) );
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, int COMPACT_THRESHOLD >
inline void *CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, COMPACT_THRESHOLD>::Alloc()
{
	if ( !m_pFirstFree )
	{
		FreeBlock_t *pNew = (FreeBlock_t *)m_Allocator.Alloc( CHUNK_SIZE );
		Assert( (unsigned)pNew % ALIGNMENT == 0 );
		m_Chunks.AddToTail( pNew );
		m_nFree = CHUNK_SIZE / BLOCK_SIZE;
		m_pFirstFree = pNew;
		for ( int i = 0; i < m_nFree - 1; i++ )
		{
			pNew->pNext = pNew + 1;
			pNew++;
		}
		pNew->pNext = NULL;
	}

	void *p = m_pFirstFree;
	m_pFirstFree = m_pFirstFree->pNext;
	m_nFree--;

	return p;
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, int COMPACT_THRESHOLD >
inline void CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, COMPACT_THRESHOLD>::Free( void *p )
{
	// Insertion sort to encourage allocation clusters in chunks
	FreeBlock_t *pFree = ((FreeBlock_t *)p);
	FreeBlock_t *pCur = m_pFirstFree;
	FreeBlock_t *pPrev = NULL;

	while ( pCur && pFree > pCur )
	{
		pPrev = pCur;
		pCur = pCur->pNext;
	}

	pFree->pNext = pCur;

	if ( pPrev )
	{
		pPrev->pNext = pFree;
	}
	else
	{
		m_pFirstFree = pFree;
	}
	m_nFree++;

	if ( m_nFree >= ( CHUNK_SIZE / BLOCK_SIZE ) * COMPACT_THRESHOLD )
	{
		float time = Plat_FloatTime();
		float compactTime = ( m_nFree >= ( CHUNK_SIZE / BLOCK_SIZE ) * COMPACT_THRESHOLD * 4 ) ? 15.0 : 30.0;
		if ( m_TimeLastCompact > time || m_TimeLastCompact + compactTime < Plat_FloatTime() )
		{
			Compact();
			m_TimeLastCompact = time;
		}
	}
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, int COMPACT_THRESHOLD >
inline int __cdecl CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, COMPACT_THRESHOLD>::CompareChunk( void * const *ppLeft, void * const *ppRight )
{
	return ((unsigned)*ppLeft) - ((unsigned)*ppRight);
}

template <int ITEM_SIZE, int ALIGNMENT, int CHUNK_SIZE, class CAllocator, int COMPACT_THRESHOLD >
inline void CAlignedMemPool<ITEM_SIZE, ALIGNMENT, CHUNK_SIZE, CAllocator, COMPACT_THRESHOLD>::Compact()
{
	FreeBlock_t *pCur = m_pFirstFree;
	FreeBlock_t *pPrev = NULL;

	m_Chunks.Sort( CompareChunk );

#ifdef VALIDATE_ALIGNED_MEM_POOL
	{
		FreeBlock_t *p = m_pFirstFree;
		while ( p )
		{
			if ( p->pNext && p > p->pNext )
			{
				__asm { int 3 }
			}
			p = p->pNext;
		}

		for ( int i = 0; i < m_Chunks.Count(); i++ )
		{
			if ( i + 1 < m_Chunks.Count() )
			{
				if ( m_Chunks[i] > m_Chunks[i + 1] )
				{
					__asm { int 3 }
				}
			}
		}
	}
#endif

	int i;

	for ( i = 0; i < m_Chunks.Count(); i++ )
	{
		int nBlocksPerChunk = CHUNK_SIZE / BLOCK_SIZE;
		FreeBlock_t *pChunkLimit = ((FreeBlock_t *)m_Chunks[i]) + nBlocksPerChunk;
		int nFromChunk = 0;
		if ( pCur == m_Chunks[i] )
		{
			FreeBlock_t *pFirst = pCur;
			while ( pCur && pCur >= m_Chunks[i] && pCur < pChunkLimit )
			{
				pCur = pCur->pNext;
				nFromChunk++;
			}
			pCur = pFirst;

		}

		while ( pCur && pCur >= m_Chunks[i] && pCur < pChunkLimit )
		{
			if ( nFromChunk != nBlocksPerChunk )
			{
				if ( pPrev )
				{
					pPrev->pNext = pCur;
				}
				else
				{
					m_pFirstFree = pCur;
				}
				pPrev = pCur;
			}
			else if ( pPrev )
			{
				pPrev->pNext = NULL;
			}
			else
			{
				m_pFirstFree = NULL;
			}

			pCur = pCur->pNext;
		}

		if ( nFromChunk == nBlocksPerChunk )
		{
			m_Allocator.Free( m_Chunks[i] );
			m_nFree -= nBlocksPerChunk;
			m_Chunks[i] = 0;
		}
	}

	for ( i = m_Chunks.Count() - 1; i >= 0 ; i-- )
	{
		if ( !m_Chunks[i] )
		{
			m_Chunks.FastRemove( i );
		}
	}
}

#endif // MEMPOOL_H

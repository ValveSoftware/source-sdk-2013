//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A fast stack memory allocator that uses virtual memory if available
//
//=============================================================================//

#ifndef MEMSTACK_H
#define MEMSTACK_H

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------------------------------

typedef unsigned MemoryStackMark_t;

class CMemoryStack
{
public:
	CMemoryStack();
	~CMemoryStack();

	bool Init( unsigned maxSize = 0, unsigned commitSize = 0, unsigned initialCommit = 0, unsigned alignment = 16 );
#ifdef _X360
	bool InitPhysical( unsigned size = 0, unsigned alignment = 16 );
#endif
	void Term();

	int GetSize();
	int GetMaxSize();
	int	GetUsed();
	
	void *Alloc( unsigned bytes, bool bClear = false ) RESTRICT;

	MemoryStackMark_t GetCurrentAllocPoint();
	void FreeToAllocPoint( MemoryStackMark_t mark, bool bDecommit = true );
	void FreeAll( bool bDecommit = true );
	
	void Access( void **ppRegion, unsigned *pBytes );

	void PrintContents();

	void *GetBase();
	const void *GetBase() const {  return const_cast<CMemoryStack *>(this)->GetBase(); }

private:
	bool CommitTo( byte * ) RESTRICT;

	byte *m_pNextAlloc;
	byte *m_pCommitLimit;
	byte *m_pAllocLimit;
	
	byte *m_pBase;

	unsigned m_maxSize;
	unsigned m_alignment;
#ifdef _WIN32
	unsigned m_commitSize;
	unsigned m_minCommit;
#endif
#ifdef _X360
	bool m_bPhysical;
#endif
};

//-------------------------------------

FORCEINLINE void *CMemoryStack::Alloc( unsigned bytes, bool bClear ) RESTRICT
{
	Assert( m_pBase );

	int alignment = m_alignment;
	if ( bytes )
	{
		bytes = AlignValue( bytes, alignment );
	}
	else
	{
		bytes = alignment;
	}


	void *pResult = m_pNextAlloc;
	byte *pNextAlloc = m_pNextAlloc + bytes;

	if ( pNextAlloc > m_pCommitLimit )
	{
		if ( !CommitTo( pNextAlloc ) )
		{
			return NULL;
		}
	}

	if ( bClear )
	{
		memset( pResult, 0, bytes );
	}

	m_pNextAlloc = pNextAlloc;

	return pResult;
}

//-------------------------------------

inline int CMemoryStack::GetMaxSize()
{ 
	return m_maxSize;
}

//-------------------------------------

inline int CMemoryStack::GetUsed() 
{ 
	return ( m_pNextAlloc - m_pBase ); 
}

//-------------------------------------

inline void *CMemoryStack::GetBase()
{
	return m_pBase;
}

//-------------------------------------

inline MemoryStackMark_t CMemoryStack::GetCurrentAllocPoint()
{
	return ( m_pNextAlloc - m_pBase );
}

//-----------------------------------------------------------------------------
// The CUtlMemoryStack class:
// A fixed memory class
//-----------------------------------------------------------------------------
template< typename T, typename I, size_t MAX_SIZE, size_t COMMIT_SIZE = 0, size_t INITIAL_COMMIT = 0 >
class CUtlMemoryStack
{
public:
	// constructor, destructor
	CUtlMemoryStack( int nGrowSize = 0, int nInitSize = 0 )	{ m_MemoryStack.Init( MAX_SIZE * sizeof(T), COMMIT_SIZE * sizeof(T), INITIAL_COMMIT * sizeof(T), 4 ); COMPILE_TIME_ASSERT( sizeof(T) % 4 == 0 );	}
	CUtlMemoryStack( T* pMemory, int numElements )			{ Assert( 0 ); 										}

	// Can we use this index?
	bool IsIdxValid( I i ) const							{ return (i >= 0) && (i < m_nAllocated); }

	// Specify the invalid ('null') index that we'll only return on failure
	static const I INVALID_INDEX = ( I )-1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex() { return INVALID_INDEX; }

	class Iterator_t
	{
		Iterator_t( I i ) : index( i ) {}
		I index;
		friend class CUtlMemoryStack<T,I,MAX_SIZE, COMMIT_SIZE, INITIAL_COMMIT>;
	public:
		bool operator==( const Iterator_t it ) const		{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const		{ return index != it.index; }
	};
	Iterator_t First() const								{ return Iterator_t( m_nAllocated ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const			{ return Iterator_t( it.index < m_nAllocated ? it.index + 1 : InvalidIndex() ); }
	I GetIndex( const Iterator_t &it ) const				{ return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const		{ return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const		{ return it.index >= 0 && it.index < m_nAllocated; }
	Iterator_t InvalidIterator() const						{ return Iterator_t( InvalidIndex() ); }

	// Gets the base address
	T* Base()												{ return (T*)m_MemoryStack.GetBase(); }
	const T* Base() const									{ return (const T*)m_MemoryStack.GetBase(); }

	// element access
	T& operator[]( I i )									{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& operator[]( I i ) const						{ Assert( IsIdxValid(i) ); return Base()[i];	}
	T& Element( I i )										{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& Element( I i ) const							{ Assert( IsIdxValid(i) ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements )	{ Assert( 0 ); }

	// Size
	int NumAllocated() const								{ return m_nAllocated; }
	int Count() const										{ return m_nAllocated; }

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 )								{ Assert( num > 0 ); m_nAllocated += num; m_MemoryStack.Alloc( num * sizeof(T) ); }

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num )							{ Assert( num <= MAX_SIZE ); if ( m_nAllocated < num ) Grow( num - m_nAllocated ); }

	// Memory deallocation
	void Purge()											{ m_MemoryStack.FreeAll(); m_nAllocated = 0; }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	// Set the size by which the memory grows
	void SetGrowSize( int size )							{}

private:
	CMemoryStack m_MemoryStack;
	int m_nAllocated;
};

//-----------------------------------------------------------------------------

#endif // MEMSTACK_H

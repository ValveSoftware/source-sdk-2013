//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable memory class.
//===========================================================================//

#ifndef UTLMEMORY_H
#define UTLMEMORY_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include <string.h>
#include "tier0/platform.h"
#include "mathlib/mathlib.h"

#include "tier0/memalloc.h"
#include "tier0/memdbgon.h"

#pragma warning (disable:4100)
#pragma warning (disable:4514)


//-----------------------------------------------------------------------------


#ifdef UTLMEMORY_TRACK
#define UTLMEMORY_TRACK_ALLOC()		MemAlloc_RegisterAllocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#define UTLMEMORY_TRACK_FREE()		if ( !m_pMemory ) ; else MemAlloc_RegisterDeallocation( "Sum of all UtlMemory", 0, m_nAllocationCount * sizeof(T), m_nAllocationCount * sizeof(T), 0 )
#else
#define UTLMEMORY_TRACK_ALLOC()		((void)0)
#define UTLMEMORY_TRACK_FREE()		((void)0)
#endif


//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template< class T, class I = int >
class CUtlMemory
{
public:
	// constructor, destructor
	CUtlMemory( int nGrowSize = 0, int nInitSize = 0 );
	CUtlMemory( T* pMemory, int numElements );
	CUtlMemory( const T* pMemory, int numElements );
	~CUtlMemory();

	// Set the size by which the memory grows
	void Init( int nGrowSize = 0, int nInitSize = 0 );

	class Iterator_t
	{
	public:
		Iterator_t( I i ) : index( i ) {}
		I index;

		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const							{ return Iterator_t( IsIdxValid( 0 ) ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( IsIdxValid( it.index + 1 ) ? it.index + 1 : InvalidIndex() ); }
	I GetIndex( const Iterator_t &it ) const			{ return it.index; }
	bool IsIdxAfter( I i, const Iterator_t &it ) const	{ return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const	{ return IsIdxValid( it.index ); }
	Iterator_t InvalidIterator() const					{ return Iterator_t( InvalidIndex() ); }

	// element access
	T& operator[]( I i );
	const T& operator[]( I i ) const;
	T& Element( I i );
	const T& Element( I i ) const;

	// Can we use this index?
	bool IsIdxValid( I i ) const;

	// Specify the invalid ('null') index that we'll only return on failure
	static const I INVALID_INDEX = ( I )-1; // For use with COMPILE_TIME_ASSERT
	static I InvalidIndex() { return INVALID_INDEX; }

	// Gets the base address (can change when adding elements!)
	T* Base();
	const T* Base() const;

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements );
	void SetExternalBuffer( const T* pMemory, int numElements );
	// Takes ownership of the passed memory, including freeing it when this buffer is destroyed.
	void AssumeMemory( T *pMemory, int nSize );

	// Fast swap
	void Swap( CUtlMemory< T, I > &mem );

	// Switches the buffer from an external memory buffer to a reallocatable buffer
	// Will copy the current contents of the external buffer to the reallocatable buffer
	void ConvertToGrowableMemory( int nGrowSize );

	// Size
	int NumAllocated() const;
	int Count() const;

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

	// Purge all but the given number of elements
	void Purge( int numElements );

	// is the memory externally allocated?
	bool IsExternallyAllocated() const;

	// is the memory read only?
	bool IsReadOnly() const;

	// Set the size by which the memory grows
	void SetGrowSize( int size );

protected:
	void ValidateGrowSize()
	{
#ifdef _X360
		if ( m_nGrowSize && m_nGrowSize != EXTERNAL_BUFFER_MARKER )
		{
			// Max grow size at 128 bytes on XBOX
			const int MAX_GROW = 128;
			if ( m_nGrowSize * sizeof(T) > MAX_GROW )
			{
				m_nGrowSize = max( 1, MAX_GROW / sizeof(T) );
			}
		}
#endif
	}

	enum
	{
		EXTERNAL_BUFFER_MARKER = -1,
		EXTERNAL_CONST_BUFFER_MARKER = -2,
	};

	T* m_pMemory;
	int m_nAllocationCount;
	int m_nGrowSize;
};


//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template< class T, size_t SIZE, class I = int >
class CUtlMemoryFixedGrowable : public CUtlMemory< T, I >
{
	typedef CUtlMemory< T, I > BaseClass;

public:
	CUtlMemoryFixedGrowable( int nGrowSize = 0, int nInitSize = SIZE ) : BaseClass( m_pFixedMemory, SIZE ) 
	{
		Assert( nInitSize == 0 || nInitSize == SIZE );
		m_nMallocGrowSize = nGrowSize;
	}

	void Grow( int nCount = 1 )
	{
		if ( this->IsExternallyAllocated() )
		{
			this->ConvertToGrowableMemory( m_nMallocGrowSize );
		}
		BaseClass::Grow( nCount );
	}

	void EnsureCapacity( int num )
	{
		if ( CUtlMemory<T>::m_nAllocationCount >= num )
			return;

		if ( this->IsExternallyAllocated() )
		{
			// Can't grow a buffer whose memory was externally allocated 
			this->ConvertToGrowableMemory( m_nMallocGrowSize );
		}

		BaseClass::EnsureCapacity( num );
	}

private:
	int m_nMallocGrowSize;
	T m_pFixedMemory[ SIZE ];
};

//-----------------------------------------------------------------------------
// The CUtlMemoryFixed class:
// A fixed memory class
//-----------------------------------------------------------------------------
template< typename T, size_t SIZE, int nAlignment = 0 >
class CUtlMemoryFixed
{
public:
	// constructor, destructor
	CUtlMemoryFixed( int nGrowSize = 0, int nInitSize = 0 )	{ Assert( nInitSize == 0 || nInitSize == SIZE ); 	}
	CUtlMemoryFixed( T* pMemory, int numElements )			{ Assert( 0 ); 										}

	// Can we use this index?
	// Use unsigned math to improve performance
	bool IsIdxValid( int i ) const							{ return (size_t)i < SIZE; }

	// Specify the invalid ('null') index that we'll only return on failure
	static const int INVALID_INDEX = -1; // For use with COMPILE_TIME_ASSERT
	static int InvalidIndex() { return INVALID_INDEX; }

	// Gets the base address
	T* Base()												{ if ( nAlignment == 0 ) return (T*)(&m_Memory[0]); else return (T*)AlignValue( &m_Memory[0], nAlignment ); }
	const T* Base() const									{ if ( nAlignment == 0 ) return (T*)(&m_Memory[0]); else return (T*)AlignValue( &m_Memory[0], nAlignment ); }

	// element access
	// Use unsigned math and inlined checks to improve performance.
	T& operator[]( int i )									{ Assert( (size_t)i < SIZE ); return Base()[i];	}
	const T& operator[]( int i ) const						{ Assert( (size_t)i < SIZE ); return Base()[i];	}
	T& Element( int i )										{ Assert( (size_t)i < SIZE ); return Base()[i];	}
	const T& Element( int i ) const							{ Assert( (size_t)i < SIZE ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements )	{ Assert( 0 ); }

	// Size
	int NumAllocated() const								{ return SIZE; }
	int Count() const										{ return SIZE; }

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 )								{ Assert( 0 ); }

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num )							{ Assert( num <= SIZE ); }

	// Memory deallocation
	void Purge()											{}

	// Purge all but the given number of elements (NOT IMPLEMENTED IN CUtlMemoryFixed)
	void Purge( int numElements )							{ Assert( 0 ); }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	// Set the size by which the memory grows
	void SetGrowSize( int size )							{}

	class Iterator_t
	{
	public:
		Iterator_t( int i ) : index( i ) {}
		int index;
		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const							{ return Iterator_t( IsIdxValid( 0 ) ? 0 : InvalidIndex() ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( IsIdxValid( it.index + 1 ) ? it.index + 1 : InvalidIndex() ); }
	int GetIndex( const Iterator_t &it ) const			{ return it.index; }
	bool IsIdxAfter( int i, const Iterator_t &it ) const { return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const	{ return IsIdxValid( it.index ); }
	Iterator_t InvalidIterator() const					{ return Iterator_t( InvalidIndex() ); }

private:
	char m_Memory[ SIZE*sizeof(T) + nAlignment ];
};

#if defined(POSIX)
// From Chris Green: Memory is a little fuzzy but I believe this class did
//	something fishy with respect to msize and alignment that was OK under our
//	allocator, the glibc allocator, etc but not the valgrind one (which has no
//	padding because it detects all forms of head/tail overwrite, including
//	writing 1 byte past a 1 byte allocation).
#define REMEMBER_ALLOC_SIZE_FOR_VALGRIND 1
#endif

//-----------------------------------------------------------------------------
// The CUtlMemoryConservative class:
// A dynamic memory class that tries to minimize overhead (itself small, no custom grow factor)
//-----------------------------------------------------------------------------
template< typename T >
class CUtlMemoryConservative
{

public:
	// constructor, destructor
	CUtlMemoryConservative( int nGrowSize = 0, int nInitSize = 0 ) : m_pMemory( NULL )
	{
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
		m_nCurAllocSize = 0;
#endif

	}
	CUtlMemoryConservative( T* pMemory, int numElements )								{ Assert( 0 ); }
	~CUtlMemoryConservative()								{ if ( m_pMemory ) free( m_pMemory ); }

	// Can we use this index?
	bool IsIdxValid( int i ) const							{ return ( IsDebug() ) ? ( i >= 0 && i < NumAllocated() ) : ( i >= 0 ); }
	static int InvalidIndex()								{ return -1; }

	// Gets the base address
	T* Base()												{ return m_pMemory; }
	const T* Base() const									{ return m_pMemory; }

	// element access
	T& operator[]( int i )									{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& operator[]( int i ) const						{ Assert( IsIdxValid(i) ); return Base()[i];	}
	T& Element( int i )										{ Assert( IsIdxValid(i) ); return Base()[i];	}
	const T& Element( int i ) const							{ Assert( IsIdxValid(i) ); return Base()[i];	}

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements )	{ Assert( 0 ); }

	// Size
	FORCEINLINE void RememberAllocSize( size_t sz )
	{
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
		m_nCurAllocSize = sz;
#endif
	}

	size_t AllocSize( void ) const
	{
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
		return m_nCurAllocSize;
#else
		return ( m_pMemory ) ? g_pMemAlloc->GetSize( m_pMemory ) : 0;
#endif
	}

	int NumAllocated() const
	{
		return AllocSize() / sizeof( T );
	}
	int Count() const
	{
		return NumAllocated();
	}

	FORCEINLINE void ReAlloc( size_t sz )
	{
		m_pMemory = (T*)realloc( m_pMemory, sz );
		RememberAllocSize( sz );
	}
	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 )
	{
		int nCurN = NumAllocated();
		ReAlloc( ( nCurN + num ) * sizeof( T ) );
	}

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num )
	{
		size_t nSize = sizeof( T ) * MAX( num, Count() );
		ReAlloc( nSize );
	}

	// Memory deallocation
	void Purge()
	{
		free( m_pMemory ); 
		RememberAllocSize( 0 );
		m_pMemory = NULL; 
	}

	// Purge all but the given number of elements
	void Purge( int numElements )							{ ReAlloc( numElements * sizeof(T) ); }

	// is the memory externally allocated?
	bool IsExternallyAllocated() const						{ return false; }

	// Set the size by which the memory grows
	void SetGrowSize( int size )							{}

	class Iterator_t
	{
	public:
		Iterator_t( int i, int _limit ) : index( i ), limit( _limit ) {}
		int index;
		int limit;
		bool operator==( const Iterator_t it ) const	{ return index == it.index; }
		bool operator!=( const Iterator_t it ) const	{ return index != it.index; }
	};
	Iterator_t First() const							{ int limit = NumAllocated(); return Iterator_t( limit ? 0 : InvalidIndex(), limit ); }
	Iterator_t Next( const Iterator_t &it ) const		{ return Iterator_t( ( it.index + 1 < it.limit ) ? it.index + 1 : InvalidIndex(), it.limit ); }
	int GetIndex( const Iterator_t &it ) const			{ return it.index; }
	bool IsIdxAfter( int i, const Iterator_t &it ) const { return i > it.index; }
	bool IsValidIterator( const Iterator_t &it ) const	{ return IsIdxValid( it.index ) && ( it.index < it.limit ); }
	Iterator_t InvalidIterator() const					{ return Iterator_t( InvalidIndex(), 0 ); }

private:
	T *m_pMemory;
#ifdef REMEMBER_ALLOC_SIZE_FOR_VALGRIND
	size_t m_nCurAllocSize;
#endif

};


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

template< class T, class I >
CUtlMemory<T,I>::CUtlMemory( int nGrowSize, int nInitAllocationCount ) : m_pMemory(0), 
	m_nAllocationCount( nInitAllocationCount ), m_nGrowSize( nGrowSize )
{
	ValidateGrowSize();
	Assert( nGrowSize >= 0 );
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)malloc( m_nAllocationCount * sizeof(T) );
	}
}

template< class T, class I >
CUtlMemory<T,I>::CUtlMemory( T* pMemory, int numElements ) : m_pMemory(pMemory),
	m_nAllocationCount( numElements )
{
	// Special marker indicating externally supplied modifyable memory
	m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}

template< class T, class I >
CUtlMemory<T,I>::CUtlMemory( const T* pMemory, int numElements ) : m_pMemory( (T*)pMemory ),
	m_nAllocationCount( numElements )
{
	// Special marker indicating externally supplied modifyable memory
	m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}

template< class T, class I >
CUtlMemory<T,I>::~CUtlMemory()
{
	Purge();
}

template< class T, class I >
void CUtlMemory<T,I>::Init( int nGrowSize /*= 0*/, int nInitSize /*= 0*/ )
{
	Purge();

	m_nGrowSize = nGrowSize;
	m_nAllocationCount = nInitSize;
	ValidateGrowSize();
	Assert( nGrowSize >= 0 );
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)malloc( m_nAllocationCount * sizeof(T) );
	}
}

//-----------------------------------------------------------------------------
// Fast swap
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMemory<T,I>::Swap( CUtlMemory<T,I> &mem )
{
	V_swap( m_nGrowSize, mem.m_nGrowSize );
	V_swap( m_pMemory, mem.m_pMemory );
	V_swap( m_nAllocationCount, mem.m_nAllocationCount );
}


//-----------------------------------------------------------------------------
// Switches the buffer from an external memory buffer to a reallocatable buffer
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMemory<T,I>::ConvertToGrowableMemory( int nGrowSize )
{
	if ( !IsExternallyAllocated() )
		return;

	m_nGrowSize = nGrowSize;
	if (m_nAllocationCount)
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();

		int nNumBytes = m_nAllocationCount * sizeof(T);
		T *pMemory = (T*)malloc( nNumBytes );
		memcpy( (void*)pMemory, (void*)m_pMemory, nNumBytes ); 
		m_pMemory = pMemory;
	}
	else
	{
		m_pMemory = NULL;
	}
}


//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMemory<T,I>::SetExternalBuffer( T* pMemory, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	m_pMemory = pMemory;
	m_nAllocationCount = numElements;

	// Indicate that we don't own the memory
	m_nGrowSize = EXTERNAL_BUFFER_MARKER;
}

template< class T, class I >
void CUtlMemory<T,I>::SetExternalBuffer( const T* pMemory, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	m_pMemory = const_cast<T*>( pMemory );
	m_nAllocationCount = numElements;

	// Indicate that we don't own the memory
	m_nGrowSize = EXTERNAL_CONST_BUFFER_MARKER;
}

template< class T, class I >
void CUtlMemory<T,I>::AssumeMemory( T* pMemory, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	// Simply take the pointer but don't mark us as external
	m_pMemory = pMemory;
	m_nAllocationCount = numElements;
}


//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template< class T, class I >
inline T& CUtlMemory<T,I>::operator[]( I i )
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert( m_nGrowSize != EXTERNAL_CONST_BUFFER_MARKER ); //Assert( !IsReadOnly() );
	Assert( (uint32)i < (uint32)m_nAllocationCount );
	return m_pMemory[(uint32)i];
}

template< class T, class I >
inline const T& CUtlMemory<T,I>::operator[]( I i ) const
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert( (uint32)i < (uint32)m_nAllocationCount );
	return m_pMemory[(uint32)i];
}

template< class T, class I >
inline T& CUtlMemory<T,I>::Element( I i )
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert( m_nGrowSize != EXTERNAL_CONST_BUFFER_MARKER ); //Assert( !IsReadOnly() );
	Assert( (uint32)i < (uint32)m_nAllocationCount );
	return m_pMemory[(uint32)i];
}

template< class T, class I >
inline const T& CUtlMemory<T,I>::Element( I i ) const
{
	// Avoid function calls in the asserts to improve debug build performance
	Assert( (uint32)i < (uint32)m_nAllocationCount );
	return m_pMemory[(uint32)i];
}


//-----------------------------------------------------------------------------
// is the memory externally allocated?
//-----------------------------------------------------------------------------
template< class T, class I >
bool CUtlMemory<T,I>::IsExternallyAllocated() const
{
	return (m_nGrowSize < 0);
}


//-----------------------------------------------------------------------------
// is the memory read only?
//-----------------------------------------------------------------------------
template< class T, class I >
bool CUtlMemory<T,I>::IsReadOnly() const
{
	return (m_nGrowSize == EXTERNAL_CONST_BUFFER_MARKER);
}


template< class T, class I >
void CUtlMemory<T,I>::SetGrowSize( int nSize )
{
	Assert( !IsExternallyAllocated() );
	Assert( nSize >= 0 );
	m_nGrowSize = nSize;
	ValidateGrowSize();
}


//-----------------------------------------------------------------------------
// Gets the base address (can change when adding elements!)
//-----------------------------------------------------------------------------
template< class T, class I >
inline T* CUtlMemory<T,I>::Base()
{
	Assert( !IsReadOnly() );
	return m_pMemory;
}

template< class T, class I >
inline const T *CUtlMemory<T,I>::Base() const
{
	return m_pMemory;
}


//-----------------------------------------------------------------------------
// Size
//-----------------------------------------------------------------------------
template< class T, class I >
inline int CUtlMemory<T,I>::NumAllocated() const
{
	return m_nAllocationCount;
}

template< class T, class I >
inline int CUtlMemory<T,I>::Count() const
{
	return m_nAllocationCount;
}


//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< class T, class I >
inline bool CUtlMemory<T,I>::IsIdxValid( I i ) const
{
	// If we always cast 'i' and 'm_nAllocationCount' to unsigned then we can
	// do our range checking with a single comparison instead of two. This gives
	// a modest speedup in debug builds.
	return (uint32)i < (uint32)m_nAllocationCount;
}

//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
inline int UtlMemory_CalcNewAllocationCount( int nAllocationCount, int nGrowSize, int nNewSize, int nBytesItem )
{
	if ( nGrowSize )
	{ 
		nAllocationCount = ((1 + ((nNewSize - 1) / nGrowSize)) * nGrowSize);
	}
	else 
	{
		if ( !nAllocationCount )
		{
			// Compute an allocation which is at least as big as a cache line...
			nAllocationCount = (31 + nBytesItem) / nBytesItem;
		}

		while (nAllocationCount < nNewSize)
		{
#ifndef _X360
			nAllocationCount *= 2;
#else
			int nNewAllocationCount = ( nAllocationCount * 9) / 8; // 12.5 %
			if ( nNewAllocationCount > nAllocationCount )
				nAllocationCount = nNewAllocationCount;
			else
				nAllocationCount *= 2;
#endif
		}
	}

	return nAllocationCount;
}

template< class T, class I >
void CUtlMemory<T,I>::Grow( int num )
{
	Assert( num > 0 );

	if ( IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	// Make sure we have at least numallocated + num allocations.
	// Use the grow rules specified for this memory (in m_nGrowSize)
	int nAllocationRequested = m_nAllocationCount + num;

	UTLMEMORY_TRACK_FREE();

	int nNewAllocationCount = UtlMemory_CalcNewAllocationCount( m_nAllocationCount, m_nGrowSize, nAllocationRequested, sizeof(T) );

	// if m_nAllocationRequested wraps index type I, recalculate
	if ( ( int )( I )nNewAllocationCount < nAllocationRequested )
	{
		if ( ( int )( I )nNewAllocationCount == 0 && ( int )( I )( nNewAllocationCount - 1 ) >= nAllocationRequested )
		{
			--nNewAllocationCount; // deal w/ the common case of m_nAllocationCount == MAX_USHORT + 1
		}
		else
		{
			if ( ( int )( I )nAllocationRequested != nAllocationRequested )
			{
				// we've been asked to grow memory to a size s.t. the index type can't address the requested amount of memory
				Assert( 0 );
				return;
			}
			while ( ( int )( I )nNewAllocationCount < nAllocationRequested )
			{
				nNewAllocationCount = ( nNewAllocationCount + nAllocationRequested ) / 2;
			}
		}
	}

	m_nAllocationCount = nNewAllocationCount;

	UTLMEMORY_TRACK_ALLOC();

	if (m_pMemory)
	{
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)realloc( m_pMemory, m_nAllocationCount * sizeof(T) );
		Assert( m_pMemory );
	}
	else
	{
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)malloc( m_nAllocationCount * sizeof(T) );
		Assert( m_pMemory );
	}
}


//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template< class T, class I >
inline void CUtlMemory<T,I>::EnsureCapacity( int num )
{
	if (m_nAllocationCount >= num)
		return;

	if ( IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	UTLMEMORY_TRACK_FREE();

	m_nAllocationCount = num;

	UTLMEMORY_TRACK_ALLOC();

	if (m_pMemory)
	{
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)realloc( m_pMemory, m_nAllocationCount * sizeof(T) );
	}
	else
	{
		MEM_ALLOC_CREDIT_CLASS();
		m_pMemory = (T*)malloc( m_nAllocationCount * sizeof(T) );
	}
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMemory<T,I>::Purge()
{
	if ( !IsExternallyAllocated() )
	{
		if (m_pMemory)
		{
			UTLMEMORY_TRACK_FREE();
			free( (void*)m_pMemory );
			m_pMemory = 0;
		}
		m_nAllocationCount = 0;
	}
}

template< class T, class I >
void CUtlMemory<T,I>::Purge( int numElements )
{
	Assert( numElements >= 0 );

	if( numElements > m_nAllocationCount )
	{
		// Ensure this isn't a grow request in disguise.
		Assert( numElements <= m_nAllocationCount );
		return;
	}

	// If we have zero elements, simply do a purge:
	if( numElements == 0 )
	{
		Purge();
		return;
	}

	if ( IsExternallyAllocated() )
	{
		// Can't shrink a buffer whose memory was externally allocated, fail silently like purge 
		return;
	}

	// If the number of elements is the same as the allocation count, we are done.
	if( numElements == m_nAllocationCount )
	{
		return;
	}


	if( !m_pMemory )
	{
		// Allocation count is non zero, but memory is null.
		Assert( m_pMemory );
		return;
	}

	UTLMEMORY_TRACK_FREE();

	m_nAllocationCount = numElements;
	
	UTLMEMORY_TRACK_ALLOC();

	// Allocation count > 0, shrink it down.
	MEM_ALLOC_CREDIT_CLASS();
	m_pMemory = (T*)realloc( m_pMemory, m_nAllocationCount * sizeof(T) );
}

//-----------------------------------------------------------------------------
// The CUtlMemory class:
// A growable memory class which doubles in size by default.
//-----------------------------------------------------------------------------
template< class T, int nAlignment >
class CUtlMemoryAligned	: public CUtlMemory<T>
{
public:
	// constructor, destructor
	CUtlMemoryAligned( int nGrowSize = 0, int nInitSize = 0 );
	CUtlMemoryAligned( T* pMemory, int numElements );
	CUtlMemoryAligned( const T* pMemory, int numElements );
	~CUtlMemoryAligned();

	// Attaches the buffer to external memory....
	void SetExternalBuffer( T* pMemory, int numElements );
	void SetExternalBuffer( const T* pMemory, int numElements );

	// Grows the memory, so that at least allocated + num elements are allocated
	void Grow( int num = 1 );

	// Makes sure we've got at least this much memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

	// Purge all but the given number of elements (NOT IMPLEMENTED IN CUtlMemoryAligned)
	void Purge( int numElements )	{ Assert( 0 ); }

private:
	void *Align( const void *pAddr );
};


//-----------------------------------------------------------------------------
// Aligns a pointer
//-----------------------------------------------------------------------------
template< class T, int nAlignment >
void *CUtlMemoryAligned<T, nAlignment>::Align( const void *pAddr )
{
	size_t nAlignmentMask = nAlignment - 1;
	return (void*)( ((size_t)pAddr + nAlignmentMask) & (~nAlignmentMask) );
}


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< class T, int nAlignment >
CUtlMemoryAligned<T, nAlignment>::CUtlMemoryAligned( int nGrowSize, int nInitAllocationCount )
{
	CUtlMemory<T>::m_pMemory = 0; 
	CUtlMemory<T>::m_nAllocationCount = nInitAllocationCount;
	CUtlMemory<T>::m_nGrowSize = nGrowSize;
	this->ValidateGrowSize();

	// Alignment must be a power of two
	COMPILE_TIME_ASSERT( (nAlignment & (nAlignment-1)) == 0 );
	Assert( (nGrowSize >= 0) && (nGrowSize != CUtlMemory<T>::EXTERNAL_BUFFER_MARKER) );
	if ( CUtlMemory<T>::m_nAllocationCount )
	{
		UTLMEMORY_TRACK_ALLOC();
		MEM_ALLOC_CREDIT_CLASS();
		CUtlMemory<T>::m_pMemory = (T*)_aligned_malloc( nInitAllocationCount * sizeof(T), nAlignment );
	}
}

template< class T, int nAlignment >
CUtlMemoryAligned<T, nAlignment>::CUtlMemoryAligned( T* pMemory, int numElements )
{
	// Special marker indicating externally supplied memory
	CUtlMemory<T>::m_nGrowSize = CUtlMemory<T>::EXTERNAL_BUFFER_MARKER;

	CUtlMemory<T>::m_pMemory = (T*)Align( pMemory );
	CUtlMemory<T>::m_nAllocationCount = ( (int)(pMemory + numElements) - (int)CUtlMemory<T>::m_pMemory ) / sizeof(T);
}

template< class T, int nAlignment >
CUtlMemoryAligned<T, nAlignment>::CUtlMemoryAligned( const T* pMemory, int numElements )
{
	// Special marker indicating externally supplied memory
	CUtlMemory<T>::m_nGrowSize = CUtlMemory<T>::EXTERNAL_CONST_BUFFER_MARKER;

	CUtlMemory<T>::m_pMemory = (T*)Align( pMemory );
	CUtlMemory<T>::m_nAllocationCount = ( (int)(pMemory + numElements) - (int)CUtlMemory<T>::m_pMemory ) / sizeof(T);
}

template< class T, int nAlignment >
CUtlMemoryAligned<T, nAlignment>::~CUtlMemoryAligned()
{
	Purge();
}


//-----------------------------------------------------------------------------
// Attaches the buffer to external memory....
//-----------------------------------------------------------------------------
template< class T, int nAlignment >
void CUtlMemoryAligned<T, nAlignment>::SetExternalBuffer( T* pMemory, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	CUtlMemory<T>::m_pMemory = (T*)Align( pMemory );
	CUtlMemory<T>::m_nAllocationCount = ( (int)(pMemory + numElements) - (int)CUtlMemory<T>::m_pMemory ) / sizeof(T);

	// Indicate that we don't own the memory
	CUtlMemory<T>::m_nGrowSize = CUtlMemory<T>::EXTERNAL_BUFFER_MARKER;
}

template< class T, int nAlignment >
void CUtlMemoryAligned<T, nAlignment>::SetExternalBuffer( const T* pMemory, int numElements )
{
	// Blow away any existing allocated memory
	Purge();

	CUtlMemory<T>::m_pMemory = (T*)Align( pMemory );
	CUtlMemory<T>::m_nAllocationCount = ( (int)(pMemory + numElements) - (int)CUtlMemory<T>::m_pMemory ) / sizeof(T);

	// Indicate that we don't own the memory
	CUtlMemory<T>::m_nGrowSize = CUtlMemory<T>::EXTERNAL_CONST_BUFFER_MARKER;
}


//-----------------------------------------------------------------------------
// Grows the memory
//-----------------------------------------------------------------------------
template< class T, int nAlignment >
void CUtlMemoryAligned<T, nAlignment>::Grow( int num )
{
	Assert( num > 0 );

	if ( this->IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	UTLMEMORY_TRACK_FREE();

	// Make sure we have at least numallocated + num allocations.
	// Use the grow rules specified for this memory (in m_nGrowSize)
	int nAllocationRequested = CUtlMemory<T>::m_nAllocationCount + num;

	CUtlMemory<T>::m_nAllocationCount = UtlMemory_CalcNewAllocationCount( CUtlMemory<T>::m_nAllocationCount, CUtlMemory<T>::m_nGrowSize, nAllocationRequested, sizeof(T) );

	UTLMEMORY_TRACK_ALLOC();

	if ( CUtlMemory<T>::m_pMemory )
	{
		MEM_ALLOC_CREDIT_CLASS();
		CUtlMemory<T>::m_pMemory = (T*)MemAlloc_ReallocAligned( CUtlMemory<T>::m_pMemory, CUtlMemory<T>::m_nAllocationCount * sizeof(T), nAlignment );
		Assert( CUtlMemory<T>::m_pMemory );
	}
	else
	{
		MEM_ALLOC_CREDIT_CLASS();
		CUtlMemory<T>::m_pMemory = (T*)MemAlloc_AllocAligned( CUtlMemory<T>::m_nAllocationCount * sizeof(T), nAlignment );
		Assert( CUtlMemory<T>::m_pMemory );
	}
}


//-----------------------------------------------------------------------------
// Makes sure we've got at least this much memory
//-----------------------------------------------------------------------------
template< class T, int nAlignment >
inline void CUtlMemoryAligned<T, nAlignment>::EnsureCapacity( int num )
{
	if ( CUtlMemory<T>::m_nAllocationCount >= num )
		return;

	if ( this->IsExternallyAllocated() )
	{
		// Can't grow a buffer whose memory was externally allocated 
		Assert(0);
		return;
	}

	UTLMEMORY_TRACK_FREE();

	CUtlMemory<T>::m_nAllocationCount = num;

	UTLMEMORY_TRACK_ALLOC();

	if ( CUtlMemory<T>::m_pMemory )
	{
		MEM_ALLOC_CREDIT_CLASS();
		CUtlMemory<T>::m_pMemory = (T*)MemAlloc_ReallocAligned( CUtlMemory<T>::m_pMemory, CUtlMemory<T>::m_nAllocationCount * sizeof(T), nAlignment );
	}
	else
	{
		MEM_ALLOC_CREDIT_CLASS();
		CUtlMemory<T>::m_pMemory = (T*)MemAlloc_AllocAligned( CUtlMemory<T>::m_nAllocationCount * sizeof(T), nAlignment );
	}
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------
template< class T, int nAlignment >
void CUtlMemoryAligned<T, nAlignment>::Purge()
{
	if ( !this->IsExternallyAllocated() )
	{
		if ( CUtlMemory<T>::m_pMemory )
		{
			UTLMEMORY_TRACK_FREE();
			MemAlloc_FreeAligned( CUtlMemory<T>::m_pMemory );
			CUtlMemory<T>::m_pMemory = 0;
		}
		CUtlMemory<T>::m_nAllocationCount = 0;
	}
}

#include "tier0/memdbgoff.h"

#endif // UTLMEMORY_H

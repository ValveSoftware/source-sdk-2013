//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// A growable array class that maintains a free list and keeps elements
// in the same location
//=============================================================================//

#ifndef UTLVECTOR_H
#define UTLVECTOR_H

#ifdef _WIN32
#pragma once
#endif

#include <algorithm>

#include <string.h>
#include "tier0/platform.h"
#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#include "tier1/utlmemory.h"
#include "tier1/utlblockmemory.h"
#include "tier1/strtools.h"
#include "vstdlib/random.h"

#define FOR_EACH_VEC( vecName, iteratorName ) \
	for ( int iteratorName = 0; (vecName).IsUtlVector && iteratorName < (vecName).Count(); iteratorName++ )
#define FOR_EACH_VEC_BACK( vecName, iteratorName ) \
	for ( int iteratorName = (vecName).Count()-1; (vecName).IsUtlVector && iteratorName >= 0; iteratorName-- )

// UtlVector derives from this so we can do the type check above
struct base_vector_t
{
public:
	enum { IsUtlVector = true }; // Used to match this at compiletime 		
};

//-----------------------------------------------------------------------------
// The CUtlVector class:
// A growable array class which doubles in size by default.
// It will always keep all elements consecutive in memory, and may move the
// elements around in memory (via a PvRealloc) when elements are inserted or
// removed. Clients should therefore refer to the elements of the vector
// by index (they should *never* maintain pointers to elements in the vector).
//-----------------------------------------------------------------------------
template< class T, class A = CUtlMemory<T> >
class CUtlVector : public base_vector_t
{
	typedef A CAllocator;
public:
	typedef T ElemType_t;
	typedef T* iterator;
	typedef const T* const_iterator;

	// Set the growth policy and initial capacity. Count will always be zero. This is different from std::vector
	// where the constructor sets count as well as capacity.
	// growSize of zero implies the default growth pattern which is exponential.
	explicit CUtlVector( int growSize = 0, int initialCapacity = 0 );

	// Initialize with separately allocated buffer, setting the capacity and count.
	// The container will not be growable.
	CUtlVector( T* pMemory, int initialCapacity, int initialCount = 0 );
	~CUtlVector();
	
	// Copy the array.
	CUtlVector<T, A>& operator=( const CUtlVector<T, A> &other );

	// element access
	T& operator[]( int i );
	const T& operator[]( int i ) const;
	T& Element( int i );
	const T& Element( int i ) const;
	T& Head();
	const T& Head() const;
	T& Tail();
	const T& Tail() const;
	T& Random();
	const T& Random() const;

	// STL compatible member functions. These allow easier use of std::sort
	// and they are forward compatible with the C++ 11 range-based for loops.
	iterator begin()						{ return Base(); }
	const_iterator begin() const			{ return Base(); }
	iterator end()							{ return Base() + Count(); }
	const_iterator end() const				{ return Base() + Count(); }

	// Gets the base address (can change when adding elements!)
	T* Base()								{ return m_Memory.Base(); }
	const T* Base() const					{ return m_Memory.Base(); }

	// Returns the number of elements in the vector
	// SIZE IS DEPRECATED!
	int Count() const;
	int Size() const;	// don't use me!

	/// are there no elements? For compatibility with lists.
	inline bool IsEmpty( void ) const
	{
		return ( Count() == 0 );
	}

	// Is element index valid?
	bool IsValidIndex( int i ) const;
	static int InvalidIndex();

	// Adds an element, uses default constructor
	int AddToHead();
	int AddToTail();
	T *AddToTailGetPtr();
	int InsertBefore( int elem );
	int InsertAfter( int elem );

	// Adds an element, uses copy constructor
	int AddToHead( const T& src );
	int AddToTail( const T& src );
	int InsertBefore( int elem, const T& src );
	int InsertAfter( int elem, const T& src );

	// Adds multiple elements, uses default constructor
	int AddMultipleToHead( int num );
	int AddMultipleToTail( int num );	   
	int AddMultipleToTail( int num, const T *pToCopy );	   
	int InsertMultipleBefore( int elem, int num );
	int InsertMultipleBefore( int elem, int num, const T *pToCopy );
	int InsertMultipleAfter( int elem, int num );

	// Calls RemoveAll() then AddMultipleToTail.
	// SetSize is a synonym for SetCount
	void SetSize( int size );
	// SetCount deletes the previous contents of the container and sets the
	// container to have this many elements.
	// Use GetCount to retrieve the current count.
	void SetCount( int count );
	void SetCountNonDestructively( int count ); //sets count by adding or removing elements to tail TODO: This should probably be the default behavior for SetCount
	
	// Calls SetSize and copies each element.
	void CopyArray( const T *pArray, int size );

	// Fast swap
	void Swap( CUtlVector< T, A > &vec );
	
	// Add the specified array to the tail.
	int AddVectorToTail( CUtlVector<T, A> const &src );

	// Finds an element (element needs operator== defined)
	int Find( const T& src ) const;

	// Helper to find using std::find_if with a predicate
	//   e.g. [] -> bool ( T &a ) { return a.IsTheThingIWant(); }
	//
	// Useful if your object doesn't define a ==
	template < typename F >
	int FindPredicate( F&& predicate ) const;

	void FillWithValue( const T& src );

	bool HasElement( const T& src ) const;

	// Makes sure we have enough memory allocated to store a requested # of elements
	// Use NumAllocated() to retrieve the current capacity.
	void EnsureCapacity( int num );

	// Makes sure we have at least this many elements
	// Use GetCount to retrieve the current count.
	void EnsureCount( int num );

	// Element removal
	void FastRemove( int elem );	// doesn't preserve order
	void Remove( int elem );		// preserves order, shifts elements
	bool FindAndRemove( const T& src );	// removes first occurrence of src, preserves order, shifts elements
	bool FindAndFastRemove( const T& src );	// removes first occurrence of src, doesn't preserve order
	void RemoveMultiple( int elem, int num );	// preserves order, shifts elements
	void RemoveMultipleFromHead(int num); // removes num elements from tail
	void RemoveMultipleFromTail(int num); // removes num elements from tail
	void RemoveAll();				// doesn't deallocate memory

	// Memory deallocation
	void Purge();

	// Purges the list and calls delete on each element in it.
	void PurgeAndDeleteElements();

	// Compacts the vector to the number of elements actually in use 
	void Compact();

	// Set the size by which it grows when it needs to allocate more memory.
	void SetGrowSize( int size )			{ m_Memory.SetGrowSize( size ); }

	int NumAllocated() const;	// Only use this if you really know what you're doing!

	void Sort( int (__cdecl *pfnCompare)(const T *, const T *) );

	void Shuffle( IUniformRandomStream* pSteam = NULL );
	
	// Call this to quickly sort non-contiguously allocated vectors
	void InPlaceQuickSort( int (__cdecl *pfnCompare)(const T *, const T *) );
	// reverse the order of elements
	void Reverse( );

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, char *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

	/// sort using std:: and expecting a "<" function to be defined for the type
	void Sort( void );

	/// sort using std:: with a predicate. e.g. [] -> bool ( T &a, T &b ) { return a < b; }
	template <class F> void SortPredicate( F &&predicate );

protected:
	// Can't copy this unless we explicitly do it!
	CUtlVector( CUtlVector const& vec ) { Assert(0); }

	// Grows the vector
	void GrowVector( int num = 1 );

	// Shifts elements....
	void ShiftElementsRight( int elem, int num = 1 );
	void ShiftElementsLeft( int elem, int num = 1 );

	CAllocator m_Memory;
	int m_Size;

#ifndef _X360
	// For easier access to the elements through the debugger
	// it's in release builds so this can be used in libraries correctly
	T *m_pElements;

	inline void ResetDbgInfo()
	{
		m_pElements = Base();
	}
#else
	inline void ResetDbgInfo() {}
#endif

private:
	void InPlaceQuickSort_r( int (__cdecl *pfnCompare)(const T *, const T *), int nLeft, int nRight );
};


// this is kind of ugly, but until C++ gets templatized typedefs in C++0x, it's our only choice
template < class T >
class CUtlBlockVector : public CUtlVector< T, CUtlBlockMemory< T, int > >
{
public:
	explicit CUtlBlockVector( int growSize = 0, int initSize = 0 )
		: CUtlVector< T, CUtlBlockMemory< T, int > >( growSize, initSize ) {}
};

//-----------------------------------------------------------------------------
// The CUtlVectorMT class:
// An array class with spurious mutex protection. Nothing is actually protected
// unless you call Lock and Unlock. Also, the Mutex_t is actually not a type
// but a member which probably isn't used.
//-----------------------------------------------------------------------------

template< class BASE_UTLVECTOR, class MUTEX_TYPE = CThreadFastMutex >
class CUtlVectorMT : public BASE_UTLVECTOR, public MUTEX_TYPE
{
	typedef BASE_UTLVECTOR BaseClass;
public:
	// MUTEX_TYPE Mutex_t;

	// constructor, destructor
	explicit CUtlVectorMT( int growSize = 0, int initSize = 0 ) : BaseClass( growSize, initSize ) {}
	CUtlVectorMT( typename BaseClass::ElemType_t* pMemory, int numElements ) : BaseClass( pMemory, numElements ) {}
};


//-----------------------------------------------------------------------------
// The CUtlVectorFixed class:
// A array class with a fixed allocation scheme
//-----------------------------------------------------------------------------
template< class T, size_t MAX_SIZE >
class CUtlVectorFixed : public CUtlVector< T, CUtlMemoryFixed<T, MAX_SIZE > >
{
	typedef CUtlVector< T, CUtlMemoryFixed<T, MAX_SIZE > > BaseClass;
public:

	// constructor, destructor
	explicit CUtlVectorFixed( int growSize = 0, int initSize = 0 ) : BaseClass( growSize, initSize ) {}
	CUtlVectorFixed( T* pMemory, int numElements ) : BaseClass( pMemory, numElements ) {}
};


//-----------------------------------------------------------------------------
// The CUtlVectorFixedGrowable class:
// A array class with a fixed allocation scheme backed by a dynamic one
//-----------------------------------------------------------------------------
template< class T, size_t MAX_SIZE >
class CUtlVectorFixedGrowable : public CUtlVector< T, CUtlMemoryFixedGrowable<T, MAX_SIZE > >
{
	typedef CUtlVector< T, CUtlMemoryFixedGrowable<T, MAX_SIZE > > BaseClass;

public:
	// constructor, destructor
	explicit CUtlVectorFixedGrowable( int growSize = 0 ) : BaseClass( growSize, MAX_SIZE ) {}
};


//-----------------------------------------------------------------------------
// The CUtlVectorConservative class:
// A array class with a conservative allocation scheme
//-----------------------------------------------------------------------------
template< class T >
class CUtlVectorConservative : public CUtlVector< T, CUtlMemoryConservative<T> >
{
	typedef CUtlVector< T, CUtlMemoryConservative<T> > BaseClass;
public:

	// constructor, destructor
	explicit CUtlVectorConservative( int growSize = 0, int initSize = 0 ) : BaseClass( growSize, initSize ) {}
	CUtlVectorConservative( T* pMemory, int numElements ) : BaseClass( pMemory, numElements ) {}
};


//-----------------------------------------------------------------------------
// The CUtlVectorUltra Conservative class:
// A array class with a very conservative allocation scheme, with customizable allocator
// Especialy useful if you have a lot of vectors that are sparse, or if you're
// carefully packing holders of vectors
//-----------------------------------------------------------------------------
#pragma warning(push)
#pragma warning(disable : 4200) // warning C4200: nonstandard extension used : zero-sized array in struct/union
#pragma warning(disable : 4815 ) // warning C4815: 'staticData' : zero-sized array in stack object will have no elements

class CUtlVectorUltraConservativeAllocator
{
public:
	static void *Alloc( size_t nSize )
	{
		return malloc( nSize );
	}

	static void *Realloc( void *pMem, size_t nSize )
	{
		return realloc( pMem, nSize );
	}

	static void Free( void *pMem )
	{
		free( pMem );
	}

	static size_t GetSize( void *pMem )
	{
		return mallocsize( pMem );
	}

};

template <typename T, typename A = CUtlVectorUltraConservativeAllocator >
class CUtlVectorUltraConservative : private A
{
public:
	// Don't inherit from base_vector_t because multiple-inheritance increases
	// class size!
	enum { IsUtlVector = true }; // Used to match this at compiletime 		

	CUtlVectorUltraConservative()
	{
		m_pData = StaticData();
	}

	~CUtlVectorUltraConservative()
	{
		RemoveAll();
	}

	int Count() const
	{
		return m_pData->m_Size;
	}

	static int InvalidIndex()
	{
		return -1;
	}

	inline bool IsValidIndex( int i ) const
	{
		return (i >= 0) && (i < Count());
	}

	T& operator[]( int i )
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	const T& operator[]( int i ) const
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	T& Element( int i )
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	const T& Element( int i ) const
	{
		Assert( IsValidIndex( i ) );
		return m_pData->m_Elements[i];
	}

	void EnsureCapacity( int num )
	{
		int nCurCount = Count();
		if ( num <= nCurCount )
		{
			return;
		}
		if ( m_pData == StaticData() )
		{
			m_pData = (Data_t *)A::Alloc( sizeof(Data_t) + ( num * sizeof(T) ) );
			m_pData->m_Size = 0;
		}
		else
		{
			int nNeeded = sizeof(Data_t) + ( num * sizeof(T) );
			int nHave = A::GetSize( m_pData );
			if ( nNeeded > nHave )
			{
				m_pData = (Data_t *)A::Realloc( m_pData, nNeeded );
			}
		}
	}

	int AddToTail( const T& src )
	{
		int iNew = Count();
		EnsureCapacity( Count() + 1 );
		m_pData->m_Elements[iNew] = src;
		m_pData->m_Size++;
		return iNew;
	}

	void RemoveAll()
	{
		if ( Count() )
		{
			for (int i = m_pData->m_Size; --i >= 0; )
			{
				// Global scope to resolve conflict with Scaleform 4.0
				::Destruct(&m_pData->m_Elements[i]);
			}
		}
		if ( m_pData != StaticData() )
		{
			A::Free( m_pData );
			m_pData = StaticData();

		}
	}

	void PurgeAndDeleteElements()
	{
		if ( m_pData != StaticData() )
		{
			for( int i=0; i < m_pData->m_Size; i++ )
			{
				delete Element(i);
			}
			RemoveAll();
		}
	}

	void FastRemove( int elem )
	{
		Assert( IsValidIndex(elem) );

		// Global scope to resolve conflict with Scaleform 4.0
		::Destruct( &Element(elem) );
		if (Count() > 0)
		{
			if ( elem != m_pData->m_Size -1 )
				memcpy( &Element(elem), &Element(m_pData->m_Size-1), sizeof(T) );
			--m_pData->m_Size;
		}
		if ( !m_pData->m_Size )
		{
			A::Free( m_pData );
			m_pData = StaticData();
		}
	}

	void Remove( int elem )
	{
		// Global scope to resolve conflict with Scaleform 4.0
		::Destruct( &Element(elem) );
		ShiftElementsLeft(elem);
		--m_pData->m_Size;
		if ( !m_pData->m_Size )
		{
			A::Free( m_pData );
			m_pData = StaticData();
		}
	}

	int Find( const T& src ) const
	{
		int nCount = Count();
		for ( int i = 0; i < nCount; ++i )
		{
			if (Element(i) == src)
				return i;
		}
		return -1;
	}

	bool FindAndRemove( const T& src )
	{
		int elem = Find( src );
		if ( elem != -1 )
		{
			Remove( elem );
			return true;
		}
		return false;
	}


	bool FindAndFastRemove( const T& src )
	{
		int elem = Find( src );
		if ( elem != -1 )
		{
			FastRemove( elem );
			return true;
		}
		return false;
	}

	bool DebugCompileError_ANonVectorIsUsedInThe_FOR_EACH_VEC_Macro( void ) const { return true; }

	struct Data_t
	{
		int m_Size;
		T m_Elements[0];
	};

	Data_t *m_pData;
private:
	void ShiftElementsLeft( int elem, int num = 1 )
	{
		int Size = Count();
		Assert( IsValidIndex(elem) || ( Size == 0 ) || ( num == 0 ));
		int numToMove = Size - elem - num;
		if ((numToMove > 0) && (num > 0))
		{
			Q_memmove( &Element(elem), &Element(elem+num), numToMove * sizeof(T) );

#ifdef _DEBUG
			Q_memset( &Element(Size-num), 0xDD, num * sizeof(T) );
#endif
		}
	}



	static Data_t *StaticData()
	{
		static Data_t staticData;
		Assert( staticData.m_Size == 0 );
		return &staticData;
	}
};

#pragma warning(pop)

// Make sure nobody adds multiple inheritance and makes this class bigger.
COMPILE_TIME_ASSERT( sizeof(CUtlVectorUltraConservative<int>) == sizeof(void*) );


//-----------------------------------------------------------------------------
// The CCopyableUtlVector class:
// A array class that allows copy construction (so you can nest a CUtlVector inside of another one of our containers)
//  WARNING - this class lets you copy construct which can be an expensive operation if you don't carefully control when it happens
// Only use this when nesting a CUtlVector() inside of another one of our container classes (i.e a CUtlMap)
//-----------------------------------------------------------------------------
template< class T >
class CCopyableUtlVector : public CUtlVector< T, CUtlMemory<T> >
{
	typedef CUtlVector< T, CUtlMemory<T> > BaseClass;
public:
	explicit CCopyableUtlVector( int growSize = 0, int initSize = 0 ) : BaseClass( growSize, initSize ) {}
	CCopyableUtlVector( T* pMemory, int numElements ) : BaseClass( pMemory, numElements ) {}
	virtual ~CCopyableUtlVector() {}
	CCopyableUtlVector( CCopyableUtlVector const& vec ) { this->CopyArray( vec.Base(), vec.Count() ); }
};

//-----------------------------------------------------------------------------
// The CCopyableUtlVector class:
// A array class that allows copy construction (so you can nest a CUtlVector inside of another one of our containers)
//  WARNING - this class lets you copy construct which can be an expensive operation if you don't carefully control when it happens
// Only use this when nesting a CUtlVector() inside of another one of our container classes (i.e a CUtlMap)
//-----------------------------------------------------------------------------
template< class T, size_t MAX_SIZE >
class CCopyableUtlVectorFixedGrowable : public CUtlVectorFixedGrowable< T, MAX_SIZE >
{
	typedef CUtlVectorFixedGrowable< T, MAX_SIZE > BaseClass;
public:
	explicit CCopyableUtlVectorFixedGrowable( int growSize = 0 ) : BaseClass( growSize ) {}
	CCopyableUtlVectorFixedGrowable( T* pMemory, int numElements ) : BaseClass( pMemory, numElements ) {}
	virtual ~CCopyableUtlVectorFixedGrowable() {}
	CCopyableUtlVectorFixedGrowable( CCopyableUtlVectorFixedGrowable const& vec ) { this->CopyArray( vec.Base(), vec.Count() ); }
};

//-----------------------------------------------------------------------------
// The CCopyableUtlVector class:
// A array class that allows copy construction (so you can nest a CUtlVector inside of another one of our containers)
//  WARNING - this class lets you copy construct which can be an expensive operation if you don't carefully control when it happens
// Only use this when nesting a CUtlVector() inside of another one of our container classes (i.e a CUtlMap)
//-----------------------------------------------------------------------------
template< class T, size_t MAX_SIZE >
class CCopyableUtlVectorFixed : public CUtlVectorFixed< T, MAX_SIZE >
{
	typedef CUtlVectorFixed< T, MAX_SIZE > BaseClass;
public:
	explicit CCopyableUtlVectorFixed( int growSize = 0, int initSize = 0 ) : BaseClass( growSize, initSize ) {}
	CCopyableUtlVectorFixed( T* pMemory, int numElements ) : BaseClass( pMemory, numElements ) {}
	virtual ~CCopyableUtlVectorFixed() {}
	CCopyableUtlVectorFixed( CCopyableUtlVectorFixed const& vec ) { this->CopyArray( vec.Base(), vec.Count() ); }
};

// TODO (Ilya): It seems like all the functions in CUtlVector are simple enough that they should be inlined.

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template< typename T, class A >
inline CUtlVector<T, A>::CUtlVector( int growSize, int initSize )	: 
	m_Memory(growSize, initSize), m_Size(0)
{
	ResetDbgInfo();
}

template< typename T, class A >
inline CUtlVector<T, A>::CUtlVector( T* pMemory, int allocationCount, int numElements )	: 
	m_Memory(pMemory, allocationCount), m_Size(numElements)
{
	ResetDbgInfo();
}

template< typename T, class A >
inline CUtlVector<T, A>::~CUtlVector()
{
	Purge();
}

template< typename T, class A >
inline CUtlVector<T, A>& CUtlVector<T, A>::operator=( const CUtlVector<T, A> &other )
{
	int nCount = other.Count();
	SetSize( nCount );
	for ( int i = 0; i < nCount; i++ )
	{
		(*this)[ i ] = other[ i ];
	}
	return *this;
}

#define StagingUtlVectorBoundsCheck( _i, _size )

//-----------------------------------------------------------------------------
// element access
//-----------------------------------------------------------------------------
template< typename T, class A >
inline T& CUtlVector<T, A>::operator[]( int i )
{
	// Do an inline unsigned check for maximum debug-build performance.
	Assert( (unsigned)i < (unsigned)m_Size );
	StagingUtlVectorBoundsCheck( i, m_Size );
	return m_Memory[ i ];
}

template< typename T, class A >
inline const T& CUtlVector<T, A>::operator[]( int i ) const
{
	// Do an inline unsigned check for maximum debug-build performance.
	Assert( (unsigned)i < (unsigned)m_Size );
	StagingUtlVectorBoundsCheck( i, m_Size );
	return m_Memory[ i ];
}

template< typename T, class A >
inline T& CUtlVector<T, A>::Element( int i )
{
	// Do an inline unsigned check for maximum debug-build performance.
	Assert( (unsigned)i < (unsigned)m_Size );
	StagingUtlVectorBoundsCheck( i, m_Size );
	return m_Memory[ i ];
}

template< typename T, class A >
inline const T& CUtlVector<T, A>::Element( int i ) const
{
	// Do an inline unsigned check for maximum debug-build performance.
	Assert( (unsigned)i < (unsigned)m_Size );
	StagingUtlVectorBoundsCheck( i, m_Size );
	return m_Memory[ i ];
}

template< typename T, class A >
inline T& CUtlVector<T, A>::Head()
{
	Assert( m_Size > 0 );
	StagingUtlVectorBoundsCheck( 0, m_Size );
	return m_Memory[ 0 ];
}

template< typename T, class A >
inline const T& CUtlVector<T, A>::Head() const
{
	Assert( m_Size > 0 );
	StagingUtlVectorBoundsCheck( 0, m_Size );
	return m_Memory[ 0 ];
}

template< typename T, class A >
inline T& CUtlVector<T, A>::Tail()
{
	Assert( m_Size > 0 );
	StagingUtlVectorBoundsCheck( 0, m_Size );
	return m_Memory[ m_Size - 1 ];
}

template< typename T, class A >
inline const T& CUtlVector<T, A>::Tail() const
{
	Assert( m_Size > 0 );
	StagingUtlVectorBoundsCheck( 0, m_Size );
	return m_Memory[ m_Size - 1 ];
}


//-----------------------------------------------------------------------------
// Count
//-----------------------------------------------------------------------------
template< typename T, class A >
inline int CUtlVector<T, A>::Size() const
{
	return m_Size;
}

template< typename T, class A >
inline T& CUtlVector<T, A>::Random()
{
	Assert( m_Size > 0 );
	return m_Memory[ RandomInt( 0, m_Size - 1 ) ];
}

template< typename T, class A >
inline const T& CUtlVector<T, A>::Random() const
{
	Assert( m_Size > 0 );
	return m_Memory[ RandomInt( 0, m_Size - 1 ) ];
}


//-----------------------------------------------------------------------------
// Shuffle - Knuth/Fisher-Yates
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::Shuffle( IUniformRandomStream* pSteam )
{
	for ( int i = 0; i < m_Size; i++ )
	{
		int j = pSteam ? pSteam->RandomInt( i, m_Size - 1 ) : RandomInt( i, m_Size - 1 );
		if ( i != j )
		{
			V_swap( m_Memory[ i ], m_Memory[ j ] );
		}
	}
}

template< typename T, class A >
inline int CUtlVector<T, A>::Count() const
{
	return m_Size;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Reverse - reverse the order of elements, akin to std::vector<>::reverse()
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::Reverse( )
{
	for ( int i = 0; i < m_Size / 2; i++ )
	{
		V_swap( m_Memory[ i ], m_Memory[ m_Size - 1 - i ] );
#if defined( UTLVECTOR_TRACK_STACKS )
		if ( bTrackingEnabled )
		{
			V_swap( m_pElementStackStatsIndices[ i ], m_pElementStackStatsIndices[ m_Size - 1 - i ] );
		}
#endif
	}
}


//-----------------------------------------------------------------------------
// Is element index valid?
//-----------------------------------------------------------------------------
template< typename T, class A >
inline bool CUtlVector<T, A>::IsValidIndex( int i ) const
{
	return (i >= 0) && (i < m_Size);
}
 

//-----------------------------------------------------------------------------
// Returns in invalid index
//-----------------------------------------------------------------------------
template< typename T, class A >
inline int CUtlVector<T, A>::InvalidIndex()
{
	return -1;
}


//-----------------------------------------------------------------------------
// Grows the vector
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::GrowVector( int num )
{
	if (m_Size + num > m_Memory.NumAllocated())
	{
		MEM_ALLOC_CREDIT_CLASS();
		m_Memory.Grow( m_Size + num - m_Memory.NumAllocated() );
	}

	m_Size += num;
	ResetDbgInfo();
}


//-----------------------------------------------------------------------------
// Sorts the vector
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::Sort( int (__cdecl *pfnCompare)(const T *, const T *) )
{
	typedef int (__cdecl *QSortCompareFunc_t)(const void *, const void *);
	if ( Count() <= 1 )
		return;

	if ( Base() )
	{
		qsort( Base(), Count(), sizeof(T), (QSortCompareFunc_t)(pfnCompare) );
	}
	else
	{
		Assert( 0 );
		// this path is untested
		// if you want to sort vectors that use a non-sequential memory allocator,
		// you'll probably want to patch in a quicksort algorithm here
		// I just threw in this bubble sort to have something just in case...

		for ( int i = m_Size - 1; i >= 0; --i )
		{
			for ( int j = 1; j <= i; ++j )
			{
				if ( pfnCompare( &Element( j - 1 ), &Element( j ) ) < 0 )
				{
					V_swap( Element( j - 1 ), Element( j ) );
				}
			}
		}
	}
}


//----------------------------------------------------------------------------------------------
// Private function that does the in-place quicksort for non-contiguously allocated vectors.
//----------------------------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::InPlaceQuickSort_r( int (__cdecl *pfnCompare)(const T *, const T *), int nLeft, int nRight )
{
	int nPivot;
	int nLeftIdx = nLeft;
	int nRightIdx = nRight;

	if ( nRight - nLeft > 0 )
	{
		nPivot = ( nLeft + nRight ) / 2;

		while ( ( nLeftIdx <= nPivot ) && ( nRightIdx >= nPivot ) )
		{
			while ( ( pfnCompare( &Element( nLeftIdx ), &Element( nPivot ) ) < 0 ) && ( nLeftIdx <= nPivot ) )
			{
				nLeftIdx++;
			}

			while ( ( pfnCompare( &Element( nRightIdx ), &Element( nPivot ) ) > 0 ) && ( nRightIdx >= nPivot ) )
			{
				nRightIdx--;
			}

			V_swap( Element( nLeftIdx ), Element( nRightIdx ) );

			nLeftIdx++;
			nRightIdx--;

			if ( ( nLeftIdx - 1 ) == nPivot )
			{
				nPivot = nRightIdx = nRightIdx + 1;
			}
			else if ( nRightIdx + 1 == nPivot )
			{
				nPivot = nLeftIdx = nLeftIdx - 1;
			}
		}

		InPlaceQuickSort_r( pfnCompare, nLeft, nPivot - 1 );
		InPlaceQuickSort_r( pfnCompare, nPivot + 1, nRight );
	}
}


//----------------------------------------------------------------------------------------------
// Call this to quickly sort non-contiguously allocated vectors. Sort uses a slower bubble sort.
//----------------------------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::InPlaceQuickSort( int (__cdecl *pfnCompare)(const T *, const T *) )
{
	InPlaceQuickSort_r( pfnCompare, 0, Count() - 1 );
}

template< typename T, class A >
void CUtlVector<T, A>::Sort( void )
{
	//STACK STATS TODO: Do we care about allocation tracking precision enough to match element origins across a sort?
	std::sort( Base(), Base() + Count() );
}

template< typename T, class A >
template <class F>
void CUtlVector<T, A>::SortPredicate( F &&predicate )
{
	std::sort( Base(), Base() + Count(), predicate );
}

//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::EnsureCapacity( int num )
{
	MEM_ALLOC_CREDIT_CLASS();
	m_Memory.EnsureCapacity(num);
	ResetDbgInfo();
}


//-----------------------------------------------------------------------------
// Makes sure we have at least this many elements
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::EnsureCount( int num )
{
	if (Count() < num)
	{
		AddMultipleToTail( num - Count() );
	}
}


//-----------------------------------------------------------------------------
// Shifts elements
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::ShiftElementsRight( int elem, int num )
{
	Assert( IsValidIndex(elem) || ( m_Size == 0 ) || ( num == 0 ));
	int numToMove = m_Size - elem - num;
	if ((numToMove > 0) && (num > 0))
		Q_memmove( &Element(elem+num), &Element(elem), numToMove * sizeof(T) );
}

template< typename T, class A >
void CUtlVector<T, A>::ShiftElementsLeft( int elem, int num )
{
	Assert( IsValidIndex(elem) || ( m_Size == 0 ) || ( num == 0 ));
	int numToMove = m_Size - elem - num;
	if ((numToMove > 0) && (num > 0))
	{
		Q_memmove( &Element(elem), &Element(elem+num), numToMove * sizeof(T) );

#ifdef _DEBUG
		Q_memset( &Element(m_Size-num), 0xDD, num * sizeof(T) );
#endif
	}
}


//-----------------------------------------------------------------------------
// Adds an element, uses default constructor
//-----------------------------------------------------------------------------
template< typename T, class A >
inline int CUtlVector<T, A>::AddToHead()
{
	return InsertBefore(0);
}

template< typename T, class A >
inline int CUtlVector<T, A>::AddToTail()
{
	return InsertBefore( m_Size );
}

template< typename T, class A >
inline T *CUtlVector<T, A>::AddToTailGetPtr()
{
	return &Element( AddToTail() );
}

template< typename T, class A >
inline int CUtlVector<T, A>::InsertAfter( int elem )
{
	return InsertBefore( elem + 1 );
}

template< typename T, class A >
int CUtlVector<T, A>::InsertBefore( int elem )
{
	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector();
	ShiftElementsRight(elem);
	Construct( &Element(elem) );
	return elem;
}


//-----------------------------------------------------------------------------
// Adds an element, uses copy constructor
//-----------------------------------------------------------------------------
template< typename T, class A >
inline int CUtlVector<T, A>::AddToHead( const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 
	return InsertBefore( 0, src );
}

template< typename T, class A >
inline int CUtlVector<T, A>::AddToTail( const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 
	return InsertBefore( m_Size, src );
}

template< typename T, class A >
inline int CUtlVector<T, A>::InsertAfter( int elem, const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 
	return InsertBefore( elem + 1, src );
}

template< typename T, class A >
int CUtlVector<T, A>::InsertBefore( int elem, const T& src )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || (&src < Base()) || (&src >= (Base() + Count()) ) ); 

	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector();
	ShiftElementsRight(elem);
	CopyConstruct( &Element(elem), src );
	return elem;
}


//-----------------------------------------------------------------------------
// Adds multiple elements, uses default constructor
//-----------------------------------------------------------------------------
template< typename T, class A >
inline int CUtlVector<T, A>::AddMultipleToHead( int num )
{
	return InsertMultipleBefore( 0, num );
}

template< typename T, class A >
inline int CUtlVector<T, A>::AddMultipleToTail( int num )
{
	return InsertMultipleBefore( m_Size, num );
}

template< typename T, class A >
inline int CUtlVector<T, A>::AddMultipleToTail( int num, const T *pToCopy )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || !pToCopy || (pToCopy + num <= Base()) || (pToCopy >= (Base() + Count()) ) ); 

	return InsertMultipleBefore( m_Size, num, pToCopy );
}

template< typename T, class A >
int CUtlVector<T, A>::InsertMultipleAfter( int elem, int num )
{
	return InsertMultipleBefore( elem + 1, num );
}


template< typename T, class A >
void CUtlVector<T, A>::SetCount( int count )
{
	RemoveAll();
	AddMultipleToTail( count );
}

template< typename T, class A >
inline void CUtlVector<T, A>::SetSize( int size )
{
	SetCount( size );
}

template< typename T, class A >
void CUtlVector<T, A>::SetCountNonDestructively( int count )
{
	int delta = count - m_Size;
	if(delta > 0) AddMultipleToTail( delta );
	else if(delta < 0) RemoveMultipleFromTail( -delta );
}

template< typename T, class A >
void CUtlVector<T, A>::CopyArray( const T *pArray, int size )
{
	// Can't insert something that's in the list... reallocation may hose us
	Assert( (Base() == NULL) || !pArray || (Base() >= (pArray + size)) || (pArray >= (Base() + Count()) ) ); 

	SetSize( size );
	for( int i=0; i < size; i++ )
	{
		(*this)[i] = pArray[i];
	}
}

template< typename T, class A >
void CUtlVector<T, A>::Swap( CUtlVector< T, A > &vec )
{
	m_Memory.Swap( vec.m_Memory );
	V_swap( m_Size, vec.m_Size );

#ifndef _X360
	V_swap( m_pElements, vec.m_pElements );
#endif
}

template< typename T, class A >
int CUtlVector<T, A>::AddVectorToTail( CUtlVector const &src )
{
	Assert( &src != this );

	int base = Count();
	
	// Make space.
	int nSrcCount = src.Count();
	EnsureCapacity( base + nSrcCount );

	// Copy the elements.	
	m_Size += nSrcCount;
	for ( int i=0; i < nSrcCount; i++ )
	{
		CopyConstruct( &Element(base+i), src[i] );
	}
	return base;
}

template< typename T, class A >
inline int CUtlVector<T, A>::InsertMultipleBefore( int elem, int num )
{
	if( num == 0 )
		return elem;

	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector(num);
	ShiftElementsRight( elem, num );

	// Invoke default constructors
	for (int i = 0; i < num; ++i )
	{
		Construct( &Element( elem+i ) );
	}

	return elem;
}

template< typename T, class A >
inline int CUtlVector<T, A>::InsertMultipleBefore( int elem, int num, const T *pToInsert )
{
	if( num == 0 )
		return elem;
	
	// Can insert at the end
	Assert( (elem == Count()) || IsValidIndex(elem) );

	GrowVector(num);
	ShiftElementsRight( elem, num );

	// Invoke default constructors
	if ( !pToInsert )
	{
		for (int i = 0; i < num; ++i )
		{
			Construct( &Element( elem+i ) );
		}
	}
	else
	{
		for ( int i=0; i < num; i++ )
		{
			CopyConstruct( &Element( elem+i ), pToInsert[i] );
		}
	}

	return elem;
}


//-----------------------------------------------------------------------------
// Finds an element (element needs operator== defined)
//-----------------------------------------------------------------------------
template< typename T, class A >
int CUtlVector<T, A>::Find( const T& src ) const
{
	for ( int i = 0; i < Count(); ++i )
	{
		if (Element(i) == src)
			return i;
	}
	return -1;
}

//-----------------------------------------------------------------------------
// Finds an element using a predicate, using std::find_if
//-----------------------------------------------------------------------------
template< typename T, class A >
template< class F >
int CUtlVector<T, A>::FindPredicate( F &&predicate ) const
{
	const T * begin = Base();
	const T * end = begin + Count();
	const T * const &elem = std::find_if( begin, end, predicate );

	if ( elem != end )
	{
		int idx = (int)std::distance( begin, elem );
		StagingUtlVectorBoundsCheck( idx, m_Size );
		return idx;
	}

	return InvalidIndex();
}

template< typename T, class A >
void CUtlVector<T, A>::FillWithValue( const T& src )
{
	for ( int i = 0; i < Count(); i++ )
	{
		Element(i) = src;
	}
}

template< typename T, class A >
bool CUtlVector<T, A>::HasElement( const T& src ) const
{
	return ( Find(src) >= 0 );
}


//-----------------------------------------------------------------------------
// Element removal
//-----------------------------------------------------------------------------
template< typename T, class A >
void CUtlVector<T, A>::FastRemove( int elem )
{
	Assert( IsValidIndex(elem) );

	// Global scope to resolve conflict with Scaleform 4.0
	::Destruct( &Element(elem) );
	if (m_Size > 0)
	{
		if ( elem != m_Size -1 )
			memcpy( &Element(elem), &Element(m_Size-1), sizeof(T) );
		--m_Size;
	}
}

template< typename T, class A >
void CUtlVector<T, A>::Remove( int elem )
{
	// Global scope to resolve conflict with Scaleform 4.0
	::Destruct( &Element(elem) );
	ShiftElementsLeft(elem);
	--m_Size;
}

template< typename T, class A >
bool CUtlVector<T, A>::FindAndRemove( const T& src )
{
	int elem = Find( src );
	if ( elem != -1 )
	{
		Remove( elem );
		return true;
	}
	return false;
}

template< typename T, class A >
bool CUtlVector<T, A>::FindAndFastRemove( const T& src )
{
	int elem = Find( src );
	if ( elem != -1 )
	{
		FastRemove( elem );
		return true;
	}
	return false;
}

template< typename T, class A >
void CUtlVector<T, A>::RemoveMultiple( int elem, int num )
{
	Assert( elem >= 0 );
	Assert( elem + num <= Count() );

	// Global scope to resolve conflict with Scaleform 4.0
	for (int i = elem + num; --i >= elem; )
		::Destruct(&Element(i));

	ShiftElementsLeft(elem, num);
	m_Size -= num;
}

template< typename T, class A >
void CUtlVector<T, A>::RemoveMultipleFromHead( int num )
{
	Assert( num <= Count() );

	// Global scope to resolve conflict with Scaleform 4.0
	for (int i = num; --i >= 0; )
		::Destruct(&Element(i));

	ShiftElementsLeft(0, num);
	m_Size -= num;
}

template< typename T, class A >
void CUtlVector<T, A>::RemoveMultipleFromTail( int num )
{
	Assert( num <= Count() );

	// Global scope to resolve conflict with Scaleform 4.0
	for (int i = m_Size-num; i < m_Size; i++)
		::Destruct(&Element(i));

	m_Size -= num;
}

template< typename T, class A >
void CUtlVector<T, A>::RemoveAll()
{
	for (int i = m_Size; --i >= 0; )
	{
		// Global scope to resolve conflict with Scaleform 4.0
		::Destruct(&Element(i));
	}

	m_Size = 0;
}


//-----------------------------------------------------------------------------
// Memory deallocation
//-----------------------------------------------------------------------------

template< typename T, class A >
inline void CUtlVector<T, A>::Purge()
{
	RemoveAll();
	m_Memory.Purge();
	ResetDbgInfo();
}


template< typename T, class A >
inline void CUtlVector<T, A>::PurgeAndDeleteElements()
{
	for( int i=0; i < m_Size; i++ )
	{
		delete Element(i);
	}
	Purge();
}

template< typename T, class A >
inline void CUtlVector<T, A>::Compact()
{
	m_Memory.Purge(m_Size);
}

template< typename T, class A >
inline int CUtlVector<T, A>::NumAllocated() const
{
	return m_Memory.NumAllocated();
}


//-----------------------------------------------------------------------------
// Data and memory validation
//-----------------------------------------------------------------------------
#ifdef DBGFLAG_VALIDATE
template< typename T, class A >
void CUtlVector<T, A>::Validate( CValidator &validator, char *pchName )
{
	validator.Push( typeid(*this).name(), this, pchName );

	m_Memory.Validate( validator, "m_Memory" );

	validator.Pop();
}
#endif // DBGFLAG_VALIDATE

// A vector class for storing pointers, so that the elements pointed to by the pointers are deleted
// on exit.
template<class T> class CUtlVectorAutoPurge : public CUtlVector< T, CUtlMemory< T, int> >
{
public:
	~CUtlVectorAutoPurge( void )
	{
		this->PurgeAndDeleteElements();
	}

};

// easy string list class with dynamically allocated strings. For use with V_SplitString, etc.
// Frees the dynamic strings in destructor.
class CUtlStringList : public CUtlVectorAutoPurge< char *>
{
public:
	void CopyAndAddToTail( char const *pString )			// clone the string and add to the end
	{
		char *pNewStr = new char[1 + strlen( pString )];
		V_strcpy( pNewStr, pString );
		AddToTail( pNewStr );
	}

	static int __cdecl SortFunc( char * const * sz1, char * const * sz2 )
	{
		return strcmp( *sz1, *sz2 );
	}

	CUtlStringList(){}

	CUtlStringList( char const *pString, char const *pSeparator )
	{
		SplitString( pString, pSeparator );
	}

	CUtlStringList( char const *pString, const char **pSeparators, int nSeparators )
	{
		SplitString2( pString, pSeparators, nSeparators );
	}

	void SplitString( char const *pString, char const *pSeparator )
	{
		V_SplitString( pString, pSeparator, *this );
	}

	void SplitString2( char const *pString, const char **pSeparators, int nSeparators )
	{
		V_SplitString2( pString, pSeparators, nSeparators, *this );
	}
private:
	CUtlStringList( const CUtlStringList &other ); // copying directly will cause double-release of the same strings; maybe we need to do a deep copy, but unless and until such need arises, this will guard against double-release
};



// <Sergiy> placing it here a few days before Cert to minimize disruption to the rest of codebase
class CSplitString: public CUtlVector<char*, CUtlMemory<char*, int> >
{
public:
	CSplitString() { m_szBuffer = nullptr; }
	CSplitString(const char *pString, const char *pSeparator);
	CSplitString(const char *pString, const char **pSeparators, int nSeparators);
	~CSplitString();
	//
	// NOTE: If you want to make Construct() public and implement Purge() here, you'll have to free m_szBuffer there
	//

	void Set( const char *pString, const char **pSeparators, int nSeparators );
	void Set( const char *pString, const char *pSeparator );

private:
	void Construct(const char *pString, const char **pSeparators, int nSeparators);
	void PurgeAndDeleteElements();
private:
	char *m_szBuffer; // a copy of original string, with '\0' instead of separators
};

inline void CSplitString::Set( const char* pString, const char* pSeparator )
{
	Set( pString, &pSeparator, 1 );
}

inline void CSplitString::Set( const char *pString, const char **pSeparators, int nSeparators )
{
	if ( m_szBuffer )
		delete[] m_szBuffer;
	Construct( pString, pSeparators, nSeparators );
}

// A fixed growable vector that's castable to CUtlVector
template< class T, size_t FIXED_SIZE >
class CUtlVectorFixedGrowableCompat : public CUtlVector< T >
{
	typedef CUtlVector< T > BaseClass;

public:
	// constructor, destructor
	CUtlVectorFixedGrowableCompat(int growSize = 0) : BaseClass(nullptr, FIXED_SIZE, growSize)
	{
		this->m_Memory.m_pMemory = m_FixedMemory.Base();
	}

	AlignedByteArray_t< FIXED_SIZE, T > m_FixedMemory;
};

#endif // CCVECTOR_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Thread-safe hash class
//===========================================================================//

#ifndef UTLTSHASH_H
#define UTLTSHASH_H

#ifdef _WIN32
#pragma once
#endif

#include <limits.h>
#include "tier0/threadtools.h"
#include "tier1/mempool.h"
#include "generichash.h"


//=============================================================================
// 
// Threadsafe Hash
//
// Number of buckets must be a power of 2.
// Key must be intp sized (32-bits on x32, 64-bits on x64)
// Designed for a usage pattern where the data is semi-static, and there
// is a well-defined point where we are guaranteed no queries are occurring.
// 
// Insertions are added into a thread-safe list, and when Commit() is called,
// the insertions are moved into a lock-free list
//
// Elements are never individually removed; clears must occur at a time
// where we and guaranteed no queries are occurring
//
typedef intp UtlTSHashHandle_t;

template < class T >
abstract_class ITSHashConstructor
{
public:
	virtual void Construct( T* pElement ) = 0;
};

template < class T >
class CDefaultTSHashConstructor : public ITSHashConstructor< T >
{
public:
	virtual void Construct( T* pElement )
	{
		::Construct( pElement );
	}
};

template < int BUCKET_COUNT, class KEYTYPE = intp >
class CUtlTSHashGenericHash
{
public:
	static int Hash( const KEYTYPE &key, int nBucketMask )
	{
		int nHash = HashIntConventional( (intp)key );
		if ( BUCKET_COUNT <= USHRT_MAX )
		{
			nHash ^= ( nHash >> 16 );
		}
		if ( BUCKET_COUNT <= UCHAR_MAX )
		{
			nHash ^= ( nHash >> 8 );
		}
		return ( nHash & nBucketMask );
	}

	static bool Compare( const KEYTYPE &lhs, const KEYTYPE &rhs )
	{
		return lhs == rhs;
	}
};

template < int BUCKET_COUNT, class KEYTYPE >
class CUtlTSHashUseKeyHashMethod
{
public:
	static int Hash( const KEYTYPE &key, int nBucketMask )
	{
		uint32 nHash = key.HashValue();
		return ( nHash & nBucketMask );
	}

	static bool Compare( const KEYTYPE &lhs, const KEYTYPE &rhs )
	{
		return lhs == rhs;
	}
};

template< class T, int BUCKET_COUNT, class KEYTYPE = intp, class HashFuncs = CUtlTSHashGenericHash< BUCKET_COUNT, KEYTYPE >, int nAlignment = 0 > 
class CUtlTSHash
{
public:
	// Constructor/Deconstructor.
	CUtlTSHash( int nAllocationCount );
	~CUtlTSHash();

	// Invalid handle.
	static UtlTSHashHandle_t InvalidHandle( void )	{ return ( UtlTSHashHandle_t )0; }

	// Retrieval. Super fast, is thread-safe
	UtlTSHashHandle_t Find( KEYTYPE uiKey );

	// Insertion ( find or add ).
	UtlTSHashHandle_t Insert( KEYTYPE uiKey, const T &data, bool *pDidInsert = NULL );
	UtlTSHashHandle_t Insert( KEYTYPE uiKey, ITSHashConstructor<T> *pConstructor, bool *pDidInsert = NULL );

	// This insertion method assumes the element is not in the hash table, skips 
	UtlTSHashHandle_t FastInsert( KEYTYPE uiKey, const T &data );
	UtlTSHashHandle_t FastInsert( KEYTYPE uiKey, ITSHashConstructor<T> *pConstructor );

	// Commit recent insertions, making finding them faster.
	// Only call when you're certain no threads are accessing the hash table
	void Commit( );

	// Removal.	Only call when you're certain no threads are accessing the hash table
	void FindAndRemove( KEYTYPE uiKey );
	void Remove( UtlTSHashHandle_t hHash ) { FindAndRemove( GetID( hHash ) ); }
	void RemoveAll( void );
	void Purge( void );

	// Returns the number of elements in the hash table
	int Count() const;

	// Returns elements in the table
	int GetElements( int nFirstElement, int nCount, UtlTSHashHandle_t *pHandles ) const;

	// Element access
	T &Element( UtlTSHashHandle_t hHash );
	T const &Element( UtlTSHashHandle_t hHash ) const;
	T &operator[]( UtlTSHashHandle_t hHash );
	T const &operator[]( UtlTSHashHandle_t hHash ) const;
	KEYTYPE GetID( UtlTSHashHandle_t hHash ) const;

	// Convert element * to hashHandle
	UtlTSHashHandle_t ElementPtrToHandle( T* pElement ) const;

private:
	// Templatized for memory tracking purposes
	template < typename Data_t >
	struct HashFixedDataInternal_t
	{
		KEYTYPE	m_uiKey;
		HashFixedDataInternal_t< Data_t >*	m_pNext;
		Data_t	m_Data;
	};

	typedef HashFixedDataInternal_t<T> HashFixedData_t;

	enum
	{
		BUCKET_MASK = BUCKET_COUNT - 1
	};

	struct HashBucket_t
	{
		HashFixedData_t *m_pFirst;
		HashFixedData_t *m_pFirstUncommitted;
		CThreadSpinRWLock m_AddLock;
	};

	UtlTSHashHandle_t Find( KEYTYPE uiKey, HashFixedData_t *pFirstElement, HashFixedData_t *pLastElement );
	UtlTSHashHandle_t InsertUncommitted( KEYTYPE uiKey, HashBucket_t &bucket );
	CMemoryPoolMT m_EntryMemory;
	HashBucket_t m_aBuckets[BUCKET_COUNT];
	bool m_bNeedsCommit;

#ifdef _DEBUG
	CInterlockedInt m_ContentionCheck;
#endif
};


//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::CUtlTSHash( int nAllocationCount ) :
	m_EntryMemory( sizeof( HashFixedData_t ), nAllocationCount, CUtlMemoryPool::GROW_SLOW, MEM_ALLOC_CLASSNAME( HashFixedData_t ), nAlignment )
{
#ifdef _DEBUG
	m_ContentionCheck = 0;
#endif
	m_bNeedsCommit = false;
	for ( int i = 0; i < BUCKET_COUNT; i++ )
	{
		HashBucket_t &bucket = m_aBuckets[ i ];
		bucket.m_pFirst = NULL;
		bucket.m_pFirstUncommitted = NULL;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::~CUtlTSHash()
{
#ifdef _DEBUG
	if ( m_ContentionCheck != 0 )
	{
		DebuggerBreak();
	}
#endif
	Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Destroy dynamically allocated hash data.
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline void CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Purge( void )
{
	RemoveAll();
}


//-----------------------------------------------------------------------------
// Returns the number of elements in the hash table
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline int CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Count() const
{
	return m_EntryMemory.Count();
}


//-----------------------------------------------------------------------------
// Returns elements in the table
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
int CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::GetElements( int nFirstElement, int nCount, UtlTSHashHandle_t *pHandles ) const
{
	int nIndex = 0;
	for ( int i = 0; i < BUCKET_COUNT; i++ )
	{
		const HashBucket_t &bucket = m_aBuckets[ i ];
		bucket.m_AddLock.LockForRead( );
		for ( HashFixedData_t *pElement = bucket.m_pFirstUncommitted; pElement; pElement = pElement->m_pNext )
		{
			if ( --nFirstElement >= 0 )
				continue;

			pHandles[ nIndex++ ] = (UtlTSHashHandle_t)pElement;
			if ( nIndex >= nCount )
			{
				bucket.m_AddLock.UnlockRead( );
				return nIndex;
			}
		}
		bucket.m_AddLock.UnlockRead( );
	}
	return nIndex;
}


//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (KEYTYPE),
//          without a check to see if the element already exists within the tree.
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::InsertUncommitted( KEYTYPE uiKey, HashBucket_t &bucket )
{
	m_bNeedsCommit = true;
	HashFixedData_t *pNewElement = static_cast< HashFixedData_t * >( m_EntryMemory.Alloc() );
	pNewElement->m_pNext = bucket.m_pFirstUncommitted;
	bucket.m_pFirstUncommitted = pNewElement;
	pNewElement->m_uiKey = uiKey;
	return (UtlTSHashHandle_t)pNewElement;	
}


//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key, with
//          a check to see if the element already exists within the tree.
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Insert( KEYTYPE uiKey, const T &data, bool *pDidInsert )
{
#ifdef _DEBUG
	if ( m_ContentionCheck != 0 )
	{
		DebuggerBreak();
	}
#endif

	if ( pDidInsert ) 
	{
		*pDidInsert = false;
	}

	int iBucket = HashFuncs::Hash( uiKey, BUCKET_MASK );
	HashBucket_t &bucket = m_aBuckets[ iBucket ];

	// First try lock-free
	UtlTSHashHandle_t h = Find( uiKey );
	if ( h != InvalidHandle() )
		return h;

	// Now, try again, but only look in uncommitted elements
	bucket.m_AddLock.LockForWrite( );

	h = Find( uiKey, bucket.m_pFirstUncommitted, bucket.m_pFirst );
	if ( h == InvalidHandle() )
	{
		h = InsertUncommitted( uiKey, bucket );
		CopyConstruct( &Element(h), data );
		if ( pDidInsert ) 
		{
			*pDidInsert = true;
		}
	}

	bucket.m_AddLock.UnlockWrite( );
	return h;
}

template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Insert( KEYTYPE uiKey, ITSHashConstructor<T> *pConstructor, bool *pDidInsert )
{
#ifdef _DEBUG
	if ( m_ContentionCheck != 0 )
	{
		DebuggerBreak();
	}
#endif

	if ( pDidInsert ) 
	{
		*pDidInsert = false;
	}

	// First try lock-free
	UtlTSHashHandle_t h = Find( uiKey );
	if ( h != InvalidHandle() )
		return h;

	// Now, try again, but only look in uncommitted elements
	int iBucket = HashFuncs::Hash( uiKey, BUCKET_MASK );
	HashBucket_t &bucket = m_aBuckets[ iBucket ];
	bucket.m_AddLock.LockForWrite( );

	h = Find( uiKey, bucket.m_pFirstUncommitted, bucket.m_pFirst );
	if ( h == InvalidHandle() )
	{
		// Useful if non-trivial work needs to happen to make data; don't want to
		// do it and then have to undo it if it turns out we don't need to add it
		h = InsertUncommitted( uiKey, bucket );
		pConstructor->Construct( &Element(h) );
		if ( pDidInsert ) 
		{
			*pDidInsert = true;
		}
	}

	bucket.m_AddLock.UnlockWrite( );
	return h;
}


//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key
//          without a check to see if the element already exists within the tree.
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::FastInsert( KEYTYPE uiKey, const T &data )
{
#ifdef _DEBUG
	if ( m_ContentionCheck != 0 )
	{
		DebuggerBreak();
	}
#endif
	int iBucket = HashFuncs::Hash( uiKey, BUCKET_MASK );
	HashBucket_t &bucket = m_aBuckets[ iBucket ];
	bucket.m_AddLock.LockForWrite( );
	UtlTSHashHandle_t h = InsertUncommitted( uiKey, bucket );
	CopyConstruct( &Element(h), data );
	bucket.m_AddLock.UnlockWrite( );
	return h;
}

template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::FastInsert( KEYTYPE uiKey, ITSHashConstructor<T> *pConstructor )
{
#ifdef _DEBUG
	if ( m_ContentionCheck != 0 )
	{
		DebuggerBreak();
	}
#endif
	int iBucket = HashFuncs::Hash( uiKey, BUCKET_MASK );
	HashBucket_t &bucket = m_aBuckets[ iBucket ];
	bucket.m_AddLock.LockForWrite( );
	UtlTSHashHandle_t h = InsertUncommitted( uiKey, bucket );
	pConstructor->Construct( &Element(h) );
	bucket.m_AddLock.UnlockWrite( );
	return h;
}


//-----------------------------------------------------------------------------
// Purpose: Commits all uncommitted insertions
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline void CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Commit( )
{
	// FIXME: Is this legal? Want this to be lock-free
	if ( !m_bNeedsCommit )
		return;

	// This must occur when no queries are occurring
#ifdef _DEBUG
	m_ContentionCheck++;
#endif

	for ( int i = 0; i < BUCKET_COUNT; i++ )
	{
		HashBucket_t &bucket = m_aBuckets[ i ];
		bucket.m_AddLock.LockForRead( );
		bucket.m_pFirst = bucket.m_pFirstUncommitted;
		bucket.m_AddLock.UnlockRead( );
	}

	m_bNeedsCommit = false;

#ifdef _DEBUG
	m_ContentionCheck--;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Remove a single element from the hash
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline void CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::FindAndRemove( KEYTYPE uiKey )
{
	if ( m_EntryMemory.Count() == 0 )
		return;

	// This must occur when no queries are occurring
#ifdef _DEBUG
	m_ContentionCheck++;
#endif

	int iBucket = HashFuncs::Hash( uiKey, BUCKET_MASK );
	HashBucket_t &bucket = m_aBuckets[ iBucket ];
	bucket.m_AddLock.LockForWrite( );

	HashFixedData_t *pPrev = NULL;
	for ( HashFixedData_t *pElement = bucket.m_pFirstUncommitted; pElement; pPrev = pElement, pElement = pElement->m_pNext )
	{
		if ( !HashFuncs::Compare( pElement->m_uiKey, uiKey ) )
			continue;

		if ( pPrev )
		{
			pPrev->m_pNext = pElement->m_pNext;
		}
		else
		{
			bucket.m_pFirstUncommitted = pElement->m_pNext;
		}

		if ( bucket.m_pFirst == pElement )
		{
			bucket.m_pFirst = bucket.m_pFirst->m_pNext;
		}

		Destruct( &pElement->m_Data );

#ifdef _DEBUG
		memset( pElement, 0xDD, sizeof(HashFixedData_t) );
#endif

		m_EntryMemory.Free( pElement );

		break;
	}

	bucket.m_AddLock.UnlockWrite( );

#ifdef _DEBUG
	m_ContentionCheck--;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Remove all elements from the hash
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline void CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::RemoveAll( void )
{
	m_bNeedsCommit = false;
	if ( m_EntryMemory.Count() == 0 )
		return;

	// This must occur when no queries are occurring
#ifdef _DEBUG
	m_ContentionCheck++;
#endif

	for ( int i = 0; i < BUCKET_COUNT; i++ )
	{
		HashBucket_t &bucket = m_aBuckets[ i ];

		bucket.m_AddLock.LockForWrite( );

		for ( HashFixedData_t *pElement = bucket.m_pFirstUncommitted; pElement; pElement = pElement->m_pNext )
		{
			Destruct( &pElement->m_Data );
		}

		bucket.m_pFirst = NULL;
		bucket.m_pFirstUncommitted = NULL;
		bucket.m_AddLock.UnlockWrite( );
	}

	m_EntryMemory.Clear();

#ifdef _DEBUG
	m_ContentionCheck--;
#endif
}

//-----------------------------------------------------------------------------
// Finds an element, but only in the committed elements
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Find( KEYTYPE uiKey, HashFixedData_t *pFirstElement, HashFixedData_t *pLastElement )
{
#ifdef _DEBUG
	if ( m_ContentionCheck != 0 )
	{
		DebuggerBreak();
	}
#endif

	for ( HashFixedData_t *pElement = pFirstElement; pElement != pLastElement; pElement = pElement->m_pNext )
	{
		if ( HashFuncs::Compare( pElement->m_uiKey, uiKey ) )
			return (UtlTSHashHandle_t)pElement;
	}
	return InvalidHandle();
}


//-----------------------------------------------------------------------------
// Finds an element, but only in the committed elements
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Find( KEYTYPE uiKey )
{
	int iBucket = HashFuncs::Hash( uiKey, BUCKET_MASK );
	const HashBucket_t &bucket = m_aBuckets[iBucket];
	UtlTSHashHandle_t h = Find( uiKey, bucket.m_pFirst, NULL );
	if ( h != InvalidHandle() )
		return h;

	// Didn't find it in the fast ( committed ) list. Let's try the slow ( uncommitted ) one
	bucket.m_AddLock.LockForRead( );
	h = Find( uiKey, bucket.m_pFirstUncommitted, bucket.m_pFirst );
	bucket.m_AddLock.UnlockRead( );

	return h;
}


//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline T &CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Element( UtlTSHashHandle_t hHash )
{
	return ((HashFixedData_t *)hHash)->m_Data;
}

template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline T const &CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::Element( UtlTSHashHandle_t hHash ) const
{
	return ((HashFixedData_t *)hHash)->m_Data;
}

template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline T &CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::operator[]( UtlTSHashHandle_t hHash )
{
	return ((HashFixedData_t *)hHash)->m_Data;
}

template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline T const &CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::operator[]( UtlTSHashHandle_t hHash ) const
{
	return ((HashFixedData_t *)hHash)->m_Data;
}


template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline KEYTYPE CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::GetID( UtlTSHashHandle_t hHash ) const
{
	return ((HashFixedData_t *)hHash)->m_uiKey;
}


// Convert element * to hashHandle
template<class T, int BUCKET_COUNT, class KEYTYPE, class HashFuncs, int nAlignment> 
inline UtlTSHashHandle_t CUtlTSHash<T,BUCKET_COUNT,KEYTYPE,HashFuncs,nAlignment>::ElementPtrToHandle( T* pElement ) const
{
	Assert( pElement );
	HashFixedData_t *pFixedData = (HashFixedData_t*)( (uint8*)pElement - offsetof( HashFixedData_t, m_Data ) );
	Assert( m_EntryMemory.IsAllocationWithinPool( pFixedData ) );
	return (UtlTSHashHandle_t)pFixedData;
}


#endif // UTLTSHASH_H

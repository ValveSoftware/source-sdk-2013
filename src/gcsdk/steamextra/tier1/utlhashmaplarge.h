//========= Copyright Valve Corporation, All rights reserved. =================//
//
// Purpose: index-based hash map container well suited for large and growing
//	datasets. It uses less memory than other hash maps and incrementally
//	rehashes to reduce reallocation spikes.
//
//=============================================================================//

#ifndef UTLHASHMAPLARGE_H
#define UTLHASHMAPLARGE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "bitvec.h"
#include "tier1/murmurhash3.h"

// fast mod for power of 2 numbers
namespace basetypes
{
template <class T>
inline bool IsPowerOf2(T n)
{
	return n > 0 && (n & (n-1)) == 0;
}

template <class T1, class T2>
inline T2 ModPowerOf2(T1 a, T2 b)
{
	return T2(a) & (b-1);
}
}

// default comparison operator
template <typename T>
class CDefEquals
{
public:
	CDefEquals() {}
	CDefEquals( int i ) {}
	inline bool operator()( const T &lhs, const T &rhs ) const { return ( lhs == rhs );	}
	inline bool operator!() const { return false; }
};


// Specialization to compare pointers
template <typename T>
class CDefEquals<T*>
{
public:
	CDefEquals() {}
	CDefEquals( int i ) {}
	inline bool operator()( const T *lhs, const T *rhs ) const 
	{ 
		if ( lhs == rhs	)
			return true;
		else if ( NULL == lhs || NULL == rhs )
			return false;
		else
			return ( *lhs == *rhs );	
	}
	inline bool operator!() const { return false; }
};


// Hash specialization for CUtlStrings
template<>
struct MurmurHash3Functor<CUtlString>
{
	typedef	uint32  TargetType ; 
	TargetType	operator()(const CUtlString &strKey) const
	{
		return MurmurHash3Functor<const char*>()( strKey.String() );
	}
};

//hash 3 function for a general case sensitive string compares
struct MurmurHash3ConstCharPtr
{
	typedef	uint32  TargetType ; 
	TargetType	operator()( const char* pszKey ) const	{ return MurmurHash3Functor<const char*>()( pszKey ); }
};
struct CaseSensitiveStrEquals
{
	bool operator()( const char* pszLhs, const char* pszRhs ) const	{ return strcmp( pszLhs, pszRhs ) == 0; }
};

//-----------------------------------------------------------------------------
//
// Purpose:	An associative container. Pretty much identical to CUtlMap without the ability to walk in-order
//	This container is well suited for large and growing datasets. It uses less 
//	memory than other hash maps and incrementally rehashes to reduce reallocation spikes.
//	However, it is slower (by about 20%) than CUtlHashTable
//
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L = CDefEquals<K>, typename H = MurmurHash3Functor<K> > 
class CUtlHashMapLarge
{
public:
	// This enum exists so that FOR_EACH_MAP and FOR_EACH_MAP_FAST cannot accidentally
	// be used on a type that is not a CUtlMap. If the code compiles then all is well.
	// The check for IsUtlMap being true should be free.
	// Using an enum rather than a static const bool ensures that this trick works even
	// with optimizations disabled on gcc.
	enum CompileTimeCheck
	{
		IsUtlMap = 1
	};

	typedef K KeyType_t;
	typedef T ElemType_t;
	typedef int IndexType_t;
	typedef L EqualityFunc_t;
	typedef H HashFunc_t;

	CUtlHashMapLarge()
	{
		m_cElements = 0;
		m_nMaxElement = 0;
		m_nMinRehashedBucket = InvalidIndex();
		m_nMaxRehashedBucket = InvalidIndex();
		m_iNodeFreeListHead = InvalidIndex();
	}

	CUtlHashMapLarge( int cElementsExpected )
	{
		m_cElements = 0;
		m_nMaxElement = 0;
		m_nMinRehashedBucket = InvalidIndex();
		m_nMaxRehashedBucket = InvalidIndex();
		m_iNodeFreeListHead = InvalidIndex();
		EnsureCapacity( cElementsExpected );
	}

	~CUtlHashMapLarge()
	{
		RemoveAll();
	}

	// gets particular elements
	ElemType_t &		Element( IndexType_t i )			{ return m_memNodes.Element( i ).m_elem; }
	const ElemType_t &	Element( IndexType_t i ) const		{ return m_memNodes.Element( i ).m_elem; }
	ElemType_t &		operator[]( IndexType_t i )			{ return m_memNodes.Element( i ).m_elem; }
	const ElemType_t &	operator[]( IndexType_t i ) const	{ return m_memNodes.Element( i ).m_elem; }
	KeyType_t &			Key( IndexType_t i )				{ return m_memNodes.Element( i ).m_key; }
	const KeyType_t &	Key( IndexType_t i ) const			{ return m_memNodes.Element( i ).m_key; }

	// Num elements
	IndexType_t Count() const								{ return m_cElements; }

	// Max "size" of the vector
	IndexType_t  MaxElement() const							{ return m_nMaxElement; }

	// Checks if a node is valid and in the map
	bool  IsValidIndex( IndexType_t i ) const				{ return i >= 0 && i < m_nMaxElement && !IsFreeNodeID( m_memNodes[i].m_iNextNode ); }

	// Invalid index
	static IndexType_t InvalidIndex()						{ return -1; }

	// Insert method
	IndexType_t  Insert( const KeyType_t &key, const ElemType_t &insert )						{ return InsertInternal( key, insert, eInsert_UpdateExisting ); }
	IndexType_t  Insert( const KeyType_t &key )													{ return InsertInternal( key, ElemType_t(), eInsert_UpdateExisting ); }
	IndexType_t  InsertWithDupes( const KeyType_t &key, const ElemType_t &insert )				{ return InsertInternal( key, insert, eInsert_CreateDupes ); }
	IndexType_t	 FindOrInsert( const KeyType_t &key, const ElemType_t &insert )					{ return InsertInternal( key, insert, eInsert_LeaveExisting ); }
	IndexType_t  InsertOrReplace( const KeyType_t &key, const ElemType_t &insert )				{ return InsertInternal( key, insert, eInsert_UpdateExisting ); }


	// Finds an element
	IndexType_t  Find( const KeyType_t &key ) const;

	// has an element
	bool HasElement( const KeyType_t &key ) const
	{
		return Find( key ) != InvalidIndex();
	}

	void EnsureCapacity( int num );

	void RemoveAt( IndexType_t i );
	bool Remove( const KeyType_t &key )
	{
		int iMap = Find( key );
		if ( iMap != InvalidIndex() )
		{
			RemoveAt( iMap );
			return true;
		}
		return false;
	}
	void RemoveAll();
	void Purge();
	void PurgeAndDeleteElements();

	void Swap( CUtlHashMapLarge<K,T,L,H> &rhs )
	{
		m_vecHashBuckets.Swap( rhs.m_vecHashBuckets );
		V_swap( m_bitsMigratedBuckets, rhs.m_bitsMigratedBuckets );
		m_memNodes.Swap( rhs.m_memNodes );
		V_swap( m_iNodeFreeListHead, rhs.m_iNodeFreeListHead );
		V_swap( m_cElements, rhs.m_cElements );
		V_swap( m_nMaxElement, rhs.m_nMaxElement );
		V_swap( m_nMinRehashedBucket, rhs.m_nMinRehashedBucket );
		V_swap( m_nMaxRehashedBucket, rhs.m_nMaxRehashedBucket );
		V_swap( m_EqualityFunc, rhs.m_EqualityFunc );
		V_swap( m_HashFunc, rhs.m_HashFunc );
	}

private:
	enum EInsertPolicy { eInsert_UpdateExisting, eInsert_LeaveExisting, eInsert_CreateDupes };
	IndexType_t InsertInternal( const KeyType_t &key, const ElemType_t &insert, EInsertPolicy ePolicy );

	inline IndexType_t FreeNodeIDToIndex( IndexType_t i ) const	{ return (0-i)-3; }
	inline IndexType_t FreeNodeIndexToID( IndexType_t i ) const	{ return (-3)-i; }
	inline bool IsFreeNodeID( IndexType_t i ) const				{ return i < InvalidIndex(); }

	int FindInBucket( int iBucket, const KeyType_t &key ) const;
	int AllocNode();
	void RehashNodesInBucket( int iBucket );
	void LinkNodeIntoBucket( int iBucket, int iNewNode );
	void UnlinkNodeFromBucket( int iBucket, int iNewNode );
	bool RemoveNodeFromBucket( int iBucket, int iNodeToRemove );
	void IncrementalRehash();

	struct HashBucket_t
	{
		IndexType_t m_iNode;
	};
	CUtlVector<HashBucket_t> m_vecHashBuckets;

	CLargeVarBitVec m_bitsMigratedBuckets;

	struct Node_t
	{
		KeyType_t m_key;
		ElemType_t m_elem;
		int m_iNextNode;
	};
	CUtlMemory<Node_t> m_memNodes;
	IndexType_t m_iNodeFreeListHead;

	IndexType_t m_cElements;
	IndexType_t m_nMaxElement;
	IndexType_t m_nMinRehashedBucket, m_nMaxRehashedBucket;
	EqualityFunc_t m_EqualityFunc;
	HashFunc_t m_HashFunc;
};


//-----------------------------------------------------------------------------
// Purpose: inserts an item into the map
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline int CUtlHashMapLarge<K,T,L,H>::InsertInternal( const KeyType_t &key, const ElemType_t &insert, EInsertPolicy ePolicy )
{
	// make sure we have room in the hash table
	if ( m_cElements >= m_vecHashBuckets.Count() )
		EnsureCapacity( MAX( 16, m_vecHashBuckets.Count() * 2 ) );
	if ( m_cElements >= m_memNodes.Count() )
		m_memNodes.Grow( m_memNodes.Count() * 2 );

	// rehash incrementally
	IncrementalRehash();

	// hash the item
	uint32 hash = m_HashFunc( key );

	// migrate data forward, if necessary
	int cBucketsToModAgainst = m_vecHashBuckets.Count() >> 1;
	int iBucket = basetypes::ModPowerOf2(hash, cBucketsToModAgainst);
	while ( iBucket >= m_nMinRehashedBucket
		&& !m_bitsMigratedBuckets.Get( iBucket ) )
	{
		RehashNodesInBucket( iBucket );
		cBucketsToModAgainst >>= 1;
		iBucket = basetypes::ModPowerOf2(hash, cBucketsToModAgainst);
	}

	// prevent duplicates if necessary
	if ( ( ePolicy != eInsert_CreateDupes ) && m_cElements )
	{
		// look in the bucket to see if we have a conflict
		int iBucket2 = basetypes::ModPowerOf2( hash, m_vecHashBuckets.Count() );
		IndexType_t iNode = FindInBucket( iBucket2, key );
		if ( iNode != InvalidIndex() )
		{
			// a duplicate - update in place (matching CUtlMap)
			if( ePolicy == eInsert_UpdateExisting )
			{
				m_memNodes[iNode].m_elem = insert;
			}
			return iNode;
		}
	}

	// make an item
	int iNewNode = AllocNode();
	m_memNodes[iNewNode].m_iNextNode = InvalidIndex();
	CopyConstruct( &m_memNodes[iNewNode].m_key, key );
	CopyConstruct( &m_memNodes[iNewNode].m_elem, insert );

	iBucket = basetypes::ModPowerOf2( hash, m_vecHashBuckets.Count() );

	// link ourselves in
	//	::OutputDebugStr( CFmtStr( "insert %d into bucket %d\n", key, iBucket ).Access() );
	LinkNodeIntoBucket( iBucket, iNewNode );

	// return the new node
	return iNewNode;
}

//-----------------------------------------------------------------------------
// Purpose: grows the map to fit the specified amount
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline void CUtlHashMapLarge<K,T,L,H>::EnsureCapacity( int amount )
{
	m_memNodes.EnsureCapacity( amount );
	// ::OutputDebugStr( CFmtStr( "grown m_memNodes from %d to %d\n", m_cElements, m_memNodes.Count() ).Access() );

	if ( amount <= m_vecHashBuckets.Count() )
		return;
	int cBucketsNeeded = MAX( 16, m_vecHashBuckets.Count() );
	while ( cBucketsNeeded < amount )
		cBucketsNeeded *= 2;

	// ::OutputDebugStr( CFmtStr( "grown m_vecHashBuckets from %d to %d\n", m_vecHashBuckets.Count(), cBucketsNeeded ).Access() );

	// grow the hash buckets
	int grow = cBucketsNeeded - m_vecHashBuckets.Count();
	int iFirst = m_vecHashBuckets.AddMultipleToTail( grow );
	// clear all the new data to invalid bits
	memset( &m_vecHashBuckets[iFirst], 0xFFFFFFFF, grow*sizeof(m_vecHashBuckets[iFirst]) );
	Assert( basetypes::IsPowerOf2( m_vecHashBuckets.Count() ) );

	// we'll have to rehash, all the buckets that existed before growth
	m_nMinRehashedBucket = 0;
	m_nMaxRehashedBucket = iFirst;
	if ( m_cElements > 0 )
	{
		// remove all the current bits
		m_bitsMigratedBuckets.Resize( 0 );
		// re-add new bits; these will all be reset to 0
		m_bitsMigratedBuckets.Resize( m_vecHashBuckets.Count() );
	}
	else
	{
		// no elements - no rehashing
		m_nMinRehashedBucket = m_vecHashBuckets.Count();
	}
}


//-----------------------------------------------------------------------------
// Purpose: gets a new node, from the free list if possible
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline int CUtlHashMapLarge<K,T,L,H>::AllocNode()
{
	// if we're out of free elements, get the max
	if ( m_cElements == m_nMaxElement )
	{
		m_cElements++;
		return m_nMaxElement++;
	}

	// pull from the free list
	Assert( m_iNodeFreeListHead != InvalidIndex() );
	int iNewNode = m_iNodeFreeListHead;
	m_iNodeFreeListHead = FreeNodeIDToIndex( m_memNodes[iNewNode].m_iNextNode );
	m_cElements++;
	return iNewNode;
}


//-----------------------------------------------------------------------------
// Purpose: takes a bucket of nodes and re-hashes them into a more optimal bucket
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline void CUtlHashMapLarge<K,T,L,H>::RehashNodesInBucket( int iBucketSrc )
{
	// mark us as migrated
	m_bitsMigratedBuckets.Set( iBucketSrc );

	// walk the list of items, re-hashing them
	IndexType_t iNode = m_vecHashBuckets[iBucketSrc].m_iNode;
	while ( iNode != InvalidIndex() )
	{
		IndexType_t iNodeNext = m_memNodes[iNode].m_iNextNode;
		Assert( iNodeNext != iNode );

		// work out where the node should go
		const KeyType_t &key = m_memNodes[iNode].m_key;
		uint32 hash = m_HashFunc( key );
		int iBucketDest = basetypes::ModPowerOf2( hash, m_vecHashBuckets.Count() );

		// if the hash bucket has changed, move it
		if ( iBucketDest != iBucketSrc )
		{
			//	::OutputDebugStr( CFmtStr( "moved key %d from bucket %d to %d\n", key, iBucketSrc, iBucketDest ).Access() );

			// remove from this bucket list
			UnlinkNodeFromBucket( iBucketSrc, iNode );

			// link into new bucket list
			LinkNodeIntoBucket( iBucketDest, iNode );
		}
		iNode = iNodeNext;
	}
}


//-----------------------------------------------------------------------------
// Purpose: searches for an item by key, returning the index handle
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline int CUtlHashMapLarge<K,T,L,H>::Find( const KeyType_t &key ) const
{
	if ( m_cElements == 0 )
		return InvalidIndex();

	// hash the item
	uint32 hash = m_HashFunc( key );

	// find the bucket
	int cBucketsToModAgainst = m_vecHashBuckets.Count();
	int iBucket = basetypes::ModPowerOf2( hash, cBucketsToModAgainst );

	// look in the bucket for the item
	int iNode = FindInBucket( iBucket, key );
	if ( iNode != InvalidIndex() )
		return iNode;

	// not found? we may have to look in older buckets
	cBucketsToModAgainst >>= 1;
	while ( cBucketsToModAgainst >= m_nMinRehashedBucket )
	{
		iBucket = basetypes::ModPowerOf2( hash, cBucketsToModAgainst );

		if ( !m_bitsMigratedBuckets.Get( iBucket ) )
		{
			int iNode2 = FindInBucket( iBucket, key );
			if ( iNode2 != InvalidIndex() )
				return iNode2;
		}

		cBucketsToModAgainst >>= 1;
	}

	return InvalidIndex();	
}


//-----------------------------------------------------------------------------
// Purpose: searches for an item by key, returning the index handle
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline int CUtlHashMapLarge<K,T,L,H>::FindInBucket( int iBucket, const KeyType_t &key ) const
{
	if ( m_vecHashBuckets[iBucket].m_iNode != InvalidIndex() )
	{
		IndexType_t iNode = m_vecHashBuckets[iBucket].m_iNode;
		Assert( iNode < m_nMaxElement );
		while ( iNode != InvalidIndex() )
		{
			// equality check
			if ( m_EqualityFunc( key, m_memNodes[iNode].m_key ) )
				return iNode;

			iNode = m_memNodes[iNode].m_iNextNode;
		}
	}

	return InvalidIndex();
}


//-----------------------------------------------------------------------------
// Purpose: links a node into a bucket
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
void CUtlHashMapLarge<K,T,L,H>::LinkNodeIntoBucket( int iBucket, int iNewNode )
{
	// add into the start of the bucket's list
	m_memNodes[iNewNode].m_iNextNode = m_vecHashBuckets[iBucket].m_iNode;
	m_vecHashBuckets[iBucket].m_iNode = iNewNode;
}


//-----------------------------------------------------------------------------
// Purpose: unlinks a node from the bucket
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
void CUtlHashMapLarge<K,T,L,H>::UnlinkNodeFromBucket( int iBucket, int iNodeToUnlink )
{
	int iNodeNext = m_memNodes[iNodeToUnlink].m_iNextNode;

	// if it's the first node, just update the bucket to point to the new place
	int iNode = m_vecHashBuckets[iBucket].m_iNode;
	if ( iNode == iNodeToUnlink )
	{
		m_vecHashBuckets[iBucket].m_iNode = iNodeNext;
		return;
	}

	// walk the list to find where
	while ( iNode != InvalidIndex() )
	{
		if ( m_memNodes[iNode].m_iNextNode == iNodeToUnlink )
		{
			m_memNodes[iNode].m_iNextNode = iNodeNext;
			return;
		}
		iNode = m_memNodes[iNode].m_iNextNode;
	}

	// should always be valid to unlink
	Assert( false );
}


//-----------------------------------------------------------------------------
// Purpose: removes a single item from the map
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline void CUtlHashMapLarge<K,T,L,H>::RemoveAt( IndexType_t i )
{
	if ( !IsValidIndex( i ) )
	{
		Assert( false );
		return;
	}

	// unfortunately, we have to re-hash to find which bucket we're in
	uint32 hash = m_HashFunc( m_memNodes[i].m_key );
	int cBucketsToModAgainst = m_vecHashBuckets.Count();
	int iBucket = basetypes::ModPowerOf2( hash, cBucketsToModAgainst );
	if ( RemoveNodeFromBucket( iBucket, i ) )
		return;

	// wasn't found; look in older buckets
	cBucketsToModAgainst >>= 1;
	while ( cBucketsToModAgainst >= m_nMinRehashedBucket )
	{
		iBucket = basetypes::ModPowerOf2( hash, cBucketsToModAgainst );

		if ( !m_bitsMigratedBuckets.Get( iBucket ) )
		{
			if ( RemoveNodeFromBucket( iBucket, i ) )
				return;
		}

		cBucketsToModAgainst >>= 1;
	}

	// never found, container is busted
	Assert( false );
}


//-----------------------------------------------------------------------------
// Purpose: removes a node from the bucket, return true if it was found
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline bool CUtlHashMapLarge<K,T,L,H>::RemoveNodeFromBucket( IndexType_t iBucket, int iNodeToRemove )
{
	IndexType_t iNode = m_vecHashBuckets[iBucket].m_iNode;
	while ( iNode != InvalidIndex() )
	{
		if ( iNodeToRemove == iNode )
		{
			// found it, remove
			UnlinkNodeFromBucket( iBucket, iNodeToRemove );
			Destruct( &m_memNodes[iNode].m_key );
			Destruct( &m_memNodes[iNode].m_elem );

			// link into free list
			m_memNodes[iNode].m_iNextNode = FreeNodeIndexToID( m_iNodeFreeListHead );
			m_iNodeFreeListHead = iNode;
			m_cElements--;
			if ( m_cElements == 0 )
			{
				m_nMinRehashedBucket = m_vecHashBuckets.Count();
			}
			return true;
		}

		iNode = m_memNodes[iNode].m_iNextNode;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: removes all items from the hash map
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline void CUtlHashMapLarge<K,T,L,H>::RemoveAll()
{
	FOR_EACH_MAP_FAST( *this, i )
	{
		Destruct( &m_memNodes[i].m_key );
		Destruct( &m_memNodes[i].m_elem );
	}

	m_cElements = 0;
	m_nMaxElement = 0;
	m_iNodeFreeListHead = InvalidIndex();
	m_nMinRehashedBucket = m_vecHashBuckets.Count();
	m_nMaxRehashedBucket = InvalidIndex();
	m_bitsMigratedBuckets.Resize( 0 );
	memset( m_vecHashBuckets.Base(), 0xFF, m_vecHashBuckets.Count() * sizeof(HashBucket_t) );
}


//-----------------------------------------------------------------------------
// Purpose: removes all items from the hash map and releases memory
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline void CUtlHashMapLarge<K,T,L,H>::Purge()
{
	FOR_EACH_MAP_FAST( *this, i )
	{
		Destruct( &m_memNodes[i].m_key );
		Destruct( &m_memNodes[i].m_elem );
	}

	m_cElements = 0;
	m_nMaxElement = 0;
	m_iNodeFreeListHead = InvalidIndex();
	m_nMinRehashedBucket = InvalidIndex();
	m_nMaxRehashedBucket = InvalidIndex();

	m_bitsMigratedBuckets.Resize( 0 );
	m_memNodes.Purge();
	m_vecHashBuckets.Purge();
}


//-----------------------------------------------------------------------------
// Purpose: removes and deletes all items from the hash map and releases memory
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline void CUtlHashMapLarge<K,T,L,H>::PurgeAndDeleteElements()
{
	FOR_EACH_MAP_FAST( *this, i )
	{
		delete this->Element( i );
	}

	Purge();
}


//-----------------------------------------------------------------------------
// Purpose: rehashes buckets
//-----------------------------------------------------------------------------
template <typename K, typename T, typename L, typename H> 
inline void CUtlHashMapLarge<K,T,L,H>::IncrementalRehash()
{
	if ( m_nMinRehashedBucket < m_nMaxRehashedBucket )
	{
		while ( m_nMinRehashedBucket < m_nMaxRehashedBucket )
		{
			// see if the bucket needs rehashing
			if ( m_vecHashBuckets[m_nMinRehashedBucket].m_iNode != InvalidIndex() 
				&& !m_bitsMigratedBuckets.Get(m_nMinRehashedBucket) )
			{
				// rehash this bucket
				RehashNodesInBucket( m_nMinRehashedBucket );
				// only actively do one - don't want to do it too fast since we may be on a rapid growth path
				++m_nMinRehashedBucket;
				break;
			}

			// nothing to rehash in that bucket - increment and look again
			++m_nMinRehashedBucket;
		}

		if ( m_nMinRehashedBucket >= m_nMaxRehashedBucket )
		{
			// we're done; don't need any bits anymore
			m_nMinRehashedBucket = m_vecHashBuckets.Count();
			m_nMaxRehashedBucket = InvalidIndex();
			m_bitsMigratedBuckets.Resize( 0 );
		}
	}
}


#endif // UTLHASHMAPLARGE_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
// Serialization/unserialization buffer
//=============================================================================//

#ifndef UTLHASH_H
#define UTLHASH_H
#pragma once

#include <assert.h>
#include <limits.h>
#include "utlmemory.h"
#include "utlvector.h"
#include "utllinkedlist.h"
#include "utllinkedlist.h"
#include "commonmacros.h"
#include "generichash.h"

typedef unsigned int UtlHashHandle_t;

template<class Data, typename C = bool (*)( Data const&, Data const& ), typename K = unsigned int (*)( Data const& ) >
class CUtlHash
{
public:
	// compare and key functions - implemented by the 
	typedef C CompareFunc_t;
	typedef K KeyFunc_t;
	
	// constructor/deconstructor
	CUtlHash( int bucketCount = 0, int growCount = 0, int initCount = 0,
		      CompareFunc_t compareFunc = 0, KeyFunc_t keyFunc = 0 );
	~CUtlHash();

	// invalid handle
	static UtlHashHandle_t InvalidHandle( void )  { return ( UtlHashHandle_t )~0; }
	bool IsValidHandle( UtlHashHandle_t handle ) const;

	// size
	int Count( void ) const;

	// memory
	void Purge( void );

	// insertion methods
	UtlHashHandle_t Insert( Data const &src );
	UtlHashHandle_t Insert( Data const &src, bool *pDidInsert );
	UtlHashHandle_t AllocEntryFromKey( Data const &src );

	// removal methods
	void Remove( UtlHashHandle_t handle );
	void RemoveAll();

	// retrieval methods
	UtlHashHandle_t Find( Data const &src ) const;

	Data &Element( UtlHashHandle_t handle );
	Data const &Element( UtlHashHandle_t handle ) const;
	Data &operator[]( UtlHashHandle_t handle );
	Data const &operator[]( UtlHashHandle_t handle ) const;

	UtlHashHandle_t GetFirstHandle() const;
	UtlHashHandle_t GetNextHandle( UtlHashHandle_t h ) const;

	// debugging!!
	void Log( const char *filename );

protected:

	int GetBucketIndex( UtlHashHandle_t handle ) const;
	int GetKeyDataIndex( UtlHashHandle_t handle ) const;
	UtlHashHandle_t BuildHandle( int ndxBucket, int ndxKeyData ) const;

	bool DoFind( Data const &src, unsigned int *pBucket, int *pIndex ) const;

protected:

	// handle upper 16 bits = bucket index (bucket heads)
	// handle lower 16 bits = key index (bucket list)
	typedef CUtlVector<Data> HashBucketList_t;
	CUtlVector<HashBucketList_t>	m_Buckets;
	
	CompareFunc_t					m_CompareFunc;			// function used to handle unique compares on data
	KeyFunc_t						m_KeyFunc;				// function used to generate the key value

	bool							m_bPowerOfTwo;			// if the bucket value is a power of two, 
	unsigned int					m_ModMask;				// use the mod mask to "mod"	
};


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
CUtlHash<Data, C, K>::CUtlHash( int bucketCount, int growCount, int initCount,
						  CompareFunc_t compareFunc, KeyFunc_t keyFunc ) :
	m_CompareFunc( compareFunc ),
	m_KeyFunc( keyFunc )
{
	m_Buckets.SetSize( bucketCount );
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		m_Buckets[ndxBucket].SetSize( initCount );
		m_Buckets[ndxBucket].SetGrowSize( growCount );
	}

	// check to see if the bucket count is a power of 2 and set up
	// optimizations appropriately
	m_bPowerOfTwo = IsPowerOfTwo( bucketCount );
	m_ModMask = m_bPowerOfTwo ? (bucketCount-1) : 0;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
CUtlHash<Data, C, K>::~CUtlHash()
{
	Purge();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline bool CUtlHash<Data, C, K>::IsValidHandle( UtlHashHandle_t handle ) const
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	// ndxBucket and ndxKeyData can't possibly be less than zero -- take a 
	// look at the definition of the Get..Index functions for why. However,
	// if you override those functions, you will need to override this one
	// as well. 
	if( /*( ndxBucket >= 0 ) && */      ( ndxBucket < m_Buckets.Count() ) )
	{
		if( /*( ndxKeyData >= 0 ) && */ ( ndxKeyData < m_Buckets[ndxBucket].Count() ) )
			return true;
	}
	
	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline int CUtlHash<Data, C, K>::Count( void ) const
{
	int count = 0;

	int bucketCount = m_Buckets.Count();
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		count += m_Buckets[ndxBucket].Count();
	}

	return count;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline int CUtlHash<Data, C, K>::GetBucketIndex( UtlHashHandle_t handle ) const
{
	return ( ( ( handle >> 16 ) & 0x0000ffff ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline int CUtlHash<Data, C, K>::GetKeyDataIndex( UtlHashHandle_t handle ) const
{
	return ( handle & 0x0000ffff );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline UtlHashHandle_t CUtlHash<Data, C, K>::BuildHandle( int ndxBucket, int ndxKeyData ) const
{
	assert( ( ndxBucket >= 0 ) && ( ndxBucket < 65536 ) );
	assert( ( ndxKeyData >= 0 ) && ( ndxKeyData < 65536 ) );

	UtlHashHandle_t handle = ndxKeyData;
	handle |= ( ndxBucket << 16 );

	return handle;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline void CUtlHash<Data, C, K>::Purge( void )
{
	int bucketCount = m_Buckets.Count();
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		m_Buckets[ndxBucket].Purge();
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline bool CUtlHash<Data, C, K>::DoFind( Data const &src, unsigned int *pBucket, int *pIndex ) const
{
	// generate the data "key"
	unsigned int key = m_KeyFunc( src );

	// hash the "key" - get the correct hash table "bucket"
	unsigned int ndxBucket;
	if( m_bPowerOfTwo )
	{
		*pBucket = ndxBucket = ( key & m_ModMask );
	}
	else
	{
		int bucketCount = m_Buckets.Count();
		*pBucket = ndxBucket = key % bucketCount;
	}

	int ndxKeyData;
	const CUtlVector<Data> &bucket = m_Buckets[ndxBucket];
	int keyDataCount = bucket.Count();
	for( ndxKeyData = 0; ndxKeyData < keyDataCount; ndxKeyData++ )
	{
		if( m_CompareFunc( bucket.Element( ndxKeyData ), src ) )
			break;
	}

	if( ndxKeyData == keyDataCount )
		return false;

	*pIndex = ndxKeyData;
	return true;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline UtlHashHandle_t CUtlHash<Data, C, K>::Find( Data const &src ) const
{
	unsigned int ndxBucket;
	int ndxKeyData;

	if ( DoFind( src, &ndxBucket, &ndxKeyData ) )
	{
		return ( BuildHandle( ndxBucket, ndxKeyData ) );
	}
	return ( InvalidHandle() );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline UtlHashHandle_t CUtlHash<Data, C, K>::Insert( Data const &src )
{
	unsigned int ndxBucket;
	int ndxKeyData;

	if ( DoFind( src, &ndxBucket, &ndxKeyData ) )
	{
		return ( BuildHandle( ndxBucket, ndxKeyData ) );
	}

	ndxKeyData = m_Buckets[ndxBucket].AddToTail( src );

	return ( BuildHandle( ndxBucket, ndxKeyData ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline UtlHashHandle_t CUtlHash<Data, C, K>::Insert( Data const &src, bool *pDidInsert )
{
	unsigned int ndxBucket;
	int ndxKeyData;

	if ( DoFind( src, &ndxBucket, &ndxKeyData ) )
	{
		*pDidInsert = false;
		return ( BuildHandle( ndxBucket, ndxKeyData ) );
	}

	*pDidInsert = true;
	ndxKeyData = m_Buckets[ndxBucket].AddToTail( src );

	return ( BuildHandle( ndxBucket, ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline UtlHashHandle_t CUtlHash<Data, C, K>::AllocEntryFromKey( Data const &src )
{
	unsigned int ndxBucket;
	int ndxKeyData;

	if ( DoFind( src, &ndxBucket, &ndxKeyData ) )
	{
		return ( BuildHandle( ndxBucket, ndxKeyData ) );
	}

	ndxKeyData = m_Buckets[ndxBucket].AddToTail();

	return ( BuildHandle( ndxBucket, ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline void CUtlHash<Data, C, K>::Remove( UtlHashHandle_t handle )
{
	assert( IsValidHandle( handle ) );

	// check to see if the bucket exists
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	if( m_Buckets[ndxBucket].IsValidIndex( ndxKeyData ) )
	{
		m_Buckets[ndxBucket].FastRemove( ndxKeyData );
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline void CUtlHash<Data, C, K>::RemoveAll()
{
	int bucketCount = m_Buckets.Count();
	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		m_Buckets[ndxBucket].RemoveAll();
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline Data &CUtlHash<Data, C, K>::Element( UtlHashHandle_t handle )
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline Data const &CUtlHash<Data, C, K>::Element( UtlHashHandle_t handle ) const
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline Data &CUtlHash<Data, C, K>::operator[]( UtlHashHandle_t handle )
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline Data const &CUtlHash<Data, C, K>::operator[]( UtlHashHandle_t handle ) const
{
	int ndxBucket = GetBucketIndex( handle );
	int ndxKeyData = GetKeyDataIndex( handle );

	return ( m_Buckets[ndxBucket].Element( ndxKeyData ) );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline UtlHashHandle_t CUtlHash<Data, C, K>::GetFirstHandle() const
{
	return GetNextHandle( ( UtlHashHandle_t )-1 );
}

template<class Data, typename C, typename K>
inline UtlHashHandle_t CUtlHash<Data, C, K>::GetNextHandle( UtlHashHandle_t handle ) const
{
	++handle; // start at the first possible handle after the one given

	int bi = GetBucketIndex( handle );
	int ki =  GetKeyDataIndex( handle );

	int nBuckets = m_Buckets.Count();
	for ( ; bi < nBuckets; ++bi )
	{
		if ( ki < m_Buckets[ bi ].Count() )
			return BuildHandle( bi, ki );

		ki = 0;
	}

	return InvalidHandle();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, typename C, typename K>
inline void CUtlHash<Data, C, K>::Log( const char *filename )
{
	FILE *pDebugFp;
	pDebugFp = fopen( filename, "w" );
	if( !pDebugFp )
		return;

	int maxBucketSize = 0;
	int numBucketsEmpty = 0;

	int bucketCount = m_Buckets.Count();
	fprintf( pDebugFp, "\n%d Buckets\n", bucketCount ); 

	for( int ndxBucket = 0; ndxBucket < bucketCount; ndxBucket++ )
	{
		int count = m_Buckets[ndxBucket].Count();
		
		if( count > maxBucketSize ) { maxBucketSize = count; }
		if( count == 0 )
			numBucketsEmpty++;

		fprintf( pDebugFp, "Bucket %d: %d\n", ndxBucket, count );
	}

	fprintf( pDebugFp, "\nBucketHeads Used: %d\n", bucketCount - numBucketsEmpty );
	fprintf( pDebugFp, "Max Bucket Size: %d\n", maxBucketSize );

	fclose( pDebugFp );
}

//=============================================================================
// 
// Fast Hash
//
// Number of buckets must be a power of 2.
// Key must be 32-bits (unsigned int).
//
typedef intp UtlHashFastHandle_t;

#define UTLHASH_POOL_SCALAR		2

class CUtlHashFastNoHash
{
public:
	static int Hash( int key, int bucketMask )
	{
		return ( key & bucketMask );
	}
};

class CUtlHashFastGenericHash
{
public:
	static int Hash( int key, int bucketMask )
	{
		return ( HashIntConventional( key ) & bucketMask );
	}
};

template<class Data, class HashFuncs = CUtlHashFastNoHash > 
class CUtlHashFast
{
public:

	// Constructor/Deconstructor.
	CUtlHashFast();
	~CUtlHashFast();

	// Memory.
	void Purge( void );

	// Invalid handle.
	static UtlHashFastHandle_t InvalidHandle( void )	{ return ( UtlHashFastHandle_t )~0; }

	// Initialize.
	bool Init( int nBucketCount );

	// Size.
	int Count( void );

	// Insertion.
	UtlHashFastHandle_t Insert( uintp uiKey, const Data &data );
	UtlHashFastHandle_t FastInsert( uintp uiKey, const Data &data );

	// Removal.
	void Remove( UtlHashFastHandle_t hHash );
	void RemoveAll( void );

	// Retrieval.
	UtlHashFastHandle_t Find( uintp uiKey );

	Data &Element( UtlHashFastHandle_t hHash );
	Data const &Element( UtlHashFastHandle_t hHash ) const;
	Data &operator[]( UtlHashFastHandle_t hHash );
	Data const &operator[]( UtlHashFastHandle_t hHash ) const;

//protected:

	// Templatized for memory tracking purposes
	template <typename HashData>
	struct HashFastData_t_
	{
		uintp		m_uiKey;
		HashData	m_Data;
	};

	typedef HashFastData_t_<Data> HashFastData_t;

	uintp								m_uiBucketMask;	
	CUtlVector<UtlHashFastHandle_t>		m_aBuckets;
	CUtlFixedLinkedList<HashFastData_t>	m_aDataPool;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> CUtlHashFast<Data,HashFuncs>::CUtlHashFast()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> CUtlHashFast<Data,HashFuncs>::~CUtlHashFast()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Destroy dynamically allocated hash data.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline void CUtlHashFast<Data,HashFuncs>::Purge( void )
{
	m_aBuckets.Purge();
	m_aDataPool.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the hash - set bucket count and hash grow amount.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> bool CUtlHashFast<Data,HashFuncs>::Init( int nBucketCount )
{
	// Verify the bucket count is power of 2.
	if ( !IsPowerOfTwo( nBucketCount ) )
		return false;

	// Set the bucket size.
	m_aBuckets.SetSize( nBucketCount );
	for ( int iBucket = 0; iBucket < nBucketCount; ++iBucket )
	{
		m_aBuckets[iBucket] = m_aDataPool.InvalidIndex();
	}

	// Set the mod mask.
	m_uiBucketMask = nBucketCount - 1;

	// Calculate the grow size.
	int nGrowSize = UTLHASH_POOL_SCALAR * nBucketCount;
	m_aDataPool.SetGrowSize( nGrowSize );

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of elements in the hash.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline int CUtlHashFast<Data,HashFuncs>::Count( void )
{
	return m_aDataPool.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (uintp), with
//          a check to see if the element already exists within the tree.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline UtlHashFastHandle_t CUtlHashFast<Data,HashFuncs>::Insert( uintp uiKey, const Data &data )
{
	// Check to see if that key already exists in the buckets (should be unique).
	UtlHashFastHandle_t hHash = Find( uiKey );
	if( hHash != InvalidHandle() )
		return hHash;

	return FastInsert( uiKey, data );
}

//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (uintp),
//          without a check to see if the element already exists within the tree.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline UtlHashFastHandle_t CUtlHashFast<Data,HashFuncs>::FastInsert( uintp uiKey, const Data &data )
{
	// Get a new element from the pool.
	intp iHashData = m_aDataPool.Alloc( true );
	HashFastData_t *pHashData = &m_aDataPool[iHashData];
	if ( !pHashData )
		return InvalidHandle();

	// Add data to new element.
	pHashData->m_uiKey = uiKey;
	pHashData->m_Data = data;
				
	// Link element.
	int iBucket = HashFuncs::Hash( uiKey, m_uiBucketMask );
	m_aDataPool.LinkBefore( m_aBuckets[iBucket], iHashData );
	m_aBuckets[iBucket] = iHashData;
	
	return iHashData;	
}

//-----------------------------------------------------------------------------
// Purpose: Remove a given element from the hash.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline void CUtlHashFast<Data,HashFuncs>::Remove( UtlHashFastHandle_t hHash )
{
	int iBucket = HashFuncs::Hash( m_aDataPool[hHash].m_uiKey, m_uiBucketMask );
	if ( m_aBuckets[iBucket] == hHash )
	{
		// It is a bucket head.
		m_aBuckets[iBucket] = m_aDataPool.Next( hHash );
	}
	else
	{
		// Not a bucket head.
		m_aDataPool.Unlink( hHash );
	}

	// Remove the element.
	m_aDataPool.Remove( hHash );
}

//-----------------------------------------------------------------------------
// Purpose: Remove all elements from the hash
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline void CUtlHashFast<Data,HashFuncs>::RemoveAll( void )
{
	m_aBuckets.RemoveAll();
	m_aDataPool.RemoveAll();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline UtlHashFastHandle_t CUtlHashFast<Data,HashFuncs>::Find( uintp uiKey )
{
	// hash the "key" - get the correct hash table "bucket"
	int iBucket = HashFuncs::Hash( uiKey, m_uiBucketMask );

	for ( intp iElement = m_aBuckets[iBucket]; iElement != m_aDataPool.InvalidIndex(); iElement = m_aDataPool.Next( iElement ) )
	{
		if ( m_aDataPool[iElement].m_uiKey == uiKey )
			return iElement;
	}

	return InvalidHandle();
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline Data &CUtlHashFast<Data,HashFuncs>::Element( UtlHashFastHandle_t hHash )
{
	return ( m_aDataPool[hHash].m_Data );
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline Data const &CUtlHashFast<Data,HashFuncs>::Element( UtlHashFastHandle_t hHash ) const
{
	return ( m_aDataPool[hHash].m_Data );
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline Data &CUtlHashFast<Data,HashFuncs>::operator[]( UtlHashFastHandle_t hHash )
{
	return ( m_aDataPool[hHash].m_Data );
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, class HashFuncs> inline Data const &CUtlHashFast<Data,HashFuncs>::operator[]( UtlHashFastHandle_t hHash ) const
{
	return ( m_aDataPool[hHash].m_Data );
}

//=============================================================================
// 
// Fixed Hash
//
// Number of buckets must be a power of 2.
// Key must be 32-bits (unsigned int).
//
typedef intp UtlHashFixedHandle_t;

template <int NUM_BUCKETS>
class CUtlHashFixedGenericHash
{
public:
	static int Hash( int key, int bucketMask )
	{
		int hash = HashIntConventional( key );
		if ( NUM_BUCKETS <= USHRT_MAX )
		{
			hash ^= ( hash >> 16 );
		}
		if ( NUM_BUCKETS <= UCHAR_MAX )
		{
			hash ^= ( hash >> 8 );
		}
		return ( hash & bucketMask );
	}
};

template<class Data, int NUM_BUCKETS, class CHashFuncs = CUtlHashFastNoHash > 
class CUtlHashFixed
{
public:

	// Constructor/Deconstructor.
	CUtlHashFixed();
	~CUtlHashFixed();

	// Memory.
	void Purge( void );

	// Invalid handle.
	static UtlHashFixedHandle_t InvalidHandle( void )	{ return ( UtlHashFixedHandle_t )~0; }

	// Size.
	int Count( void );

	// Insertion.
	UtlHashFixedHandle_t Insert( unsigned int uiKey, const Data &data );
	UtlHashFixedHandle_t FastInsert( unsigned int uiKey, const Data &data );

	// Removal.
	void Remove( UtlHashFixedHandle_t hHash );
	void RemoveAll( void );

	// Retrieval.
	UtlHashFixedHandle_t Find( unsigned int uiKey );

	Data &Element( UtlHashFixedHandle_t hHash );
	Data const &Element( UtlHashFixedHandle_t hHash ) const;
	Data &operator[]( UtlHashFixedHandle_t hHash );
	Data const &operator[]( UtlHashFixedHandle_t hHash ) const;

	//protected:

	// Templatized for memory tracking purposes
	template <typename Data_t>
	struct HashFixedData_t_
	{
		unsigned int	m_uiKey;
		Data_t			m_Data;
	};

	typedef HashFixedData_t_<Data> HashFixedData_t;

	enum
	{
		BUCKET_MASK = NUM_BUCKETS - 1
	};
	CUtlPtrLinkedList<HashFixedData_t> m_aBuckets[NUM_BUCKETS];
	int m_nElements;
};

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::CUtlHashFixed()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Deconstructor
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::~CUtlHashFixed()
{
	Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Destroy dynamically allocated hash data.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline void CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::Purge( void )
{
	RemoveAll();
}

//-----------------------------------------------------------------------------
// Purpose: Return the number of elements in the hash.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline int CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::Count( void )
{
	return m_nElements;
}

//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int), with
//          a check to see if the element already exists within the tree.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline UtlHashFixedHandle_t CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::Insert( unsigned int uiKey, const Data &data )
{
	// Check to see if that key already exists in the buckets (should be unique).
	UtlHashFixedHandle_t hHash = Find( uiKey );
	if( hHash != InvalidHandle() )
		return hHash;

	return FastInsert( uiKey, data );
}

//-----------------------------------------------------------------------------
// Purpose: Insert data into the hash table given its key (unsigned int),
//          without a check to see if the element already exists within the tree.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline UtlHashFixedHandle_t CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::FastInsert( unsigned int uiKey, const Data &data )
{
	int iBucket = HashFuncs::Hash( uiKey, NUM_BUCKETS - 1 );
	UtlPtrLinkedListIndex_t iElem = m_aBuckets[iBucket].AddToHead();

	HashFixedData_t *pHashData = &m_aBuckets[iBucket][iElem];

	Assert( (UtlPtrLinkedListIndex_t)pHashData == iElem );

	// Add data to new element.
	pHashData->m_uiKey = uiKey;
	pHashData->m_Data = data;

	m_nElements++;
	return (UtlHashFixedHandle_t)pHashData;	
}

//-----------------------------------------------------------------------------
// Purpose: Remove a given element from the hash.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline void CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::Remove( UtlHashFixedHandle_t hHash )
{
	HashFixedData_t *pHashData = (HashFixedData_t *)hHash;
	Assert( Find(pHashData->m_uiKey) != InvalidHandle() );
	int iBucket = HashFuncs::Hash( pHashData->m_uiKey, NUM_BUCKETS - 1 );
	m_aBuckets[iBucket].Remove( (UtlPtrLinkedListIndex_t)pHashData );
	m_nElements--;
}

//-----------------------------------------------------------------------------
// Purpose: Remove all elements from the hash
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline void CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::RemoveAll( void )
{
	for ( int i = 0; i < NUM_BUCKETS; i++ )
	{
		m_aBuckets[i].RemoveAll();
	}
	m_nElements = 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline UtlHashFixedHandle_t CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::Find( unsigned int uiKey )
{
	int iBucket = HashFuncs::Hash( uiKey, NUM_BUCKETS - 1 );
	CUtlPtrLinkedList<HashFixedData_t> &bucket = m_aBuckets[iBucket];

	for ( UtlPtrLinkedListIndex_t iElement = bucket.Head(); iElement != bucket.InvalidIndex(); iElement = bucket.Next( iElement ) )
	{
		if ( bucket[iElement].m_uiKey == uiKey )
			return (UtlHashFixedHandle_t)iElement;
	}

	return InvalidHandle();
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline Data &CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::Element( UtlHashFixedHandle_t hHash )
{
	return ((HashFixedData_t *)hHash)->m_Data;
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline Data const &CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::Element( UtlHashFixedHandle_t hHash ) const
{
	return ((HashFixedData_t *)hHash)->m_Data;
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline Data &CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::operator[]( UtlHashFixedHandle_t hHash )
{
	return ((HashFixedData_t *)hHash)->m_Data;
}

//-----------------------------------------------------------------------------
// Purpose: Return data given a hash handle.
//-----------------------------------------------------------------------------
template<class Data, int NUM_BUCKETS, class HashFuncs> inline Data const &CUtlHashFixed<Data,NUM_BUCKETS,HashFuncs>::operator[]( UtlHashFixedHandle_t hHash ) const
{
	return ((HashFixedData_t *)hHash)->m_Data;
}

#endif // UTLHASH_H

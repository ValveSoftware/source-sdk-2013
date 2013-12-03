//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a large symbol table (intp sized handles, can store more than 64k strings)
//
// $Header: $
// $NoKeywords: $
//===========================================================================//

#ifndef UTLSYMBOLLARGE_H
#define UTLSYMBOLLARGE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/threadtools.h"
#include "tier1/utltshash.h"
#include "tier1/stringpool.h"
#include "tier0/vprof.h"
#include "tier1/utltshash.h"

//-----------------------------------------------------------------------------
// CUtlSymbolTableLarge:
// description:
//    This class defines a symbol table, which allows us to perform mappings
//    of strings to symbols and back. 
// 
//    This class stores the strings in a series of string pools. The returned CUtlSymbolLarge is just a pointer
//     to the string data, the hash precedes it in memory and is used to speed up searching, etc.
//-----------------------------------------------------------------------------

typedef intp UtlSymLargeId_t;

#define UTL_INVAL_SYMBOL_LARGE  ((UtlSymLargeId_t)~0)

class CUtlSymbolLarge
{
public:
	// constructor, destructor
	CUtlSymbolLarge() 
	{
		u.m_Id = UTL_INVAL_SYMBOL_LARGE;
	}

	CUtlSymbolLarge( UtlSymLargeId_t id )
	{
		u.m_Id = id;
	}
	CUtlSymbolLarge( CUtlSymbolLarge const& sym )
	{
		u.m_Id = sym.u.m_Id; 
	}

	// operator=
	CUtlSymbolLarge& operator=( CUtlSymbolLarge const& src ) 
	{ 
		u.m_Id = src.u.m_Id; 
		return *this; 
	}

	// operator==
	bool operator==( CUtlSymbolLarge const& src ) const 
	{ 
		return u.m_Id == src.u.m_Id; 
	}

	// operator==
	bool operator==( UtlSymLargeId_t const& src ) const 
	{ 
		return u.m_Id == src; 
	}

	// operator==
	bool operator!=( CUtlSymbolLarge const& src ) const 
	{ 
		return u.m_Id != src.u.m_Id; 
	}

	// operator==
	bool operator!=( UtlSymLargeId_t const& src ) const 
	{ 
		return u.m_Id != src; 
	}

	// Gets at the symbol
	operator UtlSymLargeId_t const() const 
	{ 
		return u.m_Id; 
	}

	// Gets the string associated with the symbol
	inline const char* String( ) const
	{
		if ( u.m_Id == UTL_INVAL_SYMBOL_LARGE )
			return "";
		return u.m_pAsString;
	}

	inline bool IsValid() const
	{
		return u.m_Id != UTL_INVAL_SYMBOL_LARGE ? true : false;
	}

private:
	// Disallowed
	CUtlSymbolLarge( const char* pStr );       // they need to go through the table to assign the ptr
	bool operator==( const char* pStr ) const; // disallow since we don't know if the table this is from was case sensitive or not... maybe we don't care

	union
	{
		UtlSymLargeId_t m_Id;
		char const *m_pAsString;
	} u;
};

#define MIN_STRING_POOL_SIZE	2048

inline uint32 CUtlSymbolLarge_Hash( bool CASEINSENSITIVE, const char *pString, int len )
{
	return ( CASEINSENSITIVE ? HashStringCaseless( pString ) : HashString( pString ) ); 
}

typedef uint32 LargeSymbolTableHashDecoration_t; 

// The structure consists of the hash immediately followed by the string data
struct CUtlSymbolTableLargeBaseTreeEntry_t
{
	LargeSymbolTableHashDecoration_t	m_Hash;
	// Variable length string data
	char								m_String[1];

	bool IsEmpty() const
	{
		return ( ( m_Hash == 0 ) && ( 0 == m_String[0] ) );
	}

	char const *String() const
	{
		return (const char *)&m_String[ 0 ];
	}

	CUtlSymbolLarge ToSymbol() const
	{
		return reinterpret_cast< UtlSymLargeId_t >( String() );
	}
	
	LargeSymbolTableHashDecoration_t HashValue() const
	{
		return m_Hash;
	}
};
	
template< class TreeType, bool CASEINSENSITIVE >
class CTreeEntryLess
{
public:
	CTreeEntryLess( int ignored = 0 ) {} // permits default initialization to NULL in CUtlRBTree
	bool operator!() const { return false; }
	bool operator()( CUtlSymbolTableLargeBaseTreeEntry_t * const &left, CUtlSymbolTableLargeBaseTreeEntry_t * const &right ) const
	{
		// compare the hashes
		if ( left->m_Hash == right->m_Hash )
		{
			// if the hashes match compare the strings
			if ( !CASEINSENSITIVE )
				return strcmp( left->String(), right->String() ) < 0;
			else
				return V_stricmp( left->String(), right->String() ) < 0;
		}
		else
		{
			return left->m_Hash < right->m_Hash;
		}
	}
};
	
// For non-threaded versions, simply index into CUtlRBTree
template< bool CASEINSENSITIVE >
class CNonThreadsafeTree : public CUtlRBTree<CUtlSymbolTableLargeBaseTreeEntry_t *, intp, CTreeEntryLess< CNonThreadsafeTree< CASEINSENSITIVE >, CASEINSENSITIVE > >
{
public:
	typedef CUtlRBTree<CUtlSymbolTableLargeBaseTreeEntry_t *, intp, CTreeEntryLess< CNonThreadsafeTree, CASEINSENSITIVE > > CNonThreadsafeTreeType;

	CNonThreadsafeTree() : 
		CNonThreadsafeTreeType( 0, 16 ) 
	{
	}
	inline void Commit() 
	{
		// Nothing, only matters for thread-safe tables
	}
	inline int Insert( CUtlSymbolTableLargeBaseTreeEntry_t *entry )
	{
		return CNonThreadsafeTreeType::Insert( entry );
	}
	inline int Find( CUtlSymbolTableLargeBaseTreeEntry_t *entry ) const
	{
		return CNonThreadsafeTreeType::Find( entry );
	}
	inline int InvalidIndex() const
	{
		return CNonThreadsafeTreeType::InvalidIndex();
	}
	inline int GetElements( int nFirstElement, int nCount, CUtlSymbolLarge *pElements ) const
	{
		CUtlVector< CUtlSymbolTableLargeBaseTreeEntry_t * > list;
		list.EnsureCount( nCount );
		for ( int i = 0; i < nCount; ++i )
		{
			pElements[ i ] = CNonThreadsafeTreeType::Element( i )->ToSymbol();
		}

		return nCount;
	}
};

// Since CUtlSymbolTableLargeBaseTreeEntry_t already has the hash 
//  contained inside of it, don't need to recompute a hash here
template < int BUCKET_COUNT, class KEYTYPE, bool CASEINSENSITIVE >
class CCThreadsafeTreeHashMethod
{
public:
	static int Hash( const KEYTYPE &key, int nBucketMask )
	{
		uint32 nHash = key->HashValue();
		return ( nHash & nBucketMask );
	}

	static bool Compare( CUtlSymbolTableLargeBaseTreeEntry_t * const &lhs, CUtlSymbolTableLargeBaseTreeEntry_t * const &rhs )
	{
		if ( lhs->m_Hash != rhs->m_Hash )
			return false;
		if ( !CASEINSENSITIVE )
		{
			return ( !Q_strcmp( lhs->String(), rhs->String() ) ? true : false );
		}

		return ( !Q_stricmp( lhs->String(), rhs->String() ) ? true : false );
	}
};

/*
  NOTE:  So the only crappy thing about using a CUtlTSHash here is that the KEYTYPE is a CUtlSymbolTableLargeBaseTreeEntry_t ptr which has both the 
   hash and the string since with strings there is a good chance of hash collision after you have a fair number of strings so we have to implement
   a Compare method (above) which falls back to strcmp/stricmp if the hashes are equal.  This means that all of the data is in the KEYTYPE of the hash and the 
   payload doesn't matter.  So I made the payload also be a pointer to a CUtlSymbolTableLargeBaseTreeEntry_t since that makes using the API more convenient

  TODO:  If we have a CUtlTSHash that was all about the existence of the KEYTYPE and didn't require a payload (or template on 'void') then we could eliminate
   50% of the pointer overhead used for this data structure.
*/

// Thread safe version is based on the 
template < bool CASEINSENSITIVE >
class CThreadsafeTree : public CUtlTSHash< CUtlSymbolTableLargeBaseTreeEntry_t *, 2048, CUtlSymbolTableLargeBaseTreeEntry_t *, CCThreadsafeTreeHashMethod< 2048, CUtlSymbolTableLargeBaseTreeEntry_t *, CASEINSENSITIVE > >
{
public:
	typedef CUtlTSHash< CUtlSymbolTableLargeBaseTreeEntry_t *, 2048, CUtlSymbolTableLargeBaseTreeEntry_t *, CCThreadsafeTreeHashMethod< 2048, CUtlSymbolTableLargeBaseTreeEntry_t *, CASEINSENSITIVE > > CThreadsafeTreeType;

	CThreadsafeTree() : 
		CThreadsafeTreeType( 32 ) 
	{
	}
	inline void Commit() 
	{
		CThreadsafeTreeType::Commit();
	}
	inline int Insert( CUtlSymbolTableLargeBaseTreeEntry_t *entry )
	{
		return CThreadsafeTreeType::Insert( entry, entry );
	}
	inline int Find( CUtlSymbolTableLargeBaseTreeEntry_t *entry )
	{
		return CThreadsafeTreeType::Find( entry );
	}
	inline int InvalidIndex() const
	{
		return CThreadsafeTreeType::InvalidHandle();
	}
	inline int GetElements( int nFirstElement, int nCount, CUtlSymbolLarge *pElements ) const
	{
		CUtlVector< UtlTSHashHandle_t > list;
		list.EnsureCount( nCount );
		int c = CThreadsafeTreeType::GetElements( nFirstElement, nCount, list.Base() );
		for ( int i = 0; i < c; ++i )
		{
			pElements[ i ] = CThreadsafeTreeType::Element( list[ i ] )->ToSymbol();
		}
		
		return c;
	}
};

// Base Class for threaded and non-threaded types
template < class TreeType, bool CASEINSENSITIVE, size_t POOL_SIZE = MIN_STRING_POOL_SIZE >
class CUtlSymbolTableLargeBase
{
public:
	// constructor, destructor
	CUtlSymbolTableLargeBase();
	~CUtlSymbolTableLargeBase();
	
	// Finds and/or creates a symbol based on the string
	CUtlSymbolLarge AddString( const char* pString );

	// Finds the symbol for pString
	CUtlSymbolLarge Find( const char* pString ) const;
	
	// Remove all symbols in the table.
	void  RemoveAll();

	int GetNumStrings( void ) const
	{
		return m_Lookup.Count();
	}

	void Commit()
	{
		m_Lookup.Commit();
	}

	// Returns elements in the table
	int GetElements( int nFirstElement, int nCount, CUtlSymbolLarge *pElements ) const
	{
		return m_Lookup.GetElements( nFirstElement, nCount, pElements );
	}

	uint64 GetMemoryUsage() const
	{
		uint64 unBytesUsed = 0u;

		for ( int i=0; i < m_StringPools.Count(); i++ )
		{
			StringPool_t *pPool = m_StringPools[i];

			unBytesUsed += (uint64)pPool->m_TotalLen;
		}
		return unBytesUsed;
	}


protected:

	struct StringPool_t
	{	
		int m_TotalLen;		// How large is 
		int m_SpaceUsed;
		char m_Data[1];
	};

	TreeType m_Lookup;

	// stores the string data
	CUtlVector< StringPool_t * > m_StringPools;

private:
	int FindPoolWithSpace( int len ) const;
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template < class TreeType, bool CASEINSENSITIVE, size_t POOL_SIZE >
inline CUtlSymbolTableLargeBase<TreeType, CASEINSENSITIVE, POOL_SIZE >::CUtlSymbolTableLargeBase() : 
	m_StringPools( 8 )
{
}

template < class TreeType, bool CASEINSENSITIVE, size_t POOL_SIZE >
inline CUtlSymbolTableLargeBase<TreeType, CASEINSENSITIVE, POOL_SIZE>::~CUtlSymbolTableLargeBase()
{
	// Release the stringpool string data
	RemoveAll();
}

template < class TreeType, bool CASEINSENSITIVE, size_t POOL_SIZE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase<TreeType, CASEINSENSITIVE, POOL_SIZE>::Find( const char* pString ) const
{	
	VPROF( "CUtlSymbolLarge::Find" );
	if (!pString)
		return CUtlSymbolLarge();

	// Passing this special invalid symbol makes the comparison function
	// use the string passed in the context
	int len = Q_strlen( pString ) + 1;

	CUtlSymbolTableLargeBaseTreeEntry_t *search = (CUtlSymbolTableLargeBaseTreeEntry_t *)_alloca( len + sizeof( LargeSymbolTableHashDecoration_t ) );
	search->m_Hash = CUtlSymbolLarge_Hash( CASEINSENSITIVE, pString, len );
	Q_memcpy( (char *)&search->m_String[ 0 ], pString, len );

	int idx = const_cast< TreeType & >(m_Lookup).Find( search );

	if ( idx == m_Lookup.InvalidIndex() )
		return UTL_INVAL_SYMBOL_LARGE;

	const CUtlSymbolTableLargeBaseTreeEntry_t *entry = m_Lookup[ idx ];
	return entry->ToSymbol();
}

template < class TreeType, bool CASEINSENSITIVE, size_t POOL_SIZE >
inline int CUtlSymbolTableLargeBase<TreeType, CASEINSENSITIVE, POOL_SIZE>::FindPoolWithSpace( int len )	const
{
	for ( int i=0; i < m_StringPools.Count(); i++ )
	{
		StringPool_t *pPool = m_StringPools[i];

		if ( (pPool->m_TotalLen - pPool->m_SpaceUsed) >= len )
		{
			return i;
		}
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Finds and/or creates a symbol based on the string
//-----------------------------------------------------------------------------
template < class TreeType, bool CASEINSENSITIVE, size_t POOL_SIZE >
inline CUtlSymbolLarge CUtlSymbolTableLargeBase<TreeType, CASEINSENSITIVE, POOL_SIZE>::AddString( const char* pString )
{
	VPROF("CUtlSymbolLarge::AddString");
	if (!pString) 
		return UTL_INVAL_SYMBOL_LARGE;

	CUtlSymbolLarge id = Find( pString );
	if ( id != UTL_INVAL_SYMBOL_LARGE )
		return id;

	int lenString = Q_strlen(pString) + 1; // length of just the string
	int lenDecorated = lenString + sizeof(LargeSymbolTableHashDecoration_t); // and with its hash decoration
	// make sure that all strings are aligned on 2-byte boundaries so the hashes will read correctly
	// This assert seems to be invalid because LargeSymbolTableHashDecoration_t is always
	// a uint32, by design.
	//COMPILE_TIME_ASSERT(sizeof(LargeSymbolTableHashDecoration_t) == sizeof(intp));
	lenDecorated = ALIGN_VALUE(lenDecorated, sizeof( intp ) );

	// Find a pool with space for this string, or allocate a new one.
	int iPool = FindPoolWithSpace( lenDecorated );
	if ( iPool == -1 )
	{
		// Add a new pool.
		int newPoolSize = MAX( lenDecorated + sizeof( StringPool_t ), POOL_SIZE );
		StringPool_t *pPool = (StringPool_t*)malloc( newPoolSize );

		pPool->m_TotalLen = newPoolSize - sizeof( StringPool_t );
		pPool->m_SpaceUsed = 0;
		iPool = m_StringPools.AddToTail( pPool );
	}

	// Compute a hash
	LargeSymbolTableHashDecoration_t hash = CUtlSymbolLarge_Hash( CASEINSENSITIVE, pString, lenString );

	// Copy the string in.
	StringPool_t *pPool = m_StringPools[iPool];
	// Assert( pPool->m_SpaceUsed < 0xFFFF );	// Pool could be bigger than 2k
	// This should never happen, because if we had a string > 64k, it
	// would have been given its entire own pool.
	
	CUtlSymbolTableLargeBaseTreeEntry_t *entry = ( CUtlSymbolTableLargeBaseTreeEntry_t * )&pPool->m_Data[ pPool->m_SpaceUsed ];
	
	pPool->m_SpaceUsed += lenDecorated;

	entry->m_Hash = hash;
	char *pText = (char *)&entry->m_String [ 0 ];
	Q_memcpy( pText, pString, lenString );

	// insert the string into the database
	MEM_ALLOC_CREDIT();
	int idx = m_Lookup.Insert( entry );
	return m_Lookup.Element( idx )->ToSymbol();
}

//-----------------------------------------------------------------------------
// Remove all symbols in the table.
//-----------------------------------------------------------------------------
template < class TreeType, bool CASEINSENSITIVE, size_t POOL_SIZE >
inline void CUtlSymbolTableLargeBase<TreeType, CASEINSENSITIVE, POOL_SIZE>::RemoveAll()
{
	m_Lookup.Purge();

	for ( int i=0; i < m_StringPools.Count(); i++ )
	{
		StringPool_t * pString = m_StringPools[i];
		free( pString );
	}

	m_StringPools.RemoveAll();
}

// Case-sensitive
typedef CUtlSymbolTableLargeBase< CNonThreadsafeTree< false >, false > CUtlSymbolTableLarge;
// Case-insensitive
typedef CUtlSymbolTableLargeBase< CNonThreadsafeTree< true >, true > CUtlSymbolTableLarge_CI;
// Multi-threaded case-sensitive
typedef CUtlSymbolTableLargeBase< CThreadsafeTree< false >, false > CUtlSymbolTableLargeMT;
// Multi-threaded case-insensitive
typedef CUtlSymbolTableLargeBase< CThreadsafeTree< true >, true > CUtlSymbolTableLargeMT_CI;

#endif // UTLSYMBOLLARGE_H

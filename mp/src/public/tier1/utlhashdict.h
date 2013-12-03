//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef UTLHASHDICT_H
#define UTLHASHDICT_H

#if defined( _WIN32 )
#pragma once
#endif

#include "tier1/utlhash.h"
#include "tier1/generichash.h"
#include "mathlib/mathlib.h"

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive = true, bool bDupeStrings = true> 
class CUtlHashDict
{
public:
	// constructor, destructor
	CUtlHashDict( int bucketCount = 16, int growCount = 0, int initCount = 0 );
	~CUtlHashDict( );

	// gets particular elements
	T&         Element( unsigned i );
	const T&   Element( unsigned i ) const;
	T&         operator[]( unsigned i );
	const T&   operator[]( unsigned i ) const;

	// gets element names
	char const *GetElementName( unsigned i ) const;

	// Number of elements
	int Count() const;
	
	// Checks if a node is valid and in the tree
	bool  IsValidIndex( unsigned i ) const;
	
	// Invalid index
	static unsigned InvalidHandle();
	
	// Insert method (inserts in order)
	unsigned  Insert( const char *pName, const T &element );
	unsigned  Insert( const char *pName );
	
	// Find method
	unsigned  Find( const char *pName ) const;
	
	// Remove methods
	void	RemoveAt( unsigned i );
	void	Remove( const char *pName );
	void	RemoveAll( );
	
	// Purge memory
	void	Purge();
	void	PurgeAndDeleteElements();	// Call delete on each element.

	// Iteration methods
	unsigned		First() const;
	unsigned		Next( unsigned i ) const;

protected:
	struct Entry_t
	{
		const char *pszSymbol;
		T value;
	};

	template <bool bCaseInsensitive>
	class CCompare
	{
	public:
		CCompare( int ignored ) {}

		bool operator()( const Entry_t &entry1, const Entry_t &entry2 ) const
		{
			return !( ( bCaseInsensitive ) ? stricmp( entry1.pszSymbol, entry2.pszSymbol ) : strcmp( entry1.pszSymbol, entry2.pszSymbol ) );
		}
	};

	template <bool bCaseInsensitive>
	class CHash
	{
	public:
		CHash( int ignored ) {}

		unsigned operator()( const Entry_t &entry ) const
		{
			return !( ( bCaseInsensitive ) ? HashStringCaseless( entry.pszSymbol ) : HashString( entry.pszSymbol ) );
		}
	};

	typedef CUtlHash<Entry_t, CCompare<bCaseInsensitive>, CHash<bCaseInsensitive> > CHashTable;
	CHashTable m_Elements;
	int m_nCount;
};

//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::CUtlHashDict( int bucketCount = 16, int growCount = 0, int initCount = 0 ) : 
	m_Elements( SmallestPowerOfTwoGreaterOrEqual(bucketCount), growCount, initCount )
{
	Assert( SmallestPowerOfTwoGreaterOrEqual(bucketCount) <= 0xffff );
}

template <typename T, bool bCaseInsensitive, bool bDupeStrings> 
CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::~CUtlHashDict()
{
	Purge();
}

//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline T& CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Element( unsigned i )        
{ 
	return m_Elements[i].value; 
}

template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline const T& CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Element( unsigned i ) const  
{ 
	return m_Elements[i].value; 
}

//-----------------------------------------------------------------------------
// gets element names
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline char const *CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::GetElementName( unsigned i ) const
{
	return m_Elements[i].pszSymbol;
}

template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline T& CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::operator[]( unsigned i )        
{ 
	return m_Elements[i].value; 
}

template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline const T & CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::operator[]( unsigned i ) const  
{ 
	return m_Elements[i].value; 
}

//-----------------------------------------------------------------------------
// Num elements
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline int CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Count() const          
{ 
	Assert( m_nCount == m_Elements.Count() );
	return m_nCount; 
}

	
//-----------------------------------------------------------------------------
// Checks if a node is valid and in the tree
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline	bool CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::IsValidIndex( unsigned i ) const 
{
	return m_Elements.IsValidHandle(i);
}
	
	
//-----------------------------------------------------------------------------
// Invalid index
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
inline unsigned CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::InvalidHandle()         
{ 
	return CHashTable::InvalidHandle(); 
}

	
//-----------------------------------------------------------------------------
// Delete a node from the tree
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
void CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::RemoveAt(unsigned elem) 
{
	if ( bDupeStrings )
	{
		free( (void *)m_Elements[elem].pszSymbol );
	}
	m_Elements.Remove(elem);
	m_nCount--;
}


//-----------------------------------------------------------------------------
// remove a node in the tree
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings> void CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Remove( const char *search )
{
	unsigned node = Find( search );
	if (node != InvalidHandle())
	{
		RemoveAt(node);
	}
}


//-----------------------------------------------------------------------------
// Removes all nodes from the tree
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings>
void CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::RemoveAll()
{
	if ( bDupeStrings )
	{
		typename UtlHashHandle_t index = m_Elements.GetFirstHandle();
		while ( index != m_Elements.InvalidHandle() )
		{
			free( (void *)m_Elements[index].pszSymbol );
			index = m_Elements.GetNextHandle( index );
		}
	}

	m_Elements.RemoveAll();
	m_nCount = 0;
}

template <typename T, bool bCaseInsensitive, bool bDupeStrings>
void CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Purge()
{
	if ( bDupeStrings )
	{
		typename UtlHashHandle_t index = m_Elements.GetFirstHandle();
		while ( index != m_Elements.InvalidHandle() )
		{
			free( (void *)m_Elements[index].pszSymbol );
			index = m_Elements.GetNextHandle( index );
		}
	}

	m_Elements.Purge();
	m_nCount = 0;
}


template <typename T, bool bCaseInsensitive, bool bDupeStrings>
void CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::PurgeAndDeleteElements()
{
	// Delete all the elements.
	unsigned index = m_Elements.GetFirstHandle();
	while ( index != m_Elements.InvalidHandle() )
	{
		if ( bDupeStrings )
		{
			free( (void *)m_Elements[index].pszSymbol );
		}
		delete m_Elements[index].value;
		index = m_Elements.GetNextHandle( index );
	}

	m_Elements.RemoveAll();
	m_nCount = 0;
}


//-----------------------------------------------------------------------------
// inserts a node into the tree
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings> 
unsigned CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Insert( const char *pName, const T &element )
{
	MEM_ALLOC_CREDIT_CLASS();
	m_nCount++;
	Entry_t entry = 
	{
		(bDupeStrings) ? strdup( pName ) : pName,
		element
	};
	bool bInserted;
	unsigned result = m_Elements.Insert( entry, &bInserted );
	if ( bDupeStrings && !bInserted )
	{
		free( (void *)entry.pszSymbol );
	}
	return result;
}

template <typename T, bool bCaseInsensitive, bool bDupeStrings> 
unsigned CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Insert( const char *pName )
{
	MEM_ALLOC_CREDIT_CLASS();
	m_nCount++;
	Entry_t entry = 
	{
		(bDupeStrings) ? strdup( pName ) : pName
	};
	bool bInserted;
	unsigned result = m_Elements.Insert( entry, &bInserted );
	if ( bDupeStrings && !bInserted )
	{
		free( (void *)entry.pszSymbol );
	}
	return result;
}


//-----------------------------------------------------------------------------
// finds a node in the tree
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings> 
unsigned CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Find( const char *pName ) const
{
	MEM_ALLOC_CREDIT_CLASS();
	if ( pName )
		return m_Elements.Find( *((Entry_t *)&pName) );
	else
		return InvalidHandle();
}


//-----------------------------------------------------------------------------
// Iteration methods
//-----------------------------------------------------------------------------
template <typename T, bool bCaseInsensitive, bool bDupeStrings> 
unsigned CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::First() const
{
	return m_Elements.GetFirstHandle();
}

template <typename T, bool bCaseInsensitive, bool bDupeStrings> 
unsigned CUtlHashDict<T, bCaseInsensitive, bDupeStrings>::Next( unsigned i ) const
{
	return m_Elements.GetNextHandle(i);
}

#endif // UTLHASHDICT_H

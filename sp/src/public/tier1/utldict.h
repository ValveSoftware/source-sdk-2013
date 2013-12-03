//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A dictionary mapping from symbol to structure 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLDICT_H
#define UTLDICT_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "tier1/utlmap.h"

// Include this because tons of code was implicitly getting utlsymbol or utlvector via utldict.h
#include "tier1/utlsymbol.h"

#include "tier0/memdbgon.h"

enum EDictCompareType
{
	k_eDictCompareTypeCaseSensitive=0,
	k_eDictCompareTypeCaseInsensitive=1,
	k_eDictCompareTypeFilenames				// Slashes and backslashes count as the same character..
};

//-----------------------------------------------------------------------------
// A dictionary mapping from symbol to structure
//-----------------------------------------------------------------------------
#define FOR_EACH_DICT( dictName, iteratorName ) \
	for( int iteratorName=dictName.First(); iteratorName != dictName.InvalidIndex(); iteratorName = dictName.Next( iteratorName ) )

// faster iteration, but in an unspecified order
#define FOR_EACH_DICT_FAST( dictName, iteratorName ) \
	for ( int iteratorName = 0; iteratorName < dictName.MaxElement(); ++iteratorName ) if ( !dictName.IsValidIndex( iteratorName ) ) continue; else

//-----------------------------------------------------------------------------
// A dictionary mapping from symbol to structure
//-----------------------------------------------------------------------------
template <class T, class I = int > 
class CUtlDict
{
public:
	// constructor, destructor
	// Left at growSize = 0, the memory will first allocate 1 element and double in size
	// at each increment.
	CUtlDict( int compareType = k_eDictCompareTypeCaseInsensitive, int growSize = 0, int initSize = 0 );
	~CUtlDict( );

	void EnsureCapacity( int );
	
	// gets particular elements
	T&         Element( I i );
	const T&   Element( I i ) const;
	T&         operator[]( I i );
	const T&   operator[]( I i ) const;

	// gets element names
	char	   *GetElementName( I i );
	char const *GetElementName( I i ) const;

	void		SetElementName( I i, char const *pName );

	// Number of elements
	unsigned int Count() const;

	// Number of allocated slots
	I MaxElement() const;
	
	// Checks if a node is valid and in the tree
	bool  IsValidIndex( I i ) const;
	
	// Invalid index
	static I InvalidIndex();
	
	// Insert method (inserts in order)
	I  Insert( const char *pName, const T &element );
	I  Insert( const char *pName );
	
	// Find method
	I  Find( const char *pName ) const;
	bool HasElement( const char *pName ) const;
	
	// Remove methods
	void	RemoveAt( I i );
	void	Remove( const char *pName );
	void	RemoveAll( );
	
	// Purge memory
	void	Purge();
	void	PurgeAndDeleteElements();	// Call delete on each element.

	// Iteration methods
	I		First() const;
	I		Next( I i ) const;

	// Nested typedefs, for code that might need 
	// to fish out the index type from a given dict
	typedef I IndexType_t;

protected:
	typedef CUtlMap<const char *, T, I> DictElementMap_t;
	DictElementMap_t m_Elements;
};


//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------
template <class T, class I>
CUtlDict<T, I>::CUtlDict( int compareType, int growSize, int initSize ) : m_Elements( growSize, initSize )
{
	if ( compareType == k_eDictCompareTypeFilenames )
	{
		m_Elements.SetLessFunc( CaselessStringLessThanIgnoreSlashes );
	}
	else if ( compareType == k_eDictCompareTypeCaseInsensitive )
	{
		m_Elements.SetLessFunc( CaselessStringLessThan );
	}
	else
	{
		m_Elements.SetLessFunc( StringLessThan );
	}
}

template <class T, class I> 
CUtlDict<T, I>::~CUtlDict()
{
	Purge();
}

template <class T, class I>
inline void CUtlDict<T, I>::EnsureCapacity( int num )        
{ 
	return m_Elements.EnsureCapacity( num ); 
}

//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------
template <class T, class I>
inline T& CUtlDict<T, I>::Element( I i )        
{ 
	return m_Elements[i]; 
}

template <class T, class I>
inline const T& CUtlDict<T, I>::Element( I i ) const  
{ 
	return m_Elements[i]; 
}

//-----------------------------------------------------------------------------
// gets element names
//-----------------------------------------------------------------------------
template <class T, class I>
inline char *CUtlDict<T, I>::GetElementName( I i )
{
	return (char *)m_Elements.Key( i );
}

template <class T, class I>
inline char const *CUtlDict<T, I>::GetElementName( I i ) const
{
	return m_Elements.Key( i );
}

template <class T, class I>
inline T& CUtlDict<T, I>::operator[]( I i )        
{ 
	return Element(i); 
}

template <class T, class I>
inline const T & CUtlDict<T, I>::operator[]( I i ) const  
{ 
	return Element(i); 
}

template <class T, class I>
inline void CUtlDict<T, I>::SetElementName( I i, char const *pName )
{
	MEM_ALLOC_CREDIT_CLASS();
	// TODO:  This makes a copy of the old element
	// TODO:  This relies on the rb tree putting the most recently
	//  removed element at the head of the insert list
	free( (void *)m_Elements.Key( i ) );
	m_Elements.Reinsert( strdup( pName ), i );
}

//-----------------------------------------------------------------------------
// Num elements
//-----------------------------------------------------------------------------
template <class T, class I>
inline	unsigned int CUtlDict<T, I>::Count() const          
{ 
	return m_Elements.Count(); 
}

//-----------------------------------------------------------------------------
// Number of allocated slots
//-----------------------------------------------------------------------------
template <class T, class I>
inline I CUtlDict<T, I>::MaxElement() const
{
	return m_Elements.MaxElement();
}
	
//-----------------------------------------------------------------------------
// Checks if a node is valid and in the tree
//-----------------------------------------------------------------------------
template <class T, class I>
inline	bool CUtlDict<T, I>::IsValidIndex( I i ) const 
{
	return m_Elements.IsValidIndex(i);
}
	
	
//-----------------------------------------------------------------------------
// Invalid index
//-----------------------------------------------------------------------------
template <class T, class I>
inline I CUtlDict<T, I>::InvalidIndex()         
{ 
	return DictElementMap_t::InvalidIndex(); 
}

	
//-----------------------------------------------------------------------------
// Delete a node from the tree
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlDict<T, I>::RemoveAt(I elem) 
{
	free( (void *)m_Elements.Key( elem ) );
	m_Elements.RemoveAt(elem);
}


//-----------------------------------------------------------------------------
// remove a node in the tree
//-----------------------------------------------------------------------------
template <class T, class I> void CUtlDict<T, I>::Remove( const char *search )
{
	I node = Find( search );
	if (node != InvalidIndex())
	{
		RemoveAt(node);
	}
}


//-----------------------------------------------------------------------------
// Removes all nodes from the tree
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlDict<T, I>::RemoveAll()
{
	typename DictElementMap_t::IndexType_t index = m_Elements.FirstInorder();
	while ( index != m_Elements.InvalidIndex() )
	{
		free( (void *)m_Elements.Key( index ) );
		index = m_Elements.NextInorder( index );
	}

	m_Elements.RemoveAll();
}

template <class T, class I>
void CUtlDict<T, I>::Purge()
{
	RemoveAll();
}


template <class T, class I>
void CUtlDict<T, I>::PurgeAndDeleteElements()
{
	// Delete all the elements.
	I index = m_Elements.FirstInorder();
	while ( index != m_Elements.InvalidIndex() )
	{
		free( (void *)m_Elements.Key( index ) );
		delete m_Elements[index];
		index = m_Elements.NextInorder( index );
	}

	m_Elements.RemoveAll();
}


//-----------------------------------------------------------------------------
// inserts a node into the tree
//-----------------------------------------------------------------------------
template <class T, class I> 
I CUtlDict<T, I>::Insert( const char *pName, const T &element )
{
	MEM_ALLOC_CREDIT_CLASS();
	return m_Elements.Insert( strdup( pName ), element );
}

template <class T, class I> 
I CUtlDict<T, I>::Insert( const char *pName )
{
	MEM_ALLOC_CREDIT_CLASS();
	return m_Elements.Insert( strdup( pName ) );
}


//-----------------------------------------------------------------------------
// finds a node in the tree
//-----------------------------------------------------------------------------
template <class T, class I> 
I CUtlDict<T, I>::Find( const char *pName ) const
{
	MEM_ALLOC_CREDIT_CLASS();
	if ( pName )
		return m_Elements.Find( pName );
	else
		return InvalidIndex();
}

//-----------------------------------------------------------------------------
// returns true if we already have this node
//-----------------------------------------------------------------------------
template <class T, class I> 
bool CUtlDict<T, I>::HasElement( const char *pName ) const
{
	if ( pName )
		return m_Elements.IsValidIndex( m_Elements.Find( pName ) );
	else
		return false;
}


//-----------------------------------------------------------------------------
// Iteration methods
//-----------------------------------------------------------------------------
template <class T, class I> 
I CUtlDict<T, I>::First() const
{
	return m_Elements.FirstInorder();
}

template <class T, class I> 
I CUtlDict<T, I>::Next( I i ) const
{
	return m_Elements.NextInorder(i);
}

#include "tier0/memdbgoff.h"

#endif // UTLDICT_H

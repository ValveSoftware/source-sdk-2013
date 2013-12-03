//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Header: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLMAP_H
#define UTLMAP_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "utlrbtree.h"

//-----------------------------------------------------------------------------
//
// Purpose:	An associative container. Pretty much identical to std::map.
//
//-----------------------------------------------------------------------------

// This is a useful macro to iterate from start to end in order in a map
#define FOR_EACH_MAP( mapName, iteratorName ) \
	for ( int iteratorName = (mapName).FirstInorder(); (mapName).IsUtlMap && iteratorName != (mapName).InvalidIndex(); iteratorName = (mapName).NextInorder( iteratorName ) )

// faster iteration, but in an unspecified order
#define FOR_EACH_MAP_FAST( mapName, iteratorName ) \
	for ( int iteratorName = 0; (mapName).IsUtlMap && iteratorName < (mapName).MaxElement(); ++iteratorName ) if ( !(mapName).IsValidIndex( iteratorName ) ) continue; else

struct base_utlmap_t
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
};	

template <typename K, typename T, typename I = unsigned short> 
class CUtlMap : public base_utlmap_t
{
public:
	typedef K KeyType_t;
	typedef T ElemType_t;
	typedef I IndexType_t;

	// Less func typedef
	// Returns true if the first parameter is "less" than the second
	typedef bool (*LessFunc_t)( const KeyType_t &, const KeyType_t & );
	
	// constructor, destructor
	// Left at growSize = 0, the memory will first allocate 1 element and double in size
	// at each increment.
	// LessFunc_t is required, but may be set after the constructor using SetLessFunc() below
	CUtlMap( int growSize = 0, int initSize = 0, LessFunc_t lessfunc = 0 )
	 : m_Tree( growSize, initSize, CKeyLess( lessfunc ) )
	{
	}
	
	CUtlMap( LessFunc_t lessfunc )
	 : m_Tree( CKeyLess( lessfunc ) )
	{
	}
	
	void EnsureCapacity( int num )							{ m_Tree.EnsureCapacity( num ); }

	// gets particular elements
	ElemType_t &		Element( IndexType_t i )			{ return m_Tree.Element( i ).elem; }
	const ElemType_t &	Element( IndexType_t i ) const		{ return m_Tree.Element( i ).elem; }
	ElemType_t &		operator[]( IndexType_t i )			{ return m_Tree.Element( i ).elem; }
	const ElemType_t &	operator[]( IndexType_t i ) const	{ return m_Tree.Element( i ).elem; }
	KeyType_t &			Key( IndexType_t i )				{ return m_Tree.Element( i ).key; }
	const KeyType_t &	Key( IndexType_t i ) const			{ return m_Tree.Element( i ).key; }

	
	// Num elements
	unsigned int Count() const								{ return m_Tree.Count(); }
	
	// Max "size" of the vector
	IndexType_t  MaxElement() const							{ return m_Tree.MaxElement(); }
	
	// Checks if a node is valid and in the map
	bool  IsValidIndex( IndexType_t i ) const				{ return m_Tree.IsValidIndex( i ); }
	
	// Checks if the map as a whole is valid
	bool  IsValid() const									{ return m_Tree.IsValid(); }
	
	// Invalid index
	static IndexType_t InvalidIndex()						{ return CTree::InvalidIndex(); }
	
	// Sets the less func
	void SetLessFunc( LessFunc_t func )
	{
		m_Tree.SetLessFunc( CKeyLess( func ) );
	}
	
	// Insert method (inserts in order)
	IndexType_t  Insert( const KeyType_t &key, const ElemType_t &insert )
	{
		Node_t node;
		node.key = key;
		node.elem = insert;
		return m_Tree.Insert( node );
	}
	
	IndexType_t  Insert( const KeyType_t &key )
	{
		Node_t node;
		node.key = key;
		return m_Tree.Insert( node );
	}

	// Find method
	IndexType_t  Find( const KeyType_t &key ) const
	{
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.Find( dummyNode );
	}
	
	// Remove methods
	void     RemoveAt( IndexType_t i )						{ m_Tree.RemoveAt( i ); }
	bool     Remove( const KeyType_t &key )
	{
		Node_t dummyNode;
		dummyNode.key = key;
		return m_Tree.Remove( dummyNode );
	}
	
	void     RemoveAll( )									{ m_Tree.RemoveAll(); }
	void     Purge( )										{ m_Tree.Purge(); }

	// Purges the list and calls delete on each element in it.
	void PurgeAndDeleteElements();
		
	// Iteration
	IndexType_t  FirstInorder() const						{ return m_Tree.FirstInorder(); }
	IndexType_t  NextInorder( IndexType_t i ) const			{ return m_Tree.NextInorder( i ); }
	IndexType_t  PrevInorder( IndexType_t i ) const			{ return m_Tree.PrevInorder( i ); }
	IndexType_t  LastInorder() const						{ return m_Tree.LastInorder(); }		
	
	// If you change the search key, this can be used to reinsert the 
	// element into the map.
	void	Reinsert( const KeyType_t &key, IndexType_t i )
	{
		m_Tree[i].key = key;
		m_Tree.Reinsert(i);
	}

	IndexType_t InsertOrReplace( const KeyType_t &key, const ElemType_t &insert )
	{
		IndexType_t i = Find( key );
		if ( i != InvalidIndex() )
		{
			Element( i ) = insert;
			return i;
		}
		
		return Insert( key, insert );
	}

	void Swap( CUtlMap< K, T, I > &that )
	{
		m_Tree.Swap( that.m_Tree );
	}


	struct Node_t
	{
		Node_t()
		{
		}

		Node_t( const Node_t &from )
		  : key( from.key ),
			elem( from.elem )
		{
		}

		KeyType_t	key;
		ElemType_t	elem;
	};
	
	class CKeyLess
	{
	public:
		CKeyLess( LessFunc_t lessFunc ) : m_LessFunc(lessFunc) {}

		bool operator!() const
		{
			return !m_LessFunc;
		}

		bool operator()( const Node_t &left, const Node_t &right ) const
		{
			return m_LessFunc( left.key, right.key );
		}

		LessFunc_t m_LessFunc;
	};

	typedef CUtlRBTree<Node_t, I, CKeyLess> CTree;

	CTree *AccessTree()	{ return &m_Tree; }

protected:
	CTree 	   m_Tree;
};

//-----------------------------------------------------------------------------

// Purges the list and calls delete on each element in it.
template< typename K, typename T, typename I >
inline void CUtlMap<K, T, I>::PurgeAndDeleteElements()
{
	for ( I i = 0; i < MaxElement(); ++i ) 
	{
		if ( !IsValidIndex( i ) ) 
			continue; 
		
		delete Element( i );
	}

	Purge();
}

//-----------------------------------------------------------------------------

// This is horrible and slow and meant to be used only when you're dealing with really
// non-time/memory-critical code and desperately want to copy a whole map element-by-element
// for whatever reason.
template < typename K, typename T, typename I >
void DeepCopyMap( const CUtlMap<K,T,I>& pmapIn, CUtlMap<K,T,I> *out_pmapOut )
{
	Assert( out_pmapOut );

	out_pmapOut->Purge();
	FOR_EACH_MAP_FAST( pmapIn, i )
	{
		out_pmapOut->Insert( pmapIn.Key( i ), pmapIn.Element( i ) );
	}
}

#endif // UTLMAP_H

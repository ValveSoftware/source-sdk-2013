//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: N-way tree container class 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLNTREE_H
#define UTLNTREE_H

#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "utlmemory.h"
#include "tier0/dbg.h"


#define INVALID_NTREE_IDX ((I)~0)

//-----------------------------------------------------------------------------
// class CUtlNTree:
// description:
//		A lovely index-based linked list! T is the class type, I is the index
//		type, which usually should be an unsigned short or smaller.
//-----------------------------------------------------------------------------
template <class T, class I = unsigned short> 
class CUtlNTree
{
public:
	typedef T ElemType_t;
	typedef I IndexType_t;

	// constructor, destructor
	CUtlNTree( int growSize = 0, int initSize = 0 );
	CUtlNTree( void *pMemory, int memsize );
	~CUtlNTree( );

	// gets particular elements
	T&         Element( I i );
	const T&   Element( I i ) const;
	T&         operator[]( I i );
	const T&   operator[]( I i ) const;

	// Make sure we have a particular amount of memory
	void EnsureCapacity( int num );

	// Clears the tree, doesn't deallocate memory
	void RemoveAll();

	// Memory deallocation
	void Purge();

	// Allocation/deallocation methods
	I		Alloc( );
	void	Free( I elem );
	void	FreeSubTree( I elem );

	// list modification
	void	SetRoot( I root );
	void	LinkChildBefore( I parent, I before, I elem );
	void	LinkChildAfter( I parent, I after, I elem );
	void	Unlink( I elem );

	// Alloc + link combined
	I		InsertChildBefore( I parent, I before );
	I		InsertChildAfter( I parent, I after );
	I		InsertChildBefore( I parent, I before, const T &elem );
	I		InsertChildAfter( I parent, I after, const T &elem );

	// Unlink + free combined
	void	Remove( I elem );
	void	RemoveSubTree( I elem );

	// invalid index
	inline static I  InvalidIndex()  { return INVALID_NTREE_IDX; }
	inline static size_t ElementSize() { return sizeof(Node_t); }

	// list statistics
	int	Count() const;
	I	MaxElementIndex() const;

	// Traversing the list
	I  Root() const;
	I  FirstChild( I i ) const;
	I  PrevSibling( I i ) const;
	I  NextSibling( I i ) const;
	I  Parent( I i ) const;

	// Are nodes in the list or valid?
	bool  IsValidIndex( I i ) const;
	bool  IsInTree( I i ) const;
   
protected:
	// What the linked list element looks like
	struct Node_t
	{
		T  m_Element;
		I  m_Parent;
		I  m_FirstChild;
		I  m_PrevSibling;
		I  m_NextSibling;

	private:
		// No copy constructor for these...
		Node_t( const Node_t& );
	};
	
	// constructs the class
	void ConstructList();

	// Allocates the element, doesn't call the constructor
	I AllocInternal();

	// Gets at the node element....
	Node_t& InternalNode( I i ) { return m_Memory[i]; }
	const Node_t& InternalNode( I i ) const { return m_Memory[i]; }

	void ResetDbgInfo()
	{
		m_pElements = m_Memory.Base();
	}
	
	// copy constructors not allowed
	CUtlNTree( CUtlNTree<T, I> const& tree ) { Assert(0); }
	   
	CUtlMemory<Node_t> m_Memory;
	I	m_Root;
	I	m_FirstFree;
	I	m_ElementCount;		// The number actually in the tree
	I	m_MaxElementIndex;	// The max index we've ever assigned
	
	// For debugging purposes; 
	// it's in release builds so this can be used in libraries correctly
	Node_t  *m_pElements;
};
   
   
//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

template <class T, class I>
CUtlNTree<T,I>::CUtlNTree( int growSize, int initSize ) :
	m_Memory(growSize, initSize) 
{
	ConstructList();
	ResetDbgInfo();
}

template <class T, class I>
CUtlNTree<T,I>::CUtlNTree( void* pMemory, int memsize ) : 
	m_Memory(pMemory, memsize/sizeof(T))
{
	ConstructList();
	ResetDbgInfo();
}

template <class T, class I>
CUtlNTree<T,I>::~CUtlNTree( ) 
{
	RemoveAll();
}

template <class T, class I>
void CUtlNTree<T,I>::ConstructList()
{
	m_Root = InvalidIndex(); 
	m_FirstFree = InvalidIndex();
	m_ElementCount = m_MaxElementIndex = 0;
}


//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------
template <class T, class I>
inline T& CUtlNTree<T,I>::Element( I i )
{
	return m_Memory[i].m_Element; 
}

template <class T, class I>
inline const T& CUtlNTree<T,I>::Element( I i ) const
{
	return m_Memory[i].m_Element; 
}

template <class T, class I>
inline T& CUtlNTree<T,I>::operator[]( I i )        
{ 
	return m_Memory[i].m_Element; 
}

template <class T, class I>
inline const T& CUtlNTree<T,I>::operator[]( I i ) const 
{
	return m_Memory[i].m_Element; 
}


//-----------------------------------------------------------------------------
// list statistics
//-----------------------------------------------------------------------------
template <class T, class I>
inline int CUtlNTree<T,I>::Count() const      
{ 
	return m_ElementCount; 
}

template <class T, class I>
inline I CUtlNTree<T,I>::MaxElementIndex() const   
{
	return m_MaxElementIndex;
}


//-----------------------------------------------------------------------------
// Traversing the list
//-----------------------------------------------------------------------------
template <class T, class I>
inline I  CUtlNTree<T,I>::Root() const  
{ 
	return m_Root; 
}

template <class T, class I>
inline I  CUtlNTree<T,I>::FirstChild( I i ) const  
{ 
	Assert( IsInTree(i) ); 
	return InternalNode(i).m_FirstChild; 
}

template <class T, class I>
inline I  CUtlNTree<T,I>::PrevSibling( I i ) const  
{ 
	Assert( IsInTree(i) ); 
	return InternalNode(i).m_PrevSibling; 
}

template <class T, class I>
inline I  CUtlNTree<T,I>::NextSibling( I i ) const  
{ 
	Assert( IsInTree(i) ); 
	return InternalNode(i).m_NextSibling; 
}

template <class T, class I>
inline I  CUtlNTree<T,I>::Parent( I i ) const  
{ 
	Assert( IsInTree(i) ); 
	return InternalNode(i).m_Parent; 
}


//-----------------------------------------------------------------------------
// Are nodes in the list or valid?
//-----------------------------------------------------------------------------
template <class T, class I>
inline bool CUtlNTree<T,I>::IsValidIndex( I i ) const  
{ 
	return (i < m_MaxElementIndex) && (i >= 0);
}

template <class T, class I>
inline bool CUtlNTree<T,I>::IsInTree( I i ) const
{
	return (i < m_MaxElementIndex) && (i >= 0) && (InternalNode(i).m_PrevSibling != i);
}


//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlNTree<T, I>::EnsureCapacity( int num )
{
	MEM_ALLOC_CREDIT_CLASS();
	m_Memory.EnsureCapacity(num);
	ResetDbgInfo();
}


//-----------------------------------------------------------------------------
// Deallocate memory
//-----------------------------------------------------------------------------
template <class T, class I>
void  CUtlNTree<T,I>::Purge()
{
	RemoveAll();
	m_Memory.Purge( );
	m_FirstFree = InvalidIndex();
	m_MaxElementIndex = 0;
	ResetDbgInfo();
}


//-----------------------------------------------------------------------------
// Node allocation/deallocation
//-----------------------------------------------------------------------------
template <class T, class I>
I CUtlNTree<T,I>::AllocInternal( )
{
	I elem;
	if ( m_FirstFree == INVALID_NTREE_IDX )
	{
		// Nothing in the free list; add.
		// Since nothing is in the free list, m_MaxElementIndex == total # of elements
		// the list knows about.
		if ((int)m_MaxElementIndex == m_Memory.NumAllocated())
		{
			MEM_ALLOC_CREDIT_CLASS();
			m_Memory.Grow();
		}

		Assert( m_MaxElementIndex != INVALID_NTREE_IDX );

		elem = (I)m_MaxElementIndex;
		++m_MaxElementIndex;

		if ( elem == InvalidIndex() )
		{
			Error("CUtlNTree overflow!\n");
		}
	} 
	else
	{
		elem = m_FirstFree;
		m_FirstFree = InternalNode( m_FirstFree ).m_NextSibling;
	}
	
	Node_t &node = InternalNode( elem );
	node.m_NextSibling = node.m_PrevSibling = node.m_Parent = node.m_FirstChild = INVALID_NTREE_IDX;
	ResetDbgInfo();

	// one more element baby
	++m_ElementCount;

	return elem;
}

template <class T, class I>
I CUtlNTree<T,I>::Alloc( )
{
	I elem = AllocInternal();
	Construct( &Element(elem) );
	return elem;
}

template <class T, class I>
void CUtlNTree<T,I>::Free( I elem )
{
	Assert( IsInTree( elem ) );
	Unlink( elem );

	// If there's children, this will result in leaks. Use FreeSubTree instead.
	Assert( FirstChild( elem ) == INVALID_NTREE_IDX );

	Node_t &node = InternalNode( elem );
	Destruct( &node.m_Element );
	node.m_NextSibling = m_FirstFree;
	node.m_PrevSibling = elem;	// Marks it as being in the free list
	node.m_Parent = node.m_FirstChild = INVALID_NTREE_IDX;
	m_FirstFree = elem;

	// one less element baby
	--m_ElementCount;
}

template <class T, class I>
void CUtlNTree<T,I>::FreeSubTree( I elem )
{
	Assert( IsValidIndex( elem ) );

	I child = FirstChild( elem );
	while ( child != INVALID_NTREE_IDX )
	{
		I next = NextSibling( child );
		FreeSubTree( child );
		child = next;
	}

	Free( elem );
}


//-----------------------------------------------------------------------------
// Clears the tree
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlNTree<T,I>::RemoveAll()
{
	if ( m_MaxElementIndex == 0 )
		return;

	// Put everything into the free list (even unlinked things )
	I prev = InvalidIndex();
	for (int i = (int)m_MaxElementIndex; --i >= 0; prev = (I)i )
	{
		Node_t &node = InternalNode( i );
		if ( IsInTree( i ) )
		{
			Destruct( &node.m_Element );
		}

		node.m_NextSibling = prev;
		node.m_PrevSibling = (I)i;	// Marks it as being in the free list
		node.m_Parent = node.m_FirstChild = INVALID_NTREE_IDX;
	}
	
	// First free points to the first element
	m_FirstFree = 0;
	
	// Clear everything else out
	m_Root = INVALID_NTREE_IDX; 
	m_ElementCount = 0;
}


//-----------------------------------------------------------------------------
// list modification
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlNTree<T,I>::SetRoot( I root )
{
	// Resetting the root while it's got stuff in it is bad...
	Assert( m_Root == InvalidIndex() );
	m_Root = root;
}

	
//-----------------------------------------------------------------------------
// Links a node after a particular node
//-----------------------------------------------------------------------------
template <class T, class I>
void  CUtlNTree<T,I>::LinkChildAfter( I parent, I after, I elem )
{
	Assert( IsInTree(elem) );
	
	// Unlink it if it's in the list at the moment
	Unlink(elem);
	
	Node_t& newElem = InternalNode(elem);
	newElem.m_Parent = parent;
	newElem.m_PrevSibling = after;
	if ( after != INVALID_NTREE_IDX )
	{
		Node_t& prevSiblingNode = InternalNode( after );
		newElem.m_NextSibling = prevSiblingNode.m_NextSibling;
		prevSiblingNode.m_NextSibling = elem;
	}
	else
	{
		if ( parent != INVALID_NTREE_IDX )
		{
			Node_t& parentNode = InternalNode( parent );
			newElem.m_NextSibling = parentNode.m_FirstChild;
			parentNode.m_FirstChild = elem;
		}
		else
		{
			newElem.m_NextSibling = m_Root;
			if ( m_Root != INVALID_NTREE_IDX )
			{
				Node_t& rootNode = InternalNode( m_Root );
				rootNode.m_PrevSibling = elem;
			}
			m_Root = elem;
		}
	}

	if ( newElem.m_NextSibling != INVALID_NTREE_IDX )
	{
		Node_t& nextSiblingNode = InternalNode( newElem.m_NextSibling );
		nextSiblingNode.m_PrevSibling = elem;
	}
}


//-----------------------------------------------------------------------------
// Links a node before a particular node
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlNTree<T,I>::LinkChildBefore( I parent, I before, I elem )
{
	Assert( IsValidIndex(elem) );
	
	if ( before != INVALID_NTREE_IDX )
	{
		LinkChildAfter( parent, InternalNode( before ).m_PrevSibling, elem );
		return;
	}

	// NOTE: I made the choice to do an O(n) operation here
	// instead of store more data per node (LastChild).
	// This might not be the right choice. Revisit if we get perf problems.
	I after;
	if ( parent != INVALID_NTREE_IDX )
	{
		after = InternalNode( parent ).m_FirstChild;
	}
	else
	{
		after = m_Root;
	}

	if ( after == INVALID_NTREE_IDX )
	{
		LinkChildAfter( parent, after, elem );
		return;
	}

	I next = InternalNode( after ).m_NextSibling;
	while ( next != InvalidIndex() )
	{
		after = next;
		next = InternalNode( next ).m_NextSibling;
	}

	LinkChildAfter( parent, after, elem );
}


//-----------------------------------------------------------------------------
// Unlinks a node from the tree
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlNTree<T,I>::Unlink( I elem )
{
	Assert( IsInTree(elem) );

	Node_t *pOldNode = &InternalNode( elem );
	
	// If we're the first guy, reset the head
	// otherwise, make our previous node's next pointer = our next
	if ( pOldNode->m_PrevSibling != INVALID_NTREE_IDX )
	{
		InternalNode( pOldNode->m_PrevSibling ).m_NextSibling = pOldNode->m_NextSibling;
	}
	else
	{
		if ( pOldNode->m_Parent	!= INVALID_NTREE_IDX )
		{
			InternalNode( pOldNode->m_Parent ).m_FirstChild = pOldNode->m_NextSibling;
		}
		else if ( m_Root == elem )
		{
			m_Root = pOldNode->m_NextSibling;
		}
	}
	
	// If we're the last guy, reset the tail
	// otherwise, make our next node's prev pointer = our prev
	if ( pOldNode->m_NextSibling != INVALID_NTREE_IDX )
	{
		InternalNode( pOldNode->m_NextSibling ).m_PrevSibling = pOldNode->m_PrevSibling;
	}
	
	// Unlink everything except children
	pOldNode->m_Parent = pOldNode->m_PrevSibling = pOldNode->m_NextSibling = INVALID_NTREE_IDX;
}


//-----------------------------------------------------------------------------
// Alloc + link combined
//-----------------------------------------------------------------------------
template <class T, class I>
I CUtlNTree<T,I>::InsertChildBefore( I parent, I before )
{
	I elem = AllocInternal();
	Construct( &Element( elem ) );
	LinkChildBefore( parent, before, elem );
	return elem;
}

template <class T, class I>
I CUtlNTree<T,I>::InsertChildAfter( I parent, I after )
{
	I elem = AllocInternal();
	Construct( &Element( elem ) );
	LinkChildAfter( parent, after, elem );
	return elem;
}

template <class T, class I>
I CUtlNTree<T,I>::InsertChildBefore( I parent, I before, const T &data )
{
	I elem = AllocInternal();
	CopyConstruct( &Element( elem ), data );
	LinkChildBefore( parent, before, elem );
	return elem;
}

template <class T, class I>
I CUtlNTree<T,I>::InsertChildAfter( I parent, I after, const T &data )
{
	I elem = AllocInternal();
	CopyConstruct( &Element( elem ), data );
	LinkChildAfter( parent, after, elem );
	return elem;
}


//-----------------------------------------------------------------------------
// Unlink + free combined
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlNTree<T,I>::Remove( I elem )
{
	Unlink( elem );
	Free( elem );
}

template <class T, class I>
void CUtlNTree<T,I>::RemoveSubTree( I elem )
{
	UnlinkSubTree( elem );
	Free( elem );
}
  

#endif // UTLNTREE_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Multiple linked list container class 
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLMULTILIST_H
#define UTLMULTILIST_H

#ifdef _WIN32
#pragma once
#endif

#include "utllinkedlist.h"

// memdbgon must be the last include file in a .h file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// class CUtlMultiList:
// description:
//		A lovely index-based linked list! T is the class type, I is the index
//		type, which usually should be an unsigned short or smaller.
//		This list can contain multiple lists
//-----------------------------------------------------------------------------
template <class T, class I> 
class CUtlMultiList
{
protected:
	// What the linked list element looks like
	struct ListElem_t
	{
		T m_Element;
		I m_Previous;
		I m_Next;
	};
	
	struct List_t
	{
		I m_Head;
		I m_Tail;
		I m_Count;
	};

	typedef CUtlMemory<ListElem_t> M; // Keep naming similar to CUtlLinkedList
public:
	typedef I ListHandle_t;

	// constructor, destructor
	CUtlMultiList( int growSize = 0, int initSize = 0 );
	CUtlMultiList( void *pMemory, int memsize );
	~CUtlMultiList( );

	// gets particular elements
	T&         Element( I i );
	T const&   Element( I i ) const;
	T&         operator[]( I i );
	T const&   operator[]( I i ) const;

	// Make sure we have a particular amount of memory
	void EnsureCapacity( int num );

	// Memory deallocation
	void Purge();

	// List Creation/deletion
	ListHandle_t	CreateList();
	void			DestroyList( ListHandle_t list );
	bool			IsValidList( ListHandle_t list ) const;

	// Insertion methods (call default constructor)....
	I	InsertBefore( ListHandle_t list, I before );
	I	InsertAfter( ListHandle_t list, I after );
	I	AddToHead( ListHandle_t list ); 
	I	AddToTail( ListHandle_t list );

	// Insertion methods (call copy constructor)....
	I	InsertBefore( ListHandle_t list, I before, T const& src );
	I	InsertAfter( ListHandle_t list, I after, T const& src );
	I	AddToHead( ListHandle_t list, T const& src ); 
	I	AddToTail( ListHandle_t list, T const& src );

	// Removal methods
	void	Remove( ListHandle_t list, I elem );

	// Removes all items in a single list
	void	RemoveAll( ListHandle_t list );

	// Removes all items in all lists
	void	RemoveAll();

	// Allocation/deallocation methods
	// NOTE: To free, it must *not* be in a list!
	I		Alloc( );
	void	Free( I elem );

	// list modification
	void	LinkBefore( ListHandle_t list, I before, I elem );
	void	LinkAfter( ListHandle_t list, I after, I elem );
	void	Unlink( ListHandle_t list, I elem );
	void	LinkToHead( ListHandle_t list, I elem );
	void	LinkToTail( ListHandle_t list, I elem );

	// invalid index
	static I	InvalidIndex()		{ return (I)~0; }
	static bool	IndexInRange( int index );
	static size_t ElementSize()		{ return sizeof(ListElem_t); }

	// list statistics
	int	Count( ListHandle_t list ) const;
	int	TotalCount( ) const;
	I	MaxElementIndex() const;

	// Traversing the list
	I  Head( ListHandle_t list ) const;
	I  Tail( ListHandle_t list ) const;
	I  Previous( I element ) const;
	I  Next( I element ) const;

	// Are nodes in a list or valid?
	bool  IsValidIndex( I i ) const;
	bool  IsInList( I i ) const;
   
protected:
	// constructs the class
	void ConstructList( );
	
	// Gets at the list element....
	ListElem_t& InternalElement( I i ) { return m_Memory[i]; }
	ListElem_t const& InternalElement( I i ) const { return m_Memory[i]; }

	// A test for debug mode only...
	bool IsElementInList( ListHandle_t list, I elem ) const;

	// copy constructors not allowed
	CUtlMultiList( CUtlMultiList<T, I> const& list ) { Assert(0); }
	   
	M							m_Memory;
	CUtlLinkedList<List_t, I>	m_List;
	I*	m_pElementList;

	I	m_FirstFree;
	I	m_TotalElements;	
	int	m_MaxElementIndex;	// The number allocated (use int so we can catch overflow)

	void ResetDbgInfo()
	{
		m_pElements = m_Memory.Base();

#ifdef _DEBUG
		// Allocate space for the element list (which list is each element in)
		if (m_Memory.NumAllocated() > 0)
		{
			if (!m_pElementList)
			{
				m_pElementList = (I*)malloc( m_Memory.NumAllocated() * sizeof(I) );
			}
			else
			{
				m_pElementList = (I*)realloc( m_pElementList, m_Memory.NumAllocated() * sizeof(I) );
			}
		}
#endif
	}

	// For debugging purposes; 
	// it's in release builds so this can be used in libraries correctly
	ListElem_t  *m_pElements;
};
   
   
//-----------------------------------------------------------------------------
// constructor, destructor
//-----------------------------------------------------------------------------

template <class T, class I>
CUtlMultiList<T,I>::CUtlMultiList( int growSize, int initSize ) :
	m_Memory(growSize, initSize), m_pElementList(0)
{
	ConstructList();
}

template <class T, class I>
CUtlMultiList<T,I>::CUtlMultiList( void* pMemory, int memsize ) : 
	m_Memory((ListElem_t *)pMemory, memsize/sizeof(ListElem_t)), m_pElementList(0)
{
	ConstructList();
}

template <class T, class I>
CUtlMultiList<T,I>::~CUtlMultiList( ) 
{
	RemoveAll();
	if (m_pElementList)
		free(m_pElementList);
}

template <class T, class I>
void CUtlMultiList<T,I>::ConstructList( )
{
	m_FirstFree = InvalidIndex();
	m_TotalElements = 0;
	m_MaxElementIndex = 0;
	ResetDbgInfo();
}


//-----------------------------------------------------------------------------
// gets particular elements
//-----------------------------------------------------------------------------
template <class T, class I>
inline T& CUtlMultiList<T,I>::Element( I i )
{
	return m_Memory[i].m_Element; 
}

template <class T, class I>
inline T const& CUtlMultiList<T,I>::Element( I i ) const
{
	return m_Memory[i].m_Element; 
}

template <class T, class I>
inline T& CUtlMultiList<T,I>::operator[]( I i )        
{ 
	return m_Memory[i].m_Element; 
}

template <class T, class I>
inline T const& CUtlMultiList<T,I>::operator[]( I i ) const 
{
	return m_Memory[i].m_Element; 
}


//-----------------------------------------------------------------------------
// list creation/destruction
//-----------------------------------------------------------------------------
template <class T, class I> 
typename CUtlMultiList<T,I>::ListHandle_t CUtlMultiList<T,I>::CreateList()
{
	ListHandle_t l = m_List.AddToTail();
	m_List[l].m_Head = m_List[l].m_Tail = InvalidIndex();
	m_List[l].m_Count = 0;
	return l;
}

template <class T, class I>
void CUtlMultiList<T,I>::DestroyList( ListHandle_t list )
{
	Assert( IsValidList(list) );
	RemoveAll( list );
	m_List.Remove(list);
}

template <class T, class I>
bool CUtlMultiList<T,I>::IsValidList( ListHandle_t list ) const
{
	return m_List.IsValidIndex(list);
}


//-----------------------------------------------------------------------------
// list statistics
//-----------------------------------------------------------------------------
template <class T, class I>
inline int CUtlMultiList<T,I>::TotalCount() const      
{ 
	return m_TotalElements; 
}

template <class T, class I>
inline int CUtlMultiList<T,I>::Count( ListHandle_t list ) const
{
	Assert( IsValidList(list) );
	return m_List[list].m_Count;
}

template <class T, class I>
inline I CUtlMultiList<T,I>::MaxElementIndex() const   
{
	return m_MaxElementIndex;
}


//-----------------------------------------------------------------------------
// Traversing the list
//-----------------------------------------------------------------------------
template <class T, class I>
inline I  CUtlMultiList<T,I>::Head(ListHandle_t list) const  
{
	Assert( IsValidList(list) );
	return m_List[list].m_Head; 
}

template <class T, class I>
inline I  CUtlMultiList<T,I>::Tail(ListHandle_t list) const  
{ 
	Assert( IsValidList(list) );
	return m_List[list].m_Tail; 
}

template <class T, class I>
inline I  CUtlMultiList<T,I>::Previous( I i ) const  
{ 
	Assert( IsValidIndex(i) ); 
	return InternalElement(i).m_Previous; 
}

template <class T, class I>
inline I  CUtlMultiList<T,I>::Next( I i ) const  
{ 
	Assert( IsValidIndex(i) ); 
	return InternalElement(i).m_Next; 
}


//-----------------------------------------------------------------------------
// Are nodes in the list or valid?
//-----------------------------------------------------------------------------

template <class T, class I>
inline bool CUtlMultiList<T,I>::IndexInRange( int index ) // Static method
{
	// Since I is not necessarily the type returned by M (int), we need to check that M returns
	// indices which are representable by I. A common case is 'I === unsigned short', in which case
	// case CUtlMemory will have 'InvalidIndex == (int)-1' (which casts to 65535 in I), and will
	// happily return elements at index 65535 and above.

	// Do a couple of static checks here: the invalid index should be (I)~0 given how we use m_MaxElementIndex,
	// and 'I' should be unsigned (to avoid signed arithmetic errors for plausibly exhaustible ranges).
	COMPILE_TIME_ASSERT( (I)M::INVALID_INDEX == (I)~0 );
	COMPILE_TIME_ASSERT( ( sizeof(I) > 2 ) || ( ( (I)-1 ) > 0 ) );

	return ( ( (I)index == index ) && ( (I)index != InvalidIndex() ) );
}

template <class T, class I>
inline bool CUtlMultiList<T,I>::IsValidIndex( I i ) const  
{ 
	// GCC warns if I is an unsigned type and we do a ">= 0" against it (since the comparison is always 0).
	// We get the warning even if we cast inside the expression. It only goes away if we assign to another variable.
	long x = i;

 	return (i < m_MaxElementIndex) && (x >= 0) &&
		((m_Memory[i].m_Previous != i) || (m_Memory[i].m_Next == i));
}

template <class T, class I>
inline bool CUtlMultiList<T,I>::IsInList( I i ) const
{
	// GCC warns if I is an unsigned type and we do a ">= 0" against it (since the comparison is always 0).
	// We get the warning even if we cast inside the expression. It only goes away if we assign to another variable.
	long x = i;
	return (i < m_MaxElementIndex) && (x >= 0) && (Previous(i) != i);
}


//-----------------------------------------------------------------------------
// Makes sure we have enough memory allocated to store a requested # of elements
//-----------------------------------------------------------------------------
template< class T, class I >
void CUtlMultiList<T, I>::EnsureCapacity( int num )
{
	m_Memory.EnsureCapacity(num);
	ResetDbgInfo();
}


//-----------------------------------------------------------------------------
// Deallocate memory
//-----------------------------------------------------------------------------
template <class T, class I>
void  CUtlMultiList<T,I>::Purge()
{
	RemoveAll();
	m_List.Purge();
	m_Memory.Purge( );
	m_List.Purge();
	m_FirstFree = InvalidIndex();
	m_TotalElements = 0;
	m_MaxElementIndex = 0;
	ResetDbgInfo();
}


//-----------------------------------------------------------------------------
// Node allocation/deallocation
//-----------------------------------------------------------------------------
template <class T, class I>
I CUtlMultiList<T,I>::Alloc( )
{
	I elem;
	if (m_FirstFree == InvalidIndex())
	{
		// We can overflow before the utlmemory overflows, since we have have I != int
		if ( !IndexInRange( m_MaxElementIndex ) )
		{
			// We rarely if ever handle alloc failure. Continuing leads to corruption.
			Error( "CUtlMultiList overflow! (exhausted index range)\n" );
			return InvalidIndex();
		}

		// Nothing in the free list; add.
		// Since nothing is in the free list, m_TotalElements == total # of elements
		// the list knows about.
		if (m_MaxElementIndex == m_Memory.NumAllocated())
		{
			m_Memory.Grow();
			ResetDbgInfo();
			
			if ( m_MaxElementIndex >= m_Memory.NumAllocated() )
			{
				// We rarely if ever handle alloc failure. Continuing leads to corruption.
				Error( "CUtlMultiList overflow! (exhausted memory allocator)\n" );
				return InvalidIndex();
			}
		}
		
		elem = (I)m_MaxElementIndex;
		++m_MaxElementIndex;
	}
	else
	{
		elem = m_FirstFree;
		m_FirstFree = InternalElement(m_FirstFree).m_Next;
	}

	// Mark the element as not being in a list
	InternalElement(elem).m_Next = InternalElement(elem).m_Previous = elem;

	++m_TotalElements;

	Construct( &Element(elem) );

	return elem;
}

template <class T, class I>
void  CUtlMultiList<T,I>::Free( I elem )
{
	Assert( IsValidIndex(elem) && !IsInList(elem) );
	Destruct( &Element(elem) );
	InternalElement(elem).m_Next = m_FirstFree;
	m_FirstFree = elem;
	--m_TotalElements;
}


//-----------------------------------------------------------------------------
// A test for debug mode only...
//-----------------------------------------------------------------------------
template <class T, class I>
inline bool CUtlMultiList<T,I>::IsElementInList( ListHandle_t list, I elem ) const
{
	if (!m_pElementList)
		return true;

	return m_pElementList[elem] == list;
}


//-----------------------------------------------------------------------------
// list modification
//-----------------------------------------------------------------------------
template <class T, class I>
void CUtlMultiList<T,I>::LinkBefore( ListHandle_t list, I before, I elem )
{
	Assert( IsValidIndex(elem) && IsValidList(list) );
	
	// Unlink it if it's in the list at the moment
	Unlink(list, elem);
	
	ListElem_t& newElem = InternalElement(elem);
	
	// The element *after* our newly linked one is the one we linked before.
	newElem.m_Next = before;
	
	if (before == InvalidIndex())
	{
		// In this case, we're linking to the end of the list, so reset the tail
		newElem.m_Previous = m_List[list].m_Tail;
		m_List[list].m_Tail = elem;
	}
	else
	{
		// Here, we're not linking to the end. Set the prev pointer to point to
		// the element we're linking.
		Assert( IsInList(before) );
		ListElem_t& beforeElem = InternalElement(before);
		newElem.m_Previous = beforeElem.m_Previous;
		beforeElem.m_Previous = elem;
	}
	
	// Reset the head if we linked to the head of the list
	if (newElem.m_Previous == InvalidIndex())
		m_List[list].m_Head = elem;
	else
		InternalElement(newElem.m_Previous).m_Next = elem;
	
	// one more element baby
	++m_List[list].m_Count;

	// Store the element into the list
	if (m_pElementList)
		m_pElementList[elem] = list;
}

template <class T, class I>
void  CUtlMultiList<T,I>::LinkAfter( ListHandle_t list, I after, I elem )
{
	Assert( IsValidIndex(elem) );
	
	// Unlink it if it's in the list at the moment
	Unlink(list, elem);
	
	ListElem_t& newElem = InternalElement(elem);
	
	// The element *before* our newly linked one is the one we linked after
	newElem.m_Previous = after;
	if (after == InvalidIndex())
	{
		// In this case, we're linking to the head of the list, reset the head
		newElem.m_Next = m_List[list].m_Head;
		m_List[list].m_Head = elem;
	}
	else
	{
		// Here, we're not linking to the end. Set the next pointer to point to
		// the element we're linking.
		Assert( IsInList(after) );
		ListElem_t& afterElem = InternalElement(after);
		newElem.m_Next = afterElem.m_Next;
		afterElem.m_Next = elem;
	}
	
	// Reset the tail if we linked to the tail of the list
	if (newElem.m_Next == InvalidIndex())
		m_List[list].m_Tail = elem;
	else
		InternalElement(newElem.m_Next).m_Previous = elem;
	
	// one more element baby
	++m_List[list].m_Count;

	// Store the element into the list
	if (m_pElementList)
		m_pElementList[elem] = list;
}

template <class T, class I>
void  CUtlMultiList<T,I>::Unlink( ListHandle_t list, I elem )
{
	Assert( IsValidIndex(elem) && IsValidList(list) );

	if (IsInList(elem))
	{
		// Make sure the element is in the right list
		Assert( IsElementInList( list, elem ) );
		ListElem_t& oldElem = InternalElement(elem);
		
		// If we're the first guy, reset the head
		// otherwise, make our previous node's next pointer = our next
		if (oldElem.m_Previous != InvalidIndex())
			InternalElement(oldElem.m_Previous).m_Next = oldElem.m_Next;
		else
			m_List[list].m_Head = oldElem.m_Next;
		
		// If we're the last guy, reset the tail
		// otherwise, make our next node's prev pointer = our prev
		if (oldElem.m_Next != InvalidIndex())
			InternalElement(oldElem.m_Next).m_Previous = oldElem.m_Previous;
		else
			m_List[list].m_Tail = oldElem.m_Previous;
		
		// This marks this node as not in the list, 
		// but not in the free list either
		oldElem.m_Previous = oldElem.m_Next = elem;
		
		// One less puppy
		--m_List[list].m_Count;

		// Store the element into the list
		if (m_pElementList)
			m_pElementList[elem] = m_List.InvalidIndex();
	}
}

template <class T, class I>
inline void CUtlMultiList<T,I>::LinkToHead( ListHandle_t list, I elem ) 
{
	LinkAfter( list, InvalidIndex(), elem ); 
}

template <class T, class I>
inline void CUtlMultiList<T,I>::LinkToTail( ListHandle_t list, I elem ) 
{
	LinkBefore( list, InvalidIndex(), elem ); 
}


//-----------------------------------------------------------------------------
// Insertion methods; allocates and links (uses default constructor)
//-----------------------------------------------------------------------------
template <class T, class I>
I CUtlMultiList<T,I>::InsertBefore( ListHandle_t list, I before )
{
	// Make a new node
	I   newNode = Alloc();
	if ( newNode == InvalidIndex() )
		return newNode;

	// Link it in
	LinkBefore( list, before, newNode );
	
	// Construct the data
	Construct( &Element(newNode) );
	
	return newNode;
}

template <class T, class I>
I CUtlMultiList<T,I>::InsertAfter( ListHandle_t list, I after )
{
	// Make a new node
	I   newNode = Alloc();
	if ( newNode == InvalidIndex() )
		return newNode;

	// Link it in
	LinkAfter( list, after, newNode );
	
	// Construct the data
	Construct( &Element(newNode) );
	
	return newNode;
}

template <class T, class I>
inline I CUtlMultiList<T,I>::AddToHead( ListHandle_t list ) 
{ 
	return InsertAfter( list, InvalidIndex() ); 
}

template <class T, class I>
inline I CUtlMultiList<T,I>::AddToTail( ListHandle_t list ) 
{ 
	return InsertBefore( list, InvalidIndex() ); 
}


//-----------------------------------------------------------------------------
// Insertion methods; allocates and links (uses copy constructor)
//-----------------------------------------------------------------------------
template <class T, class I>
I CUtlMultiList<T,I>::InsertBefore( ListHandle_t list, I before, T const& src )
{
	// Make a new node
	I   newNode = Alloc();
	if ( newNode == InvalidIndex() )
		return newNode;

	// Link it in
	LinkBefore( list, before, newNode );
	
	// Construct the data
	CopyConstruct( &Element(newNode), src );
	
	return newNode;
}

template <class T, class I>
I CUtlMultiList<T,I>::InsertAfter( ListHandle_t list, I after, T const& src )
{
	// Make a new node
	I   newNode = Alloc();
	if ( newNode == InvalidIndex() )
		return newNode;

	// Link it in
	LinkAfter( list, after, newNode );
	
	// Construct the data
	CopyConstruct( &Element(newNode), src );
	
	return newNode;
}

template <class T, class I>
inline I CUtlMultiList<T,I>::AddToHead( ListHandle_t list, T const& src ) 
{ 
	return InsertAfter( list, InvalidIndex(), src ); 
}

template <class T, class I>
inline I CUtlMultiList<T,I>::AddToTail( ListHandle_t list, T const& src ) 
{ 
	return InsertBefore( list, InvalidIndex(), src ); 
}


//-----------------------------------------------------------------------------
// Removal methods
//-----------------------------------------------------------------------------
template <class T, class I>
void  CUtlMultiList<T,I>::Remove( ListHandle_t list, I elem )
{
	if (IsInList(elem))
		Unlink(list, elem);
	Free( elem );
}

// Removes all items in a single list
template <class T, class I>
void	CUtlMultiList<T,I>::RemoveAll( ListHandle_t list )
{
	Assert( IsValidList(list) );
	I i = Head(list);
	I next;
	while( i != InvalidIndex() )
	{
		next = Next(i);
		Remove(list, i);
		i = next;
	}
}


template <class T, class I>
void  CUtlMultiList<T,I>::RemoveAll()
{
	if (m_MaxElementIndex == 0)
		return;

	// Put everything into the free list
	I prev = InvalidIndex();
	for (int i = (int)m_MaxElementIndex; --i >= 0; )
	{
		// Invoke the destructor
		if (IsValidIndex((I)i))
			Destruct( &Element((I)i) );
		
		// next points to the next free list item
		InternalElement((I)i).m_Next = prev;
		
		// Indicates it's in the free list
		InternalElement((I)i).m_Previous = (I)i;
		prev = (I)i;
	}
	
	// First free points to the first element
	m_FirstFree = 0;
	
	// Clear everything else out
	for (I list = m_List.Head(); list != m_List.InvalidIndex(); list = m_List.Next(list) )
	{
		m_List[list].m_Head = InvalidIndex(); 
		m_List[list].m_Tail = InvalidIndex();
		m_List[list].m_Count = 0;
	}

	m_TotalElements = 0;
}


#include "tier0/memdbgoff.h"

#endif // UTLMULTILIST_H

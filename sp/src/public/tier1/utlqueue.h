//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef UTLQUEUE_H
#define UTLQUEUE_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

// T is the type stored in the stack
template< class T > 
class CUtlQueue
{
public:

	// constructor: lessfunc is required, but may be set after the constructor with
	// SetLessFunc
	CUtlQueue( int growSize = 0, int initSize = 0 );
	CUtlQueue( T *pMemory, int numElements );

	// return the item from the front of the queue and delete it
	T const& RemoveAtHead();
	// return the item from the end of the queue and delete it
	T const& RemoveAtTail();

	// return item at the front of the queue
	T const& Head();
	// return item at the end of the queue
	T const& Tail();

	// Add a new item to the end of the queue
	void	Insert( T const &element );

	// checks if an element of this value already exists on the stack, returns true if it does
	bool		Check( T const element );

	// Returns the count of elements in the stack
	int			Count() const { return m_heap.Count(); }
	
	// doesn't deallocate memory
	void		RemoveAll() { m_heap.RemoveAll(); }

	// Memory deallocation
	void		Purge() { m_heap.Purge(); }

protected:
	CUtlVector<T>	m_heap;
	T				m_current;
};

template< class T >
inline CUtlQueue<T>::CUtlQueue( int growSize, int initSize ) :
	m_heap(growSize, initSize)
{
}

template< class T >
inline CUtlQueue<T>::CUtlQueue( T *pMemory, int numElements )	: 
	m_heap(pMemory, numElements)
{
}

template <class T>
inline T const& CUtlQueue<T>::RemoveAtHead()
{
	m_current = m_heap[0];
	m_heap.Remove((int)0);
	return m_current; 
}

template <class T>
inline T const& CUtlQueue<T>::RemoveAtTail()
{
	m_current = m_heap[ m_heap.Count() - 1 ];
	m_heap.Remove((int)(m_heap.Count() - 1));
	return m_current; 
}

template <class T>
inline T const& CUtlQueue<T>::Head()
{
	m_current = m_heap[0];
	return m_current; 
}

template <class T>
inline T const& CUtlQueue<T>::Tail()
{
	m_current = m_heap[ m_heap.Count() - 1 ];
	return m_current; 
}

template <class T>
void CUtlQueue<T>::Insert( T const &element )
{
	int index = m_heap.AddToTail();
	m_heap[index] = element;
}

template <class T>
bool CUtlQueue<T>::Check( T const element )
{
	int index = m_heap.Find(element);
	return ( index != -1 );
}


#endif // UTLQUEUE_H

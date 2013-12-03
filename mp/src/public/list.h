//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef __LIST_H__
#define __LIST_H__

// TODO:
// GetPositionAtIndex needs to keep a cache of the previous call so 
// that it doesn't do a linear search every time.

#include <stdlib.h>

// FIXME: change the name of this to something more specific.
typedef struct
{
} _Position;
typedef _Position *Position;

template <class T> class GList;
template <class T> class GListIterator;

// GListNode: Class decleration and definition
template <class T> class GListNode
{
private:
	T data;
	GListNode<T> *next;
	GListNode<T> *prev;
	GListNode();
	GListNode( T item );
	friend class GList<T>;
	friend class GListIterator<T>;
};

template <class T>
GListNode<T>::GListNode()
{
	next = NULL;
	prev = NULL;
}

template <class T>
GListNode<T>::GListNode( T item )
{
	data = item;
	next = NULL;
	prev = NULL;
}

// GList: Class decleration and definition
template <class T> class GList
{
public:
	// Contructors/destructors
	GList();
	
	//
	Position InsertAtHead( T );
	Position InsertAtTail( T );
	T Remove( Position position );
	void RemoveAll( void (*DeleteItem)( T ) );
	void RemoveSelectedItems( bool (*NeedsRemovalFunc)( T ), void (*DeleteItemFunc)( T ) );	
	Position InsertAfter( T item, Position position );
	Position InsertBefore( T item, Position position );
	bool IsEmpty();
	int GetNumItems();
	T GetItemAtPosition( Position position );
	Position GetPositionAtIndex( int index );
	T GetItemAtIndex( int index );
	
protected:
	GListNode<T> *head;
	GListNode<T> *tail;
	int numItems;
	friend class GListIterator<T>;
};

template<class T> 
GList<T>::GList()
{
	// Set up a dummy head node and a dummy tail node.
	head = new GListNode<T>;
	tail = new GListNode<T>;
	head->next = tail;
	head->prev = head;
	tail->next = tail;
	tail->prev = head;
	numItems = 0;
}

template<class T> 
Position GList<T>::InsertAtHead( T item )
{
	GListNode<T> *newNode = new GListNode<T>( item );
	head->next->prev = newNode;
	newNode->next = head->next;
	newNode->prev = head;
	head->next = newNode;
	numItems++;
	return ( Position )( void * )newNode;
}

template<class T> 
Position GList<T>::InsertAtTail( T item )
{
	GListNode<T> *newNode = new GListNode<T>( item );
	tail->prev->next = newNode;
	newNode->prev = tail->prev;
	tail->prev = newNode;
	newNode->next = tail;
	numItems++;
	return ( Position )( void * )newNode;
}

template<class T>
T GList<T>::Remove( Position position )
{
	GListNode<T> *node = ( GListNode<T> * )( void * )position;
	node->prev->next = node->next;
	node->next->prev = node->prev;
	T data = node->data;
	numItems--;
	delete node;
	return data;
}

template<class T>
void GList<T>::RemoveAll( void (*DeleteItemFunc)( T ) )
{
	GListNode<T> *tmpNode;
	GListNode<T> *node = head->next;
	while( node != tail )
	{
		if( DeleteItemFunc )
		{
			DeleteItemFunc( node->data );
		}
		tmpNode = node->next;
		delete node;
		node = tmpNode;
	}
	head->next = tail;
	head->prev = head;
	tail->next = tail;
	tail->prev = head;
	numItems = 0;
}

template<class T>
void GList<T>::RemoveSelectedItems( bool (*NeedsRemovalFunc)( T ), void (*DeleteItemFunc)( T ) )
{
	GListNode<T> *tmpNode;
	GListNode<T> *node = head->next;
	while( node != tail )
	{
		if( NeedsRemovalFunc( node->data ) )
		{
			DeleteItemFunc( node->data );
			node->prev->next = node->next;
			node->next->prev = node->prev;
			tmpNode = node;
			node = node->next;
			delete tmpNode;
			numItems--;
		}
		else
		{
			node = node->next;
		}
	}
}

template<class T>
Position GList<T>::InsertAfter( T item, Position position )
{
	GListNode<T> *node = ( GListNode<T> * )( void * )position;
	GListNode<T> *newNode = new GListNode<T>( item );
	newNode->prev = node;
	newNode->next = node->next;
	node->next->prev = newNode;
	node->next = newNode;
	numItems++;
	return ( Position )( void * )newNode;
}

template<class T>
Position GList<T>::InsertBefore( T item, Position position )
{
	GListNode<T> *node = ( GListNode<T> * )( void * )position;
	GListNode<T> *newNode = new GListNode<T>( item );
	newNode->prev = node->prev;
	newNode->next = node;
	node->prev->next = newNode;
	node->prev = newNode;
	numItems++;
	return ( Position )( void * )newNode;
}

template<class T>
bool GList<T>::IsEmpty()
{
	return ( numItems == 0 );
}

template <class T>
int GList<T>::GetNumItems()
{
	return numItems;
}

template<class T>
T GList<T>::GetItemAtPosition( Position position )
{
	return ( ( GListNode<T> * )( void * )position )->data;
}

template<class T>
Position GList<T>::GetPositionAtIndex( int index )
{
	int i;
	GListNode<T> *node = head->next;
	for( i = 0; i < index; i++ )
    {
		node = node->next;
    }
	return ( Position )( void * )node;
}

template<class T>
T GList<T>::GetItemAtIndex( int index )
{
	return GetItemAtPosition( GetPositionAtIndex( index ) );
}

// GListIterator: Class decleration and definition
template<class T> class GListIterator
{
public:
	GListIterator( GList<T> *GList );
	void GotoHead( void );
	void GotoTail( void );
	void Goto( int index );
	void Increment();
	void Decrement();
	T GetCurrentAndIncrement();
	T GetCurrentAndDecrement();
	T IncrementAndGetCurrent();
	T DecrementAndGetCurrent();
	// postfix
	T operator++( int ) { return GetCurrentAndIncrement(); }
	T operator--( int ) { return GetCurrentAndDecrement(); }
	// prefix
	T operator++() { return IncrementAndGetCurrent(); }
	T operator--() { return DecrementAndGetCurrent(); }
	T GetCurrent();
	bool AtEnd( void );
	bool AtBeginning( void );
protected:
	GList<T> *list;
	GListNode<T> *currentNode;
};

template<class T>
GListIterator<T>::GListIterator( GList<T> *list )
{
	this->list = list;
	GotoHead();
}

template<class T>
void GListIterator<T>::GotoHead()
{
	this->currentNode = list->head->next;
}

template<class T>
void GListIterator<T>::GotoTail()
{
	this->currentNode = list->tail->prev;
}

template<class T>
void GListIterator<T>::Goto( int index )
{
	currentNode = ( GListNode<T> * )( void * )list->GetPositionAtIndex( index );
}

template<class T>
void GListIterator<T>::Increment()
{
	currentNode = currentNode->next;
}

template<class T>
void GListIterator<T>::Decrement()
{
	currentNode = currentNode->prev;
}

template<class T>
T GListIterator<T>::GetCurrentAndIncrement()
{
	T retval = currentNode->data;
	Increment();
	return retval;
}

template<class T>
T GListIterator<T>::GetCurrentAndDecrement()
{
	T retval = currentNode->data;
	Decrement();
	return retval;
}

template<class T>
T GListIterator<T>::IncrementAndGetCurrent()
{
	Increment();
	return GetCurrent();
}

template<class T>
T GListIterator<T>::DecrementAndGetCurrent()
{
	Decrement();
	return GetCurrent();
}

template<class T>
T GListIterator<T>::GetCurrent()
{
	return currentNode->data;
}

template<class T>
bool GListIterator<T>::AtEnd( void )
{
	return currentNode->next == currentNode;
}

template<class T>
bool GListIterator<T>::AtBeginning( void )
{
	return currentNode->prev == currentNode;
}

#endif /* __LIST_H__ */

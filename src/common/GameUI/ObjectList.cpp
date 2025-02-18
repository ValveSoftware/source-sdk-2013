//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include <stdio.h>
#include <malloc.h>
#include "ObjectList.h"
#include "tier1/strtools.h"

//#include "port.h"
//#include "mem.h"
// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

ObjectList::ObjectList()
{
	head = tail = current = NULL;
	number = 0;
}

ObjectList::~ObjectList()
{
	Clear( false );
}

bool ObjectList::AddHead(void * newObject)
{
	// create new element
	element_t * newElement = (element_t *) calloc(1, sizeof(element_t));

	if (newElement == NULL )
		return false; // out of memory

	// insert element
	newElement->object = newObject;

	if (head)
	{
		newElement->next = head;
		head->prev = newElement;
	};

	head = newElement;
	
	// if list was empty set new tail
	if (tail==NULL) tail = head;	

	number++;

	return true;

}

void * ObjectList::RemoveHead()
{
	void * retObj;
	
	// check head is present
	if (head)
	{
		retObj = head->object;
		element_t * newHead  = head->next;
		if (newHead) newHead->prev = NULL;

		// if only one element is in list also update tail
		// if we remove this prev element
		if (tail==head) tail = NULL;

		free(head);
		head = newHead;

		number--;

	} else
	  retObj = NULL;

	return retObj;
}

bool ObjectList::AddTail(void * newObject)
{
	element_t * newElement = (element_t *) calloc(1, sizeof(element_t));

	if (newElement == NULL) 
		return false; // out of memory;

	newElement->object = newObject;

	if (tail)
	{
		newElement->prev = tail;
		tail->next = newElement;
	}

	tail = newElement;

	// if list was empty set new head
	if (head==NULL) head = tail;

	number++;

	return true;

}

void * ObjectList::RemoveTail()
{
	void * retObj;
	
	// check tail is present
	if (tail)
	{
		retObj = tail->object;
		element_t * newTail  = tail->prev;
		if (newTail) newTail->next = NULL;

		// if only one element is in list also update tail
		// if we remove this prev element
		if (head==tail) head = NULL;

		free(tail);
		tail = newTail;

		number--;

	} else
		retObj = NULL;

	return retObj;
}

bool ObjectList::IsEmpty()
{
	return ( head == NULL );
}

int ObjectList::CountElements()
{
	return number;
}

bool ObjectList::Contains(void * object)
{
	element_t * e = head;

	while(e && e->object!=object) { e = e->next;}

	if ( e )
	{
		current = e;
		return true;
	}
	else
	{
		return false;
	}
}

void ObjectList::Clear( bool freeElementsMemory )
{
	element_t * ne;

	element_t * e = head;
	while(e)
	{
		ne = e->next; 

		if ( freeElementsMemory && e->object )
			free( e->object );

		free(e);
		e = ne; 
	}

	head = tail = current = NULL;
	number = 0;

}

bool ObjectList::Remove( void * object )
{
	element_t * e = head;

	while(e && e->object!=object) { e = e->next;}

	if (e!=NULL)
	{
		if (e->prev) e->prev->next = e->next;
		if (e->next) e->next->prev = e->prev;
		if (head==e) head = e->next;
		if (tail==e) tail = e->prev;
		if (current == e) current= e->next;
		free(e);
		number--;
	}

	return (e!=NULL);
}

void ObjectList::Init()
{
	head = tail = current = NULL;
	number = 0;
}

void * ObjectList::GetFirst()
{
	if (head)
	{
		current = head->next;
		return head->object;
	} 
	else
	{
		current = NULL;
		return NULL;
	};
	
}

void * ObjectList::GetNext()
{
	void * retObj = NULL;
	if (current)
	{	
		retObj = current->object;
		current = current->next;
	}
	return retObj;
}

bool ObjectList::Add(void *newObject)
{
	return AddTail( newObject );
}


//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Intrusive linked list templates, both for singly and doubly linked lists
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//

#ifndef UTILINTRUSIVELIST_H
#define UTILINTRUSIVELIST_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/basetypes.h"
#include "utlmemory.h"
#include "tier0/dbg.h"


// 
// These templates are used for intrusive linked list classes. Intrusive linked list templates
// force the structs and classes contained within them to have their own m_pNext, (optionally),
// m_pPrev, and other fields contained within. All memory management is up to the caller and their
// classes. No data will ever be copied. Nodes can only exist on one list at a time, because of
// only having on m_Next field, and manipulating the list while walking it requires that care on
// the part of the caller. All accessing and searching functions work by passing and returning
// pointers.
//
//
//
// naming and field conventions:
// functions referring to a DList are for doubly linked lists. nodes must have m_pHead and
// m_pPrev pointer fields.
// Functions using Priority require an m_Priority field, which must be comparable.
//
// Some functions are mean for use with lists which maintain both a head and tail pointer
// in order to support fast adding to the end.


/// validates that the doubly linked list has the proper structure, pointer-wise

namespace IntrusiveList
{
#ifdef SUPERSLOW_DEBUG_VERSION                              
	template<class T> inline void ValidateDList(T *head)
	{                                                           
		if (head)
		{
			Assert(head->m_pPrev==0);
		}
		while(head)
		{
			if (head->m_pNext)
			{
				Assert(head->m_pNext->m_pPrev==head);
			}
			if (head->m_pPrev)
			{
				Assert(head->m_pPrev->m_pNext==head);
			}
			head=head->m_pNext;
		}
	}
#else
	template<class T> inline void ValidateDList(T * /*head*/)
	{
	}
#endif

	

// move a node in a doubly linked list backwards one step.
	template <class T> inline void MoveDNodeBackwards( T *which, T * &head)
	{
		if (which->m_pPrev)
		{
			T *p=which->m_pPrev;
			T *pp=p->m_pPrev;
			T *n=which->m_pNext;
			Assert(p->m_pNext == which);
			if (n)
			{
				Assert(n->m_pPrev==which);
				n->m_pPrev=p;
			}
			if (pp)
			{
				Assert(pp->m_pNext==p);
				pp->m_pNext=which;
			}
			else
			{
				head=which;                     // this node is the new root!
			}
			which->m_pNext=p;
			which->m_pPrev=pp;
			p->m_pNext=n;
			p->m_pPrev=which;
		}
		ValidateDList(head);
	}

    

	// removes node 'which' from doubly linked list with 'head'
	template<class T> inline void RemoveFromDList(T * &head, T *which)
	{
		if (which->m_pPrev)
		{
			Assert(which->m_pPrev->m_pNext==which);
			which->m_pPrev->m_pNext=which->m_pNext;
			if (which->m_pNext)
			{
				Assert(which->m_pNext->m_pPrev==which);
				which->m_pNext->m_pPrev=which->m_pPrev;
			}
		}
		else
		{
			if (head==which)
			{
				head=which->m_pNext;
				if (head)
				{
					Assert(head->m_pPrev==which);
					head->m_pPrev=0;
				}
			}
		}
		which->m_pNext=which->m_pPrev=0;
		ValidateDList(head);
  
	}

	//checks to see if node is in doubly linked list
	template<class T> bool OnDList(T const *head, T const *which)
	{
		return (head==which) || (which->m_pNext !=0) || (which->m_pPrev !=0);
	}

	// add a node to the end of a singly linked list
	template<class T> void AddToDTail(T * & head, T * node)
	{
		node->m_pNext=0;
		if (! head)
		{
			head=node;
		}
		else
		{
			T *ptr=head;
			while(ptr->m_pNext)
			{
				ptr=ptr->m_pNext;
			}
			ptr->m_pNext=node;
			node->m_pPrev=ptr;   //
		}
	}

    // add a node to end of doubly linked list.
	template<class T> inline void AddToDHead(T * &head, T *which)
	{
		which->m_pNext=head;
		if (head)
		{
			head->m_pPrev=which;
		}
		which->m_pPrev=0;
		head=which;
		ValidateDList(head);
	}

	// add a node to front of doubly linked list which maintains a tail ptr
	template<class T> inline void AddToDHeadWithTailPtr(T * &head, T *which, T * &tailptr)
	{
		which->m_pNext=head;
		if (head)
		{
			head->m_pPrev=which;
		}
		else
		{
			tailptr=which;
		}
		which->m_pPrev=0;
		head=which;
		ValidateDList(head);
	}

	// add a node to end of doubly linked list which maintains a tail ptr
	template<class T> inline void AddToDTailWithTailPtr(T * &head, T *which, T * & tailptr)
	{
		if (! tailptr)
		{
			Assert(! head);
			which->m_pPrev=which->m_pNext=0;
			tailptr=head=which;
		}
		else
		{
			which->m_pNext=0;
			which->m_pPrev=tailptr;
			tailptr->m_pNext=which;
			tailptr=which;
		}
		ValidateDList( head );
	}
	
	// Remove a node from a dlist , maintaining the tail ptr. node is not 'delete' d
	template<class T> inline void RemoveFromDListWithTailPtr(T * &head, T *which, T * & tailptr)
	{
		if (which==tailptr)
		{
			tailptr=which->m_pPrev;
		}
		if (which->m_pPrev)
		{
			Assert(which->m_pPrev->m_pNext==which);
			which->m_pPrev->m_pNext=which->m_pNext;
			if (which->m_pNext)
			{
				Assert(which->m_pNext->m_pPrev==which);
				which->m_pNext->m_pPrev=which->m_pPrev;
			}
		}
		else
		{
			if (head==which)
			{
				head=which->m_pNext;
				if (head)
				{
					Assert(head->m_pPrev==which);
					head->m_pPrev=0;
				}
			}
		}
		which->m_pNext=which->m_pPrev=0;
		ValidateDList(head);
  
	}

	// this function removes a node, and delete's the node
	template<class T> inline void DeleteFromDListWithTailPtr(T * &head, T *which, T * & tailptr)
	{
		T *tmp=which;
		if (which==tailptr)
		{
			tailptr=which->m_pPrev;
		}
		if (which->m_pPrev)
		{
			Assert(which->m_pPrev->m_pNext==which);
			which->m_pPrev->m_pNext=which->m_pNext;
			if (which->m_pNext)
			{
				Assert(which->m_pNext->m_pPrev==which);
				which->m_pNext->m_pPrev=which->m_pPrev;
			}
		}
		else
		{
			if (head==which)
			{
				head=which->m_pNext;
				if (head)
				{
					Assert(head->m_pPrev==which);
					head->m_pPrev=0;
				}
			}
		}
		which->m_pNext=which->m_pPrev=0;
		delete tmp;
		ValidateDList(head);
	}

	// Add a node to a d-list, keeping the highest priority nodes first. This is a simple
	// linear search to insert, NOT a O(logn) heap.
	template<class T> inline void AddToDPriority(T * &head, T *which)
	{
		T* prevnode=0;
		for(T *curnode=head;curnode;curnode=curnode->m_pNext)
		{
			if (which->m_Priority>=curnode->m_Priority)
				break;
			prevnode=curnode;
		}
		// now, we have either run out of list, or we have found an
		// element to add this one before
		if (! prevnode)
		{
			AddToDHead(head,which);
		}
		else
		{
			which->m_pNext=prevnode->m_pNext;
			prevnode->m_pNext=which;
			which->m_pPrev=prevnode;
			if (which->m_pNext)
				which->m_pNext->m_pPrev=which;
		}
	}

	// same as AddToDPriority, except with reverse order
	template<class T> inline void AddToDPriorityLowestFirst(T * &head, T *which)
	{
		T* prevnode=0;
		for(T *curnode=head;curnode;curnode=curnode->m_pNext)
		{
			if (which->m_Priority<=curnode->m_Priority)
				break;
			prevnode=curnode;
		}
		// now, we have either run out of list, or we have found an
		// element to add this one before
		if (! prevnode)
		{
			AddToDHead(head,which);
		}
		else
		{
			which->m_pNext=prevnode->m_pNext;
			prevnode->m_pNext=which;
			which->m_pPrev=prevnode;
			if (which->m_pNext)
				which->m_pNext->m_pPrev=which;
		}
	}


	// return a pointer to the last node in a singly-linked (or doubly) list
	template<class T> T * LastNode(T * head)
	{
		if (head)
		{
			while(head->m_pNext)
			{
				head=head->m_pNext;
			}
		}
		return head;
	}


	// Remove from a singly linked list. no delete called.
	template<class T,class V> void RemoveFromList(T * & head, V *which)
	{
		if (head==which)
		{
			head=which->m_pNext;
		}
		else
		{
			for(T * i=head; i; i=i->m_pNext)
			{
				if (i->m_pNext==which)
				{
					i->m_pNext=which->m_pNext;
					return;
				}
			}
		}
	}

	// same as RemoveFromList, but 'delete' is called.
	template<class T,class V> void DeleteFromList(T * & head, V *which)
	{
		T *tmp;
		if (head==which)
		{
			tmp=which->m_pNext;
			delete(head);
			head=tmp;
		}
		else
		{
			for(T * i=head; i; i=i->m_pNext)
			{
				if (i->m_pNext==which)
				{
					tmp=which->m_pNext;
					delete(which);
					i->m_pNext=tmp;
					return;
				}
			}
		}
	}

	// find the position in a list of a node. -1 if not found. Linear search.
	// nodes must have comparison functions
	template<class T,class V> int PositionInList(T *head, V *node)
	{
		int pos=0;
		while(head)
		{
			if (head==node) return pos;
			head=head->m_pNext;
			pos++;
		}
		return -1;
	}

	// find the Nth node in a list. null if index too high.
	template<class T> T *NthNode(T * head, int idx)
	{
		while(idx && head)
		{
			idx--;
			head=head->m_pNext;
		}
		return head;
	}

	//Add a node to the head of a singly-linked
	// Note that the head var passed to this will be modified.
	template<class T,class V> static inline void AddToHead(T * & head, V * node)
	{
		node->m_pNext=head;
		head=node;
	}  

	//Add a node to the tail of a singly-linked. Not fast
	// Note that the head var passed to this will be modified.
	template<class T,class V> static inline void AddToTail(T * & head, V * node)
	{
		node->m_pNext = NULL;
		if ( ! head )
			head = node;
		else
		{
			T *pLastNode = head;
			while( pLastNode->m_pNext )
				pLastNode = pLastNode->m_pNext;
			pLastNode->m_pNext = node;
		}
	}  

	//Add a node to the head of a singly-linked list, maintaining a tail pointer
	template<class T,class V> static inline void AddToHead(T * & head, T * &tail,V * node)
	{
		if (! head)
		{
			tail=node;
		}
		node->m_pNext=head;
		head=node;
	}  



	// return the node in head before in a singly linked list. returns null if head is empty, n is
	// null, or if n is the first node. not fast.
	template<class T> static inline T * PrevNode(T *head, T *node)
	{
		T *i;
		for(i=head;i;i=i->m_pNext)
		{
			if (i->m_pNext == node)
				break;
		}
		return i;
	}

  
	// add a node to the end of a singly linked list. Not fast.
	template<class T,class V> void AddToEnd(T * & head, V * node)
	{
		node->m_pNext=0;
		if (! head) 
		{
			head=node;
		}
		else
		{
			T *ptr=head;
			while(ptr->m_pNext)
			{
				ptr=ptr->m_pNext;
			}
			ptr->m_pNext=node;
		}
	}

	// add a node to the end of a singly linked list, maintaining a tail pointer.
	// the head and tail pointer can be modified by this routine.
	template<class T,class V> void AddToEndWithTail(T * & head, T * & tail,V * node)
	{
		Assert((head && tail) || ((!head) && (!tail)));
		node->m_pNext=0;
		if (! head)
		{
			head=tail=node;
		}
		else
		{
			tail->m_pNext=node;
			tail=node;
		}
	}

	// Add a node to a singly linked list, sorting by the m_Name field
	template<class T> void AddSortedByName(T * & head, T * node)
	{
		if ( (! head) ||                                          // empty list?
			 (stricmp(node->m_Name,head->m_Name)==-1))                // or we should be first?
		{
			node->m_pNext=head;                                        // make us head
			head=node;
		}
		else
		{
			T *t;
			for(t=head;t->m_pNext;t=t->m_pNext)                        // find the node we should be before
				if (stricmp(t->m_pNext->m_Name,node->m_Name)>=0)
					break;
			node->m_pNext=t->m_pNext;
			t->m_pNext=node;
		}
	}

	// count # of elements in list
	template<class T> int ListLength(T *head)
	{
		int len=0;
		while(head)
		{
			len++;
			head=head->m_pNext;
		}
		return len;
	}

	// this will kill a list if the list is of objects which automatically
	// remove themselves from the list when delete is called
	template<class T> void KillList(T * & head)
	{
		while(head)
		{
			delete head;
		}
	}


	// this will kill all elements in a list if
	// the elements are of a type which does NOT remove itself from
	// the list when the destructor is called.
	template<class T> void DeleteList(T * & head)
	{
		while (head)
		{
			T* tmp=head->m_pNext;
			delete head;
			head=tmp;
		}
	}

	// find a named node in any list which has both a Next field and a Name field.
	template <class T> static inline T * FindNamedNode(T * head, char const *name)
	{
		for(;head && stricmp(head->m_Name,name); head=head->m_pNext)
		{
		}
		return head;
	}

	template <class T> static inline T * FindNamedNodeCaseSensitive(T * head, char const *name)
	{
		for(;head && strcmp(head->m_Name,name); head=head->m_pNext)
		{
		}
		return head;
	}

	// find data in a singly linked list, using equality match on any field
	// usage: FindNodeByField(listptr,data,&list::fieldname)
	template <class T, class U, class V> static inline T * FindNodeByField(T * head,  U data, U V::*field)
	{
		while(head)
		{
			if (data==(*head).*field)
				return head;
			head=head->m_pNext;
		}
		return 0;
	}

	// find a node and its predecessor, matching on equality of a given field.
	// usage: FindNodeByFieldWithPrev(listptr,data,&list::fieldname, prevptr)
	template <class T, class U, class V> static inline T * FindNodeByFieldWithPrev(T * head,  U data, U V::*field, T * & prev)
	{
		prev=0;
		for(T *i=head; i; i=i->m_pNext)
		{
			if(data==(*i).*field)
				return i;
			prev=i;
		}
		prev=0;
		return 0;
	}


	/// sort a list. comparefn should return 0 if the items are equal, 1 if A goes first, and -1 if A goes last.
	// NOT fast.
	template<class T> void SortList(T * &head, int (*comparefn)(T * a, T * b))
	{
		int didswap=1;
		while(didswap)
		{
			didswap=0;
			T *prev=0;
			for(T *i=head;i && i->m_pNext; i=i->m_pNext)
			{
				/// compare i and i+1
				int rslt=(*comparefn)(i,i->m_pNext);
				if (rslt==-1)
				{
					/// need to swap
					didswap=1;
					T *newfirst=i->m_pNext;
					if (prev)
					{
						prev->m_pNext=newfirst;
						i->m_pNext=newfirst->m_pNext;
						newfirst->m_pNext=i;
					}
					else
					{
						head=i->m_pNext;
						i->m_pNext=newfirst->m_pNext;
						newfirst->m_pNext=i;
					}
					i=newfirst;
				}
				prev=i;
			}
		}    
	}

	// sort a doubly linked list. NOt fast.
	template <class T> void SortDList(T * & head, int (*comparefn)(T * a, T * b))
	{
		SortList(head,comparefn);
		/// now, regen prev ptrs
		T *prev=0;
		for(T *i=head;i;i=i->m_pNext)
		{
			i->m_pPrev=prev;
			prev=i;
		}
	}

	// reverse a singly linked list. not recommended for anything other than valve programming
	// interview :-)
	template <class T> T *ReversedList( T * head )
	{
		T * pNewHead=NULL;
		while( head )
		{
			T *pNext=head->m_pNext;
#ifdef INTERVIEW_QUESTION
			head->m_pNext=pNewHead;
			pNewHead = head;
#else
			AddToHead( pNewHead, head );
#endif
			head = pNext;
		}
		return pNewHead;
	}
};

// singly linked list
template<class T> class CUtlIntrusiveList
{
public:
	T *m_pHead;

	FORCEINLINE T *Head( void ) const
	{
		return m_pHead;
	}
	
	FORCEINLINE CUtlIntrusiveList(void)
	{
		m_pHead = NULL;
	}


	FORCEINLINE void RemoveAll( void )
	{
		// empty list. doesn't touch nodes at all
		m_pHead = NULL;
	}
	FORCEINLINE void AddToHead( T * node )
	{
		IntrusiveList::AddToHead( m_pHead, node );
	}

	FORCEINLINE void AddToTail( T * node )
	{
		IntrusiveList::AddToTail( m_pHead, node );
	}

	void RemoveNode(T *which)
	{
		IntrusiveList::RemoveFromList( m_pHead, which );
	}

	// this will kill a list if the list is of objects which automatically
	// remove themselves from the list when delete is called
	void KillList( void )
	{
		while(m_pHead)
		{
			delete m_pHead;
		}
	}


	// return the node in head before in a singly linked list. returns null if head is empty, n is
	// null, or if n is the first node. not fast. Fast for dlists
	T * PrevNode(T *node)
	{
		return IntrusiveList::PrevNode( m_pHead, node );
	}

	int NthNode( int n )
	{
		return NthNode( m_pHead, n );
	}

	// this will kill all elements in a list if
	// the elements are of a type which does NOT remove itself from
	// the list when the destructor is called.
	void Purge( void )
	{
		while (m_pHead)
		{
			T* tmp=m_pHead->m_pNext;
			delete m_pHead;
			m_pHead=tmp;
		}
	}

	int Count( void ) const
	{
		return IntrusiveList::ListLength( m_pHead );
	}

	FORCEINLINE T * FindNamedNodeCaseSensitive( char const *pName ) const
	{
		return IntrusiveList::FindNamedNodeCaseSensitive( m_pHead, pName );

	}

	T *RemoveHead( void )
	{
		if ( m_pHead )
		{
			T *pRet = m_pHead;
			m_pHead = pRet->m_pNext;
			return pRet;
		}
		else
			return NULL;
	}
};

// doubly linked list
template<class T> class CUtlIntrusiveDList : public CUtlIntrusiveList<T>
{
public:

	FORCEINLINE void AddToHead( T * node )
	{
		IntrusiveList::AddToDHead( CUtlIntrusiveList<T>::m_pHead, node );
	}
	FORCEINLINE void AddToTail( T * node )
	{
		IntrusiveList::AddToDTail( CUtlIntrusiveList<T>::m_pHead, node );
	}

	void RemoveNode(T *which)
	{
		IntrusiveList::RemoveFromDList( CUtlIntrusiveList<T>::m_pHead, which );
	}

	T *RemoveHead( void )
	{
		if ( CUtlIntrusiveList<T>::m_pHead )
		{
			T *pRet = CUtlIntrusiveList<T>::m_pHead;
			CUtlIntrusiveList<T>::m_pHead = CUtlIntrusiveList<T>::m_pHead->m_pNext;
			if ( CUtlIntrusiveList<T>::m_pHead )
				CUtlIntrusiveList<T>::m_pHead->m_pPrev = NULL;
			return pRet;
		}
		else
			return NULL;
	}

	T * PrevNode(T *node)
	{
		return ( node )?node->m_Prev:NULL;
	}

};

template<class T> class CUtlIntrusiveDListWithTailPtr : public CUtlIntrusiveDList<T>
{
public:

	T *m_pTailPtr;

	FORCEINLINE CUtlIntrusiveDListWithTailPtr( void ) : CUtlIntrusiveDList<T>()
	{
		m_pTailPtr = NULL;
	}

	FORCEINLINE void AddToHead( T * node )
	{
		IntrusiveList::AddToDHeadWithTailPtr( CUtlIntrusiveList<T>::m_pHead, node, m_pTailPtr );
	}
	FORCEINLINE void AddToTail( T * node )
	{
		IntrusiveList::AddToDTailWithTailPtr( CUtlIntrusiveList<T>::m_pHead, node, m_pTailPtr );
	}

	void RemoveNode( T *pWhich )
	{
		IntrusiveList::RemoveFromDListWithTailPtr( CUtlIntrusiveList<T>::m_pHead, pWhich, m_pTailPtr );
	}

	void Purge( void )
	{
		CUtlIntrusiveList<T>::Purge();
		m_pTailPtr = NULL;
	}

	void Kill( void )
	{
		CUtlIntrusiveList<T>::Purge();
		m_pTailPtr = NULL;
	}

	T *RemoveHead( void )
	{
		if ( CUtlIntrusiveDList<T>::m_pHead )
		{
			T *pRet = CUtlIntrusiveDList<T>::m_pHead;
			CUtlIntrusiveDList<T>::m_pHead = CUtlIntrusiveDList<T>::m_pHead->m_pNext;
			if ( CUtlIntrusiveDList<T>::m_pHead )
				CUtlIntrusiveDList<T>::m_pHead->m_pPrev = NULL;
			if (! CUtlIntrusiveDList<T>::m_pHead )
				m_pTailPtr = NULL;
			ValidateDList( CUtlIntrusiveDList<T>::m_pHead );
			return pRet;
		}
		else
			return NULL;
	}

	T * PrevNode(T *node)
	{
		return ( node )?node->m_Prev:NULL;
	}

};

template<class T> void PrependDListWithTailToDList( CUtlIntrusiveDListWithTailPtr<T> &src, 
												   CUtlIntrusiveDList<T> &dest )
{
	if ( src.m_pHead )
	{
		src.m_pTailPtr->m_pNext = dest.m_pHead;
		if ( dest.m_pHead )
			dest.m_pHead->m_pPrev = src.m_pTailPtr;
		dest.m_pHead = src.m_pHead;
		IntrusiveList::ValidateDList( dest.m_pHead );
	}
}

#endif

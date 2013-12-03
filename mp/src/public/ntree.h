//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef __TREE_H__
#define __TREE_H__

#include "List.h"
#include "ArrayStack.h"

// NTreeNode: Class decleration and definition
template <class T> class NTreeNode
{
public:
	// constructor
	NTreeNode<T>( T data );
	
	NTreeNode<T> *PrependChild( NTreeNode<T> *node );
	NTreeNode<T> *AppendChild( NTreeNode<T> *node );
	NTreeNode<T> *InsertChildAfterIndex( NTreeNode<T> *node, int index );
	NTreeNode<T> *InsertChildBeforeIndex( NTreeNode<T> *node, int index );
	NTreeNode<T> *RemoveChild( Position position );
	NTreeNode<T> *RemoveChild( int index );
	Position InsertAfter( NTreeNode<T> *node, Position position );
	Position InsertBefore( NTreeNode<T> *node, Position position );
	int GetNumChildren();
	Position GetChildPosition( int childNum );
	NTreeNode<T> *GetChild( int childNum );
	NTreeNode<T> *GetChild( Position position );
	int GetIndexRelativeToParent();
	T GetItem();
	NTreeNode<T> *GetParent();
	NTreeNode<T> *GetRoot();
	NTreeNode<T> *GetNextSibling();
	void Traverse( void (*VisitFunc)( T, int depth ), int maxTreeDepth );
	NTreeNode<T> *ReentrantTraversalGetFirst( int maxTreeDepth );
	NTreeNode<T> *ReentrantTraversalGetNext( void );
	
protected:
	GList<NTreeNode<T> * > *list;
	T data;
	NTreeNode<T> *parent;
	ArrayStack<NTreeNode<T> *> *reentrantStack;
};

template <class T>
NTreeNode<T>::NTreeNode( T data )
{
	list = new GList<NTreeNode<T> * >;
	this->data = data;
	this->parent = NULL;
	this->reentrantStack = NULL;
}

template <class T>
NTreeNode<T> *NTreeNode<T>::PrependChild( NTreeNode<T> *node )
{
	node->parent = this;
	return list->GetItemAtPosition( list->InsertAtHead( node ) );
}

template <class T>
NTreeNode<T> *NTreeNode<T>::AppendChild( NTreeNode<T> *node )
{
	node->parent = this;
	return list->GetItemAtPosition( list->InsertAtTail( node ) );
}

template <class T>
NTreeNode<T> *NTreeNode<T>::InsertChildAfterIndex( NTreeNode<T> *node, int index )
{
	node->parent = this;
	if( index < 0 )
	{
		// if out of range in the negative direction, prepend
		this->PrependChild( node );
	}
	else if( index > list->GetNumItems() - 1 )
	{
		// if out of range, just append.
		this->AppendChild( node );
	}
	else
	{
		Position pos;
		pos = list->GetPositionAtIndex( index );
		list->InsertAfter( node, pos );
	}
	return node;
}

template <class T>
NTreeNode<T> *NTreeNode<T>::InsertChildBeforeIndex( NTreeNode<T> *node, int index )
{
	node->parent = this;
	if( index < 0 )
	{
		// if out of range in the negative direction, prepend
		this->PrependChild( node );
	}
	else if( index > list->GetNumItems() - 1 )
	{
		// if out of range, just append.
		this->AppendChild( node );
	}
	else
	{
		Position pos;
		pos = list->GetPositionAtIndex( index );
		list->InsertBefore( node, pos );
	}
	return node;
}

template <class T>
NTreeNode<T> *NTreeNode<T>::RemoveChild( Position position )
{
	NTreeNode<T> **node = ( NTreeNode<T> ** )( void * )position;
	( *node )->parent = NULL;
	return list->Remove( position );
}

template <class T>
NTreeNode<T> *NTreeNode<T>::RemoveChild( int index )
{
	Position position = list->GetPositionAtIndex( index );
	NTreeNode<T> **node = ( NTreeNode<T> ** )( void * )position;
	( *node )->parent = NULL;
	return list->Remove( position );
}

template <class T>
Position NTreeNode<T>::InsertAfter( NTreeNode<T> *node, Position position )
{
	node->parent = this;
	return list->InsertAfter( node, position );
}

template <class T>
Position NTreeNode<T>::InsertBefore( NTreeNode<T> *node, Position position )
{
	node->parent = this;
	return list->InsertBefore( node, position );
}

template <class T>
int NTreeNode<T>::GetNumChildren()
{
	return list->GetNumItems();
}

template <class T>
Position NTreeNode<T>::GetChildPosition( int childNum )
{
	return list->GetPositionAtIndex( childNum );
}

template <class T>
NTreeNode<T> *NTreeNode<T>::GetChild( int childNum )
{
	return list->GetItemAtIndex( childNum );
}

template <class T>
NTreeNode<T> *NTreeNode<T>::GetChild( Position position )
{
	return list->GetItemAtIndex( position );
}

template <class T>	
int NTreeNode<T>::GetIndexRelativeToParent()
{
	if( !parent )
	{
		assert( 0 ); // hack
		return -1;
	}
	GListIterator<NTreeNode<T> *> iterator( parent->list );
	int i;
	for( i = 0, iterator.GotoHead(); !iterator.AtEnd(); iterator++, i++ )
	{
		if( iterator.GetCurrent() == this )
		{
			return i;
		}
	}
	assert( 0 ); // hack
	return -1;
}

template <class T>
T NTreeNode<T>::GetItem()
{
	return data;
}

template <class T>
NTreeNode<T> *NTreeNode<T>::GetParent()
{
	return parent;
}

template <class T>
NTreeNode<T> *NTreeNode<T>::GetRoot()
{
	NTreeNode<T> *node;
	node = this;
	while( node->GetParent() )
	{
		node = node->GetParent();
	}
	return node;
}

template <class T>
NTreeNode<T> *NTreeNode<T>::GetNextSibling()
{
	int currentID, siblingID;
	NTreeNode<T> *parent;
	parent = this->GetParent();
	if( !parent )
	{
		return NULL;
	}
	currentID = this->GetIndexRelativeToParent();
	siblingID = currentID + 1;
	if( siblingID < parent->GetNumChildren() )
	{
		return parent->GetChild( siblingID );
	}
	else
	{
		return NULL;
	}
}

template <class T>
void NTreeNode<T>::Traverse( void (*VisitFunc)( T, int depth ), int maxTreeDepth )
{
	ArrayStack<NTreeNode<T> *> stack( maxTreeDepth );
	NTreeNode<T> *current, *nextSibling;

	stack.Push( this );	
	Visit( this->GetItem(), 0 );
	while( !stack.IsEmpty() )
	{
		current = stack.Pop();
		if( current->GetNumChildren() > 0 )
		{
			stack.Push( current );
			stack.Push( current->GetChild( 0 ) );
			Visit( current->GetChild( 0 )->GetItem(), stack.GetDepth() - 1 );
		}
		else
		{
			while( !stack.IsEmpty() && !( nextSibling = current->GetNextSibling() ) )
			{
				current = stack.Pop();
			}
			if( !stack.IsEmpty() )
			{
				stack.Push( nextSibling );
				Visit( nextSibling->GetItem(), stack.GetDepth() - 1 );
			}	
		}
	}
}

template <class T>
NTreeNode<T> *NTreeNode<T>::ReentrantTraversalGetFirst( int maxTreeDepth )
{
	if( reentrantStack )
	{
		delete reentrantStack;
	}
	reentrantStack = new ArrayStack<NTreeNode<T> *>( maxTreeDepth );
	reentrantStack->Push( this );
	return this;
}

template <class T>
NTreeNode<T> *NTreeNode<T>::ReentrantTraversalGetNext( void )
{
	NTreeNode<T> *current, *nextSibling;

	while( !reentrantStack->IsEmpty() )
	{
		current = reentrantStack->Pop();
		if( current->GetNumChildren() > 0 )
		{
			reentrantStack->Push( current );
			reentrantStack->Push( current->GetChild( 0 ) );
			return current->GetChild( 0 );
		}
		else
		{
			while( !reentrantStack->IsEmpty() && !( nextSibling = current->GetNextSibling() ) )
			{
				current = reentrantStack->Pop();
			}
			if( !reentrantStack->IsEmpty() )
			{
				reentrantStack->Push( nextSibling );
				return nextSibling;
			}	
		}
	}
	delete reentrantStack;
	reentrantStack = NULL;
	return NULL;
}

#endif /* __TREE_H__ */

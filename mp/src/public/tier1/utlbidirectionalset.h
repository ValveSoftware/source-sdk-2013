//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Bi-directional set. A Bucket knows about the elements that lie
// in it, and the elements know about the buckets they lie in.
//
// $Revision: $
// $NoKeywords: $
//=============================================================================//

#ifndef UTLBIDIRECTIONALSET_H
#define UTLBIDIRECTIONALSET_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/dbg.h"
#include "utllinkedlist.h"

//-----------------------------------------------------------------------------
// Templatized helper class to deal with the kinds of things that spatial
// partition code always seems to have; buckets with lists of lots of elements
// and elements that live in lots of buckets. This makes it really quick to
// add and remove elements, and to iterate over all elements in a bucket.
//
// For this to work, you must initialize the set with two functions one that
// maps from bucket to the index of the first element in that bucket, and one 
// that maps from element to the index of the first bucket that element lies in.
// The set will completely manage the index, it's just expected that those
// indices will be stored outside the set.
//
// S is the storage type of the index; it is the type that you may use to 
// save indices into memory. I is the local iterator type, which you should
// use in any local scope (eg, inside a for() loop.) The reason for this is 
// that you may wish to use unsigned shorts inside the structs you are
// saving with a CBidirectionalSet; but 16-bit arithmetic is catastrophically
// slow on a PowerPC -- during testing we saw CBidirectionalSet:: operations
// consume as much as 8% of the frame.
// 
// For this reason, on the 360, the handles have been typedef'd to native
// register types (U32) which are accepted as parameters by the functions. 
// The implicit assumption is that CBucketHandle and CElementHandle can
// be safely cast to ints! You can increase to U64 without performance 
// penalty if necessary; the PowerPC is a 64-bit processor.
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I = S >
class CBidirectionalSet
{
public:
	// Install methods to get at the first bucket given a element
	// and vice versa...
	typedef S& (*FirstElementFunc_t)(CBucketHandle);
	typedef S& (*FirstBucketFunc_t)(CElementHandle);

#ifdef _X360
	typedef uint32 CBucketHandlePram;
	typedef uint32 CElementHandlePram;
#else
	typedef CBucketHandle  CBucketHandlePram;
	typedef CElementHandle CElementHandlePram;
#endif

	// Constructor
	CBidirectionalSet();

	// Call this before using the set
	void Init( FirstElementFunc_t elemFunc, FirstBucketFunc_t bucketFunc );

	// Add an element to a particular bucket
	void AddElementToBucket( CBucketHandlePram bucket, CElementHandlePram element );

	// Prevalidate an add to a particular bucket
	// NOTE: EXPENSIVE!!!
	void ValidateAddElementToBucket( CBucketHandlePram bucket, CElementHandlePram element );

	// Test if an element is in a particular bucket.
	// NOTE: EXPENSIVE!!!
	bool IsElementInBucket( CBucketHandlePram bucket, CElementHandlePram element );
	
	// Remove an element from a particular bucket
	void RemoveElementFromBucket( CBucketHandlePram bucket, CElementHandlePram element );

	// Remove an element from all buckets
	void RemoveElement( CElementHandlePram element );
	void RemoveBucket( CBucketHandlePram element );

	// Used to iterate elements in a bucket; I is the iterator
	I FirstElement( CBucketHandlePram bucket ) const;
	I NextElement( I idx ) const;
	CElementHandle Element( I idx ) const;

	// Used to iterate buckets associated with an element; I is the iterator
	I FirstBucket( CElementHandlePram bucket ) const;
	I NextBucket( I idx ) const;
	CBucketHandle Bucket( I idx ) const;

	static S InvalidIndex();

	// Ensure capacity
	void	EnsureCapacity( int count );

	// Deallocate....
	void	Purge();

	int		NumAllocated( void ) const;

private:
	struct BucketListInfo_t
	{
		CElementHandle	m_Element;
		S				m_BucketListIndex;	// what's the m_BucketsUsedByElement index of the entry?
	};

	struct ElementListInfo_t
	{
		CBucketHandle	m_Bucket;
		S				m_ElementListIndex;	// what's the m_ElementsInBucket index of the entry?
	};

	// Maintains a list of all elements in a particular bucket 
	CUtlLinkedList< BucketListInfo_t, S, true, I >	m_ElementsInBucket;

	// Maintains a list of all buckets a particular element lives in
	CUtlLinkedList< ElementListInfo_t, S, true, I >	m_BucketsUsedByElement;

	FirstBucketFunc_t	m_FirstBucket;
	FirstElementFunc_t	m_FirstElement;
};


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::CBidirectionalSet( )
{
	m_FirstBucket = NULL;
	m_FirstElement = NULL;
}


//-----------------------------------------------------------------------------
// Call this before using the set
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::Init( FirstElementFunc_t elemFunc, FirstBucketFunc_t bucketFunc )
{
	m_FirstBucket = bucketFunc;
	m_FirstElement = elemFunc;
}


//-----------------------------------------------------------------------------
// Adds an element to the bucket
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::ValidateAddElementToBucket( CBucketHandlePram bucket, CElementHandlePram element )
{
#ifdef _DEBUG
	// Make sure that this element doesn't already exist in the list of elements in the bucket
	I elementInBucket = m_FirstElement( bucket );
	while( elementInBucket != m_ElementsInBucket.InvalidIndex() )
	{
		// If you hit an Assert here, fix the calling code.  It's too expensive to ensure
		// that each item only shows up once here.  Hopefully you can do something better
		// outside of here.
		Assert( m_ElementsInBucket[elementInBucket].m_Element != element );
		elementInBucket = m_ElementsInBucket.Next( elementInBucket );
	}
	// Make sure that this bucket doesn't already exist in the element's list of buckets.
	I bucketInElement = m_FirstBucket( element );
	while( bucketInElement != m_BucketsUsedByElement.InvalidIndex() )
	{
		// If you hit an Assert here, fix the calling code.  It's too expensive to ensure
		// that each item only shows up once here.  Hopefully you can do something better
		// outside of here.
		Assert( m_BucketsUsedByElement[bucketInElement].m_Bucket != bucket );
		bucketInElement = m_BucketsUsedByElement.Next( bucketInElement );
	}
#endif
}

	
//-----------------------------------------------------------------------------
// Adds an element to the bucket
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::AddElementToBucket( CBucketHandlePram bucket, CElementHandlePram element )
{
	Assert( m_FirstBucket && m_FirstElement );

	// Allocate new element + bucket entries
	I idx = m_ElementsInBucket.Alloc(true);
	I list = m_BucketsUsedByElement.Alloc( true );

	// Store off the element data
	m_ElementsInBucket[idx].m_Element = element;
	m_ElementsInBucket[idx].m_BucketListIndex = list;

	// Here's the bucket data
	m_BucketsUsedByElement[list].m_Bucket = bucket;
	m_BucketsUsedByElement[list].m_ElementListIndex = idx;

	// Insert the element into the list of elements in the bucket
	S& firstElementInBucket = m_FirstElement( bucket );
	if ( firstElementInBucket != m_ElementsInBucket.InvalidIndex() )
		m_ElementsInBucket.LinkBefore( firstElementInBucket, idx );
	firstElementInBucket = idx;

	// Insert the bucket into the element's list of buckets
	S& firstBucketInElement = m_FirstBucket( element );
	if ( firstBucketInElement != m_BucketsUsedByElement.InvalidIndex() )
		m_BucketsUsedByElement.LinkBefore( firstBucketInElement, list );
	firstBucketInElement = list;
}

//-----------------------------------------------------------------------------
// Test if an element is in a particular bucket.
// NOTE: EXPENSIVE!!!
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
bool CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::IsElementInBucket( CBucketHandlePram bucket, CElementHandlePram element )
{
	// Search through all elements in this bucket to see if element is in there.
	I elementInBucket = m_FirstElement( bucket );
	while( elementInBucket != m_ElementsInBucket.InvalidIndex() )
	{
		if( m_ElementsInBucket[elementInBucket].m_Element == element )
		{
			return true;
		}
		elementInBucket = m_ElementsInBucket.Next( elementInBucket );
	}
	return false;
}


//-----------------------------------------------------------------------------
// Remove an element from a particular bucket
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::RemoveElementFromBucket( CBucketHandlePram bucket, CElementHandlePram element )
{
	// FIXME: Implement me!
	Assert(0);
}


//-----------------------------------------------------------------------------
// Removes an element from all buckets
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::RemoveElement( CElementHandlePram element )
{
	Assert( m_FirstBucket && m_FirstElement );

	// Iterate over the list of all buckets the element is in
	I i = m_FirstBucket( element );
	while (i != m_BucketsUsedByElement.InvalidIndex())
	{
		CBucketHandlePram bucket = m_BucketsUsedByElement[i].m_Bucket;
		I elementListIndex = m_BucketsUsedByElement[i].m_ElementListIndex; 

		// Unhook the element from the bucket's list of elements
		if (elementListIndex == m_FirstElement(bucket))
			m_FirstElement(bucket) = m_ElementsInBucket.Next(elementListIndex);
		m_ElementsInBucket.Free(elementListIndex);

		I prevNode = i;
		i = m_BucketsUsedByElement.Next(i);
		m_BucketsUsedByElement.Free(prevNode);
	}

	// Mark the list as empty
	m_FirstBucket( element ) = m_BucketsUsedByElement.InvalidIndex();
}

//-----------------------------------------------------------------------------
// Removes a bucket from all elements
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::RemoveBucket( CBucketHandlePram bucket )
{
	// Iterate over the list of all elements in the bucket
	I i = m_FirstElement( bucket );
	while (i != m_ElementsInBucket.InvalidIndex())
	{
		CElementHandlePram element = m_ElementsInBucket[i].m_Element;
		I bucketListIndex = m_ElementsInBucket[i].m_BucketListIndex; 

		// Unhook the bucket from the element's list of buckets
		if (bucketListIndex == m_FirstBucket(element))
			m_FirstBucket(element) = m_BucketsUsedByElement.Next(bucketListIndex);
		m_BucketsUsedByElement.Free(bucketListIndex);

		// Remove the list element
		I prevNode = i;
		i = m_ElementsInBucket.Next(i);
		m_ElementsInBucket.Free(prevNode);
	}

	// Mark the bucket list as empty
	m_FirstElement( bucket ) = m_ElementsInBucket.InvalidIndex();
}


//-----------------------------------------------------------------------------
// Ensure capacity
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::EnsureCapacity( int count )
{
	m_ElementsInBucket.EnsureCapacity( count );
	m_BucketsUsedByElement.EnsureCapacity( count );
}


//-----------------------------------------------------------------------------
// Deallocate....
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
void CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::Purge()
{
	m_ElementsInBucket.Purge( );
	m_BucketsUsedByElement.Purge( );
}


//-----------------------------------------------------------------------------
// Number of elements allocated in each linked list (should be the same)
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
int CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::NumAllocated( void ) const
{
	Assert( m_ElementsInBucket.NumAllocated() == m_BucketsUsedByElement.NumAllocated() );
	return m_ElementsInBucket.NumAllocated();
}


//-----------------------------------------------------------------------------
// Invalid index for iteration..
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
inline S CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::InvalidIndex()
{
	return CUtlLinkedList< CElementHandle, I >::InvalidIndex();
}


//-----------------------------------------------------------------------------
// Used to iterate elements in a bucket; I is the iterator
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::FirstElement( CBucketHandlePram bucket ) const
{
	Assert( m_FirstElement );
	return m_FirstElement(bucket);
}

template< class CBucketHandle, class CElementHandle, class S, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::NextElement( I idx ) const
{
	return m_ElementsInBucket.Next(idx);
}

template< class CBucketHandle, class CElementHandle, class S, class I >
inline CElementHandle CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::Element( I idx ) const
{
	return m_ElementsInBucket[idx].m_Element;
}

//-----------------------------------------------------------------------------
// Used to iterate buckets an element lies in; I is the iterator
//-----------------------------------------------------------------------------
template< class CBucketHandle, class CElementHandle, class S, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::FirstBucket( CElementHandlePram element ) const
{
	Assert( m_FirstBucket );
	return m_FirstBucket(element);
}

template< class CBucketHandle, class CElementHandle, class S, class I >
inline I CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::NextBucket( I idx ) const
{
	return m_BucketsUsedByElement.Next(idx);
}

template< class CBucketHandle, class CElementHandle, class S, class I >
inline CBucketHandle CBidirectionalSet<CBucketHandle,CElementHandle,S,I>::Bucket( I idx ) const
{
	return m_BucketsUsedByElement[idx].m_Bucket;
}
   
#endif // UTLBIDIRECTIONALSET_H

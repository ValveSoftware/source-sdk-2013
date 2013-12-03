//========= Copyright Valve Corporation, All rights reserved. ============//
//
// $Revision: $
// $NoKeywords: $
//===========================================================================//

#ifndef UTLOBJECTREFERENCE_H
#define UTLOBJECTREFERENCE_H

#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlintrusivelist.h"
#include "mathlib/mathlib.h"


// Purpose: class for keeping track of all the references that exist to an object.  When the object
// being referenced is freed, all of the references pointing at it will become null.
//
// To Use:
//   Add a DECLARE_REFERENCED_CLASS to the class that you want to use CutlReferences with.
//   Replace pointers to that class with CUtlReferences.
//   Check these references for null in appropriate places.
//
//  NOTE : You can still happily use pointers instead of references where you want to - these
//  pointers will not magically become null like references would, but if you know no one is going
//  to delete the underlying object during a partcular section of code, it doesn't
//  matter. Basically, CUtlReferences don't rely on every use of an object using one.




template<class T> class CUtlReference
{
public:
	FORCEINLINE CUtlReference(void)
	{
		m_pNext = m_pPrev = NULL;
		m_pObject = NULL;
	}
  
	FORCEINLINE CUtlReference(T *pObj)
	{
		m_pNext = m_pPrev = NULL;
		AddRef( pObj );
	}

	FORCEINLINE ~CUtlReference(void)
	{
		KillRef();
	}
  
	FORCEINLINE void Set(T *pObj)
	{
		if ( m_pObject != pObj )
		{
			KillRef();
			AddRef( pObj );
		}
	}
  
	FORCEINLINE T * operator()(void) const
	{
		return m_pObject;
	}

	FORCEINLINE operator T*()
	{
		return m_pObject;
	}

	FORCEINLINE operator const T*() const
	{
		return m_pObject;
	}

	FORCEINLINE T* operator->()
	{ 
		return m_pObject; 
	}

	FORCEINLINE const T* operator->() const
	{ 
		return m_pObject; 
	}

	FORCEINLINE CUtlReference &operator=( const CUtlReference& otherRef )
	{
		Set( otherRef.m_pObject );
		return *this;
	}

	FORCEINLINE CUtlReference &operator=( T *pObj )
	{
		Set( pObj );
		return *this;
	}


	FORCEINLINE bool operator==( const CUtlReference& o ) const
	{
		return ( o.m_pObject == m_pObject );
	}	

public:
	CUtlReference *m_pNext;
	CUtlReference *m_pPrev;

	T *m_pObject;

	FORCEINLINE void AddRef( T *pObj )
	{
		m_pObject = pObj;
		if ( pObj )
		{
			pObj->m_References.AddToHead( this );
		}
	}

	FORCEINLINE void KillRef(void)
	{
		if ( m_pObject )
		{
			m_pObject->m_References.RemoveNode( this );
			m_pObject = NULL;
		}
	}

};

template<class T> class CUtlReferenceList : public CUtlIntrusiveDList< CUtlReference<T> >
{
public:
	~CUtlReferenceList( void )
	{
		CUtlReference<T> *i = CUtlIntrusiveDList<CUtlReference<T> >::m_pHead;
		while( i )
		{
			CUtlReference<T> *n = i->m_pNext;
			i->m_pNext = NULL;
			i->m_pPrev = NULL;
			i->m_pObject = NULL;
			i = n;
		}
		CUtlIntrusiveDList<CUtlReference<T> >::m_pHead = NULL;
	}
};


//-----------------------------------------------------------------------------
// Put this macro in classes that are referenced by CUtlReference
//-----------------------------------------------------------------------------
#define DECLARE_REFERENCED_CLASS( _className )				\
	private:												\
		CUtlReferenceList< _className > m_References;		\
		template<class T> friend class CUtlReference;


#endif






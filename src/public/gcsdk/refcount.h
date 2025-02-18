//========= Copyright Valve Corporation, All rights reserved. ============//
//
// $NoKeywords: $
//=============================================================================

#ifndef GCREFCOUNT_H
#define GCREFCOUNT_H

#include "tier0/memdbgon.h"

namespace GCSDK
{

// Base class for ref counted classes.  Derive from this to be refcounted.  Note:
// you can no longer be deleted directly or declared on the stack.  Make your
// derived class' destructor private to ensure you can't be deleted directly or declared
// on the stack.

// utility class
template< class T >
class CAutoPtr
{
	T *m_pT;
public:
	CAutoPtr()
	{
		m_pT = NULL;
	}

	~CAutoPtr()
	{
		delete m_pT;
	}

	T *reset( T *p )
	{
		delete m_pT;
		m_pT = p;
		return m_pT;
	}

	T *TakeOwnership()
	{
		T *p = m_pT;
		m_pT = NULL;
		return p;
	}

	T *release( )
	{
		T *pT = m_pT;
		m_pT = NULL;
		return pT;
	}

	T *operator->()
	{
		return m_pT;
	}

	operator T*()
	{
		return m_pT;
	}

protected:
	T *operator=(T*p)
	{
		AssertMsg( NULL == m_pT, "If this assert fires, you're leaking.\n" );
		m_pT = p;
		return m_pT;
	}
};

class CRefCount 
{
public:
	CRefCount() { m_cRef = 1; }		// we are born with a ref count of 1

	// increment ref count
	int AddRef() { return ThreadInterlockedIncrement( &m_cRef ); }

	// delete ourselves when ref count reaches 0
	int Release() 
	{ 
		Assert( m_cRef > 0 ); 
		int cRef = ThreadInterlockedDecrement( &m_cRef ); 
		if ( 0 == cRef )
			DestroyThis();
		return cRef;
	}
protected:
	// Classes that derive from this should make their destructors private and virtual!
	virtual ~CRefCount() { Assert( 0 == m_cRef ); }

	virtual void DestroyThis() { delete this; }		// derived classes may override this if they want to be part of a mem pool

	volatile int32 m_cRef;					// ref count of this object
};

#define SAFE_RELEASE( x )		if ( NULL != ( x ) ) { ( x )->Release(); x = NULL; }

} // namespace GCSDK

#include "tier0/memdbgoff.h"

#endif // GCREFCOUNT_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Tools for correctly implementing & handling reference counted
//			objects
//
//=============================================================================

#ifndef REFCOUNT_H
#define REFCOUNT_H

#include "tier0/threadtools.h"

#if defined( _WIN32 )
#pragma once
#endif

template <typename T>
inline void SafeAssign(T** ppInoutDst, T* pInoutSrc )
{
	Assert( ppInoutDst );

	// Do addref before release
	if ( pInoutSrc )
		( pInoutSrc )->AddRef();

	// Do addref before release
	if ( *ppInoutDst )
		( *ppInoutDst )->Release();

	// Do the assignment
	( *ppInoutDst ) = pInoutSrc;
}

template <typename T>
inline void SafeAddRef( T* pObj )
{
	if ( pObj )
		pObj->AddRef();
}

template <typename T>
inline void SafeRelease( T** ppInoutPtr )
{
	Assert( ppInoutPtr  );
	if ( *ppInoutPtr )
		( *ppInoutPtr )->Release();

	( *ppInoutPtr ) = NULL;
}

//-----------------------------------------------------------------------------
// Purpose:	Implement a standard reference counted interface. Use of this
//			is optional insofar as all the concrete tools only require
//			at compile time that the function signatures match.
//-----------------------------------------------------------------------------

class IRefCounted
{
public:
	virtual int AddRef() = 0;
	virtual int Release() = 0;
};


//-----------------------------------------------------------------------------
// Purpose:	Release a pointer and mark it NULL
//-----------------------------------------------------------------------------

template <class REFCOUNTED_ITEM_PTR>
inline int SafeRelease( REFCOUNTED_ITEM_PTR &pRef )
{
	// Use funny syntax so that this works on "auto pointers"
	REFCOUNTED_ITEM_PTR *ppRef = &pRef;
	if ( *ppRef )
	{
		int result = (*ppRef)->Release();
		*ppRef = NULL;
		return result;
	}
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose:	Maintain a reference across a scope
//-----------------------------------------------------------------------------

template <class T = IRefCounted>
class CAutoRef
{
public:
	CAutoRef( T *pRef )
	  : m_pRef( pRef )
	{
		if ( m_pRef )
			m_pRef->AddRef();
	}

   ~CAutoRef()
   {
      if (m_pRef)
         m_pRef->Release();
   }

private:
   T *m_pRef;
};

//-----------------------------------------------------------------------------
// Purpose:	Do a an inline AddRef then return the pointer, useful when
//			returning an object from a function
//-----------------------------------------------------------------------------

#define RetAddRef( p ) ( (p)->AddRef(), (p) )
#define InlineAddRef( p ) ( (p)->AddRef(), (p) )


//-----------------------------------------------------------------------------
// Purpose:	A class to both hold a pointer to an object and its reference.
//			Base exists to support other cleanup models
//-----------------------------------------------------------------------------

template <class T>
class CBaseAutoPtr
{
public:
	CBaseAutoPtr()                                         	: m_pObject(0) {}
	CBaseAutoPtr(T *pFrom)                        			: m_pObject(pFrom) {}

	operator const void *() const          					{ return m_pObject; }
	operator void *()                      					{ return m_pObject; }

	operator const T *() const							    { return m_pObject; }
	operator const T *()          							{ return m_pObject; }
	operator T *()											{ return m_pObject; }

	int			operator=( int i )							{ AssertMsg( i == 0, "Only NULL allowed on integer assign" ); m_pObject = 0; return 0; }
	T *			operator=( T *p )							{ m_pObject = p; return p; }

    bool        operator !() const							{ return ( !m_pObject ); }
    bool        operator!=( int i ) const					{ AssertMsg( i == 0, "Only NULL allowed on integer compare" ); return (m_pObject != NULL); }
	bool		operator==( const void *p ) const			{ return ( m_pObject == p ); }
	bool		operator!=( const void *p ) const			{ return ( m_pObject != p ); }
	bool		operator==( T *p ) const					{ return operator==( (void *)p ); }
	bool		operator!=( T *p ) const					{ return operator!=( (void *)p ); }
	bool		operator==( const CBaseAutoPtr<T> &p ) const { return operator==( (const void *)p ); }
	bool		operator!=( const CBaseAutoPtr<T> &p ) const { return operator!=( (const void *)p ); }

	T *  		operator->()								{ return m_pObject; }
	T &  		operator *()								{ return *m_pObject; }
	T ** 		operator &()								{ return &m_pObject; }

	const T *   operator->() const							{ return m_pObject; }
	const T &   operator *() const							{ return *m_pObject; }
	T * const * operator &() const							{ return &m_pObject; }

protected:
	CBaseAutoPtr( const CBaseAutoPtr<T> &from )				: m_pObject( from.m_pObject ) {}
	void operator=( const CBaseAutoPtr<T> &from ) 			{ m_pObject = from.m_pObject; }

	T *m_pObject;
};

//---------------------------------------------------------

template <class T>
class CRefPtr : public CBaseAutoPtr<T>
{
	typedef CBaseAutoPtr<T> BaseClass;
public:
	CRefPtr()												{}
	CRefPtr( T *pInit )										: BaseClass( pInit ) {}
	CRefPtr( const CRefPtr<T> &from )						: BaseClass( from ) {}
	~CRefPtr()												{ if ( BaseClass::m_pObject ) BaseClass::m_pObject->Release(); }

	void operator=( const CRefPtr<T> &from )				{ BaseClass::operator=( from ); }

	int operator=( int i )									{ return BaseClass::operator=( i ); }
	T *operator=( T *p )									{ return BaseClass::operator=( p ); }

	operator bool() const									{ return !BaseClass::operator!(); }
	operator bool()											{ return !BaseClass::operator!(); }

	void SafeRelease()										{ if ( BaseClass::m_pObject ) BaseClass::m_pObject->Release(); BaseClass::m_pObject = 0; }
	void AssignAddRef( T *pFrom )							{ SafeRelease(); if (pFrom) pFrom->AddRef(); BaseClass::m_pObject = pFrom; }
	void AddRefAssignTo( T *&pTo )							{ ::SafeRelease( pTo ); if ( BaseClass::m_pObject ) BaseClass::m_pObject->AddRef(); pTo = BaseClass::m_pObject; }
};


//-----------------------------------------------------------------------------
// Purpose:	Traits classes defining reference count threading model
//-----------------------------------------------------------------------------

class CRefMT
{
public:
	static int Increment( int *p) { return ThreadInterlockedIncrement( (int32 *)p ); }
	static int Decrement( int *p) { return ThreadInterlockedDecrement( (int32 *)p ); }
};

class CRefST
{
public:
	static int Increment( int *p) { return ++(*p); }
	static int Decrement( int *p) { return --(*p); }
};

//-----------------------------------------------------------------------------
// Purpose:	Actual reference counting implementation. Pulled out to reduce
//			code bloat.
//-----------------------------------------------------------------------------

template <const bool bSelfDelete, typename CRefThreading = CRefMT>
class NO_VTABLE CRefCountServiceBase
{
protected:
	CRefCountServiceBase()
	  : m_iRefs( 1 )
	{
	}

	virtual ~CRefCountServiceBase()
	{
	}

	virtual bool OnFinalRelease()
	{
		return true;
	}

	int GetRefCount() const
	{
		return m_iRefs;
	}

	int DoAddRef()
	{
		return CRefThreading::Increment( &m_iRefs );
	}

	int DoRelease()
	{
		int result = CRefThreading::Decrement( &m_iRefs );
		if ( result )
			return result;
		if ( OnFinalRelease() && bSelfDelete )
			delete this;
		return 0;
	}

private:
	int m_iRefs;
};

class CRefCountServiceNull
{
protected:
	static int DoAddRef() { return 1; }
	static int DoRelease() { return 1; }
};

template <typename CRefThreading = CRefMT>
class NO_VTABLE CRefCountServiceDestruct
{
protected:
	CRefCountServiceDestruct()
		: m_iRefs( 1 )
	{
	}

	virtual ~CRefCountServiceDestruct()
	{
	}

	int GetRefCount() const
	{
		return m_iRefs;
	}

	int DoAddRef()
	{
		return CRefThreading::Increment( &m_iRefs );
	}

	int DoRelease()
	{
		int result = CRefThreading::Decrement( &m_iRefs );
		if ( result )
			return result;
		this->~CRefCountServiceDestruct();
		return 0;
	}

private:
	int m_iRefs;
};


typedef CRefCountServiceBase<true, CRefST>	CRefCountServiceST;
typedef CRefCountServiceBase<false, CRefST>	CRefCountServiceNoDeleteST;

typedef CRefCountServiceBase<true, CRefMT>	CRefCountServiceMT;
typedef CRefCountServiceBase<false, CRefMT> CRefCountServiceNoDeleteMT;

// Default to threadsafe
typedef CRefCountServiceNoDeleteMT			CRefCountServiceNoDelete;
typedef CRefCountServiceMT					CRefCountService;

//-----------------------------------------------------------------------------
// Purpose:	Base classes to implement reference counting
//-----------------------------------------------------------------------------

template < class REFCOUNT_SERVICE = CRefCountService > 
class NO_VTABLE CRefCounted : public REFCOUNT_SERVICE
{
public:
	virtual ~CRefCounted()	{}
	int AddRef() 			{ return REFCOUNT_SERVICE::DoAddRef(); }
	int Release()			{ return REFCOUNT_SERVICE::DoRelease(); }
};

//-------------------------------------

template < class BASE1, class REFCOUNT_SERVICE = CRefCountService > 
class NO_VTABLE CRefCounted1 : public BASE1,
							   public REFCOUNT_SERVICE
{
public:
	virtual ~CRefCounted1()	{}
	int AddRef() 			{ return REFCOUNT_SERVICE::DoAddRef(); }
	int Release()			{ return REFCOUNT_SERVICE::DoRelease(); }
};

//-------------------------------------

template < class BASE1, class BASE2, class REFCOUNT_SERVICE = CRefCountService > 
class NO_VTABLE CRefCounted2 : public BASE1, public BASE2,
							   public REFCOUNT_SERVICE
{
public:
	virtual ~CRefCounted2()	{}
	int AddRef() 			{ return REFCOUNT_SERVICE::DoAddRef(); }
	int Release()			{ return REFCOUNT_SERVICE::DoRelease(); }
};

//-------------------------------------

template < class BASE1, class BASE2, class BASE3, class REFCOUNT_SERVICE = CRefCountService > 
class NO_VTABLE CRefCounted3 : public BASE1, public BASE2, public BASE3,
							   public REFCOUNT_SERVICE
{
	virtual ~CRefCounted3()	{}
	int AddRef() 			{ return REFCOUNT_SERVICE::DoAddRef(); }
	int Release()			{ return REFCOUNT_SERVICE::DoRelease(); }
};

//-------------------------------------

template < class BASE1, class BASE2, class BASE3, class BASE4, class REFCOUNT_SERVICE = CRefCountService > 
class NO_VTABLE CRefCounted4 : public BASE1, public BASE2, public BASE3, public BASE4,
							   public REFCOUNT_SERVICE
{
public:
	virtual ~CRefCounted4()	{}
	int AddRef() 			{ return REFCOUNT_SERVICE::DoAddRef(); }
	int Release()			{ return REFCOUNT_SERVICE::DoRelease(); }
};

//-------------------------------------

template < class BASE1, class BASE2, class BASE3, class BASE4, class BASE5, class REFCOUNT_SERVICE = CRefCountService > 
class NO_VTABLE CRefCounted5 : public BASE1, public BASE2, public BASE3, public BASE4, public BASE5,
							   public REFCOUNT_SERVICE
{
public:
	virtual ~CRefCounted5()	{}
	int AddRef() 			{ return REFCOUNT_SERVICE::DoAddRef(); }
	int Release()			{ return REFCOUNT_SERVICE::DoRelease(); }
};

//-----------------------------------------------------------------------------
// Purpose:	Class to throw around a reference counted item to debug
//			referencing problems
//-----------------------------------------------------------------------------

template <class BASE_REFCOUNTED, int FINAL_REFS, const char *pszName>
class CRefDebug : public BASE_REFCOUNTED
{
public:
#ifdef _DEBUG
	CRefDebug()
	{
		AssertMsg( this->GetRefCount() == 1, "Expected initial ref count of 1" );
		DevMsg( "%s:create 0x%x\n", ( pszName ) ? pszName : "", this );
	}

	virtual ~CRefDebug()
	{
		AssertDevMsg( this->GetRefCount() == FINAL_REFS, "Object still referenced on destroy?" );
		DevMsg( "%s:destroy 0x%x\n", ( pszName ) ? pszName : "", this );
	}

	int AddRef()
	{
		DevMsg( "%s:(0x%x)->AddRef() --> %d\n", ( pszName ) ? pszName : "", this, this->GetRefCount() + 1 );
		return BASE_REFCOUNTED::AddRef();
	}

	int Release()
	{
		DevMsg( "%s:(0x%x)->Release() --> %d\n", ( pszName ) ? pszName : "", this, this->GetRefCount() - 1 );
		Assert( this->GetRefCount() > 0 );
		return BASE_REFCOUNTED::Release();
	}
#endif
};

//-----------------------------------------------------------------------------

#endif // REFCOUNT_H

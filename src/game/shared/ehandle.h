//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef EHANDLE_H
#define EHANDLE_H
#ifdef _WIN32
#pragma once
#endif

#if defined( _DEBUG ) && defined( GAME_DLL )
#include "tier0/dbg.h"
#include "cbase.h"
#endif


#include "const.h"
#include "basehandle.h"
#include "entitylist_base.h"


class IHandleEntity;


// -------------------------------------------------------------------------------------------------- //
// Game-code CBaseHandle implementation.
// -------------------------------------------------------------------------------------------------- //

inline IHandleEntity* CBaseHandle::Get() const
{
	extern CBaseEntityList *g_pEntityList;
	return g_pEntityList->LookupEntity( *this );
}


// -------------------------------------------------------------------------------------------------- //
// CHandle.
//
// Only safe to use in cases where you can statically verify that T* can safely be reinterpret-casted
// to IHandleEntity*; that is, that it's derived from IHandleEntity and IHandleEntity is the
// first base class.
//
// Unfortunately some classes are forward-declared and the compiler can't determine at compile time
// how to static_cast<> them to IHandleEntity.
// -------------------------------------------------------------------------------------------------- //
template< class T >
class CHandle : public CBaseHandle
{
public:

			CHandle();
			CHandle( int iEntry, int iSerialNumber );
	/*implicit*/ CHandle( T *pVal );
	/*implicit*/ CHandle( INVALID_EHANDLE_tag );

	// NOTE: The following two constructor functions are not type-safe, and can allow creating a
	// CHandle<T> that doesn't actually point to an object of type T.
	//
	// It is your responsibility to ensure that the target of the handle actually points to the
	// correct type of object before calling these functions.

	static CHandle<T> UnsafeFromBaseHandle( const CBaseHandle& handle );

	// The index should have come from a call to CBaseHandle::ToInt(). If it hasn't, you're in trouble.
	static CHandle<T> UnsafeFromIndex( int index );

	// h.ChangedFrom(p) is similar but not the same as h != p.
	// There is one case where they are different:
	//     h = someEntity;
	//     UTIL_Remove(someEntity);
	//     (wait for deletions to happen, usually at end of frame)
	// h is now a stale pointer to a deleted entity; h.Get() returns nullptr.
	//
	// In this case
	//    h != nullptr           -> false
	//    h.ChangedFrom(nullptr) -> true
	//
	// This is useful when you want to use a handle as a cache of some observed object, and need to tell
	// when your target has changed.  Using == fails if the target gets destroyed in the same frame that
	// your target is changed to null (which is actually pretty common!)
	//
	// In this case you can use this pattern:
	//    T* target = GetTargetedThing(); // might return null
	//    if( m_target.ChangedFrom( target ) )
	//    {
	//        m_target = target;
	//        // update stuff related to m_target
	//    }
	bool    ChangedFrom( T* ) const;

	T*		Get() const;
	void	Set( const T* pVal );

	/*implicit*/ operator T*();
	/*implicit*/ operator T*() const;

	bool	operator !() const;
	bool	operator==( T *val ) const;
	bool	operator!=( T *val ) const;
	CHandle& operator=( const T *val );

	T*		operator->() const;
};

// ----------------------------------------------------------------------- //
// Inlines.
// ----------------------------------------------------------------------- //

template<class T>
inline CHandle<T>::CHandle()
{
}

template<class T>
inline CHandle<T>::CHandle( INVALID_EHANDLE_tag )
	: CBaseHandle( INVALID_EHANDLE )
{
}

template<class T>
inline CHandle<T>::CHandle( int iEntry, int iSerialNumber )
{
	Init( iEntry, iSerialNumber );
}

template<class T>
inline CHandle<T>::CHandle( T *pObj )
	: CBaseHandle( INVALID_EHANDLE )
{
	Set( pObj );
}

template<class T>
inline CHandle<T> CHandle<T>::UnsafeFromBaseHandle( const CBaseHandle &handle )
{
	CHandle<T> ret;
	// REI Hack: CBaseHandle doesn't allow us to directly access m_Index, but
	// exposes it via ToInt().  Warning: code has not been tested
	COMPILE_TIME_ASSERT( sizeof( uint32 ) == sizeof( handle.ToInt() ) );
	ret.m_Index = (uint32)handle.ToInt();
	return ret;
}

template<class T>
inline CHandle<T> CHandle<T>::UnsafeFromIndex( int index )
{
	CHandle<T> ret;
	ret.m_Index = index;
	return ret;
}

template<class T>
inline bool CHandle<T>::ChangedFrom( T* ent ) const
{
	// "valid" is kind of a misnomer here.  It really means 'not pointing to NULL',
	// but it could be a stale handle (to a deleted entity), in which case
	// Get() == null.  We really want to show up as different from null in that case.
	if( ent == nullptr )
		return IsValid();

	return ent != Get();
}

template<class T>
inline T* CHandle<T>::Get() const
{
	return (T*)CBaseHandle::Get();
}


template<class T>
inline CHandle<T>::operator T *() 
{ 
	return Get( ); 
}

template<class T>
inline CHandle<T>::operator T *() const
{ 
	return Get( ); 
}


template<class T>
inline bool CHandle<T>::operator !() const
{
	return !Get();
}

template<class T>
inline bool CHandle<T>::operator==( T *val ) const
{
	return Get() == val;
}

template<class T>
inline bool CHandle<T>::operator!=( T *val ) const
{
	return Get() != val;
}

template<class T>
void CHandle<T>::Set( const T* pVal )
{
	// We can't even verify that the class can successfully reinterpret-cast to IHandleEntity*
	// because that will cause this function to fail to compile in Debug in the case of forward-declared
	// pointer types.
	//Assert( reinterpret_cast< const IHandleEntity* >( pVal ) == static_cast< IHandleEntity* >( pVal ) );

	const IHandleEntity* pValInterface = reinterpret_cast<const IHandleEntity*>( pVal );
	CBaseHandle::Set( pValInterface );
}

template<class T>
inline CHandle<T>& CHandle<T>::operator=( const T *val )
{
	Set( val );
	return *this;
}

template<class T>
T* CHandle<T>::operator -> () const
{
	return Get();
}

// specialization of EnsureValidValue for CHandle<T>
template<typename T>
FORCEINLINE void EnsureValidValue( CHandle<T> &x ) { x = INVALID_EHANDLE; }


#endif // EHANDLE_H

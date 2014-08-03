//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef DMEHANDLE_H
#define DMEHANDLE_H

#ifdef _WIN32
#pragma once
#endif


#include "datamodel/idatamodel.h"
#include "datamodel/dmelement.h"
#include "datamodel/dmattribute.h"
#include "datamodel/dmattributevar.h"


//-----------------------------------------------------------------------------
// Purpose: CDmeHandle is a templatized wrapper around DmElementHandle_t
//-----------------------------------------------------------------------------
template< class DmeType, bool Counted = false >
class CDmeHandle : public CDmeElementRefHelper
{
public:
	CDmeHandle() : m_handle( DMELEMENT_HANDLE_INVALID )
	{
	}

	explicit CDmeHandle( CDmElement *pObject ) : m_handle( DMELEMENT_HANDLE_INVALID )
	{
		Set( pObject );
	}

	CDmeHandle( DmElementHandle_t h ) : m_handle( DMELEMENT_HANDLE_INVALID )
	{
		Set( h );
	}

	CDmeHandle( const CDmeHandle< DmeType, Counted > &handle ) : m_handle( DMELEMENT_HANDLE_INVALID )
	{
		Set( handle.m_handle );
	}

	template < class T, bool B >
	CDmeHandle( const CDmeHandle< T, B > &handle ) : m_handle( DMELEMENT_HANDLE_INVALID )
	{
		DmeType *p = ( T* )NULL; // triggers compiler error if converting from invalid handle type
		NOTE_UNUSED( p );

		Set( handle.GetHandle() );
	}

	~CDmeHandle()
	{
		if ( !g_pDataModel )
			return; // some handles are static, and don't get destroyed until program termination

		Unref( m_handle, Counted );
	}

	template < class T, bool B >
	CDmeHandle& operator=( const CDmeHandle< T, B > &handle )
	{
		DmeType *p = ( T* )NULL; // triggers compiler error if converting from invalid handle type
		NOTE_UNUSED( p );

		Set( handle.GetHandle() );
		return *this;
	}

	DmeType *Get()
	{
		return static_cast< DmeType* >( g_pDataModel->GetElement( m_handle ) );
	}

	const DmeType *Get() const
	{
		return static_cast< DmeType* >( g_pDataModel->GetElement( m_handle ) );
	}

	DmElementHandle_t GetHandle() const
	{
		return m_handle;
	}

	void Set( CDmElement *pObject )
	{
		Set( pObject ? pObject->GetHandle() : DMELEMENT_HANDLE_INVALID );
	}

	void Set( DmElementHandle_t h )
	{
		if ( h == m_handle )
			return;

		Unref( m_handle, Counted );

		m_handle = h;
		if ( h != DMELEMENT_HANDLE_INVALID )
		{
			CDmElement *pElement = g_pDataModel->GetElement( m_handle );
			Assert( pElement );
			if ( pElement && !pElement->IsA( DmeType::GetStaticTypeSymbol() ) )
			{
				m_handle = DMELEMENT_HANDLE_INVALID;
			}
		}

		Ref( m_handle, Counted );
	}

	operator DmeType*()
	{
		return Get();
	}

	operator const DmeType*() const
	{
		return Get();
	}

	operator DmElementHandle_t() const
	{
		return m_handle;
	}

	DmeType* operator->()
	{ 
		return Get(); 
	}

	const DmeType* operator->() const
	{ 
		return Get(); 
	}

	CDmeHandle& operator=( DmElementHandle_t h )
	{
		Set( h );
		return *this;
	}

	CDmeHandle& operator=( CDmElement *pObject )
	{
		Set( pObject );
		return *this;
	}

	bool operator==( const CDmeHandle< DmeType > &h ) const
	{
		return m_handle == h.m_handle;
	}

	bool operator!=( const CDmeHandle< DmeType > &h ) const
	{
		return !operator==( h );
	}

	bool operator<( const CDmeHandle< DmeType > &h ) const
	{
		return m_handle < h.m_handle;
	}

	bool operator==( DmeType *pObject )	const
	{
		DmElementHandle_t h = pObject ? pObject->GetHandle() : DMELEMENT_HANDLE_INVALID;
		return m_handle == h;
	}

	bool operator!=( DmeType *pObject )	const
	{
		return !operator==( pObject );
	}

	bool operator==( DmElementHandle_t h ) const
	{
		return ( m_handle == h );
	}

	bool operator!=( DmElementHandle_t h ) const
	{
		return ( m_handle != h );
	}

	operator bool() const
	{ 
		return ( Get() != NULL );
	}

	bool operator!() const
	{
		return ( Get() == NULL );
	}

private:
	DmElementHandle_t m_handle;
};

typedef CDmeHandle< CDmElement, true > CDmeCountedHandle;


//-----------------------------------------------------------------------------
// Vector of element handles
//-----------------------------------------------------------------------------
typedef CUtlVector< CDmeHandle<CDmElement> > DmeHandleVec_t;



//-----------------------------------------------------------------------------
// helper class for undo classes to allow them to hold onto refcounted element handles
//-----------------------------------------------------------------------------

template< typename T >
class CDmAttributeUndoStorageType
{
public:
	typedef T UndoStorageType;
};

template<>
class CDmAttributeUndoStorageType< DmElementHandle_t >
{
public:
	typedef CDmeCountedHandle UndoStorageType;
};

template<>
class CDmAttributeUndoStorageType< CUtlVector< DmElementHandle_t > >
{
public:
	typedef CUtlVector< CDmeCountedHandle > UndoStorageType;
};

#endif // DMEHANDLE_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include <windows.h>
#include "threadhelpers.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------- //
// CVMPICriticalSection implementation.
// -------------------------------------------------------------------------------- //

CVMPICriticalSection::CVMPICriticalSection()
{
	Assert( sizeof( CRITICAL_SECTION ) == SIZEOF_CS );

#if defined( _DEBUG )
	InitializeCriticalSection( (CRITICAL_SECTION*)&m_DeadlockProtect );
#endif

	InitializeCriticalSection( (CRITICAL_SECTION*)&m_CS );
}


CVMPICriticalSection::~CVMPICriticalSection()
{
	DeleteCriticalSection( (CRITICAL_SECTION*)&m_CS );

#if defined( _DEBUG )
	DeleteCriticalSection( (CRITICAL_SECTION*)&m_DeadlockProtect );
#endif
}



void CVMPICriticalSection::Lock()
{
#if defined( _DEBUG )
	// Check if this one is already locked.
	DWORD id = GetCurrentThreadId();
	EnterCriticalSection( (CRITICAL_SECTION*)&m_DeadlockProtect );
		Assert( m_Locks.Find( id ) == m_Locks.InvalidIndex() );
		m_Locks.AddToTail( id );
	LeaveCriticalSection( (CRITICAL_SECTION*)&m_DeadlockProtect );
#endif

	EnterCriticalSection( (CRITICAL_SECTION*)&m_CS );
}


void CVMPICriticalSection::Unlock()
{
#if defined( _DEBUG )
	// Check if this one is already locked.
	DWORD id = GetCurrentThreadId();
	EnterCriticalSection( (CRITICAL_SECTION*)&m_DeadlockProtect );
		int index = m_Locks.Find( id );
		Assert( index != m_Locks.InvalidIndex() );
		m_Locks.Remove( index );
	LeaveCriticalSection( (CRITICAL_SECTION*)&m_DeadlockProtect );
#endif
	
	LeaveCriticalSection( (CRITICAL_SECTION*)&m_CS );
}



// -------------------------------------------------------------------------------- //
// CVMPICriticalSectionLock implementation.
// -------------------------------------------------------------------------------- //

CVMPICriticalSectionLock::CVMPICriticalSectionLock( CVMPICriticalSection *pCS )
{
	m_pCS = pCS;
	m_bLocked = false;
}


CVMPICriticalSectionLock::~CVMPICriticalSectionLock()
{
	if ( m_bLocked )
		m_pCS->Unlock();
}


void CVMPICriticalSectionLock::Lock()
{
	Assert( !m_bLocked );
	m_bLocked = true;
	m_pCS->Lock();
}


void CVMPICriticalSectionLock::Unlock()
{
	Assert( m_bLocked );
	m_bLocked = false;
	m_pCS->Unlock();
}


// -------------------------------------------------------------------------------- //
// CEvent implementation.
// -------------------------------------------------------------------------------- //

CEvent::CEvent()
{
	m_hEvent = NULL;
}

CEvent::~CEvent()
{
	Term();
}

bool CEvent::Init( bool bManualReset, bool bInitialState )
{
	Term();

	m_hEvent = (void*)CreateEvent( NULL, bManualReset, bInitialState, NULL );
	return (m_hEvent != NULL);
}

void CEvent::Term()
{
	if ( m_hEvent )
	{
		CloseHandle( (HANDLE)m_hEvent );
		m_hEvent = NULL;
	}
}

void* CEvent::GetEventHandle() const
{
	Assert( m_hEvent );
	return m_hEvent;
}

bool CEvent::SetEvent()
{
	Assert( m_hEvent );
	return ::SetEvent( (HANDLE)m_hEvent ) != 0;
}

bool CEvent::ResetEvent()
{
	Assert( m_hEvent );
	return ::ResetEvent( (HANDLE)m_hEvent ) != 0;
}



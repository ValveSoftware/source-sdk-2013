//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "timedeventmgr.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// ------------------------------------------------------------------------------------------ //
// CEventRegister.
// ------------------------------------------------------------------------------------------ //

CEventRegister::CEventRegister()
{
	m_bRegistered = false;
	m_pEventMgr = NULL;
}


CEventRegister::~CEventRegister()
{
	Term();
}


void CEventRegister::Init( CTimedEventMgr *pMgr, IEventRegisterCallback *pCallback )
{
	Term();
	m_pEventMgr = pMgr;
	m_pCallback = pCallback;
}


void CEventRegister::Term()
{
	// Unregister.
	if ( m_pEventMgr && m_bRegistered )
	{
		m_pEventMgr->RemoveEvent( this );
	}
}
	

void CEventRegister::SetUpdateInterval( float interval )
{
	Assert( m_pEventMgr );

	if ( m_pEventMgr )
	{
		// Register for this event.
		m_flUpdateInterval = interval;
		m_flNextEventTime = gpGlobals->curtime + m_flUpdateInterval;

		m_pEventMgr->RegisterForNextEvent( this );
	}
}


void CEventRegister::StopUpdates()
{
	if ( m_pEventMgr )
	{
		// Unregister our next event.
		m_pEventMgr->RemoveEvent( this );
	}
}


void CEventRegister::Reregister()
{
	if ( m_flUpdateInterval > 1e-6 && m_pEventMgr )
	{
		while ( m_flNextEventTime <= gpGlobals->curtime )
		{
			m_flNextEventTime += m_flUpdateInterval;
		}

		m_pEventMgr->RegisterForNextEvent( this );
	}
}


// ------------------------------------------------------------------------------------------ //
// CTimedEventMgr.
// ------------------------------------------------------------------------------------------ //

bool TimedEventMgr_LessFunc( CEventRegister* const &a, CEventRegister* const &b )
{
	return a->m_flNextEventTime > b->m_flNextEventTime;
}


CTimedEventMgr::CTimedEventMgr()
{
	m_Events.SetLessFunc( TimedEventMgr_LessFunc );
}


void CTimedEventMgr::FireEvents()
{
	VPROF( "CTimedEventMgr::FireEvents" );
	while ( m_Events.Count() )
	{
		// Fire the top element, then break out.
		CEventRegister *pEvent = m_Events.ElementAtHead();
		if ( gpGlobals->curtime >= pEvent->m_flNextEventTime )
		{
			// Reregister for the timed event, then fire the callback for the event.
			m_Events.RemoveAtHead();
			pEvent->m_bRegistered = false;
			pEvent->Reregister();

			pEvent->m_pCallback->FireEvent();
		}
		else
		{
			break;
		}
	}
}


void CTimedEventMgr::RegisterForNextEvent( CEventRegister *pEvent )
{
	RemoveEvent( pEvent );
	m_Events.Insert( pEvent );
	pEvent->m_bRegistered = true;
}


void CTimedEventMgr::RemoveEvent( CEventRegister *pEvent )
{
	if ( pEvent->m_bRegistered )
	{
		// Find the event in the list and remove it.
		int cnt = m_Events.Count();
		for ( int i=0; i < cnt; i++ )
		{
			if ( m_Events.Element( i ) == pEvent )
			{
				m_Events.RemoveAt( i );
				break;
			}
		}
	}
}

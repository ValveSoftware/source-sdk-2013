//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "base_transmit_proxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CBaseTransmitProxy::CBaseTransmitProxy( CBaseEntity *pEnt )
{
	m_hEnt = pEnt;
	m_refCount = 0;
}


CBaseTransmitProxy::~CBaseTransmitProxy()
{
	// Unlink from our parent entity.
	if ( m_hEnt )
	{
		m_refCount = 0xFFFF; // Prevent us from deleting ourselves again.
		// m_hEnt->NetworkProp()->SetTransmitProxy( NULL );
	}
}


int CBaseTransmitProxy::ShouldTransmit( const CCheckTransmitInfo *pInfo, int nPrevShouldTransmitResult )
{
	// Anyone implementing a transmit proxy should override this since that's the point!!
	Assert( false );
	return FL_EDICT_DONTSEND;
}


void CBaseTransmitProxy::AddRef()
{
	m_refCount++;
}


void CBaseTransmitProxy::Release()
{
	if ( m_refCount == 0xFFFF )
	{
		// This means we are inside our destructor already, so we don't want to do anything here.
	}
	else if ( m_refCount <= 1 )
	{
		delete this;
	}
	else
	{
		--m_refCount;
	}
}


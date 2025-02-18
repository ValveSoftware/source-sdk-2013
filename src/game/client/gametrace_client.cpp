//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "gametrace.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool CGameTrace::DidHitWorld() const
{
	return m_pEnt == ClientEntityList().GetBaseEntity( 0 );
}


bool CGameTrace::DidHitNonWorldEntity() const
{
	return m_pEnt != NULL && !DidHitWorld();
}


int CGameTrace::GetEntityIndex() const
{
	if ( m_pEnt )
		return m_pEnt->entindex();
	else
		return -1;
}


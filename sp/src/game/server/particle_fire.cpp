//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

#include "particle_fire.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


IMPLEMENT_SERVERCLASS_ST_NOBASE(CParticleFire, DT_ParticleFire)
	SendPropVector(SENDINFO(m_vOrigin),    0, SPROP_COORD),
	SendPropVector(SENDINFO(m_vDirection), 0, SPROP_NOSCALE)
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_particlefire, CParticleFire );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CParticleFire )

	DEFINE_FIELD( m_vOrigin,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vDirection,	FIELD_VECTOR ),

END_DATADESC()


CParticleFire::CParticleFire()
{
#ifdef _DEBUG
	m_vOrigin.Init();
	m_vDirection.Init();
#endif
}




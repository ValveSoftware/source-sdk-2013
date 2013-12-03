//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Drops particles where the entity was.
//
//=============================================================================//

#include "cbase.h"
#include "entityparticletrail_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Default values
//-----------------------------------------------------------------------------
EntityParticleTrailInfo_t::EntityParticleTrailInfo_t()
{
	m_strMaterialName = NULL_STRING;
	m_flLifetime = 4.0f;
	m_flStartSize = 2.0f;
	m_flEndSize = 3.0f;
}

//-----------------------------------------------------------------------------
// Save/load 
//-----------------------------------------------------------------------------
#ifndef CLIENT_DLL

BEGIN_SIMPLE_DATADESC( EntityParticleTrailInfo_t )

	DEFINE_KEYFIELD( m_strMaterialName,	FIELD_STRING,	"ParticleTrailMaterial" ),
	DEFINE_KEYFIELD( m_flLifetime,		FIELD_FLOAT,	"ParticleTrailLifetime" ),
	DEFINE_KEYFIELD( m_flStartSize,		FIELD_FLOAT,	"ParticleTrailStartSize" ),
	DEFINE_KEYFIELD( m_flEndSize,		FIELD_FLOAT,	"ParticleTrailEndSize" ),

END_DATADESC()

#endif

//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
BEGIN_NETWORK_TABLE_NOBASE( EntityParticleTrailInfo_t, DT_EntityParticleTrailInfo )

#ifdef CLIENT_DLL
	RecvPropFloat( RECVINFO( m_flLifetime ) ),
	RecvPropFloat( RECVINFO( m_flStartSize ) ),
	RecvPropFloat( RECVINFO( m_flEndSize ) ),
#else
	SendPropFloat( SENDINFO( m_flLifetime ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flStartSize ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_flEndSize ), 0, SPROP_NOSCALE ),
#endif

END_NETWORK_TABLE()




//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "particle_light.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_particlelight, CParticleLight );


//Save/restore
BEGIN_DATADESC( CParticleLight )

	//Keyvalue fields
	DEFINE_KEYFIELD( m_flIntensity,		FIELD_FLOAT,	"Intensity" ),
	DEFINE_KEYFIELD( m_vColor,			FIELD_VECTOR,	"Color" ),
	DEFINE_KEYFIELD( m_PSName,			FIELD_STRING,	"PSName" ),
	DEFINE_KEYFIELD( m_bDirectional,	FIELD_BOOLEAN,	"Directional" )

END_DATADESC()



//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
CParticleLight::CParticleLight()
{
	m_flIntensity = 5000;
	m_vColor.Init( 1, 0, 0 );
	m_PSName = NULL_STRING;
	m_bDirectional = false;
}



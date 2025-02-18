//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: An entity that spawns and controls a particle system
//
//=============================================================================

#include "cbase.h"
#include "particles/particles.h"
#include "networkstringtable_gamedll.h"
#include "particle_system.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern void SendProxy_Origin( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );
extern void SendProxy_Angles( const SendProp *pProp, const void *pStruct, const void *pData, DVariant *pOut, int iElement, int objectID );

// Stripped down CBaseEntity send table
IMPLEMENT_SERVERCLASS_ST_NOBASE(CParticleSystem, DT_ParticleSystem)
	SendPropVector	(SENDINFO(m_vecOrigin), -1,  SPROP_COORD|SPROP_CHANGES_OFTEN, 0.0f, HIGH_DEFAULT, SendProxy_Origin ),
	SendPropEHandle (SENDINFO(m_hOwnerEntity)),
	SendPropEHandle (SENDINFO_NAME(m_hMoveParent, moveparent)),
	SendPropInt		(SENDINFO(m_iParentAttachment), NUM_PARENTATTACHMENT_BITS, SPROP_UNSIGNED),
	SendPropQAngles	(SENDINFO(m_angRotation), 13, SPROP_CHANGES_OFTEN, SendProxy_Angles ),

	SendPropInt( SENDINFO(m_iEffectIndex), MAX_PARTICLESYSTEMS_STRING_BITS, SPROP_UNSIGNED ),
	SendPropBool( SENDINFO(m_bActive) ),
	SendPropFloat( SENDINFO(m_flStartTime) ),

	SendPropArray3( SENDINFO_ARRAY3(m_hControlPointEnts), SendPropEHandle( SENDINFO_ARRAY(m_hControlPointEnts) ) ),
	SendPropArray3( SENDINFO_ARRAY3(m_iControlPointParents), SendPropInt( SENDINFO_ARRAY(m_iControlPointParents), 3, SPROP_UNSIGNED ) ),
	SendPropBool( SENDINFO(m_bWeatherEffect) ),
END_SEND_TABLE()

BEGIN_DATADESC( CParticleSystem )
	DEFINE_KEYFIELD( m_bStartActive,	FIELD_BOOLEAN, "start_active" ),
	DEFINE_KEYFIELD( m_bWeatherEffect,	FIELD_BOOLEAN, "flag_as_weather" ),
	DEFINE_FIELD( m_bActive,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flStartTime,		FIELD_TIME ),
	DEFINE_KEYFIELD( m_iszEffectName,	FIELD_STRING, "effect_name" ),
	//DEFINE_FIELD( m_iEffectIndex, FIELD_INTEGER ),	// Don't save. Refind after loading.

	DEFINE_KEYFIELD( m_iszControlPointNames[0], FIELD_STRING, "cpoint1" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[1], FIELD_STRING, "cpoint2" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[2], FIELD_STRING, "cpoint3" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[3], FIELD_STRING, "cpoint4" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[4], FIELD_STRING, "cpoint5" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[5], FIELD_STRING, "cpoint6" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[6], FIELD_STRING, "cpoint7" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[7], FIELD_STRING, "cpoint8" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[8], FIELD_STRING, "cpoint9" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[9], FIELD_STRING, "cpoint10" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[10], FIELD_STRING, "cpoint11" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[11], FIELD_STRING, "cpoint12" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[12], FIELD_STRING, "cpoint13" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[13], FIELD_STRING, "cpoint14" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[14], FIELD_STRING, "cpoint15" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[15], FIELD_STRING, "cpoint16" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[16], FIELD_STRING, "cpoint17" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[17], FIELD_STRING, "cpoint18" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[18], FIELD_STRING, "cpoint19" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[19], FIELD_STRING, "cpoint20" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[20], FIELD_STRING, "cpoint21" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[21], FIELD_STRING, "cpoint22" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[22], FIELD_STRING, "cpoint23" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[23], FIELD_STRING, "cpoint24" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[24], FIELD_STRING, "cpoint25" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[25], FIELD_STRING, "cpoint26" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[26], FIELD_STRING, "cpoint27" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[27], FIELD_STRING, "cpoint28" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[28], FIELD_STRING, "cpoint29" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[29], FIELD_STRING, "cpoint30" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[30], FIELD_STRING, "cpoint31" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[31], FIELD_STRING, "cpoint32" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[32], FIELD_STRING, "cpoint33" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[33], FIELD_STRING, "cpoint34" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[34], FIELD_STRING, "cpoint35" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[35], FIELD_STRING, "cpoint36" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[36], FIELD_STRING, "cpoint37" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[37], FIELD_STRING, "cpoint38" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[38], FIELD_STRING, "cpoint39" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[39], FIELD_STRING, "cpoint40" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[40], FIELD_STRING, "cpoint41" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[41], FIELD_STRING, "cpoint42" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[42], FIELD_STRING, "cpoint43" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[43], FIELD_STRING, "cpoint44" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[44], FIELD_STRING, "cpoint45" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[45], FIELD_STRING, "cpoint46" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[46], FIELD_STRING, "cpoint47" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[47], FIELD_STRING, "cpoint48" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[48], FIELD_STRING, "cpoint49" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[49], FIELD_STRING, "cpoint50" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[50], FIELD_STRING, "cpoint51" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[51], FIELD_STRING, "cpoint52" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[52], FIELD_STRING, "cpoint53" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[53], FIELD_STRING, "cpoint54" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[54], FIELD_STRING, "cpoint55" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[55], FIELD_STRING, "cpoint56" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[56], FIELD_STRING, "cpoint57" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[57], FIELD_STRING, "cpoint58" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[58], FIELD_STRING, "cpoint59" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[59], FIELD_STRING, "cpoint60" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[60], FIELD_STRING, "cpoint61" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[61], FIELD_STRING, "cpoint62" ),
	DEFINE_KEYFIELD( m_iszControlPointNames[62], FIELD_STRING, "cpoint63" ),

	DEFINE_KEYFIELD( m_iControlPointParents[0], FIELD_CHARACTER, "cpoint1_parent" ),
	DEFINE_KEYFIELD( m_iControlPointParents[1], FIELD_CHARACTER, "cpoint2_parent" ),
	DEFINE_KEYFIELD( m_iControlPointParents[2], FIELD_CHARACTER, "cpoint3_parent" ),
	DEFINE_KEYFIELD( m_iControlPointParents[3], FIELD_CHARACTER, "cpoint4_parent" ),
	DEFINE_KEYFIELD( m_iControlPointParents[4], FIELD_CHARACTER, "cpoint5_parent" ),
	DEFINE_KEYFIELD( m_iControlPointParents[5], FIELD_CHARACTER, "cpoint6_parent" ),
	DEFINE_KEYFIELD( m_iControlPointParents[6], FIELD_CHARACTER, "cpoint7_parent" ),
	
	DEFINE_AUTO_ARRAY( m_hControlPointEnts, FIELD_EHANDLE ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Start", InputStart ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Stop", InputStop ),

	DEFINE_THINKFUNC( StartParticleSystemThink ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( info_particle_system, CParticleSystem );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CParticleSystem::CParticleSystem()
{
	m_bWeatherEffect = false;
}

//-----------------------------------------------------------------------------
// Precache 
//-----------------------------------------------------------------------------
void CParticleSystem::Precache( void )
{
	const char *pParticleSystemName = STRING( m_iszEffectName );
	if ( pParticleSystemName == NULL || pParticleSystemName[0] == '\0' )
	{
		Warning( "info_particle_system (%s) has no particle system name specified!\n", GetEntityName().ToCStr() );
	}

	PrecacheParticleSystem( pParticleSystemName );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystem::Spawn( void )
{
	BaseClass::Spawn();

	Precache();
	m_iEffectIndex = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystem::Activate( void )
{
	BaseClass::Activate();

	// Find our particle effect index
	m_iEffectIndex = GetParticleSystemIndex( STRING(m_iszEffectName) );

	if ( m_bStartActive )
	{
		m_bStartActive = false;
		StartParticleSystem();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystem::StartParticleSystemThink( void )
{
	StartParticleSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Always transmitted to clients
//-----------------------------------------------------------------------------
int CParticleSystem::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystem::StartParticleSystem( void )
{
	if ( m_bActive == false )
	{
		m_flStartTime = gpGlobals->curtime;
		m_bActive = true;
		
		// Setup our control points at this time (in case our targets weren't around at spawn time)
		ReadControlPointEnts();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystem::StopParticleSystem( void )
{
	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystem::InputStart( inputdata_t &inputdata )
{
	StartParticleSystem();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticleSystem::InputStop( inputdata_t &inputdata )
{
	StopParticleSystem();
}

//-----------------------------------------------------------------------------
// Purpose: Find each entity referred to by m_iszControlPointNames and 
// resolve it into the corresponding slot in m_hControlPointEnts
//-----------------------------------------------------------------------------
void CParticleSystem::ReadControlPointEnts( void )
{
	for ( int i = 0 ; i < kMAXCONTROLPOINTS; ++i )
	{
		if ( m_iszControlPointNames[i] == NULL_STRING )
			continue;

		CBaseEntity *pPointEnt = gEntList.FindEntityGeneric( NULL, STRING( m_iszControlPointNames[i] ), this );
		Assert( pPointEnt != NULL );
		if ( pPointEnt == NULL )
		{
			Warning("Particle system %s could not find control point entity (%s)\n", GetEntityName().ToCStr(), m_iszControlPointNames[i].ToCStr() );
			continue;
		}

		m_hControlPointEnts.Set( i, pPointEnt );
	}
}

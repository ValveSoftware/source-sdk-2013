//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Implements the server side of a steam jet particle system entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "steamjet.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//Networking
IMPLEMENT_SERVERCLASS_ST(CSteamJet, DT_SteamJet)
	SendPropFloat(SENDINFO(m_SpreadSpeed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Speed), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_StartSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_EndSize), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Rate), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_JetLength), 0, SPROP_NOSCALE),
	SendPropInt(SENDINFO(m_bEmit), 1, SPROP_UNSIGNED),
	SendPropInt(SENDINFO(m_bFaceLeft), 1, SPROP_UNSIGNED), // For support of legacy env_steamjet, which faced left instead of forward.
	SendPropInt(SENDINFO(m_nType), 32, SPROP_UNSIGNED),
	SendPropInt( SENDINFO(m_spawnflags), 8, SPROP_UNSIGNED ),
	SendPropFloat(SENDINFO(m_flRollSpeed), 0, SPROP_NOSCALE),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_steam, CSteamJet );
LINK_ENTITY_TO_CLASS( env_steamjet, CSteamJet ); // For support of legacy env_steamjet, which faced left instead of forward.

//Save/restore
BEGIN_DATADESC( CSteamJet )

	//Keyvalue fields
	DEFINE_KEYFIELD( m_StartSize,	FIELD_FLOAT,	"StartSize" ),
	DEFINE_KEYFIELD( m_EndSize,		FIELD_FLOAT,	"EndSize" ),
	DEFINE_KEYFIELD( m_InitialState,	FIELD_BOOLEAN,	"InitialState" ),
	DEFINE_KEYFIELD( m_nType,		FIELD_INTEGER,	"Type" ),
	DEFINE_KEYFIELD( m_flRollSpeed, FIELD_FLOAT, "RollSpeed" ),

	//Regular fields
	DEFINE_FIELD( m_bEmit, FIELD_INTEGER ),
	DEFINE_FIELD( m_bFaceLeft, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUT( m_JetLength, FIELD_FLOAT, "JetLength" ),
	DEFINE_INPUT( m_SpreadSpeed, FIELD_FLOAT, "SpreadSpeed" ),
	DEFINE_INPUT( m_Speed, FIELD_FLOAT, "Speed" ),
	DEFINE_INPUT( m_Rate, FIELD_FLOAT, "Rate" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()


CSteamJet::CSteamJet( void )
{
	m_flRollSpeed = 8.0f;
}
//-----------------------------------------------------------------------------
// Purpose: Called before spawning, after key values have been set.
//-----------------------------------------------------------------------------
void CSteamJet::Spawn( void )
{
	Precache();

	//
	// Legacy env_steamjet pointed left instead of forward.
	//
	if ( FClassnameIs( this, "env_steamjet" ))
	{
		m_bFaceLeft = true;
	}

	if ( m_InitialState )
	{
		m_bEmit = true;
	}
}

void CSteamJet::Precache( void )
{
	PrecacheMaterial( "particle/particle_smokegrenade" );
	PrecacheMaterial( "sprites/heatwave" );
}

 void CSteamJet::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
 {
	if (!pActivator->IsPlayer())
	{
		if (useType == USE_ON)
		{
			m_bEmit = true;
		}
		else if (useType == USE_OFF)
		{
			m_bEmit = false;
		}
	}
 }


//-----------------------------------------------------------------------------
// Purpose: Input handler for toggling the steam jet on/off.
//-----------------------------------------------------------------------------
void CSteamJet::InputToggle(inputdata_t &data)
{
	m_bEmit = !m_bEmit;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning on the steam jet.
//-----------------------------------------------------------------------------
void CSteamJet::InputTurnOn(inputdata_t &data)
{
	m_bEmit = true;
}


//-----------------------------------------------------------------------------
// Purpose: Input handler for turning off the steam jet.
//-----------------------------------------------------------------------------
void CSteamJet::InputTurnOff(inputdata_t &data)
{
	m_bEmit = false;
}

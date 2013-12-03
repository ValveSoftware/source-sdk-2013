//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "tesla.h"
#include "te_effect_dispatch.h"
#include "sendproxy.h"
#include "engine/IEngineSound.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( point_tesla, CTesla );

BEGIN_DATADESC( CTesla )

	DEFINE_KEYFIELD( m_SourceEntityName,FIELD_STRING,	"m_SourceEntityName" ),
	DEFINE_KEYFIELD( m_SoundName,		FIELD_STRING,	"m_SoundName" ),
	DEFINE_KEYFIELD( m_iszSpriteName,	FIELD_STRING,	"texture" ),

	DEFINE_KEYFIELD( m_Color,			FIELD_COLOR32,	"m_Color" ),
	DEFINE_KEYFIELD( m_flRadius,		FIELD_FLOAT,	"m_flRadius" ),

	//DEFINE_ARRAY( m_flThickness, FIELD_FLOAT, 2 ),
	DEFINE_KEYFIELD( m_flThickness[0],	FIELD_FLOAT,	"thick_min" ),
	DEFINE_KEYFIELD( m_flThickness[1],	FIELD_FLOAT,	"thick_max" ),
	
	//DEFINE_ARRAY( m_flTimeVisible, FIELD_FLOAT, 2 ),
	DEFINE_KEYFIELD( m_flTimeVisible[0],FIELD_FLOAT,	"lifetime_min" ),
	DEFINE_KEYFIELD( m_flTimeVisible[1],FIELD_FLOAT,	"lifetime_max" ),

	//DEFINE_ARRAY( m_flArcInterval, FIELD_FLOAT, 2 ),
	DEFINE_KEYFIELD( m_flArcInterval[0],FIELD_FLOAT,	"interval_min" ),
	DEFINE_KEYFIELD( m_flArcInterval[1],FIELD_FLOAT,	"interval_max" ),
	
	//DEFINE_ARRAY( m_NumBeams, FIELD_INTEGER, 2 ),
	DEFINE_KEYFIELD( m_NumBeams[0],		FIELD_INTEGER,	"beamcount_min" ),
	DEFINE_KEYFIELD( m_NumBeams[1],		FIELD_INTEGER,	"beamcount_max" ),

	DEFINE_KEYFIELD( m_bOn,				FIELD_BOOLEAN,	"m_bOn" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn",  InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "DoSpark", InputDoSpark ),

	DEFINE_FUNCTION( ShootArcThink )

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST( CTesla, DT_Tesla )
	SendPropStringT( SENDINFO( m_SoundName ) ),
	SendPropStringT( SENDINFO( m_iszSpriteName ) )
END_SEND_TABLE()


CTesla::CTesla()
{
	m_SourceEntityName = NULL_STRING;
	m_SoundName = NULL_STRING;
	m_iszSpriteName = NULL_STRING;
	m_NumBeams[0] = m_NumBeams[1] = 6;
	m_flRadius = 200;
	m_flThickness[0] = m_flThickness[1] = 5;
	m_flTimeVisible[0] = 0.3;
	m_flTimeVisible[1] = 0.55;
	m_flArcInterval[0] = m_flArcInterval[1] = 0.5;
	
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}


void CTesla::Spawn()
{
	if ( m_iszSpriteName.Get() == NULL_STRING )
	{
		m_iszSpriteName = AllocPooledString("sprites/physbeam.vmt");
	}

	Precache();
	BaseClass::Spawn();
}


void CTesla::Activate()
{
	BaseClass::Activate();

	SetThink( &CTesla::ShootArcThink );
	SetupForNextArc();
}


void CTesla::Precache()
{
	PrecacheModel( STRING(m_iszSpriteName.Get()) );
	BaseClass::Precache();

	PrecacheScriptSound( STRING( m_SoundName.Get() ) );
}


void CTesla::SetupForNextArc()
{
	if (m_bOn)
	{
		float flTimeToNext = RandomFloat( m_flArcInterval[0], m_flArcInterval[1] );
		SetNextThink( gpGlobals->curtime + flTimeToNext );
	}
	else
	{
		SetNextThink( TICK_NEVER_THINK );
	}
}


CBaseEntity* CTesla::GetSourceEntity()
{
	if ( m_SourceEntityName != NULL_STRING )
	{
		CBaseEntity *pRet = gEntList.FindEntityByName( NULL, m_SourceEntityName );
		if ( pRet )
			return pRet;
	}

	return this;
}		


void CTesla::ShootArcThink()
{
	DoSpark();
	SetupForNextArc();
}


void CTesla::DoSpark()
{
	// Shoot out an arc.
	EntityMessageBegin( this );

		CBaseEntity *pEnt = GetSourceEntity();
		
		WRITE_VEC3COORD( pEnt->GetAbsOrigin() );
		WRITE_SHORT( pEnt->entindex() );
		WRITE_FLOAT( m_flRadius );
		WRITE_BYTE( m_Color.r );
		WRITE_BYTE( m_Color.g );
		WRITE_BYTE( m_Color.b );
		WRITE_BYTE( m_Color.a );
		WRITE_CHAR( RandomInt( m_NumBeams[0], m_NumBeams[1] ) );
		WRITE_FLOAT( RandomFloat( m_flThickness[0], m_flThickness[1] ) );
		WRITE_FLOAT( RandomFloat( m_flTimeVisible[0], m_flTimeVisible[1] ) );

	MessageEnd();
}


void CTesla::InputDoSpark( inputdata_t &inputdata )
{
	DoSpark();
}


void CTesla::InputTurnOn( inputdata_t &inputdata )
{
	m_bOn = true;
	SetupForNextArc();
}


void CTesla::InputTurnOff( inputdata_t &inputdata )
{
	m_bOn = false;
	SetupForNextArc();
}


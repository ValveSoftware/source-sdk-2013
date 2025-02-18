//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_roster.cpp
// entity that dictates what classes a bot can choose when spawning
// Tom Bui, April 2010

#include "cbase.h"

#include "tf_shareddefs.h"
#include "bot/map_entities/tf_bot_roster.h"

//------------------------------------------------------------------------------

BEGIN_DATADESC( CTFBotRoster )
	DEFINE_KEYFIELD( m_teamName,								FIELD_STRING,	"team" ),
	DEFINE_KEYFIELD( m_bAllowClassChanges,						FIELD_BOOLEAN,	"allowClassChanges" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_SCOUT],			FIELD_BOOLEAN,	"allowScout" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_SNIPER],		FIELD_BOOLEAN,	"allowSniper" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_SOLDIER],		FIELD_BOOLEAN,	"allowSoldier" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_DEMOMAN],		FIELD_BOOLEAN,	"allowDemoman" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_MEDIC],			FIELD_BOOLEAN,	"allowMedic" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_HEAVYWEAPONS],	FIELD_BOOLEAN,	"allowHeavy" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_PYRO],			FIELD_BOOLEAN,	"allowPyro" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_SPY],			FIELD_BOOLEAN,	"allowSpy" ),
	DEFINE_KEYFIELD( m_bAllowedClasses[TF_CLASS_ENGINEER],		FIELD_BOOLEAN,	"allowEngineer" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetTeam", InputSetTeam ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowScout", InputSetAllowScout ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowSniper", InputSetAllowSniper ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowSoldier", InputSetAllowSoldier ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowDemoman", InputSetAllowDemoman ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowMedic", InputSetAllowMedic ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowHeavy", InputSetAllowHeavy ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowPyro", InputSetAllowPyro ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowSpy", InputSetAllowSpy ),
	DEFINE_INPUTFUNC( FIELD_BOOLEAN, "SetAllowEngineer", InputSetAllowEngineer ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( bot_roster, CTFBotRoster );

//------------------------------------------------------------------------------

CTFBotRoster::CTFBotRoster()
{
	memset( m_bAllowedClasses, 0, sizeof( m_bAllowedClasses ) );
}

//------------------------------------------------------------------------------

void CTFBotRoster::InputSetAllowScout( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_SCOUT] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowSniper( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_SNIPER] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowSoldier( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_SOLDIER] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowDemoman( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_DEMOMAN] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowMedic( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_MEDIC] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowHeavy( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_HEAVYWEAPONS] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowPyro( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_PYRO] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowSpy( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_SPY] = inputdata.value.Bool();
}

void CTFBotRoster::InputSetAllowEngineer( inputdata_t &inputdata )
{
	m_bAllowedClasses[TF_CLASS_ENGINEER] = inputdata.value.Bool();
}

//------------------------------------------------------------------------------

bool CTFBotRoster::IsClassAllowed( int iBotClass ) const
{
	return iBotClass > TF_CLASS_UNDEFINED && iBotClass < TF_LAST_NORMAL_CLASS && m_bAllowedClasses[iBotClass];
}

//------------------------------------------------------------------------------

bool CTFBotRoster::IsClassChangeAllowed() const
{
	return m_bAllowClassChanges;
}

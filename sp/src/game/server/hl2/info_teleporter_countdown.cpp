//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//-----------------------------------------------------------------------------
// Amount of time before breen teleports away
//-----------------------------------------------------------------------------
class CInfoTeleporterCountdown : public CPointEntity
{
	DECLARE_CLASS( CInfoTeleporterCountdown, CPointEntity );
	DECLARE_SERVERCLASS();
 	DECLARE_DATADESC();

public:
	virtual int UpdateTransmitState();

private:
	void InputDisable(inputdata_t &inputdata);
	void InputEnable(inputdata_t &inputdata);
	void InputStartCountdown(inputdata_t &inputdata);
	void InputStopCountdown(inputdata_t &inputdata);

	CNetworkVar( bool, m_bCountdownStarted );
	CNetworkVar( bool, m_bDisabled );
	CNetworkVar( float, m_flStartTime );
	CNetworkVar( float, m_flTimeRemaining );
};


//-----------------------------------------------------------------------------
// Save/load
//-----------------------------------------------------------------------------
BEGIN_DATADESC( CInfoTeleporterCountdown )

	DEFINE_FIELD( m_bCountdownStarted,	FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDisabled,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flStartTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flTimeRemaining,	FIELD_FLOAT ),

	// Outputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "StartCountdown", InputStartCountdown ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopCountdown", InputStopCountdown ),

END_DATADESC()


LINK_ENTITY_TO_CLASS( info_teleporter_countdown, CInfoTeleporterCountdown );


//-----------------------------------------------------------------------------
// Networking
//-----------------------------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST( CInfoTeleporterCountdown, DT_InfoTeleporterCountdown )
	SendPropInt( SENDINFO( m_bCountdownStarted ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bDisabled ), 1, SPROP_UNSIGNED ),
	SendPropTime( SENDINFO( m_flStartTime ) ),
	SendPropFloat( SENDINFO( m_flTimeRemaining ), 0, SPROP_NOSCALE ),	
END_SEND_TABLE()


//-----------------------------------------------------------------------------
// Starts/stops countdown
//-----------------------------------------------------------------------------
void CInfoTeleporterCountdown::InputStartCountdown(inputdata_t &inputdata)
{
	if (!m_bCountdownStarted)
	{
		m_bCountdownStarted = true;
		m_bDisabled = false;
		m_flStartTime = gpGlobals->curtime;
		m_flTimeRemaining = inputdata.value.Float();
	}
}

void CInfoTeleporterCountdown::InputStopCountdown(inputdata_t &inputdata)
{
	m_bCountdownStarted = false;
}


//-----------------------------------------------------------------------------
// Disables/reenables an active countdown
//-----------------------------------------------------------------------------
void CInfoTeleporterCountdown::InputDisable(inputdata_t &inputdata)
{
	if ( !m_bDisabled )
	{
		m_bDisabled = true;
		if ( m_bCountdownStarted )
		{
			m_flTimeRemaining -= gpGlobals->curtime - m_flStartTime;
		}
	}
}

void CInfoTeleporterCountdown::InputEnable(inputdata_t &inputdata)
{
	if ( m_bDisabled )
	{
		m_bDisabled = false;
		if ( m_bCountdownStarted )
		{
			m_flStartTime = gpGlobals->curtime;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: Always send the teleporter countdown
//-----------------------------------------------------------------------------
int CInfoTeleporterCountdown::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

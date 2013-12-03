//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseparticleentity.h"
#include "sendproxy.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CEnvStarfield : public CBaseEntity
{
	DECLARE_CLASS( CEnvStarfield, CBaseEntity );
public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Precache();
	virtual void Spawn( void );
	virtual int  UpdateTransmitState(void);

	// Inputs
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputSetDensity( inputdata_t &inputdata );

private:
	CNetworkVar( bool, m_bOn );
	CNetworkVar( float, m_flDensity );
};

BEGIN_DATADESC( CEnvStarfield )
	DEFINE_FIELD( m_bOn, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDensity, FIELD_FLOAT ),

	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOn", InputTurnOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOff", InputTurnOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetDensity", InputSetDensity ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CEnvStarfield, DT_EnvStarfield )
	SendPropInt( SENDINFO(m_bOn), 1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_flDensity), 0, SPROP_NOSCALE),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( env_starfield, CEnvStarfield );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvStarfield::Spawn()
{
	BaseClass::Spawn();

	m_flDensity = 1.0;
	m_bOn = false;

	Precache();
}

void CEnvStarfield::Precache()
{
	BaseClass::Precache();

	PrecacheMaterial( "effects/spark_noz" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CEnvStarfield::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CEnvStarfield::InputTurnOn( inputdata_t &inputdata )
{
	m_bOn = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvStarfield::InputTurnOff( inputdata_t &inputdata )
{
	m_bOn = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CEnvStarfield::InputSetDensity( inputdata_t &inputdata )
{
	m_flDensity = inputdata.value.Float();
}
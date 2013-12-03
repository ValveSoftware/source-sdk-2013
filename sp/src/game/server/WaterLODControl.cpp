//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Shadow control entity.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Water LOD control entity
//------------------------------------------------------------------------------
class CWaterLODControl : public CBaseEntity
{
public:
	DECLARE_CLASS( CWaterLODControl, CBaseEntity );

	CWaterLODControl();

	void Spawn( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	int  UpdateTransmitState();
	void SetCheapWaterStartDistance( inputdata_t &inputdata );
	void SetCheapWaterEndDistance( inputdata_t &inputdata );

	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	CNetworkVar( float, m_flCheapWaterStartDistance );
	CNetworkVar( float, m_flCheapWaterEndDistance );
};

LINK_ENTITY_TO_CLASS(water_lod_control, CWaterLODControl);

BEGIN_DATADESC( CWaterLODControl )

	DEFINE_KEYFIELD( m_flCheapWaterStartDistance, FIELD_FLOAT, "cheapwaterstartdistance" ),
	DEFINE_KEYFIELD( m_flCheapWaterEndDistance, FIELD_FLOAT, "cheapwaterenddistance" ),

	// Inputs
	DEFINE_INPUT( m_flCheapWaterStartDistance,	FIELD_FLOAT, "SetCheapWaterStartDistance" ),
	DEFINE_INPUT( m_flCheapWaterEndDistance,	FIELD_FLOAT, "SetCheapWaterEndDistance" ),

END_DATADESC()


IMPLEMENT_SERVERCLASS_ST_NOBASE(CWaterLODControl, DT_WaterLODControl)
	SendPropFloat(SENDINFO(m_flCheapWaterStartDistance), 0, SPROP_NOSCALE ),
	SendPropFloat(SENDINFO(m_flCheapWaterEndDistance), 0, SPROP_NOSCALE ),
END_SEND_TABLE()


CWaterLODControl::CWaterLODControl()
{
	m_flCheapWaterStartDistance = 1000.0f;
	m_flCheapWaterEndDistance = 2000.0f;
}


//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model
//------------------------------------------------------------------------------
int CWaterLODControl::UpdateTransmitState()
{
	// ALWAYS transmit to all clients.
	return SetTransmitState( FL_EDICT_ALWAYS );
}


bool CWaterLODControl::KeyValue( const char *szKeyName, const char *szValue )
{
	if ( FStrEq( szKeyName, "cheapwaterstartdistance" ) )
	{
		m_flCheapWaterStartDistance = atof( szValue );
		return true;
	}

	if ( FStrEq( szKeyName, "cheapwaterenddistance" ) )
	{
		m_flCheapWaterEndDistance = atof( szValue );
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

//------------------------------------------------------------------------------
// Purpose :
//------------------------------------------------------------------------------
void CWaterLODControl::Spawn( void )
{
	Precache();
	SetSolid( SOLID_NONE );
}

//------------------------------------------------------------------------------
// Input values
//------------------------------------------------------------------------------
void CWaterLODControl::SetCheapWaterStartDistance( inputdata_t &inputdata )
{
	m_flCheapWaterStartDistance = atof( inputdata.value.String() );
}

void CWaterLODControl::SetCheapWaterEndDistance( inputdata_t &inputdata )
{
	m_flCheapWaterEndDistance = atof( inputdata.value.String() );
}

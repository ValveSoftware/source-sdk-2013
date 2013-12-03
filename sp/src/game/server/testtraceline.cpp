//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "cbase.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// -------------------------------------------------------------------------------- //
// An entity used to test traceline
// -------------------------------------------------------------------------------- //

class CTestTraceline : public CPointEntity
{
public:
	DECLARE_CLASS( CTestTraceline, CPointEntity );

	void	Spawn( void );
	int  	UpdateTransmitState();

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

private:
	void	Spin( void );
};
							  

// This table encodes the CBaseEntity data.
IMPLEMENT_SERVERCLASS_ST_NOBASE(CTestTraceline, DT_TestTraceline)
	SendPropInt		(SENDINFO(m_clrRender),	32, SPROP_UNSIGNED ),
	SendPropVector (SENDINFO(m_vecOrigin), 19, 0,	MIN_COORD_INTEGER, MAX_COORD_INTEGER),
	SendPropFloat	(SENDINFO_VECTORELEM(m_angRotation, 0), 19, 0,	MIN_COORD_INTEGER, MAX_COORD_INTEGER),
	SendPropFloat	(SENDINFO_VECTORELEM(m_angRotation, 1), 19, 0,	MIN_COORD_INTEGER, MAX_COORD_INTEGER),
	SendPropFloat	(SENDINFO_VECTORELEM(m_angRotation, 2), 19, 0,	MIN_COORD_INTEGER, MAX_COORD_INTEGER),
	SendPropEHandle (SENDINFO_NAME(m_hMoveParent, moveparent)),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( test_traceline, CTestTraceline );

BEGIN_DATADESC( CTestTraceline )

	// Function Pointers
	DEFINE_FUNCTION( Spin ),

END_DATADESC()


void	CTestTraceline::Spawn( void )
{
	SetRenderColor( 255, 255, 255, 255 );
	SetNextThink( gpGlobals->curtime );

	SetThink( &CTestTraceline::Spin );
}

void	CTestTraceline::Spin( void )
{
	static ConVar	traceline_spin( "traceline_spin","1" );

	if (traceline_spin.GetInt())
	{
		float s = sin( gpGlobals->curtime );
		QAngle angles = GetLocalAngles();

		angles[0] = 180.0 * 0.5 * (s * s * s + 1.0f) + 90;
		angles[1] = gpGlobals->curtime * 10;
		   
		SetLocalAngles( angles );

	}
	SetNextThink( gpGlobals->curtime );
}

int CTestTraceline::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

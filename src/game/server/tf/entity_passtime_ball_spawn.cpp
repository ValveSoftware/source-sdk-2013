//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "entity_passtime_ball_spawn.h"
#include "tf_passtime_ball.h"
#include "tf_passtime_logic.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
BEGIN_DATADESC( CPasstimeBallSpawn )
	DEFINE_KEYFIELD( m_bDisabled, FIELD_BOOLEAN, "StartDisabled" ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_OUTPUT( m_onSpawnBall, "OnSpawnBall" ),
END_DATADESC()

//-----------------------------------------------------------------------------
LINK_ENTITY_TO_CLASS( info_passtime_ball_spawn, CPasstimeBallSpawn );

//-----------------------------------------------------------------------------
IMPLEMENT_AUTO_LIST( IPasstimeBallSpawnAutoList );

//-----------------------------------------------------------------------------
CPasstimeBallSpawn::CPasstimeBallSpawn() : m_bDisabled( false ) {}
bool CPasstimeBallSpawn::IsEnabled() const { return !m_bDisabled; }
void CPasstimeBallSpawn::InputEnable( inputdata_t& input ) { m_bDisabled = false; }
void CPasstimeBallSpawn::InputDisable( inputdata_t& input ) {	m_bDisabled = true; }

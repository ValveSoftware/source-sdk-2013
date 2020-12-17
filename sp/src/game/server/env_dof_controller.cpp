//====== Copyright © 1996-2004, Valve Corporation, All rights reserved. =======
//
// Purpose: Depth of field controller entity
//
//=============================================================================

#include "cbase.h"
#include "baseentity.h"
#include "entityoutput.h"
#include "env_dof_controller.h"
#include "ai_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

LINK_ENTITY_TO_CLASS( env_dof_controller, CEnvDOFController );

BEGIN_DATADESC( CEnvDOFController )
	
	DEFINE_KEYFIELD( m_bDOFEnabled,			FIELD_BOOLEAN,	"enabled" ),
	DEFINE_KEYFIELD( m_flNearBlurDepth, 	FIELD_FLOAT,	"near_blur" ),
	DEFINE_KEYFIELD( m_flNearFocusDepth,	FIELD_FLOAT,	"near_focus" ),
	DEFINE_KEYFIELD( m_flFarFocusDepth, 	FIELD_FLOAT,	"far_focus" ),
	DEFINE_KEYFIELD( m_flFarBlurDepth,		FIELD_FLOAT,	"far_blur" ),
	DEFINE_KEYFIELD( m_flNearBlurRadius,	FIELD_FLOAT,	"near_radius" ),
	DEFINE_KEYFIELD( m_flFarBlurRadius,		FIELD_FLOAT,	"far_radius" ),
	DEFINE_KEYFIELD( m_strFocusTargetName,	FIELD_STRING,	"focus_target" ),
	DEFINE_KEYFIELD( m_flFocusTargetRange,	FIELD_FLOAT,	"focus_range" ),
	
	DEFINE_FIELD( m_hFocusTarget,		FIELD_EHANDLE ),

	DEFINE_THINKFUNC( UpdateParamBlend ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetNearBlurDepth",		InputSetNearBlurDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetNearFocusDepth",	InputSetNearFocusDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetFarFocusDepth",		InputSetFarFocusDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetFarBlurDepth",		InputSetFarBlurDepth ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetNearBlurRadius",	InputSetNearBlurRadius ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetFarBlurRadius",		InputSetFarBlurRadius ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetFocusTarget",		InputSetFocusTarget ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetFocusTargetRange",	InputSetFocusTargetRange ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CEnvDOFController, DT_EnvDOFController )
	SendPropInt( SENDINFO(m_bDOFEnabled), 1, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO(m_flNearBlurDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flNearFocusDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flFarFocusDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flFarBlurDepth), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flNearBlurRadius), 0, SPROP_NOSCALE),
	SendPropFloat( SENDINFO(m_flFarBlurRadius), 0, SPROP_NOSCALE),
END_SEND_TABLE()

void CEnvDOFController::Spawn()
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );

#ifdef MAPBASE
	// Find our target entity and hold on to it
	m_hFocusTarget = gEntList.FindEntityByName( NULL, m_strFocusTargetName, this );

	// Update if we have a focal target
	if ( m_hFocusTarget )
	{
		SetThink( &CEnvDOFController::UpdateParamBlend );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
#endif
}

void CEnvDOFController::Activate()
{
	BaseClass::Activate();

#ifndef MAPBASE // Mapbase moves this to Spawn() to avoid issues with save/restore and entities set via the SetFocusTarget input
	// Find our target entity and hold on to it
	m_hFocusTarget = gEntList.FindEntityByName( NULL, m_strFocusTargetName );

	// Update if we have a focal target
	if ( m_hFocusTarget )
	{
		SetThink( &CEnvDOFController::UpdateParamBlend );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
#endif
}

int CEnvDOFController::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CEnvDOFController::InputSetNearBlurDepth( inputdata_t &inputdata )
{
	m_flNearBlurDepth = inputdata.value.Float();
}

void CEnvDOFController::InputSetNearFocusDepth( inputdata_t &inputdata )
{
	m_flNearFocusDepth = inputdata.value.Float();
}

void CEnvDOFController::InputSetFarFocusDepth( inputdata_t &inputdata )
{
	m_flFarFocusDepth = inputdata.value.Float();
}

void CEnvDOFController::InputSetFarBlurDepth( inputdata_t &inputdata )
{
	m_flFarBlurDepth = inputdata.value.Float();
}

void CEnvDOFController::InputSetNearBlurRadius( inputdata_t &inputdata )
{
	m_flNearBlurRadius = inputdata.value.Float();
	m_bDOFEnabled = ( m_flNearBlurRadius > 0.0f ) || ( m_flFarBlurRadius > 0.0f );
}

void CEnvDOFController::InputSetFarBlurRadius( inputdata_t &inputdata )
{
	m_flFarBlurRadius = inputdata.value.Float();
	m_bDOFEnabled = ( m_flNearBlurRadius > 0.0f ) || ( m_flFarBlurRadius > 0.0f );
}

void CEnvDOFController::SetControllerState( DOFControlSettings_t setting )
{
	m_flNearBlurDepth = setting.flNearBlurDepth;
	m_flNearBlurRadius = setting.flNearBlurRadius;
	m_flNearFocusDepth = setting.flNearFocusDistance;
	
	m_flFarBlurDepth = setting.flFarBlurDepth;
	m_flFarBlurRadius = setting.flFarBlurRadius;
	m_flFarFocusDepth = setting.flFarFocusDistance;

	m_bDOFEnabled = ( m_flNearBlurRadius > 0.0f ) || ( m_flFarBlurRadius > 0.0f );
}

#define BLUR_DEPTH 500.0f

//-----------------------------------------------------------------------------
// Purpose: Blend the parameters to the specified value
//-----------------------------------------------------------------------------
void CEnvDOFController::UpdateParamBlend()
{ 
	// Update our focal target if we have one
	if ( m_hFocusTarget )
	{
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		float flDistToFocus = ( m_hFocusTarget->GetAbsOrigin() - pPlayer->GetAbsOrigin() ).Length();
		m_flFarFocusDepth.GetForModify() = flDistToFocus + m_flFocusTargetRange;
		m_flFarBlurDepth.GetForModify() = m_flFarFocusDepth + BLUR_DEPTH;

		SetThink( &CEnvDOFController::UpdateParamBlend );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the "focus" target entity
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetFocusTarget( inputdata_t &inputdata )
{
#ifdef MAPBASE
	m_hFocusTarget = gEntList.FindEntityByName( NULL, inputdata.value.String(), this, inputdata.pActivator, inputdata.pCaller );
#else
	m_hFocusTarget = gEntList.FindEntityByName( NULL, inputdata.value.String() );
#endif
	
	// Update if we have a focal target
	if ( m_hFocusTarget )
	{
		SetThink( &CEnvDOFController::UpdateParamBlend );
		SetNextThink( gpGlobals->curtime + 0.1f );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Set the range behind the focus entity that we'll blur (in units)
//-----------------------------------------------------------------------------
void CEnvDOFController::InputSetFocusTargetRange( inputdata_t &inputdata )
{
	m_flFocusTargetRange = inputdata.value.Float();
}

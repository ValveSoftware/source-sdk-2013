//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//===========================================================================//

#include "cbase.h"
#include "eventqueue.h"
#include "script_intro.h"
#include "point_camera.h"
#include "ai_utils.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// Used by server and client to calculate our FOV blend at any given frame
extern float ScriptInfo_CalculateFOV( float flFOVBlendStartTime, float flNextFOVBlendTime, int nFOV, int nNextFOV, bool bSplineRamp );

// Global point to the active intro script
CHandle<CScriptIntro> g_hIntroScript;

LINK_ENTITY_TO_CLASS(script_intro, CScriptIntro);

BEGIN_DATADESC(CScriptIntro)
	// Keys
	DEFINE_FIELD( m_vecCameraView, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecCameraViewAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecPlayerView, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecPlayerViewAngles, FIELD_VECTOR ),
	DEFINE_FIELD( m_iBlendMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_iQueuedBlendMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_iQueuedNextBlendMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_iNextBlendMode, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextBlendTime, FIELD_TIME ),
	DEFINE_FIELD( m_flBlendStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_bActive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_iNextFOV, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextFOVBlendTime, FIELD_TIME ),
	DEFINE_FIELD( m_flFOVBlendStartTime, FIELD_TIME ),
	DEFINE_FIELD( m_iFOV, FIELD_INTEGER ),
	DEFINE_ARRAY( m_flFadeColor, FIELD_FLOAT, 3 ),
	DEFINE_FIELD( m_flFadeAlpha, FIELD_FLOAT ),
	DEFINE_FIELD( m_flFadeDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_hCameraEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_iStartFOV, FIELD_INTEGER ),

	DEFINE_KEYFIELD( m_bAlternateFOV, FIELD_BOOLEAN, "alternatefovchange" ),

	// Inputs
	DEFINE_INPUTFUNC(FIELD_STRING, "SetCameraViewEntity", InputSetCameraViewEntity ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetBlendMode", InputSetBlendMode ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetNextFOV", InputSetNextFOV ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetFOVBlendTime", InputSetFOVBlendTime ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetFOV", InputSetFOV ),
	DEFINE_INPUTFUNC(FIELD_INTEGER, "SetNextBlendMode", InputSetNextBlendMode ),
	DEFINE_INPUTFUNC(FIELD_FLOAT, "SetNextBlendTime", InputSetNextBlendTime ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC(FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC(FIELD_STRING, "FadeTo", InputFadeTo ),
	DEFINE_INPUTFUNC(FIELD_STRING, "SetFadeColor", InputSetFadeColor ),

	DEFINE_THINKFUNC( BlendComplete ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CScriptIntro, DT_ScriptIntro )
	SendPropVector(SENDINFO(m_vecCameraView), -1, SPROP_COORD),
	SendPropVector(SENDINFO(m_vecCameraViewAngles), -1, SPROP_COORD),
	SendPropInt( SENDINFO( m_iBlendMode ), 5 ),
	SendPropInt( SENDINFO( m_iNextBlendMode ), 5 ),
	SendPropFloat( SENDINFO( m_flNextBlendTime ), 10 ),
	SendPropFloat( SENDINFO( m_flBlendStartTime ), 10 ),
	SendPropBool( SENDINFO( m_bActive ) ),


	// Fov & fov blends
	SendPropInt( SENDINFO( m_iFOV ), 9 ),
	SendPropInt( SENDINFO( m_iNextFOV ), 9 ),
	SendPropInt( SENDINFO( m_iStartFOV ), 9 ),
	SendPropFloat( SENDINFO( m_flNextFOVBlendTime ), 10 ),
	SendPropFloat( SENDINFO( m_flFOVBlendStartTime ), 10 ),

	SendPropBool( SENDINFO( m_bAlternateFOV ) ),

	// Fades
	SendPropFloat( SENDINFO( m_flFadeAlpha ), 10 ),
	SendPropArray(
		SendPropFloat( SENDINFO_ARRAY(m_flFadeColor), 32, SPROP_NOSCALE),
		m_flFadeColor),
	SendPropFloat( SENDINFO( m_flFadeDuration ), 10, SPROP_ROUNDDOWN, 0.0f, 255.0 ),
	SendPropEHandle(SENDINFO( m_hCameraEntity ) ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::Spawn( void )
{
	m_iNextBlendMode = -1;
	m_iQueuedBlendMode = -1;
	m_iQueuedNextBlendMode = -1;
	AddSolidFlags( FSOLID_NOT_SOLID );
	SetSize( -Vector(5,5,5), Vector(5,5,5) );
	m_bActive = false;
	m_iNextFOV = 0;
	m_iFOV = 0;
	m_iStartFOV = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::Activate( void )
{
	// Restore our script pointer, this is necessary to trigger other internal logic to due with PVS checks
	if ( m_bActive )
	{
		g_hIntroScript = this;
	}

	BaseClass::Activate();
}

void CScriptIntro::Precache()
{
	PrecacheMaterial( "scripted/intro_screenspaceeffect" );
	BaseClass::Precache();
}

//------------------------------------------------------------------------------
// Purpose : Send even though we don't have a model.
//------------------------------------------------------------------------------
int CScriptIntro::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetCameraViewEntity( inputdata_t &inputdata )
{
	// Find the specified entity
	string_t iszEntityName = inputdata.value.StringID();
	if ( iszEntityName == NULL_STRING )
		return;

	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, iszEntityName, NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pEntity )
	{
		Warning("script_intro %s couldn't find SetCameraViewEntity named %s\n", STRING(GetEntityName()), STRING(iszEntityName) );
		return;
	}

	m_hCameraEntity = pEntity;
	m_vecCameraView = pEntity->GetAbsOrigin();
	m_vecCameraViewAngles = pEntity->GetAbsAngles();
}

//-----------------------------------------------------------------------------
// Purpose: Fill out the origin that should be included in the player's PVS
//-----------------------------------------------------------------------------
bool CScriptIntro::GetIncludedPVSOrigin( Vector *pOrigin, CBaseEntity **ppCamera )
{
	if ( m_bActive && m_hCameraEntity.Get() )
	{
		*ppCamera = m_hCameraEntity.Get();
		*pOrigin = m_hCameraEntity.Get()->GetAbsOrigin();
		return true;
	}

	return false;
}


//-----------------------------------------------------------------------------
// Used for debugging
//-----------------------------------------------------------------------------
static ConVar cl_spewscriptintro( "cl_spewscriptintro", "0" );


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetBlendMode( inputdata_t &inputdata )
{
	m_iBlendMode = m_iNextBlendMode = inputdata.value.Int();
	m_flBlendStartTime = m_flNextBlendTime = gpGlobals->curtime;
	m_iQueuedBlendMode = -1;
	SetContextThink( NULL, gpGlobals->curtime, "BlendComplete" );

	if ( cl_spewscriptintro.GetInt() )
	{
		DevMsg( 1, "%.2f INPUT: Blend mode set to %d\n", gpGlobals->curtime, m_iBlendMode.Get() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetNextBlendMode( inputdata_t &inputdata )
{
	m_iQueuedNextBlendMode = inputdata.value.Int();

	if ( cl_spewscriptintro.GetInt() )
	{
		DevMsg( 1, "%.2f INPUT: Next Blend mode set to %d\n", gpGlobals->curtime, m_iQueuedNextBlendMode );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetNextFOV( inputdata_t &inputdata )
{
	m_iNextFOV = inputdata.value.Int();
}

//------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetFOVBlendTime( inputdata_t &inputdata )
{
	// Cache our FOV starting point before we update our data here
	if ( m_flNextFOVBlendTime >= gpGlobals->curtime )
	{
		// We're already in a blend, so capture where we are at this point in time
		m_iStartFOV = ScriptInfo_CalculateFOV( m_flFOVBlendStartTime, m_flNextFOVBlendTime, m_iStartFOV, m_iNextFOV, m_bAlternateFOV );
	}
	else
	{
		// If we weren't blending, then we need to construct a proper starting point from scratch
		CBasePlayer *pPlayer = AI_GetSinglePlayer();
		if ( pPlayer )
		{
			m_iStartFOV = ( m_iFOV ) ? m_iFOV : pPlayer->GetFOV();
		}
		else
		{
			m_iStartFOV = m_iFOV;
		}
	}

	m_flNextFOVBlendTime = gpGlobals->curtime + inputdata.value.Float();
	m_flFOVBlendStartTime = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetFOV( inputdata_t &inputdata )
{
	m_iFOV = inputdata.value.Int();
	m_iStartFOV = m_iFOV;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetNextBlendTime( inputdata_t &inputdata )
{
	m_flNextBlendTime = gpGlobals->curtime + inputdata.value.Float();
	m_flBlendStartTime = gpGlobals->curtime;

	// This logic is used to support continued calls to SetNextBlendMode
	// without intervening calls to SetBlendMode
	if ( m_iQueuedBlendMode >= 0 )
	{
		m_iBlendMode = m_iQueuedBlendMode;
	}

	if ( m_iQueuedNextBlendMode < 0 )
	{
		Warning( "script_intro: Warning!! Set blend time without setting next blend mode!\n" );
		m_iQueuedNextBlendMode = m_iBlendMode;
	}

	m_iNextBlendMode = m_iQueuedNextBlendMode; 
	m_iQueuedNextBlendMode = -1;
	m_iQueuedBlendMode = m_iNextBlendMode;

	if ( cl_spewscriptintro.GetInt() )
	{
		DevMsg( 1, "%.2f BLEND STARTED: %d to %d, end at %.2f\n", gpGlobals->curtime, m_iBlendMode.Get(), m_iNextBlendMode.Get(), m_flNextBlendTime.Get() );
	}

	SetContextThink( &CScriptIntro::BlendComplete, m_flNextBlendTime, "BlendComplete" );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CScriptIntro::BlendComplete( )
{
	m_iBlendMode = m_iNextBlendMode;
	m_flBlendStartTime = m_flNextBlendTime = gpGlobals->curtime;
	m_iQueuedBlendMode = -1;
	SetContextThink( NULL, gpGlobals->curtime, "BlendComplete" );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputActivate( inputdata_t &inputdata )
{
	m_bActive = true;
	g_hIntroScript = this;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputDeactivate( inputdata_t &inputdata )
{
	m_bActive = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputFadeTo( inputdata_t &inputdata )
{
	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get the fade alpha
	char *pszParam = strtok(parseString," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received FadeTo input without an alpha. Syntax: <fade alpha> <fade duration>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flAlpha = atof( pszParam );

	// Get the fade duration
	pszParam = strtok(NULL," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received FadeTo input without a duration. Syntax: <fade alpha> <fade duration>\n", GetClassname(), GetDebugName() );
		return;
	}

	// Set the two variables
	m_flFadeAlpha = flAlpha;
	m_flFadeDuration = atof( pszParam );

	//Msg("%.2f INPUT FADE: Fade to %.2f. End at %.2f\n", gpGlobals->curtime, m_flFadeAlpha.Get(), gpGlobals->curtime + m_flFadeDuration.Get() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CScriptIntro::InputSetFadeColor( inputdata_t &inputdata )
{
	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get the fade colors
	char *pszParam = strtok(parseString," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received SetFadeColor input without correct parameters. Syntax: <Red> <Green> <Blue>>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flR = atof( pszParam );

	pszParam = strtok(NULL," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received SetFadeColor input without correct parameters. Syntax: <Red> <Green> <Blue>>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flG = atof( pszParam );

	pszParam = strtok(NULL," ");
	if ( !pszParam || !pszParam[0] )
	{
		Warning("%s (%s) received SetFadeColor input without correct parameters. Syntax: <Red> <Green> <Blue>>\n", GetClassname(), GetDebugName() );
		return;
	}
	float flB = atof( pszParam );

	// Use the colors
	m_flFadeColor.Set( 0, flR );
	m_flFadeColor.Set( 1, flG );
	m_flFadeColor.Set( 2, flB );
}

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "playerlocaldata.h"
#include "player.h"
#include "mathlib/mathlib.h"
#include "entitylist.h"
#include "SkyCamera.h"
#include "playernet_vars.h"
#include "fogcontroller.h"
#include "tier0/vprof.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=============================================================================

BEGIN_SEND_TABLE_NOBASE( CPlayerLocalData, DT_Local )

	SendPropArray3  (SENDINFO_ARRAY3(m_chAreaBits), SendPropInt(SENDINFO_ARRAY(m_chAreaBits), 8, SPROP_UNSIGNED)),
	SendPropArray3  (SENDINFO_ARRAY3(m_chAreaPortalBits), SendPropInt(SENDINFO_ARRAY(m_chAreaPortalBits), 8, SPROP_UNSIGNED)),
	
	SendPropInt		(SENDINFO(m_iHideHUD), HIDEHUD_BITCOUNT, SPROP_UNSIGNED),
	SendPropFloat	(SENDINFO(m_flFOVRate), 0, SPROP_NOSCALE ),
	SendPropInt		(SENDINFO(m_bDucked),	1, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_bDucking),	1, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_bInDuckJump),	1, SPROP_UNSIGNED ),
	SendPropFloat	(SENDINFO(m_flDucktime), 12, SPROP_ROUNDDOWN|SPROP_CHANGES_OFTEN, 0.0f, 2048.0f ),
	SendPropFloat	(SENDINFO(m_flDuckJumpTime), 12, SPROP_ROUNDDOWN, 0.0f, 2048.0f ),
	SendPropFloat	(SENDINFO(m_flJumpTime), 12, SPROP_ROUNDDOWN, 0.0f, 2048.0f ),
#if PREDICTION_ERROR_CHECK_LEVEL > 1 
	SendPropFloat	(SENDINFO(m_flFallVelocity), 32, SPROP_NOSCALE ),

	SendPropFloat		( SENDINFO_VECTORELEM(m_vecPunchAngle, 0), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecPunchAngle, 1), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecPunchAngle, 2), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),

	SendPropFloat		( SENDINFO_VECTORELEM(m_vecPunchAngleVel, 0), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecPunchAngleVel, 1), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),
	SendPropFloat		( SENDINFO_VECTORELEM(m_vecPunchAngleVel, 2), 32, SPROP_NOSCALE|SPROP_CHANGES_OFTEN ),

#else
	SendPropFloat	(SENDINFO(m_flFallVelocity), 17, SPROP_CHANGES_OFTEN, -4096.0f, 4096.0f ),
	SendPropVector	(SENDINFO(m_vecPunchAngle),      -1,  SPROP_COORD|SPROP_CHANGES_OFTEN),
	SendPropVector	(SENDINFO(m_vecPunchAngleVel),      -1,  SPROP_COORD),
#endif
	SendPropInt		(SENDINFO(m_bDrawViewmodel), 1, SPROP_UNSIGNED ),
	SendPropInt		(SENDINFO(m_bWearingSuit), 1, SPROP_UNSIGNED ),
	SendPropBool	(SENDINFO(m_bPoisoned)),

	SendPropFloat	(SENDINFO(m_flStepSize), 16, SPROP_ROUNDUP, 0.0f, 128.0f ),
	SendPropInt		(SENDINFO(m_bAllowAutoMovement),1, SPROP_UNSIGNED ),

	// 3d skybox data
	SendPropInt(SENDINFO_STRUCTELEM(m_skybox3d.scale), 12),
	SendPropVector	(SENDINFO_STRUCTELEM(m_skybox3d.origin),      -1,  SPROP_COORD),
	SendPropInt	(SENDINFO_STRUCTELEM(m_skybox3d.area),	8, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_STRUCTELEM( m_skybox3d.fog.enable ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_STRUCTELEM( m_skybox3d.fog.blend ), 1, SPROP_UNSIGNED ),
	SendPropVector( SENDINFO_STRUCTELEM(m_skybox3d.fog.dirPrimary), -1, SPROP_COORD),
	SendPropInt( SENDINFO_STRUCTELEM( m_skybox3d.fog.colorPrimary ), 32, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO_STRUCTELEM( m_skybox3d.fog.colorSecondary ), 32, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_skybox3d.fog.start ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_skybox3d.fog.end ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO_STRUCTELEM( m_skybox3d.fog.maxdensity ), 0, SPROP_NOSCALE ),

	SendPropEHandle( SENDINFO_STRUCTELEM( m_PlayerFog.m_hCtrl ) ),

	// audio data
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 0 ), -1, SPROP_COORD),
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 1 ), -1, SPROP_COORD),
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 2 ), -1, SPROP_COORD),
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 3 ), -1, SPROP_COORD),
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 4 ), -1, SPROP_COORD),
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 5 ), -1, SPROP_COORD),
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 6 ), -1, SPROP_COORD),
	SendPropVector( SENDINFO_STRUCTARRAYELEM( m_audio.localSound, 7 ), -1, SPROP_COORD),
	SendPropInt( SENDINFO_STRUCTELEM( m_audio.soundscapeIndex ), 17, 0 ),
	SendPropInt( SENDINFO_STRUCTELEM( m_audio.localBits ), NUM_AUDIO_LOCAL_SOUNDS, SPROP_UNSIGNED ),
	SendPropEHandle( SENDINFO_STRUCTELEM( m_audio.ent ) ),
END_SEND_TABLE()

BEGIN_SIMPLE_DATADESC( fogplayerparams_t )
	DEFINE_FIELD( m_hCtrl, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flTransitionTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_OldColor, FIELD_COLOR32 ),
	DEFINE_FIELD( m_flOldStart, FIELD_FLOAT ),
	DEFINE_FIELD( m_flOldEnd, FIELD_FLOAT ),
	DEFINE_FIELD( m_NewColor, FIELD_COLOR32 ),
	DEFINE_FIELD( m_flNewStart, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNewEnd, FIELD_FLOAT ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( fogparams_t )

	DEFINE_FIELD( enable, FIELD_BOOLEAN ),
	DEFINE_FIELD( blend, FIELD_BOOLEAN ),
	DEFINE_FIELD( dirPrimary, FIELD_VECTOR ),
	DEFINE_FIELD( colorPrimary, FIELD_COLOR32 ),
	DEFINE_FIELD( colorSecondary, FIELD_COLOR32 ),
	DEFINE_FIELD( start, FIELD_FLOAT ),
	DEFINE_FIELD( end, FIELD_FLOAT ),
	DEFINE_FIELD( farz, FIELD_FLOAT ),
	DEFINE_FIELD( maxdensity, FIELD_FLOAT ),
	DEFINE_FIELD( colorPrimaryLerpTo, FIELD_COLOR32 ),
	DEFINE_FIELD( colorSecondaryLerpTo, FIELD_COLOR32 ),
	DEFINE_FIELD( startLerpTo, FIELD_FLOAT ),
	DEFINE_FIELD( endLerpTo, FIELD_FLOAT ),
	DEFINE_FIELD( lerptime, FIELD_TIME ),
	DEFINE_FIELD( duration, FIELD_FLOAT ),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( sky3dparams_t )

	DEFINE_FIELD( scale, FIELD_INTEGER ),
	DEFINE_FIELD( origin, FIELD_VECTOR ),
	DEFINE_FIELD( area, FIELD_INTEGER ),
	DEFINE_EMBEDDED( fog ),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( audioparams_t )

	DEFINE_AUTO_ARRAY( localSound, FIELD_VECTOR ),
	DEFINE_FIELD( soundscapeIndex, FIELD_INTEGER ),
	DEFINE_FIELD( localBits, FIELD_INTEGER ),
	DEFINE_FIELD( ent, FIELD_EHANDLE ),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( CPlayerLocalData )
	DEFINE_AUTO_ARRAY( m_chAreaBits, FIELD_CHARACTER ),
	DEFINE_AUTO_ARRAY( m_chAreaPortalBits, FIELD_CHARACTER ),
	DEFINE_FIELD( m_iHideHUD, FIELD_INTEGER ),
	DEFINE_FIELD( m_flFOVRate, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecOverViewpoint, FIELD_VECTOR ),
	DEFINE_FIELD( m_bDucked, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bDucking, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bInDuckJump, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flDucktime, FIELD_TIME ),
	DEFINE_FIELD( m_flDuckJumpTime, FIELD_TIME ),
	DEFINE_FIELD( m_flJumpTime, FIELD_TIME ),
	DEFINE_FIELD( m_nStepside, FIELD_INTEGER ),
	DEFINE_FIELD( m_flFallVelocity, FIELD_FLOAT ),
	DEFINE_FIELD( m_nOldButtons, FIELD_INTEGER ),
	DEFINE_FIELD( m_vecPunchAngle, FIELD_VECTOR ),
	DEFINE_FIELD( m_vecPunchAngleVel, FIELD_VECTOR ),
	DEFINE_FIELD( m_bDrawViewmodel, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bWearingSuit, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPoisoned, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flStepSize, FIELD_FLOAT ),
	DEFINE_FIELD( m_bAllowAutoMovement, FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( m_skybox3d ),
	DEFINE_EMBEDDED( m_PlayerFog ),
	DEFINE_EMBEDDED( m_fog ),
	DEFINE_EMBEDDED( m_audio ),
	
	// "Why don't we save this field, grandpa?"
	//
	// "You see Billy, trigger_vphysics_motion uses vPhysics to touch the player,
	// so if the trigger is Disabled via an input while the player is inside it,
	// the trigger won't get its EndTouch until the player moves. Since touchlinks
	// aren't saved and restored, if the we save before EndTouch is called, it
	// will never be called after we load."
	//DEFINE_FIELD( m_bSlowMovement, FIELD_BOOLEAN ),
	
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: Constructor.
//-----------------------------------------------------------------------------
CPlayerLocalData::CPlayerLocalData()
{
#ifdef _DEBUG
	m_vecOverViewpoint.Init();
	m_vecPunchAngle.Init();
#endif
	m_audio.soundscapeIndex = 0;
	m_audio.localBits = 0;
	m_audio.ent.Set( NULL );
	m_pOldSkyCamera = NULL;
	m_bDrawViewmodel = true;
}


void CPlayerLocalData::UpdateAreaBits( CBasePlayer *pl, unsigned char chAreaPortalBits[MAX_AREA_PORTAL_STATE_BYTES] )
{
	Vector origin = pl->EyePosition();

	unsigned char tempBits[32];
	COMPILE_TIME_ASSERT( sizeof( tempBits ) >= sizeof( ((CPlayerLocalData*)0)->m_chAreaBits ) );

	int i;
	int area = engine->GetArea( origin );
	engine->GetAreaBits( area, tempBits, sizeof( tempBits ) );
	for ( i=0; i < m_chAreaBits.Count(); i++ )
	{
		if ( tempBits[i] != m_chAreaBits[ i ] )
		{
			m_chAreaBits.Set( i, tempBits[i] );
		}
	}

	for ( i=0; i < MAX_AREA_PORTAL_STATE_BYTES; i++ )
	{
		if ( chAreaPortalBits[i] != m_chAreaPortalBits[i] )
			m_chAreaPortalBits.Set( i, chAreaPortalBits[i] );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Fills in CClientData values for local player just before sending over wire
// Input  : player - 
//-----------------------------------------------------------------------------

void ClientData_Update( CBasePlayer *pl )
{
	// HACKHACK: for 3d skybox 
	// UNDONE: Support multiple sky cameras?
	CSkyCamera *pSkyCamera = GetCurrentSkyCamera();
	if ( pSkyCamera != pl->m_Local.m_pOldSkyCamera )
	{
		pl->m_Local.m_pOldSkyCamera = pSkyCamera;
		pl->m_Local.m_skybox3d.CopyFrom(pSkyCamera->m_skyboxData);
	}
	else if ( !pSkyCamera )
	{
		pl->m_Local.m_skybox3d.area = 255;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void UpdateAllClientData( void )
{
	VPROF( "UpdateAllClientData" );
	int i;
	CBasePlayer *pl;

	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		pl = ( CBasePlayer * )UTIL_PlayerByIndex( i );
		if ( !pl )
			continue;

		ClientData_Update( pl );
	}
}


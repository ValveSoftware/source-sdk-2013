//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "igamesystem.h"
#include "point_camera.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define CAM_THINK_INTERVAL 0.05

// Spawnflags
#define SF_CAMERA_START_OFF				0x01

// UNDONE: Share properly with the client code!!!
#define POINT_CAMERA_MSG_SETACTIVE		1

CEntityClassList<CPointCamera> g_PointCameraList;
template <> CPointCamera *CEntityClassList<CPointCamera>::m_pClassList = NULL;

CPointCamera* GetPointCameraList()
{
	return g_PointCameraList.m_pClassList;
}

// These are already built into CBaseEntity
//	DEFINE_KEYFIELD( m_iName, FIELD_STRING, "targetname" ),
//	DEFINE_KEYFIELD( m_iParent, FIELD_STRING, "parentname" ),
//	DEFINE_KEYFIELD( m_target, FIELD_STRING, "target" ),

LINK_ENTITY_TO_CLASS( point_camera, CPointCamera );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CPointCamera::~CPointCamera()
{
	g_PointCameraList.Remove( this );
}

CPointCamera::CPointCamera()
{
	// Set these to opposites so that it'll be sent the first time around.
	m_bActive = false;
	m_bIsOn = false;
	
	m_bFogEnable = false;
	m_bFogRadial = false;

	// By default, transmit to everyone
	m_bitsTransmitPlayers.SetAll();

	g_PointCameraList.Insert( this );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::Spawn( void )
{
	BaseClass::Spawn();

	if ( m_spawnflags & SF_CAMERA_START_OFF )
	{
		m_bIsOn = false;
	}
	else
	{
		m_bIsOn = true;
	}

	SetActive( m_bIsOn );
}

//-----------------------------------------------------------------------------
// Purpose: Transmit only to players who are in PVS of the camera and its link
//			See PointCameraSetupVisibility
//-----------------------------------------------------------------------------
int CPointCamera::ShouldTransmit( const CCheckTransmitInfo *pInfo )
{
	if ( m_bitsTransmitPlayers.IsBitSet( pInfo->m_pClientEnt->m_EdictIndex ) )
		return FL_EDICT_ALWAYS;

	return FL_EDICT_DONTSEND;
}

//-----------------------------------------------------------------------------
// Purpose: Override ShouldTransmit since we want to be sent even though we don't have a model, etc.
//			All that matters is if we are in the pvs.
//-----------------------------------------------------------------------------
int CPointCamera::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_FULLCHECK );
}

//-----------------------------------------------------------------------------
// Purpose: Toggle networking of the camera to the specified player
//-----------------------------------------------------------------------------
void CPointCamera::TransmitToPlayer( int nPlayerIndex, bool bTransmit )
{
	if ( bTransmit )
		m_bitsTransmitPlayers.Set( nPlayerIndex );
	else
		m_bitsTransmitPlayers.Clear( nPlayerIndex );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::SetActive( bool bActive )
{
	if ( m_bActive != bActive )
	{
		m_bActive = bActive;
		DispatchUpdateTransmitState();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputChangeFOV( inputdata_t &inputdata )
{
	// Parse the keyvalue data
	char parseString[255];

	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get FOV
	char *pszParam = strtok(parseString," ");
	if(pszParam)
	{
		m_TargetFOV = atof( pszParam );
	}
	else
	{
		// Assume no change
		m_TargetFOV = m_FOV;
	}

	// Get Time
	float flChangeTime;
	pszParam = strtok(NULL," ");
	if(pszParam)
	{
		flChangeTime = atof( pszParam );
	}
	else
	{
		// Assume 1 second.
		flChangeTime = 1.0;
	}

	m_DegreesPerSecond = ( m_TargetFOV - m_FOV ) / flChangeTime;

	SetThink( &CPointCamera::ChangeFOVThink );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::ChangeFOVThink( void )
{
	SetNextThink( gpGlobals->curtime + CAM_THINK_INTERVAL );

	float newFOV = m_FOV;

	newFOV += m_DegreesPerSecond * CAM_THINK_INTERVAL;

	if( m_DegreesPerSecond < 0 )
	{
		if( newFOV <= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}
	else
	{
		if( newFOV >= m_TargetFOV )
		{
			newFOV = m_TargetFOV;
			SetThink( NULL );
		}
	}

	m_FOV = newFOV;
}

//-----------------------------------------------------------------------------
// Purpose: Turn this camera on, and turn all other cameras off
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOnAndTurnOthersOff( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = NULL;
	while ((pEntity = gEntList.FindEntityByClassname( pEntity, "point_camera" )) != NULL)
	{
		CPointCamera *pCamera = (CPointCamera*)pEntity;
		pCamera->InputSetOff( inputdata );
	}

	// Now turn myself on
	InputSetOn( inputdata );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOn( inputdata_t &inputdata )
{
	m_bIsOn = true;
	SetActive( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointCamera::InputSetOff( inputdata_t &inputdata )
{
	m_bIsOn = false;
	SetActive( false );
}

BEGIN_DATADESC( CPointCamera )

	// Save/restore Keyvalue fields
	DEFINE_KEYFIELD( m_FOV,			FIELD_FLOAT, "FOV" ),
	DEFINE_KEYFIELD( m_Resolution,	FIELD_FLOAT, "resolution" ),
	DEFINE_KEYFIELD( m_bFogEnable,	FIELD_BOOLEAN, "fogEnable" ),
	DEFINE_KEYFIELD( m_FogColor,	FIELD_COLOR32,	"fogColor" ),
	DEFINE_KEYFIELD( m_flFogStart,	FIELD_FLOAT, "fogStart" ),
	DEFINE_KEYFIELD( m_flFogEnd,	FIELD_FLOAT, "fogEnd" ),
	DEFINE_KEYFIELD( m_flFogMaxDensity,	FIELD_FLOAT, "fogMaxDensity" ),
	DEFINE_KEYFIELD( m_bFogRadial, FIELD_BOOLEAN, "fogRadial" ),
	DEFINE_KEYFIELD( m_bUseScreenAspectRatio, FIELD_BOOLEAN, "UseScreenAspectRatio" ),
	DEFINE_FIELD( m_bActive,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bIsOn,			FIELD_BOOLEAN ),

	DEFINE_FIELD( m_TargetFOV,		FIELD_FLOAT ),
	DEFINE_FIELD( m_DegreesPerSecond, FIELD_FLOAT ),
	// This is re-set up in the constructor
	//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),

	DEFINE_FUNCTION( ChangeFOVThink ),

	// Input
	DEFINE_INPUTFUNC( FIELD_STRING, "ChangeFOV", InputChangeFOV ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOnAndTurnOthersOff", InputSetOnAndTurnOthersOff ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOn", InputSetOn ),
	DEFINE_INPUTFUNC( FIELD_VOID, "SetOff", InputSetOff ),

END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CPointCamera, DT_PointCamera )
	SendPropFloat( SENDINFO( m_FOV ), 0, SPROP_NOSCALE ),
	SendPropFloat( SENDINFO( m_Resolution ), 0, SPROP_NOSCALE ),
	SendPropInt( SENDINFO( m_bFogEnable ), 1, SPROP_UNSIGNED ),	
	SendPropInt( SENDINFO_STRUCTELEM( m_FogColor ), 32, SPROP_UNSIGNED ),
	SendPropFloat( SENDINFO( m_flFogStart ), 0, SPROP_NOSCALE ),	
	SendPropFloat( SENDINFO( m_flFogEnd ), 0, SPROP_NOSCALE ),	
	SendPropFloat( SENDINFO( m_flFogMaxDensity ), 0, SPROP_NOSCALE ),	
	SendPropInt( SENDINFO( m_bFogRadial ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bActive ), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO( m_bUseScreenAspectRatio ), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

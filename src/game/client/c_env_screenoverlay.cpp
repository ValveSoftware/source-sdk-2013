//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "shareddefs.h"
#include "materialsystem/imesh.h"
#include "materialsystem/imaterial.h"
#include "view.h"
#include "iviewrender.h"
#include "view_shared.h"
#include "texture_group_names.h"
#include "tier0/icommandline.h"
#include "KeyValues.h"
#include "ScreenSpaceEffects.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_EnvScreenOverlay : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvScreenOverlay, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	void	PreDataUpdate( DataUpdateType_t updateType );
	void	PostDataUpdate( DataUpdateType_t updateType );

	void	HandleOverlaySwitch( void );
	void	StartOverlays( void );
	void	StopOverlays( void );
	void	StartCurrentOverlay( void );
	void	ClientThink( void );

protected:
	char	m_iszOverlayNames[ MAX_SCREEN_OVERLAYS ][255];
	float	m_flOverlayTimes[ MAX_SCREEN_OVERLAYS ];
	float	m_flStartTime;
	int     m_iDesiredOverlay;
	bool	m_bIsActive;
	bool	m_bWasActive;
	int		m_iCachedDesiredOverlay;
	int		m_iCurrentOverlay;
	float	m_flCurrentOverlayTime;
};

IMPLEMENT_CLIENTCLASS_DT( C_EnvScreenOverlay, DT_EnvScreenOverlay, CEnvScreenOverlay )
	RecvPropArray( RecvPropString( RECVINFO( m_iszOverlayNames[0]) ), m_iszOverlayNames ),
	RecvPropArray( RecvPropFloat( RECVINFO( m_flOverlayTimes[0] ) ), m_flOverlayTimes ),
	RecvPropFloat( RECVINFO( m_flStartTime ) ),
	RecvPropInt( RECVINFO( m_iDesiredOverlay ) ),
	RecvPropBool( RECVINFO( m_bIsActive ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::PreDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PreDataUpdate( updateType );

	m_bWasActive = m_bIsActive;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::PostDataUpdate( DataUpdateType_t updateType )
{
	BaseClass::PostDataUpdate( updateType );

	// If we have a start time now, start the overlays going
	if ( m_bIsActive && m_flStartTime > 0 && view->GetScreenOverlayMaterial() == NULL )
	{
		StartOverlays();
	}
	
	if ( m_flStartTime == -1 )
	{
		 StopOverlays();
	}

	HandleOverlaySwitch();

	if ( updateType == DATA_UPDATE_CREATED &&
		CommandLine()->FindParm( "-makereslists" ) )
	{
		for ( int i = 0; i < MAX_SCREEN_OVERLAYS; ++i )
		{
			if ( m_iszOverlayNames[ i ][ 0 ] )
			{
				materials->FindMaterial( m_iszOverlayNames[ i ], TEXTURE_GROUP_CLIENT_EFFECTS, false );
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::StopOverlays( void )
{
	SetNextClientThink( CLIENT_THINK_NEVER );

	if ( m_bWasActive && !m_bIsActive )
	{
		view->SetScreenOverlayMaterial( NULL );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::StartOverlays( void )
{
	m_iCurrentOverlay = 0;
	m_flCurrentOverlayTime = 0;
	m_iCachedDesiredOverlay	= 0;
	SetNextClientThink( CLIENT_THINK_ALWAYS );

	StartCurrentOverlay();
	HandleOverlaySwitch();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::HandleOverlaySwitch( void )
{
	if( m_iCachedDesiredOverlay != m_iDesiredOverlay )
	{
		m_iCurrentOverlay = m_iDesiredOverlay;
		m_iCachedDesiredOverlay = m_iDesiredOverlay;
		StartCurrentOverlay();
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::StartCurrentOverlay( void )
{
	if ( m_iCurrentOverlay == MAX_SCREEN_OVERLAYS || !m_iszOverlayNames[m_iCurrentOverlay][0] )
	{
		// Hit the end of our overlays, so stop.
		m_flStartTime = 0;
		StopOverlays();
		return;
	}

	if ( m_flOverlayTimes[m_iCurrentOverlay] == -1 )
		 m_flCurrentOverlayTime = -1;
	else
		 m_flCurrentOverlayTime = gpGlobals->curtime + m_flOverlayTimes[m_iCurrentOverlay];

	// Bring up the current overlay
	IMaterial *pMaterial = materials->FindMaterial( m_iszOverlayNames[m_iCurrentOverlay], TEXTURE_GROUP_CLIENT_EFFECTS, false );
	if ( !IsErrorMaterial( pMaterial ) )
	{
		view->SetScreenOverlayMaterial( pMaterial );
	}
	else
	{
		Warning("env_screenoverlay couldn't find overlay %s.\n", m_iszOverlayNames[m_iCurrentOverlay] );
		StopOverlays();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_EnvScreenOverlay::ClientThink( void )
{
	// If the current overlay's run out, go to the next one
	if ( m_flCurrentOverlayTime != -1 && m_flCurrentOverlayTime < gpGlobals->curtime )
	{
		m_iCurrentOverlay++;
		StartCurrentOverlay();
	}
}

// Effect types
enum 
{
	SCREENEFFECT_EP2_ADVISOR_STUN,
	SCREENEFFECT_EP1_INTRO,
	SCREENEFFECT_EP2_GROGGY,
};

// ============================================================================
//  Screenspace effect
// ============================================================================

class C_EnvScreenEffect : public C_BaseEntity
{
	DECLARE_CLASS( C_EnvScreenEffect, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	virtual void ReceiveMessage( int classID, bf_read &msg );

private:
	float	m_flDuration;
	int		m_nType;
};

IMPLEMENT_CLIENTCLASS_DT( C_EnvScreenEffect, DT_EnvScreenEffect, CEnvScreenEffect )
	RecvPropFloat( RECVINFO( m_flDuration ) ),
	RecvPropInt( RECVINFO( m_nType ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : classID - 
//			&msg - 
//-----------------------------------------------------------------------------
void C_EnvScreenEffect::ReceiveMessage( int classID, bf_read &msg )
{
	// Make sure our IDs match
	if ( classID != GetClientClass()->m_ClassID )
	{
		// Message is for subclass
		BaseClass::ReceiveMessage( classID, msg );
		return;
	}

	int messageType = msg.ReadByte();
	switch( messageType )
	{
		// Effect turning on
		case 0: // FIXME: Declare
			{		
				// Create a keyvalue block to set these params
				KeyValues *pKeys = new KeyValues( "keys" );
				if ( pKeys == NULL )
					return;

				if ( m_nType == SCREENEFFECT_EP1_INTRO )
				{
					if( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 )
					{
						return;
					}

					// Set our keys
					pKeys->SetFloat( "duration", m_flDuration );
					pKeys->SetInt( "fadeout", 0 );

					g_pScreenSpaceEffects->SetScreenSpaceEffectParams( "episodic_intro", pKeys );
					g_pScreenSpaceEffects->EnableScreenSpaceEffect( "episodic_intro" );
				}
				else if ( m_nType == SCREENEFFECT_EP2_ADVISOR_STUN )
				{
					// Set our keys
					pKeys->SetFloat( "duration", m_flDuration );

					g_pScreenSpaceEffects->SetScreenSpaceEffectParams( "episodic_stun", pKeys );
					g_pScreenSpaceEffects->EnableScreenSpaceEffect( "episodic_stun" );
				}
				else if ( m_nType == SCREENEFFECT_EP2_GROGGY )
				{
					if( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 )
						return;

					// Set our keys
					pKeys->SetFloat( "duration", m_flDuration );
					pKeys->SetInt( "fadeout", 0 );

					g_pScreenSpaceEffects->SetScreenSpaceEffectParams( "ep2_groggy", pKeys );
					g_pScreenSpaceEffects->EnableScreenSpaceEffect( "ep2_groggy" );
				}
                
				pKeys->deleteThis();
			}
			break;

		// Effect turning off
		case 1:	// FIXME: Declare
			
			if ( m_nType == SCREENEFFECT_EP1_INTRO )
			{
				if( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 )
				{
					return;
				}
				// Create a keyvalue block to set these params
				KeyValues *pKeys = new KeyValues( "keys" );
				if ( pKeys == NULL )
					return;

				// Set our keys
				pKeys->SetFloat( "duration", m_flDuration );
				pKeys->SetInt( "fadeout", 1 );

				g_pScreenSpaceEffects->SetScreenSpaceEffectParams( "episodic_intro", pKeys );
			}
			else if ( m_nType == SCREENEFFECT_EP2_ADVISOR_STUN )
			{
				g_pScreenSpaceEffects->DisableScreenSpaceEffect( "episodic_stun" );
			}
			else if ( m_nType == SCREENEFFECT_EP2_GROGGY )
			{
				if( g_pMaterialSystemHardwareConfig->GetDXSupportLevel() < 80 )
				{
					return;
				}
				// Create a keyvalue block to set these params
				KeyValues *pKeys = new KeyValues( "keys" );
				if ( pKeys == NULL )
					return;

				// Set our keys
				pKeys->SetFloat( "duration", m_flDuration );
				pKeys->SetInt( "fadeout", 1 );

				g_pScreenSpaceEffects->SetScreenSpaceEffectParams( "ep2_groggy", pKeys );
			}

			break;
	}
}

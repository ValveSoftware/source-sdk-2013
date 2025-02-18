//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: A camera entity that's used by the -makedevshots system to take
//			dev screenshots everytime the map is checked into source control.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "tier0/icommandline.h"
#include "igamesystem.h"
#include "filesystem.h"
#include <KeyValues.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

int g_iDevShotCameraCount = 0;
#define DEVSHOT_INITIAL_WAIT		5			// Time after the level spawn before the first devshot camera takes it's shot
#define DEVSHOT_INTERVAL			5			// Time between each devshot camera taking it's shot

//-----------------------------------------------------------------------------
// Purpose: A camera entity that's used by the -makedevshots system to take
//			dev screenshots everytime the map is checked into source control.
//-----------------------------------------------------------------------------
class CPointDevShotCamera : public CBaseEntity
{
	DECLARE_CLASS( CPointDevShotCamera, CBaseEntity );
public:
	DECLARE_DATADESC();

	void Spawn( void );
	void DevShotThink_Setup( void );
	void DevShotThink_TakeShot( void );
	void DevShotThink_PostShot( void );

	// Always transmit to clients so they know where to move the view to
	virtual int UpdateTransmitState();

private:
	string_t	m_iszCameraName;
	int			m_iFOV;
};

BEGIN_DATADESC( CPointDevShotCamera )
	DEFINE_FUNCTION( DevShotThink_Setup ),
	DEFINE_FUNCTION( DevShotThink_TakeShot ),
	DEFINE_FUNCTION( DevShotThink_PostShot ),

	DEFINE_KEYFIELD( m_iszCameraName,	FIELD_STRING,	"cameraname" ),
	DEFINE_KEYFIELD( m_iFOV,	FIELD_INTEGER,	"FOV" ),
END_DATADESC()

LINK_ENTITY_TO_CLASS( point_devshot_camera, CPointDevShotCamera );

//-----------------------------------------------------------------------------
// Purpose: Convenience function so we don't have to make this check all over
//-----------------------------------------------------------------------------
static CBasePlayer * UTIL_GetLocalPlayerOrListenServerHost( void )
{
	if ( gpGlobals->maxClients > 1 )
	{
		if ( engine->IsDedicatedServer() )
		{
			return NULL;
		}

		return UTIL_GetListenServerHost();
	}

	return UTIL_GetLocalPlayer();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointDevShotCamera::Spawn( void )
{
	BaseClass::Spawn();

	// Remove this entity immediately if we're not making devshots
	if ( !CommandLine()->FindParm("-makedevshots") )
	{
		UTIL_Remove( this );
		return;
	}

	// Take a screenshot when it's my turn
	SetThink( &CPointDevShotCamera::DevShotThink_Setup );
	SetNextThink( gpGlobals->curtime + DEVSHOT_INITIAL_WAIT + (g_iDevShotCameraCount * DEVSHOT_INTERVAL) );

	g_iDevShotCameraCount++;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CPointDevShotCamera::UpdateTransmitState()
{
	// always transmit if currently used by a monitor
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointDevShotCamera::DevShotThink_Setup( void )
{
	// Move the player to the devshot camera
	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	if ( !pPlayer )
		return;

	// Hide stuff
	engine->ClientCommand( pPlayer->edict(), "developer 0" );
	engine->ClientCommand( pPlayer->edict(), "cl_drawhud 0" );
	engine->ClientCommand( pPlayer->edict(), "sv_cheats 1" );
	engine->ClientCommand( pPlayer->edict(), "god" );
	engine->ClientCommand( pPlayer->edict(), "notarget" );

	pPlayer->AddSolidFlags( FSOLID_NOT_SOLID );
	pPlayer->EnableControl(FALSE);
	pPlayer->SetViewEntity( this );
	pPlayer->SetFOV( this, m_iFOV );

	// Hide the player's viewmodel
	if ( pPlayer->GetActiveWeapon() )
	{
		pPlayer->GetActiveWeapon()->AddEffects( EF_NODRAW );
	}

	DispatchUpdateTransmitState();

	// Now take the shot next frame
	SetThink( &CPointDevShotCamera::DevShotThink_TakeShot );
	SetNextThink( gpGlobals->curtime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointDevShotCamera::DevShotThink_TakeShot( void )
{
	// Take the screenshot
	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	if ( !pPlayer )
		return;

	engine->ClientCommand( pPlayer->edict(), "devshots_screenshot \"%s\"", STRING(m_iszCameraName) );

	// Now take the shot next frame
	SetThink( &CPointDevShotCamera::DevShotThink_PostShot );
	SetNextThink( gpGlobals->curtime + (DEVSHOT_INTERVAL - 1) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CPointDevShotCamera::DevShotThink_PostShot( void )
{
	// Take the screenshot
	CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
	if ( !pPlayer )
		return;

	pPlayer->SetFOV( this, 0 );

	// If all cameras have taken their shots, move to the next map
	g_iDevShotCameraCount--;
	if ( !g_iDevShotCameraCount )
	{
		engine->ClientCommand( pPlayer->edict(), "devshots_nextmap" );
	}
}


//-----------------------------------------------------------------------------
// Purpose: Game system to detect maps without cameras in them, and move on
//-----------------------------------------------------------------------------
class CDevShotSystem : public CAutoGameSystemPerFrame
{
public:

	CDevShotSystem( char const *name ) : CAutoGameSystemPerFrame( name )
	{
	}

	virtual void LevelInitPreEntity()
	{
		m_bIssuedNextMapCommand = false;
		g_iDevShotCameraCount = 0;
		m_bParsedMapFile = false;
	}

	virtual void SafeRemoveIfDesired( void )
	{
		// If we're not making devshots, remove this system immediately
		if ( !CommandLine()->FindParm("-makedevshots") )
		{
			Remove( this );
			return;
		}
	}

	virtual void FrameUpdatePostEntityThink( void )
	{
		// Wait until we're all spawned in
		if ( gpGlobals->curtime < 5 )
			return;

		if ( m_bIssuedNextMapCommand )
			return;

		if ( !m_bParsedMapFile )
		{
			m_bParsedMapFile = true;

			// See if we've got a camera file to import cameras from
			char szFullName[512];
			Q_snprintf(szFullName,sizeof(szFullName), "maps/%s.txt", STRING( gpGlobals->mapname ));
			KeyValues *pkvMapCameras = new KeyValues( "MapCameras" );
			if ( pkvMapCameras->LoadFromFile( filesystem, szFullName, "MOD" ) )
			{
				Warning( "Devshots: Loading point_devshot_camera positions from %s. \n", szFullName );

				// Get each camera, and add it to our list
				KeyValues *pkvCamera = pkvMapCameras->GetFirstSubKey();
				while ( pkvCamera )
				{
					// Get camera name
					const char *pCameraName = pkvCamera->GetName();

					// Make a camera, and move it to the position specified
					CPointDevShotCamera	*pCamera = (CPointDevShotCamera*)CreateEntityByName( "point_devshot_camera" );
					Assert( pCamera );
					pCamera->KeyValue( "cameraname", pCameraName );
					pCamera->KeyValue( "origin", pkvCamera->GetString( "origin", "0 0 0" ) );
					pCamera->KeyValue( "angles", pkvCamera->GetString( "angles", "0 0 0" ) );
					pCamera->KeyValue( "FOV", pkvCamera->GetString( "FOV", "75" ) );
					DispatchSpawn( pCamera );
					pCamera->Activate();

					// Move to next camera
					pkvCamera = pkvCamera->GetNextKey();
				}
			}

			if ( !g_iDevShotCameraCount )
			{
				Warning( "Devshots: No point_devshot_camera in %s. Moving to next map.\n", STRING( gpGlobals->mapname ) );

				CBasePlayer *pPlayer = UTIL_GetLocalPlayerOrListenServerHost();
				if ( pPlayer )
				{
					engine->ClientCommand( pPlayer->edict(), "devshots_nextmap" );
					m_bIssuedNextMapCommand = true;
					return;
				}
			}
		}
	}

private:
	bool	m_bIssuedNextMapCommand;
	bool	m_bParsedMapFile;
};

CDevShotSystem	DevShotSystem( "CDevShotSystem" );

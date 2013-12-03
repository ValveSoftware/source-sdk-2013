//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: MapOverview.cpp: implementation of the CMapOverview class.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "mapoverview.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include <filesystem.h>
#include <KeyValues.h>
#include <convar.h>
#include "mathlib/mathlib.h"
#include <game/client/iviewport.h>
#include <igameresources.h>
#include "gamevars_shared.h"
#include "spectatorgui.h"
#include "c_playerresource.h"
#include "view.h"

#include "clientmode.h"
#include <vgui_controls/AnimationController.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar overview_health( "overview_health", "1", FCVAR_ARCHIVE | FCVAR_CLIENTCMD_CAN_EXECUTE, "Show player's health in map overview.\n" );
ConVar overview_names ( "overview_names",  "1", FCVAR_ARCHIVE | FCVAR_CLIENTCMD_CAN_EXECUTE, "Show player's names in map overview.\n" );
ConVar overview_tracks( "overview_tracks", "1", FCVAR_ARCHIVE | FCVAR_CLIENTCMD_CAN_EXECUTE, "Show player's tracks in map overview.\n" );
ConVar overview_locked( "overview_locked", "1", FCVAR_ARCHIVE | FCVAR_CLIENTCMD_CAN_EXECUTE, "Locks map angle, doesn't follow view angle.\n" );
ConVar overview_alpha( "overview_alpha",  "1.0", FCVAR_ARCHIVE | FCVAR_CLIENTCMD_CAN_EXECUTE, "Overview map translucency.\n" );

IMapOverviewPanel *g_pMapOverview = NULL; // we assume only one overview is created

static int AdjustValue( int curValue, int targetValue, int amount )
{
	if ( curValue > targetValue )
	{
		curValue -= amount;

		if ( curValue < targetValue )
			curValue = targetValue;
	}
	else if ( curValue < targetValue )
	{
		curValue += amount;

		if ( curValue > targetValue )
			curValue = targetValue;
	}

	return curValue;
}

CON_COMMAND( overview_zoom, "Sets overview map zoom: <zoom> [<time>] [rel]" )
{
	if ( !g_pMapOverview || args.ArgC() < 2 )
		return;

	float zoom = Q_atof( args[ 1 ] );

	float time = 0;
	
	if ( args.ArgC() >= 3 )
		time = Q_atof( args[ 2 ] );

	if ( args.ArgC() == 4 )
		zoom *= g_pMapOverview->GetZoom();

	// We are going to store their zoom pick as the resultant overview size that it sees.  This way, the value will remain
	// correct even on a different map that has a different intrinsic zoom.
	float desiredViewSize = 0.0f;
	desiredViewSize = (zoom * OVERVIEW_MAP_SIZE * g_pMapOverview->GetFullZoom()) / g_pMapOverview->GetMapScale();
	g_pMapOverview->SetPlayerPreferredViewSize( desiredViewSize );

	if( !g_pMapOverview->AllowConCommandsWhileAlive() )
	{
		C_BasePlayer *localPlayer = CBasePlayer::GetLocalPlayer();
		if( localPlayer && CBasePlayer::GetLocalPlayer()->IsAlive() )
			return;// Not allowed to execute commands while alive
		else if( localPlayer && localPlayer->GetObserverMode() == OBS_MODE_DEATHCAM )
			return;// In the death cam spiral counts as alive
	}

	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( g_pMapOverview->GetAsPanel(), "zoom", zoom, 0.0, time, vgui::AnimationController::INTERPOLATOR_LINEAR );
}

CON_COMMAND( overview_mode, "Sets overview map mode off,small,large: <0|1|2>" )
{
	if ( !g_pMapOverview )
		return;

	int mode;

	if ( args.ArgC() < 2 )
	{
		// toggle modes
		mode = g_pMapOverview->GetMode() + 1;

		if ( mode >  CMapOverview::MAP_MODE_FULL )
			mode = CMapOverview::MAP_MODE_OFF;
	}
	else
	{
		// set specific mode
		mode = Q_atoi( args[ 1 ] );
	}

	if( mode != CMapOverview::MAP_MODE_RADAR )
		g_pMapOverview->SetPlayerPreferredMode( mode );

	if( !g_pMapOverview->AllowConCommandsWhileAlive() )
	{
		C_BasePlayer *localPlayer = CBasePlayer::GetLocalPlayer();
		if( localPlayer && CBasePlayer::GetLocalPlayer()->IsAlive() )
			return;// Not allowed to execute commands while alive
		else if( localPlayer && localPlayer->GetObserverMode() == OBS_MODE_DEATHCAM )
			return;// In the death cam spiral counts as alive
	}

	g_pMapOverview->SetMode( mode );
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


using namespace vgui;

CMapOverview::CMapOverview( const char *pElementName ) : BaseClass( NULL, pElementName ), CHudElement( pElementName )
{
	SetParent( g_pClientMode->GetViewport()->GetVPanel() );

	SetBounds( 0,0, 256, 256 );
	SetBgColor( Color( 0,0,0,100 ) );
	SetPaintBackgroundEnabled( true );
	ShowPanel( false );

	// Make sure we actually have the font...
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	
	m_hIconFont = pScheme->GetFont( "DefaultSmall" );
	
	m_nMapTextureID = -1;
	m_MapKeyValues = NULL;

	m_MapOrigin = Vector( 0, 0, 0 );
	m_fMapScale = 1.0f;
	m_bFollowAngle = false;
	SetMode( MAP_MODE_OFF );

	m_fZoom = 3.0f;
	m_MapCenter = Vector2D( 512, 512 );
	m_ViewOrigin = Vector2D( 512, 512 );
	m_fViewAngle = 0;
	m_fTrailUpdateInterval = 1.0f;

	m_bShowNames = true;
	m_bShowHealth = true;
	m_bShowTrails = true;

	m_flChangeSpeed = 1000;
	m_flIconSize = 64.0f;

	m_ObjectCounterID = 1;

	Reset();
	
	Q_memset( m_Players, 0, sizeof(m_Players) );

	InitTeamColorsAndIcons();

	g_pMapOverview = this;  // for cvars access etc
}

void CMapOverview::Init( void )
{
	// register for events as client listener
	ListenForGameEvent( "game_newmap" );
	ListenForGameEvent( "round_start" );
	ListenForGameEvent( "player_connect" );
	ListenForGameEvent( "player_info" );
	ListenForGameEvent( "player_team" );
	ListenForGameEvent( "player_spawn" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_disconnect" );
}

void CMapOverview::InitTeamColorsAndIcons()
{
	Q_memset( m_TeamIcons, 0, sizeof(m_TeamIcons) );
	Q_memset( m_TeamColors, 0, sizeof(m_TeamColors) );
	Q_memset( m_ObjectIcons, 0, sizeof(m_ObjectIcons) );

	m_TextureIDs.RemoveAll();
}

int CMapOverview::AddIconTexture(const char *filename)
{
	int index = m_TextureIDs.Find( filename );

    if ( m_TextureIDs.IsValidIndex( index ) )
	{
		// already known, return texture ID
		return m_TextureIDs.Element(index);
	}

	index = surface()->CreateNewTextureID();
	surface()->DrawSetTextureFile( index , filename, true, false);

	m_TextureIDs.Insert( filename, index );

	return index;
}

void CMapOverview::ApplySchemeSettings(vgui::IScheme *scheme)
{
	BaseClass::ApplySchemeSettings( scheme );

	SetBgColor( Color( 0,0,0,100 ) );
	SetPaintBackgroundEnabled( true );
}

CMapOverview::~CMapOverview()
{
	if ( m_MapKeyValues )
		m_MapKeyValues->deleteThis();

	g_pMapOverview = NULL;

	//TODO release Textures ? clear lists
}

void CMapOverview::UpdatePlayers()
{
	if ( !g_PR )
		return;

	// first disable all players health
	for ( int i=0; i<MAX_PLAYERS; i++ )
	{
		m_Players[i].health = 0;
		m_Players[i].team = TEAM_SPECTATOR;
	}

	for ( int i = 1; i<= gpGlobals->maxClients; i++)
	{
		// update from global player resources
		if ( g_PR && g_PR->IsConnected(i) )
		{
			MapPlayer_t *player = &m_Players[i-1];

			player->health = g_PR->GetHealth( i );

			if ( !g_PR->IsAlive( i ) )
			{
				player->health = 0;
			}

			if ( player->team != g_PR->GetTeam( i ) )
			{
				player->team = g_PR->GetTeam( i );
				player->icon = m_TeamIcons[ GetIconNumberFromTeamNumber(player->team)  ];
				player->color = m_TeamColors[ GetIconNumberFromTeamNumber(player->team) ];
			}
		}

		C_BasePlayer *pPlayer = UTIL_PlayerByIndex( i );

		if ( !pPlayer )
			continue;
		
		// don't update if player is dormant
		if ( pPlayer->IsDormant() )
			continue;

		// update position of active players in our PVS
		Vector position = pPlayer->EyePosition();
		QAngle angles = pPlayer->EyeAngles();

		SetPlayerPositions( i-1, position, angles );
	}
}

void CMapOverview::UpdatePlayerTrails()
{
	if ( m_fNextTrailUpdate > m_fWorldTime )
		return;

	m_fNextTrailUpdate = m_fWorldTime + 1.0f; // update once a second

	for (int i=0; i<MAX_PLAYERS; i++)
	{
		MapPlayer_t *p = &m_Players[i];
		
		// no trails for spectators or dead players
		if ( (p->team <= TEAM_SPECTATOR) || (p->health <= 0) )
		{
			continue;
		}

		// move old trail points 
		for ( int j=MAX_TRAIL_LENGTH-1; j>0; j--)
		{
			p->trail[j]=p->trail[j-1];
		}

		p->trail[0] = WorldToMap ( p->position );
	}
}

void CMapOverview::UpdateFollowEntity()
{
	if ( m_nFollowEntity != 0 )
	{
		C_BaseEntity *ent = ClientEntityList().GetEnt( m_nFollowEntity );

		if ( ent )
		{
			Vector position = MainViewOrigin();	// Use MainViewOrigin so SourceTV works in 3rd person
			QAngle angle = ent->EyeAngles();

			if ( m_nFollowEntity <= MAX_PLAYERS )
			{
				SetPlayerPositions( m_nFollowEntity-1, position, angle );
			}

			SetCenter( WorldToMap(position) );
			SetAngle( angle[YAW] );
		}
	}
	else
	{
		SetCenter( Vector2D(OVERVIEW_MAP_SIZE/2,OVERVIEW_MAP_SIZE/2) );
		SetAngle( 0 );
	}
}

void CMapOverview::Paint()
{
	UpdateSizeAndPosition();

	UpdateFollowEntity();

	UpdateObjects();

	UpdatePlayers();

	UpdatePlayerTrails();

	DrawMapTexture();

	DrawMapPlayerTrails();

	DrawObjects();

	DrawMapPlayers();

	DrawCamera();

	BaseClass::Paint();
}

bool CMapOverview::CanPlayerBeSeen(MapPlayer_t *player)
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !localPlayer || !player )
		return false;

	// don't draw ourself
	if ( localPlayer->GetUserID() == (player->userid) )
		return false;

	// Invalid guy.
	if( player->position == Vector(0,0,0) )
		return false; 

	// if local player is on spectator team, he can see everyone
	if ( localPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return true;

	// we never track unassigned or real spectators
	if ( player->team <= TEAM_SPECTATOR )
		return false;

	// if observer is an active player, check mp_forcecamera:

	if ( mp_forcecamera.GetInt() == OBS_ALLOW_NONE )
		return false;

	if ( mp_forcecamera.GetInt() == OBS_ALLOW_TEAM )
	{
		// true if both players are on the same team
		return (localPlayer->GetTeamNumber() == player->team );
	}

	// by default we can see all players
	return true;
}

/// allows mods to restrict health
/// Note: index is 0-based
bool CMapOverview::CanPlayerHealthBeSeen(MapPlayer_t *player)
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !localPlayer )
		return false;

	// real spectators can see everything
	if ( localPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return true;

	if ( mp_forcecamera.GetInt() != OBS_ALLOW_ALL )
	{
		// if forcecamera is on, only show health for teammates
		return ( localPlayer->GetTeamNumber() == player->team );
	}

	return true;
}

// usually name rule is same as health rule
bool CMapOverview::CanPlayerNameBeSeen(MapPlayer_t *player)
{
	return CanPlayerHealthBeSeen( player );
}

void CMapOverview::SetPlayerPositions(int index, const Vector &position, const QAngle &angle)
{
	MapPlayer_t *p = &m_Players[index];

	p->angle = angle;
	p->position = position;
}

//-----------------------------------------------------------------------------
// Purpose: shows/hides the buy menu
//-----------------------------------------------------------------------------
void CMapOverview::ShowPanel(bool bShow)
{
	SetVisible( bShow );
}

void CMapOverview::OnThink( void )
{
	if ( NeedsUpdate() )
	{
		Update();
		m_fNextUpdateTime = gpGlobals->curtime + 0.2f; // update 5 times a second
	}
}

bool CMapOverview::NeedsUpdate( void )
{
	return m_fNextUpdateTime < gpGlobals->curtime;
}

void CMapOverview::Update( void )
{
	// update settings
	m_bShowNames = overview_names.GetBool() && ( GetMode() != MAP_MODE_RADAR );
	m_bShowHealth = overview_health.GetBool() && ( GetMode() != MAP_MODE_RADAR );
	m_bFollowAngle = ( GetMode() != MAP_MODE_RADAR  && !overview_locked.GetBool() ) || ( GetMode() == MAP_MODE_RADAR  &&  !IsRadarLocked() );
	m_fTrailUpdateInterval = overview_tracks.GetInt() && ( GetMode() != MAP_MODE_RADAR );

	m_fWorldTime = gpGlobals->curtime;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !pPlayer )
		return;

	int specmode = GetSpectatorMode();

	if ( specmode == OBS_MODE_IN_EYE || specmode == OBS_MODE_CHASE )
	{
		// follow target
		SetFollowEntity( GetSpectatorTarget() );
	}
	else 
	{
		// follow ourself otherwise
		SetFollowEntity( pPlayer->entindex() );
	}
}

void CMapOverview::Reset( void )
{
	m_fNextUpdateTime = 0;
}

void CMapOverview::SetData(KeyValues *data)
{
	m_fZoom =  data->GetFloat( "zoom", m_fZoom );
	m_nFollowEntity =  data->GetInt( "entity", m_nFollowEntity );
}


CMapOverview::MapPlayer_t* CMapOverview::GetPlayerByUserID( int userID )
{
	for (int i=0; i<MAX_PLAYERS; i++)
	{
		MapPlayer_t *player = &m_Players[i];

		if ( player->userid == userID )
			return player;
	}

	return NULL;
}

bool CMapOverview::IsInPanel(Vector2D &pos)
{
	int x,y,w,t;

	GetBounds( x,y,w,t );

	return ( pos.x >= 0 && pos.x < w && pos.y >= 0 && pos.y < t );
}

void CMapOverview::DrawMapTexture()
{
	// now draw a box around the outside of this panel
	int x0, y0, x1, y1;
	int wide, tall;

	GetSize(wide, tall);
	x0 = 0; y0 = 0; x1 = wide - 2; y1 = tall - 2 ;

	if ( m_nMapTextureID < 0 )
		return;
	
	Vertex_t points[4] =
	{
		Vertex_t( MapToPanel ( Vector2D(0,0) ), Vector2D(0,0) ),
		Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,0) ), Vector2D(1,0) ),
		Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,OVERVIEW_MAP_SIZE-1) ), Vector2D(1,1) ),
		Vertex_t( MapToPanel ( Vector2D(0,OVERVIEW_MAP_SIZE-1) ), Vector2D(0,1) )
	};

	int alpha = 255.0f * overview_alpha.GetFloat(); clamp( alpha, 1, 255 );
	
	surface()->DrawSetColor( 255,255,255, alpha );
	surface()->DrawSetTexture( m_nMapTextureID );
	surface()->DrawTexturedPolygon( 4, points );
}

void CMapOverview::DrawMapPlayerTrails()
{
	if ( m_fTrailUpdateInterval <= 0 )
		return; // turned off

	for (int i=0; i<MAX_PLAYERS; i++)
	{
		MapPlayer_t *player = &m_Players[i];

		if ( !CanPlayerBeSeen(player) )
			continue;
		
		player->trail[0] = WorldToMap ( player->position );

		for ( int i=0; i<(MAX_TRAIL_LENGTH-1); i++)
		{
			if ( player->trail[i+1].x == 0 && player->trail[i+1].y == 0 )
				break;

			Vector2D pos1 = MapToPanel( player->trail[i] );
			Vector2D pos2 = MapToPanel( player->trail[i+1] );

			int intensity = 255 - float(255.0f * i) / MAX_TRAIL_LENGTH;

			Vector2D dist = pos1 - pos2;
			
			// don't draw too long lines, player probably teleported
			if ( dist.LengthSqr() < (128*128) )
			{	
				surface()->DrawSetColor( player->color[0], player->color[1], player->color[2], intensity );
				surface()->DrawLine( pos1.x, pos1.y, pos2.x, pos2.y );
			}
		}
	}
}

void CMapOverview::DrawObjects( )
{
	surface()->DrawSetTextFont( m_hIconFont );

	for (int i=0; i<m_Objects.Count(); i++)
	{
		MapObject_t *obj = &m_Objects[i];

		const char *text = NULL;

		if ( Q_strlen(obj->name) > 0 )
			text = obj->name;

		float flAngle = obj->angle[YAW];

		if ( obj->flags & MAP_OBJECT_ALIGN_TO_MAP && m_bRotateMap )
		{
			if ( m_bRotateMap )
                flAngle = 90;
			else
				flAngle = 0;
		}

		MapObject_t tempObj = *obj;
		tempObj.angle[YAW] = flAngle;
		tempObj.text = text;
		tempObj.statusColor = obj->color;

		// draw icon
		if ( !DrawIcon( &tempObj ) )
			continue;
	}
}

bool CMapOverview::DrawIcon( MapObject_t *obj )
{
	int textureID = obj->icon;
	Vector pos = obj->position;
	float scale = obj->size;
	float angle = obj->angle[YAW];
	const char *text = obj->text;
	Color *textColor = &obj->color;
	float status = obj->status;
	Color *statusColor = &obj->statusColor;

	Vector offset;	offset.z = 0;
	
	Vector2D pospanel = WorldToMap( pos );
	pospanel = MapToPanel( pospanel );

	if ( !IsInPanel( pospanel ) )
		return false; // player is not within overview panel

	offset.x = -scale;	offset.y = scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos1 = WorldToMap( pos + offset );

	offset.x = scale;	offset.y = scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos2 = WorldToMap( pos + offset );

	offset.x = scale;	offset.y = -scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos3 = WorldToMap( pos + offset );

	offset.x = -scale;	offset.y = -scale;
	VectorYawRotate( offset, angle, offset );
	Vector2D pos4 = WorldToMap( pos + offset );

	Vertex_t points[4] =
	{
		Vertex_t( MapToPanel ( pos1 ), Vector2D(0,0) ),
		Vertex_t( MapToPanel ( pos2 ), Vector2D(1,0) ),
		Vertex_t( MapToPanel ( pos3 ), Vector2D(1,1) ),
		Vertex_t( MapToPanel ( pos4 ), Vector2D(0,1) )
	};

	surface()->DrawSetColor( 255, 255, 255, 255 );
	surface()->DrawSetTexture( textureID );
	surface()->DrawTexturedPolygon( 4, points );

	int d = GetPixelOffset( scale);

	pospanel.y += d + 4;

	if ( status >=0.0f  && status <= 1.0f && statusColor )
	{
		// health bar is 50x3 pixels
		surface()->DrawSetColor( 0,0,0,255 );
		surface()->DrawFilledRect( pospanel.x-d, pospanel.y-1, pospanel.x+d, pospanel.y+1 );

		int length = (float)(d*2)*status;
		surface()->DrawSetColor( statusColor->r(), statusColor->g(), statusColor->b(), 255 );
		surface()->DrawFilledRect( pospanel.x-d, pospanel.y-1, pospanel.x-d+length, pospanel.y+1 );

		pospanel.y += 3;
	}

	if ( text && textColor )
	{
		wchar_t iconText[ MAX_PLAYER_NAME_LENGTH*2 ];

		g_pVGuiLocalize->ConvertANSIToUnicode( text, iconText, sizeof( iconText ) );

		int wide, tall;
		surface()->GetTextSize( m_hIconFont, iconText, wide, tall );

		int x = pospanel.x-(wide/2);
		int y = pospanel.y;

		// draw black shadow text
		surface()->DrawSetTextColor( 0, 0, 0, 255 );
		surface()->DrawSetTextPos( x+1, y );
		surface()->DrawPrintText( iconText, wcslen(iconText) );

		// draw name in color 
		surface()->DrawSetTextColor( textColor->r(), textColor->g(), textColor->b(), 255 );
		surface()->DrawSetTextPos( x, y );
		surface()->DrawPrintText( iconText, wcslen(iconText) );
	}

	return true;
}

int CMapOverview::GetPixelOffset( float height )
{
	Vector2D pos2 = WorldToMap( Vector( height,0,0) );
	pos2 = MapToPanel( pos2 );

	Vector2D pos3 = WorldToMap( Vector(0,0,0) );
	pos3 = MapToPanel( pos3 );

	int a = pos2.y-pos3.y; 
	int b = pos2.x-pos3.x;

	return (int)sqrt((float)(a*a+b*b)); // number of panel pixels for "scale" units in world
}

void CMapOverview::DrawMapPlayers()
{
	surface()->DrawSetTextFont( m_hIconFont );

	Color colorGreen( 0, 255, 0, 255 );	// health bar color
	
	for (int i=0; i<MAX_PLAYERS; i++)
	{
		MapPlayer_t *player = &m_Players[i];

		if ( !CanPlayerBeSeen( player ) )
			continue;

		// don't draw dead players / spectators
		if ( player->health <= 0 )
			continue;
		
		float status = -1;
		const char *name = NULL;

		if ( m_bShowNames && CanPlayerNameBeSeen( player ) )
			name = player->name;

		if ( m_bShowHealth && CanPlayerHealthBeSeen( player ) )
			status = player->health/100.0f;

		// convert from PlayerObject_t
		MapObject_t tempObj;
		memset( &tempObj, 0, sizeof(MapObject_t) );
		tempObj.icon = player->icon;
		tempObj.position = player->position;
		tempObj.size = m_flIconSize;
		tempObj.angle = player->angle;
		tempObj.text = name;
		tempObj.color = player->color;
		tempObj.status = status;
		tempObj.statusColor = colorGreen;

		DrawIcon( &tempObj );
	}
}

Vector2D CMapOverview::WorldToMap( const Vector &worldpos )
{
	Vector2D offset( worldpos.x - m_MapOrigin.x, worldpos.y - m_MapOrigin.y);

	offset.x /=  m_fMapScale;
	offset.y /= -m_fMapScale;

	return offset;
}

float CMapOverview::GetViewAngle( void )
{
	float viewAngle = m_fViewAngle - 90.0f;

	if ( !m_bFollowAngle )
	{
		// We don't use fViewAngle.  We just show straight at all times.
		if ( m_bRotateMap )
			viewAngle = 90.0f;
		else
			viewAngle = 0.0f;
	}

	return viewAngle;
}

Vector2D CMapOverview::MapToPanel( const Vector2D &mappos )
{
	int pwidth, pheight; 
	Vector2D panelpos;
	float viewAngle = GetViewAngle();

	GetSize(pwidth, pheight);

	Vector offset;
	offset.x = mappos.x - m_MapCenter.x;
	offset.y = mappos.y - m_MapCenter.y;
	offset.z = 0;

	VectorYawRotate( offset, viewAngle, offset );

	// find the actual zoom from the animationvar m_fZoom and the map zoom scale
	float fScale = (m_fZoom * m_fFullZoom) / OVERVIEW_MAP_SIZE;

	offset.x *= fScale;
	offset.y *= fScale;

	panelpos.x = (pwidth * 0.5f) + (pheight * offset.x);
	panelpos.y = (pheight * 0.5f) + (pheight * offset.y);

	return panelpos;
}

void CMapOverview::SetTime( float time )
{
	m_fWorldTime = time;
}

void CMapOverview::SetMap(const char * levelname)
{
	// Reset players and objects, even if the map is the same as the previous one
	m_Objects.RemoveAll();
	
	m_fNextTrailUpdate = 0;// Set to 0 for immediate update.  Our WorldTime var hasn't been updated to 0 for the new map yet
	m_fWorldTime = 0;// In release, we occasionally race and get this bug again if we gt a paint before an update.  Reset this before the old value gets in to the timer.
	// Please note, UpdatePlayerTrails comes from PAINT, not UPDATE.

	InitTeamColorsAndIcons();

	// load new KeyValues
	if ( m_MapKeyValues && Q_strcmp( levelname, m_MapKeyValues->GetName() ) == 0 )
	{
		return;	// map didn't change
	}

	if ( m_MapKeyValues )
		m_MapKeyValues->deleteThis();

	m_MapKeyValues = new KeyValues( levelname );

	char tempfile[MAX_PATH];
	Q_snprintf( tempfile, sizeof( tempfile ), "resource/overviews/%s.txt", levelname );
	
	if ( !m_MapKeyValues->LoadFromFile( g_pFullFileSystem, tempfile, "GAME" ) )
	{
		DevMsg( 1, "Error! CMapOverview::SetMap: couldn't load file %s.\n", tempfile );
		m_nMapTextureID = -1;
		m_MapOrigin.x = 0;
		m_MapOrigin.y = 0;
		m_fMapScale = 1;
		m_bRotateMap = false;
		m_fFullZoom = 1;
		return;
	}

	// TODO release old texture ?

	m_nMapTextureID = surface()->CreateNewTextureID();

	//if we have not uploaded yet, lets go ahead and do so
	surface()->DrawSetTextureFile( m_nMapTextureID, m_MapKeyValues->GetString("material"), true, false);

	int wide, tall;

	surface()->DrawGetTextureSize( m_nMapTextureID, wide, tall );

	if ( wide != tall )
	{
		DevMsg( 1, "Error! CMapOverview::SetMap: map image must be a square.\n" );
		m_nMapTextureID = -1;
		return;
	}

	m_MapOrigin.x	= m_MapKeyValues->GetInt("pos_x");
	m_MapOrigin.y	= m_MapKeyValues->GetInt("pos_y");
	m_fMapScale		= m_MapKeyValues->GetFloat("scale", 1.0f);
	m_bRotateMap	= m_MapKeyValues->GetInt("rotate")!=0;
	m_fFullZoom		= m_MapKeyValues->GetFloat("zoom", 1.0f );
}

void CMapOverview::ResetRound()
{
	for (int i=0; i<MAX_PLAYERS; i++)
	{
		MapPlayer_t *p = &m_Players[i];
		
		if ( p->team > TEAM_SPECTATOR )
		{
			p->health = 100;
		}

		Q_memset( p->trail, 0, sizeof(p->trail) );

		p->position = Vector( 0, 0, 0 );
	}

	m_Objects.RemoveAll();
}

void CMapOverview::OnMousePressed( MouseCode code )
{
	
}

void CMapOverview::DrawCamera()
{
	// draw a red center point
	surface()->DrawSetColor( 255,0,0,255 );
	Vector2D center = MapToPanel( m_ViewOrigin );
	surface()->DrawFilledRect( center.x-2, center.y-2, center.x+2, center.y+2);
}

void CMapOverview::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp(type, "game_newmap") == 0 )
	{
		SetMap( event->GetString("mapname") );
		ResetRound();
	}

	else if ( Q_strcmp(type, "round_start") == 0 )
	{
		ResetRound();
	}

	else if ( Q_strcmp(type,"player_connect") == 0 )
	{
		int index = event->GetInt("index"); // = entity index - 1 

		if ( index < 0 || index >= MAX_PLAYERS )
			return;

		MapPlayer_t *player = &m_Players[index];
		
		player->index = index;
		player->userid = event->GetInt("userid");
		Q_strncpy( player->name, event->GetString("name","unknown"), sizeof(player->name) );

		// Reset settings
		Q_memset( player->trail, 0, sizeof(player->trail) );
		player->team = TEAM_UNASSIGNED;
		player->health = 0;
	}

	else if ( Q_strcmp(type,"player_info") == 0 )
	{
		int index = event->GetInt("index"); // = entity index - 1

		if ( index < 0 || index >= MAX_PLAYERS )
			return;

		MapPlayer_t *player = &m_Players[index];
		
		player->index = index;
		player->userid = event->GetInt("userid");
		Q_strncpy( player->name, event->GetString("name","unknown"), sizeof(player->name) );
	}

	else if ( Q_strcmp(type,"player_team") == 0 )
	{
		MapPlayer_t *player = GetPlayerByUserID( event->GetInt("userid") );

		if ( !player )
			return;

		player->team = event->GetInt("team");
		player->icon = m_TeamIcons[ GetIconNumberFromTeamNumber(player->team)  ];
		player->color = m_TeamColors[ GetIconNumberFromTeamNumber(player->team) ];
	}

	else if ( Q_strcmp(type,"player_death") == 0 )
	{
		MapPlayer_t *player = GetPlayerByUserID( event->GetInt("userid") );

		if ( !player )
			return;

		player->health = 0;
		Q_memset( player->trail, 0, sizeof(player->trail) ); // clear trails
	}

	else if ( Q_strcmp(type,"player_spawn") == 0 )
	{
		MapPlayer_t *player = GetPlayerByUserID( event->GetInt("userid") );

		if ( !player )
			return;

		player->health = 100;
		Q_memset( player->trail, 0, sizeof(player->trail) ); // clear trails
	}

	else if ( Q_strcmp(type,"player_disconnect") == 0 )
	{
		MapPlayer_t *player = GetPlayerByUserID( event->GetInt("userid") );
		
		if ( !player )
			return;

		Q_memset( player, 0, sizeof(MapPlayer_t) ); // clear player field
	}
}

void CMapOverview::SetMode(int mode)
{
	m_flChangeSpeed = 0; // change size instantly

	if ( mode == MAP_MODE_OFF )
	{
		ShowPanel( false );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapOff" );
	}
	else if ( mode == MAP_MODE_INSET )
	{
		if( m_nMapTextureID == -1 )
		{
			SetMode( MAP_MODE_OFF );
			return;
		}

		if ( m_nMode != MAP_MODE_OFF )
			m_flChangeSpeed = 1000; // zoom effect

		C_BasePlayer *pPlayer = CBasePlayer::GetLocalPlayer();

		if ( pPlayer )
            SetFollowEntity( pPlayer->entindex() );

		ShowPanel( true );

		if ( mode != m_nMode && RunHudAnimations() )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomToSmall" );
		}
	}
	else if ( mode == MAP_MODE_FULL )
	{
		if( m_nMapTextureID == -1 )
		{
			SetMode( MAP_MODE_OFF );
			return;
		}

		if ( m_nMode != MAP_MODE_OFF )
			m_flChangeSpeed = 1000; // zoom effect

		SetFollowEntity( 0 );

		ShowPanel( true );

		if ( mode != m_nMode && RunHudAnimations() )
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomToLarge" );
		}
	}

	// finally set mode
	m_nMode = mode;

	UpdateSizeAndPosition();
}

bool CMapOverview::ShouldDraw( void )
{
	return ( m_nMode != MAP_MODE_OFF ) && CHudElement::ShouldDraw();
}

void CMapOverview::UpdateSizeAndPosition()
{
	if ( g_pSpectatorGUI && g_pSpectatorGUI->IsVisible() )
	{
		int iScreenWide, iScreenTall;
		GetHudSize( iScreenWide, iScreenTall );

		int iTopBarHeight = g_pSpectatorGUI->GetTopBarHeight();
		int iBottomBarHeight = g_pSpectatorGUI->GetBottomBarHeight();

		iScreenTall -= ( iTopBarHeight + iBottomBarHeight );

		int x,y,w,h;
		GetBounds( x,y,w,h );

		if ( y < iTopBarHeight )
			y = iTopBarHeight;

        SetBounds( x,y,w,MIN(h,iScreenTall) );
	}
}

void CMapOverview::SetCenter(const Vector2D &mappos)
{
	int width, height;

	GetSize( width, height);

	m_ViewOrigin = mappos;
	m_MapCenter = mappos;

	float fTwiceZoom = m_fZoom * m_fFullZoom * 2;

	width = height = OVERVIEW_MAP_SIZE / (fTwiceZoom);

	if( GetMode() != MAP_MODE_RADAR )
	{
		if ( m_MapCenter.x < width )
			m_MapCenter.x = width;

		if ( m_MapCenter.x > (OVERVIEW_MAP_SIZE-width) )
			m_MapCenter.x = (OVERVIEW_MAP_SIZE-width);

		if ( m_MapCenter.y < height )
			m_MapCenter.y = height;

		if ( m_MapCenter.y > (OVERVIEW_MAP_SIZE-height) )
			m_MapCenter.y = (OVERVIEW_MAP_SIZE-height);

		//center if in full map mode
		if ( m_fZoom <= 1.0 )
		{
			m_MapCenter.x = OVERVIEW_MAP_SIZE/2;
			m_MapCenter.y = OVERVIEW_MAP_SIZE/2;
		}
	}

}

void CMapOverview::SetFollowAngle(bool state)
{
	m_bFollowAngle = state;
}

void CMapOverview::SetFollowEntity(int entindex)
{
	m_nFollowEntity = entindex;
}

float CMapOverview::GetZoom( void )
{
	return m_fZoom;
}

int CMapOverview::GetMode( void )
{
	return m_nMode;
}

void CMapOverview::SetAngle(float angle)
{
	m_fViewAngle = angle;
}

void CMapOverview::ShowPlayerNames(bool state)
{
	m_bShowNames = state;
}


void CMapOverview::ShowPlayerHealth(bool state)
{
	m_bShowHealth = state;
}

void CMapOverview::ShowPlayerTracks(float seconds)
{
	m_fTrailUpdateInterval = seconds;
}

bool CMapOverview::SetTeamColor(int team, Color color)
{
	if ( team < 0 || team>= MAX_TEAMS )
		return false;

	m_TeamColors[team] = color;

	return true;
}

CMapOverview::MapObject_t* CMapOverview::FindObjectByID(int objectID)
{
	for ( int i = 0; i < m_Objects.Count(); i++ )
	{
		if ( m_Objects[i].objectID == objectID )
			return &m_Objects[i];
	}

	return NULL;
}

int	CMapOverview::AddObject( const char *icon, int entity, float timeToLive )
{
	MapObject_t obj; Q_memset( &obj, 0, sizeof(obj) );

	obj.objectID = m_ObjectCounterID++;
	obj.index = entity;
	obj.icon = AddIconTexture( icon );
	obj.size = m_flIconSize;
	obj.status = -1;
	
	if ( timeToLive > 0 )
		obj.endtime = gpGlobals->curtime + timeToLive;
	else
		obj.endtime = -1;

	m_Objects.AddToTail( obj );

	return obj.objectID;
}

void CMapOverview::SetObjectText( int objectID, const char *text, Color color )
{
	MapObject_t* obj = FindObjectByID( objectID );

	if ( !obj )
		return;

	if ( text )
	{
		Q_strncpy( obj->name, text, sizeof(obj->name) );
	}
	else
	{
		Q_memset( obj->name, 0, sizeof(obj->name) );
	}

	obj->color = color;
}

void CMapOverview::SetObjectStatus( int objectID, float status, Color color )
{
	MapObject_t* obj = FindObjectByID( objectID );

	if ( !obj )
		return;

	obj->status = status;
	obj->statusColor = color;
}

void CMapOverview::SetObjectIcon( int objectID, const char *icon, float size )
{
	MapObject_t* obj = FindObjectByID( objectID );

	if ( !obj )
		return;

	obj->icon = AddIconTexture( icon );
	obj->size = size;
}

void CMapOverview::SetObjectPosition( int objectID, const Vector &position, const QAngle &angle )
{
	MapObject_t* obj = FindObjectByID( objectID );

	if ( !obj )
		return;

	obj->angle = angle;
	obj->position = position;
}

void CMapOverview::AddObjectFlags( int objectID, int flags )
{
	MapObject_t* obj = FindObjectByID( objectID );

	if ( !obj )
		return;

	obj->flags |= flags;
}

void CMapOverview::SetObjectFlags( int objectID, int flags )
{
	MapObject_t* obj = FindObjectByID( objectID );

	if ( !obj )
		return;

	obj->flags = flags;
}

void CMapOverview::RemoveObjectByIndex( int index )
{
	for ( int i = 0; i < m_Objects.Count(); i++ )
	{
		if ( m_Objects[i].index == index )
		{
			m_Objects.Remove( i );
			return;
		}
	}
}

void CMapOverview::RemoveObject( int objectID )
{
	for ( int i = 0; i < m_Objects.Count(); i++ )
	{
		if ( m_Objects[i].objectID == objectID )
		{
			m_Objects.Remove( i );
			return;
		}
	}
}

void CMapOverview::UpdateObjects()
{
	for ( int i = 0; i < m_Objects.Count(); i++ )
	{
		MapObject_t *obj = &m_Objects[i];

		if ( obj->endtime > 0 && obj->endtime < gpGlobals->curtime )
		{
			m_Objects.Remove( i );
			i--;
			continue;
		}
		
		if ( obj->index <= 0 )
			continue;

		C_BaseEntity *entity = ClientEntityList().GetEnt( obj->index );

		if ( !entity )
			continue;

		obj->position = entity->GetAbsOrigin();
		obj->angle = entity->GetAbsAngles();
	}
}
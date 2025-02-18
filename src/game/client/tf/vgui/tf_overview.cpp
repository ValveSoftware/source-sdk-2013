//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include <vgui/ISurface.h>
#include <vgui/ILocalize.h>
#include "tf_shareddefs.h"
#include "tf_overview.h"
#include "c_playerresource.h"	
#include "c_tf_objective_resource.h"
#include "usermessages.h"
#include "coordsize.h"
#include "clientmode.h"
#include <vgui_controls/AnimationController.h>
#include "voice_status.h"
#include "spectatorgui.h"
#include "c_team_objectiveresource.h"

using namespace vgui;

void __MsgFunc_UpdateRadar( bf_read &msg )
{
	if ( !g_pMapOverview )
		return;

	int iPlayerEntity = msg.ReadByte();

	while ( iPlayerEntity > 0 )
	{
		int x = msg.ReadSBitLong( COORD_INTEGER_BITS-1 ) * 4;
		int y = msg.ReadSBitLong( COORD_INTEGER_BITS-1 ) * 4;
		int a = msg.ReadSBitLong( 9 );

		Vector origin( x, y, 0 );
		QAngle angles( 0, a, 0 );

		g_pMapOverview->SetPlayerPositions( iPlayerEntity-1, origin, angles );

		iPlayerEntity = msg.ReadByte(); // read index for next player
	}
}

extern ConVar _overview_mode;
ConVar _cl_minimapzoom( "_cl_minimapzoom", "1", FCVAR_ARCHIVE );
ConVar _overview_mode( "_overview_mode", "1", FCVAR_ARCHIVE, "Overview mode - 0=off, 1=inset, 2=full\n", true, 0, true, 2 );


CTFMapOverview *GetTFOverview( void )
{
	return dynamic_cast<CTFMapOverview *>( g_pMapOverview );
}

// overview_togglezoom rotates through 3 levels of zoom for the small map
//-----------------------------------------------------------------------
void ToggleZoom( void )
{
	if ( !GetTFOverview() )
		return;

	GetTFOverview()->ToggleZoom();
}
static ConCommand overview_togglezoom( "overview_togglezoom", ToggleZoom );

// overview_largemap toggles showing the large map
//------------------------------------------------
void ShowLargeMap( void )
{
	if ( !GetTFOverview() )
		return;

	GetTFOverview()->ShowLargeMap();
}
static ConCommand overview_showlargemap( "+overview_largemap", ShowLargeMap );

void HideLargeMap( void )
{
	if ( !GetTFOverview() )
		return;

	GetTFOverview()->HideLargeMap();
}
static ConCommand overview_hidelargemap( "-overview_largemap", HideLargeMap );

//--------------------------------
// map border ?
// icon minimum zoom
// flag swipes
// chatting icon
// voice com icon
//---------------------------------

DECLARE_HUDELEMENT( CTFMapOverview );

ConVar tf_overview_voice_icon_size( "tf_overview_voice_icon_size", "64", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFMapOverview::CTFMapOverview( const char *pElementName ) : BaseClass( pElementName )
{
	InitTeamColorsAndIcons();
	m_flIconSize = 96.0f;
	m_iLastMode = MAP_MODE_OFF;
	m_bDisabled = false;
	m_nMapTextureOverlayID = -1;
	usermessages->HookMessage( "UpdateRadar", __MsgFunc_UpdateRadar );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::Update()
{
	UpdateCapturePoints();

	BaseClass::Update();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::VidInit( void )
{
	BaseClass::VidInit();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::UpdateCapturePoints()
{
	if ( !g_pObjectiveResource )
		return;

	Color colorGreen( 0,255,0,255 );

	for( int i = 0 ; i < g_pObjectiveResource->GetNumControlPoints() ; i++ )
	{
		// check if CP is visible at all
		if( !g_pObjectiveResource->IsCPVisible( i ) )
		{
			if ( m_CapturePoints[i] != 0 )
			{
				// remove capture point from map
				RemoveObject( m_CapturePoints[i] );
				m_CapturePoints[i] = 0;
			}

			continue;
		}

		// ok, show CP
		int iOwningTeam = g_pObjectiveResource->GetOwningTeam(i);
		int iCappingTeam = g_pObjectiveResource->GetCappingTeam(i);

		int iOwningIcon = g_pObjectiveResource->GetIconForTeam( i, iOwningTeam );
		if ( iOwningIcon <= 0 )
			continue;	// baah

		const char *textureName = GetMaterialNameFromIndex( iOwningIcon );

		int objID = m_CapturePoints[i];

		if ( objID == 0 )
		{
			// add object if not already there
			objID = m_CapturePoints[i] = AddObject( textureName, 0, -1 );

			// objective positions never change (so far)
			SetObjectPosition( objID, g_pObjectiveResource->GetCPPosition(i), vec3_angle );

			AddObjectFlags( objID, MAP_OBJECT_ALIGN_TO_MAP );
		}

		SetObjectIcon( objID, textureName, 128.0 );

		// draw cap percentage
		if( iCappingTeam != TEAM_UNASSIGNED )
		{
			SetObjectStatus( objID, g_pObjectiveResource->GetCPCapPercentage(i), colorGreen );
		}
		else
		{
			SetObjectStatus( objID, -1, colorGreen ); // turn it off
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::InitTeamColorsAndIcons()
{
	BaseClass::InitTeamColorsAndIcons();

	m_TeamColors[TF_TEAM_RED] = COLOR_TF_RED;
	m_TeamIcons[TF_TEAM_RED] = AddIconTexture( "sprites/minimap_icons/red_player" );
	m_CameraIcons[TF_TEAM_RED] = AddIconTexture( "sprites/minimap_icons/red_camera" );

	m_TeamColors[TF_TEAM_BLUE] = COLOR_TF_BLUE;
	m_TeamIcons[TF_TEAM_BLUE] = AddIconTexture( "sprites/minimap_icons/blue_player" );
	m_CameraIcons[TF_TEAM_BLUE] = AddIconTexture( "sprites/minimap_icons/blue_camera" );

	Q_memset( m_flPlayerChatTime, 0, sizeof(m_flPlayerChatTime ) );
	m_iVoiceIcon = AddIconTexture( "voice/icntlk_pl" );
	m_iChatIcon = AddIconTexture( "sprites/minimap_icons/voiceIcon" );

	Q_memset( m_CapturePoints, 0, sizeof(m_CapturePoints) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::DrawCamera()
{
	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !localPlayer )
		return;

	int iTexture = m_CameraIcons[localPlayer->GetTeamNumber()];

	if ( localPlayer->IsObserver() || iTexture <= 0 )
	{
		BaseClass::DrawCamera();
	}
	else
	{
		MapObject_t obj;
		memset( &obj, 0, sizeof(MapObject_t) );

		obj.icon = iTexture;
		obj.position = localPlayer->GetAbsOrigin();
		obj.size = m_flIconSize * 1.5;
		obj.angle = localPlayer->EyeAngles();
		obj.status = -1;

		DrawIcon( &obj );

		DrawVoiceIconForPlayer( localPlayer->entindex() - 1 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::FireGameEvent( IGameEvent *event )
{
	const char * type = event->GetName();

	if ( Q_strcmp( type, "player_death" ) == 0 )
	{
		MapPlayer_t *player = GetPlayerByUserID( event->GetInt( "userid" ) );

		if ( player && CanPlayerBeSeen( player ) )
		{
			// create skull icon for 3 seconds
			int handle = AddObject( "sprites/minimap_icons/death", 0, 3 );
			SetObjectText( handle, player->name, player->color );
			SetObjectPosition( handle, player->position, player->angle );
		}
	}
	else if ( Q_strcmp( type, "game_newmap" ) == 0 )
	{
		SetMode( _overview_mode.GetInt() );
	}

	BaseClass::FireGameEvent( event	);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapOverview::CanPlayerBeSeen( MapPlayer_t *player )
{
	// rules that define if you can see a player on the overview or not
    C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

	if ( !localPlayer || !player )
		return false;

	// don't draw ourselves
	if ( localPlayer->entindex() == (player->index+1) )
		return false;

	// if local player is on spectator team, he can see everyone
	if ( localPlayer->GetTeamNumber() <= TEAM_SPECTATOR )
		return true;

	// we never track unassigned or real spectators
	if ( player->team <= TEAM_SPECTATOR )
		return false;

	// ingame and as dead player we can only see our own teammates
	return ( localPlayer->GetTeamNumber() == player->team );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::ShowLargeMap( void )
{
	if ( IsDisabled() )
	{
		return;
	}

	// remember old mode
	m_iLastMode = GetMode();

	// if we hit the toggle while full, set to disappear when we release
	if ( m_iLastMode == MAP_MODE_FULL )
	{
        m_iLastMode = MAP_MODE_OFF;
	}

	SetMode( MAP_MODE_FULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::HideLargeMap( void )
{
	if ( IsDisabled() )
	{
		return;
	}

	SetMode( m_iLastMode );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::ToggleZoom( void )
{
	if ( IsDisabled() )
	{
		return;
	}

	if ( GetMode() != MAP_MODE_INSET )
		return;

	int iZoomLevel = ( _cl_minimapzoom.GetInt() + 1 ) % TF_MAP_ZOOM_LEVELS;

	_cl_minimapzoom.SetValue( iZoomLevel );

	switch( _cl_minimapzoom.GetInt() )
	{
	case 0:
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomLevel1" );
		break;
	case 1:
	default:
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomLevel2" );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::SetMode(int mode)
{
	if ( IsDisabled() )
	{
		return;
	}

	m_flChangeSpeed = 0; // change size instantly

	if ( mode == MAP_MODE_OFF )
	{
		ShowPanel( false );

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapOff" );
	}
	else if ( mode == MAP_MODE_INSET )
	{
		switch( _cl_minimapzoom.GetInt() )
		{
		case 0:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomLevel1" );
			break;
		case 1:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomLevel2" );
			break;
		case 2:
		default:
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapZoomLevel3" );
			break;
		}

		C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

		if ( pPlayer )
			SetFollowEntity( pPlayer->entindex() );

		ShowPanel( true );

		if ( m_nMode == MAP_MODE_FULL )
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "MapScaleToSmall" );
		else
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "SnapToSmall" );
	}
	else if ( mode == MAP_MODE_FULL )
	{
		SetFollowEntity( 0 );

		ShowPanel( true );

		if ( m_nMode == MAP_MODE_INSET )
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "ZoomToLarge" );
		else
            g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "SnapToLarge" );
	}

	// finally set mode
	m_nMode = mode;

	// save in a cvar for archive
	_overview_mode.SetValue( m_nMode );

	UpdateSizeAndPosition();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::UpdateSizeAndPosition()
{
	// move back up if the spectator menu is not visible
	if ( !g_pSpectatorGUI || ( !g_pSpectatorGUI->IsVisible() && GetMode() == MAP_MODE_INSET ) )
	{
		int x,y,w,h;

		GetBounds( x,y,w,h );

		y = YRES(5);	// hax, align to top of the screen

		SetBounds( x,y,w,h );
	}

	BaseClass::UpdateSizeAndPosition();
}

ConVar cl_voicetest( "cl_voicetest", "0", FCVAR_CHEAT );
ConVar cl_overview_chat_time( "cl_overview_chat_time", "2.0", FCVAR_ARCHIVE );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::PlayerChat( int index )
{
	index = index-1;

	if ( !IsIndexIntoPlayerArrayValid(index) )
		return;
		
	m_flPlayerChatTime[index] = gpGlobals->curtime + cl_overview_chat_time.GetFloat();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::DrawMapPlayers()
{
	BaseClass::DrawMapPlayers();

	C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();

	Assert( localPlayer );

	int iLocalPlayer = localPlayer->entindex() - 1;

	for ( int i = 0 ; i < MAX_PLAYERS ; i++ )
	{
		if ( i == iLocalPlayer )
			continue;

		MapPlayer_t *player = &m_Players[i];

		if ( !CanPlayerBeSeen( player ) )
			continue;

		if ( player->health <= 0 )	// don't draw dead players / spectators
			continue;

		DrawVoiceIconForPlayer( i );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::DrawVoiceIconForPlayer( int playerIndex )
{
	Assert( playerIndex >= 0 && playerIndex < MAX_PLAYERS );

	MapPlayer_t *player = &m_Players[playerIndex];

	// if they just sent a chat msg, or are using voice, or did a hand signal or voice command
	// draw a chat icon

	if ( cl_voicetest.GetInt() || GetClientVoiceMgr()->IsPlayerSpeaking( player->index+1 ) )
	{
		MapObject_t obj;
		memset( &obj, 0, sizeof(MapObject_t) );

		obj.icon = m_iVoiceIcon;
		obj.position = player->position;
		obj.size = tf_overview_voice_icon_size.GetFloat();
		obj.status = -1;

		DrawIcon( &obj );
	}
	else if ( m_flPlayerChatTime[player->index] > gpGlobals->curtime )
	{
		MapObject_t obj;
		memset( &obj, 0, sizeof(MapObject_t) );

		obj.icon = m_iChatIcon;
		obj.position = player->position;
		obj.size = tf_overview_voice_icon_size.GetFloat();
		obj.status = -1;

		DrawIcon( &obj );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapOverview::DrawIcon( MapObject_t *obj )
{
	for ( int i = 0 ; i < MAX_CONTROL_POINTS ; i++ )
	{
		if ( obj->objectID == m_CapturePoints[i] && obj->objectID != 0 )
		{
			return DrawCapturePoint( i, obj );
		}
	}

	return  BaseClass::DrawIcon( obj );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::DrawQuad( Vector pos, int scale, float angle, int textureID, int alpha )
{
	Vector offset;
	offset.z = 0;

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

	surface()->DrawSetColor( 255, 255, 255, alpha );
	surface()->DrawSetTexture( textureID );
	surface()->DrawTexturedPolygon( 4, points );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapOverview::DrawCapturePoint( int iCP, MapObject_t *obj )
{
	int textureID = obj->icon;
	Vector pos = obj->position;
	float scale = obj->size;

	Vector2D pospanel = WorldToMap( pos );
	pospanel = MapToPanel( pospanel );

	if ( !IsInPanel( pospanel ) )
		return false; // player is not within overview panel

	// draw capture swipe
	DrawQuad( pos, scale, 0, textureID, 255 );

	int iCappingTeam = g_pObjectiveResource->GetCappingTeam( iCP );

	if ( iCappingTeam != TEAM_UNASSIGNED )
	{
		int iCapperIcon = g_pObjectiveResource->GetCPCappingIcon( iCP );
		const char *textureName = GetMaterialNameFromIndex( iCapperIcon );

		float flCapPercent = g_pObjectiveResource->GetCPCapPercentage(iCP);
		bool bSwipeLeft = ( iCappingTeam == TF_TEAM_RED ) ? true : false;

		DrawHorizontalSwipe( pos, scale, AddIconTexture( textureName ), flCapPercent, bSwipeLeft );
	}

	// fixup for noone is capping, but someone is in the area
	int iNumBlue = g_pObjectiveResource->GetNumPlayersInArea( iCP, TF_TEAM_BLUE );
	int iNumRed = g_pObjectiveResource->GetNumPlayersInArea( iCP, TF_TEAM_RED );

	int iOwningTeam = g_pObjectiveResource->GetOwningTeam( iCP );
	if ( iCappingTeam == TEAM_UNASSIGNED )
	{
		if ( iNumBlue > 0 && iNumRed == 0 && iOwningTeam != TF_TEAM_BLUE )
		{
			iCappingTeam = TF_TEAM_BLUE;
		}
		else if ( iNumRed > 0 && iNumBlue == 0 && iOwningTeam != TF_TEAM_RED )
		{
			iCappingTeam = TF_TEAM_RED;
		}
	}

	if ( iCappingTeam != TEAM_UNASSIGNED )
	{
		// Draw the number of cappers below the icon
		int numPlayers = g_pObjectiveResource->GetNumPlayersInArea( iCP, iCappingTeam );
		int requiredPlayers = g_pObjectiveResource->GetRequiredCappers( iCP, iCappingTeam );

		if ( requiredPlayers > 1 )
		{
			numPlayers = MIN( numPlayers, requiredPlayers );

			wchar_t wText[6];
			_snwprintf( wText, sizeof(wText)/sizeof(wchar_t), L"%d", numPlayers );

			int wide, tall;
			surface()->GetTextSize( m_hIconFont, wText, wide, tall );

			int x = pospanel.x-(wide/2);
			int y = pospanel.y;

			// match the offset that MapOverview uses
			y += GetPixelOffset( scale ) + 4;

			// draw black shadow text
			surface()->DrawSetTextColor( 0, 0, 0, 255 );
			surface()->DrawSetTextPos( x+1, y );
			surface()->DrawPrintText( wText, wcslen(wText) );

			// draw name in color 
			surface()->DrawSetTextColor( g_PR->GetTeamColor( iCappingTeam ) );
			surface()->DrawSetTextPos( x, y );
			surface()->DrawPrintText( wText, wcslen(wText) );
		}
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::DrawHorizontalSwipe( Vector pos, int scale, int textureID, float flCapPercentage, bool bSwipeLeft )
{
	float flIconSize = scale * 2;
	float width = ( flIconSize * flCapPercentage );

	float uv1 = 0.0f;
	float uv2 = 1.0f;

	Vector2D uv11( uv1, uv2 );
	Vector2D uv21( flCapPercentage, uv2 );
	Vector2D uv22( flCapPercentage, uv1 );
	Vector2D uv12( uv1, uv1 );

	// reversing the direction of the swipe effect
	if ( bSwipeLeft )
	{
		uv11.x = uv2 - flCapPercentage;
		uv21.x = uv2;
		uv22.x = uv2;
		uv12.x = uv2 - flCapPercentage;
	}

	float flXPos = pos.x - scale;
	float flYPos = pos.y - scale;

	Vector upperLeft( flXPos,			flYPos, 0 );
	Vector upperRight( flXPos + width,	flYPos, 0 );
	Vector lowerRight( flXPos + width,	flYPos + flIconSize, 0 );
	Vector lowerLeft ( flXPos,			flYPos + flIconSize, 0 );

	/// reversing the direction of the swipe effect
	if ( bSwipeLeft )
	{
		upperLeft.x  = flXPos + flIconSize - width;
		upperRight.x = flXPos + flIconSize;
		lowerRight.x = flXPos + flIconSize;
		lowerLeft.x  = flXPos + flIconSize - width;
	}

	vgui::Vertex_t vert[4];	

	Vector2D pos0 = WorldToMap( upperLeft );
	vert[0].Init( MapToPanel( pos0 ), uv11 );

	Vector2D pos3 = WorldToMap( lowerLeft );
	vert[1].Init( MapToPanel( pos3 ), uv12 );

	Vector2D pos2 = WorldToMap( lowerRight );
	vert[2].Init( MapToPanel( pos2 ), uv22 );

	Vector2D pos1 = WorldToMap( upperRight );
	vert[3].Init( MapToPanel( pos1 ), uv21 );

	surface()->DrawSetColor( 255, 255, 255, 255 );
	surface()->DrawSetTexture( textureID );
	surface()->DrawTexturedPolygon( 4, vert );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::SetMap( const char * levelname )
{
	BaseClass::SetMap( levelname );

	if ( m_nMapTextureID != -1 )
	{
		// we found a texture for this map
		SetDisabled( false );
		SetMode( m_nMode );
	}
	else
	{
		// we failed to load a map image
		SetDisabled( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFMapOverview::ShouldDraw( void )
{
	if ( IsDisabled() )
	{
		return false;
	}

	return BaseClass::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::Paint()
{
	UpdateMapOverlayTexture();

	UpdateSizeAndPosition();

	UpdateFollowEntity();

	UpdateObjects();

	UpdatePlayers();

//	UpdatePlayerTrails();

	DrawMapTexture();

	DrawMapOverlayTexture();

//	DrawMapPlayerTrails();

	DrawObjects();

	DrawMapPlayers();

	DrawCamera();

	Panel::Paint();
}

extern ConVar overview_alpha;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::DrawMapOverlayTexture()
{
	// now draw a box around the outside of this panel
	int x0, y0, x1, y1;
	int wide, tall;

	GetSize( wide, tall );
	x0 = 0; y0 = 0; x1 = wide - 2; y1 = tall - 2;

	if ( m_nMapTextureOverlayID < 0 )
	{
		return;
	}

	Vertex_t points[4] =
	{
		Vertex_t( MapToPanel ( Vector2D(0,0) ), Vector2D(0,0) ),
			Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,0) ), Vector2D(1,0) ),
			Vertex_t( MapToPanel ( Vector2D(OVERVIEW_MAP_SIZE-1,OVERVIEW_MAP_SIZE-1) ), Vector2D(1,1) ),
			Vertex_t( MapToPanel ( Vector2D(0,OVERVIEW_MAP_SIZE-1) ), Vector2D(0,1) )
	};

	int alpha = 255.0f * overview_alpha.GetFloat(); clamp( alpha, 1, 255 );

	surface()->DrawSetColor( 255,255,255, alpha );
	surface()->DrawSetTexture( m_nMapTextureOverlayID );
	surface()->DrawTexturedPolygon( 4, points );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFMapOverview::UpdateMapOverlayTexture()
{


/*
	char tempfile[MAX_PATH];
	Q_snprintf( tempfile, sizeof( tempfile ), "overviews/%s_%s_%s_%s", levelname, roundname, capname, teamname );

	// TODO release old texture ?

	m_nMapTextureOverlayID = surface()->CreateNewTextureID();

	//if we have not uploaded yet, lets go ahead and do so
	surface()->DrawSetTextureFile( m_nMapTextureOverlayID, tempfile, true, false );

	int wide, tall;

	surface()->DrawGetTextureSize( m_nMapTextureOverlayID, wide, tall );

	if ( wide != tall )
	{
		DevMsg( 1, "Error! CTFMapOverview::UpdateMapOverlayTexture: map overlay image must be a square.\n" );
		m_nMapTextureOverlayID = -1;
		return;
	}
*/
}
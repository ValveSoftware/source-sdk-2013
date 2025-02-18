//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#include "cbase.h"
#include "hud.h"
#include "hud_macros.h"
#include "c_tf_player.h"
#include "iclientmode.h"
#include "ienginevgui.h"
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Label.h>
#include "tf_shareddefs.h"
#include "tf_hud_notification_panel.h"
#include "tf_hud_freezepanel.h"
#include <filesystem.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

ConVar tf_hud_notification_duration( "tf_hud_notification_duration", "3.0", 0, "How long to display hud notification panels before fading them" );
ConVar tf_hud_notification_show_count_kart_controls( "tf_hud_notification_show_count_kart_controls", "0", FCVAR_ARCHIVE );
ConVar tf_hud_notification_show_count_ghost_controls( "tf_hud_notification_show_count_ghost_controls", "0", FCVAR_ARCHIVE );
ConVar tf_hud_notification_show_count_ghost_controls_no_respawn( "tf_hud_notification_show_count_ghost_controls_no_respawn", "0", FCVAR_ARCHIVE );

DECLARE_HUDELEMENT( CHudNotificationPanel );

DECLARE_HUD_MESSAGE( CHudNotificationPanel, HudNotify );
DECLARE_HUD_MESSAGE( CHudNotificationPanel, HudNotifyCustom );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudNotificationPanel::CHudNotificationPanel( const char *pElementName ) : CHudElement( pElementName ), BaseClass( NULL, "NotificationPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	vgui::ivgui()->AddTickSignal( GetVPanel() );

	m_flFadeTime = 0;

	// listen for one version that just passes an int, for prebuilt notifications
	// and another that takes a res file to load

	m_pText = new Label( this, "Notification_Label", "" );
	m_pIcon = new CIconPanel( this, "Notification_Icon" );
	m_pBackground = new ImagePanel( this, "Notification_Background" );

	RegisterForRenderGroup( "mid" );
	RegisterForRenderGroup( "commentary" );

	LoadManifest();
	m_mapShowCounts.SetLessFunc( DefLessFunc( int ) ) ;
	m_mapShowCounts.Insert( HUD_NOTIFY_HOW_TO_CONTROL_GHOST, ShowCount_t( 3, 300.f, &tf_hud_notification_show_count_ghost_controls ) );
	m_mapShowCounts.Insert( HUD_NOTIFY_HOW_TO_CONTROL_KART, ShowCount_t( 3, 300.f, &tf_hud_notification_show_count_kart_controls ) );
	m_mapShowCounts.Insert( HUD_NOTIFY_HOW_TO_CONTROL_GHOST_NO_RESPAWN, ShowCount_t( 3, 300.f, &tf_hud_notification_show_count_ghost_controls_no_respawn ) );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::Init( void )
{
	CHudElement::Init();

	HOOK_HUD_MESSAGE( CHudNotificationPanel, HudNotify );
	HOOK_HUD_MESSAGE( CHudNotificationPanel, HudNotifyCustom );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::ApplySchemeSettings( IScheme *pScheme )
{
	// load control settings...
	LoadControlSettings( "resource/UI/notifications/base_notification.res" );

	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::MsgFunc_HudNotify( bf_read &msg )
{
	int iType = msg.ReadByte();
	bool bForceShow = msg.ReadByte();

	// Ignore notifications in minmode
	if ( !bForceShow )
	{
		ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
		if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
			return;
	}

	float flDuration = tf_hud_notification_duration.GetFloat();

	// Check if we're only supposed to show a limited number of times
	auto idx = m_mapShowCounts.Find( iType );
	if ( m_mapShowCounts.IsValidIndex( idx ) )
	{
		auto& showCount = m_mapShowCounts[ idx ];
		// Stop here if we've met our max show count, or it's too soon
		if ( showCount.m_pConVar->GetInt() >= showCount.m_nMaxShowCount 
			|| showCount.m_flNextAllowedTime > Plat_FloatTime() )
		{
			return;
		}

		// Increment show count
		showCount.m_pConVar->SetValue( showCount.m_pConVar->GetInt() + 1 );
		// Set next time we're allowed to show
		showCount.m_flNextAllowedTime = Plat_FloatTime() + showCount.m_flCooldown;
	}
	
	
	InvalidateLayout( true, true );
	LoadControlSettings( GetNotificationByType( iType, flDuration ) );
	
	// set up the fade time
	m_flFadeTime = gpGlobals->curtime + flDuration;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::MsgFunc_HudNotifyCustom( bf_read &msg )
{
	// Ignore notifications in minmode
	ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
	if ( cl_hud_minmode.IsValid() && cl_hud_minmode.GetBool() )
		return;

	// Reload the base
	LoadControlSettings( "resource/UI/notifications/base_notification.res" );

	char szText[256];
	char szIcon[256];

	msg.ReadString( szText, sizeof(szText) );
	msg.ReadString( szIcon, sizeof(szIcon) );
	int iBackgroundTeam = msg.ReadByte();

	SetupNotifyCustom( szText, szIcon, iBackgroundTeam );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::SetupNotifyCustom( const char *pszText, const char *pszIcon, int iBackgroundTeam )
{
	// Reload the base
	LoadControlSettings( "resource/UI/notifications/base_notification.res" );

	m_pIcon->SetIcon( pszIcon );
	m_pText->SetText( pszText );

	if ( iBackgroundTeam == TF_TEAM_RED )
	{
		m_pBackground->SetImage( "../hud/score_panel_red_bg" );
	}
	else if ( iBackgroundTeam == TF_TEAM_BLUE )
	{
		m_pBackground->SetImage( "../hud/score_panel_blue_bg" );
	}
	else
	{
		m_pBackground->SetImage( "../hud/notification_black" );
	}

	// set up the fade time
	m_flFadeTime = gpGlobals->curtime + tf_hud_notification_duration.GetFloat();

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::SetupNotifyCustom( const wchar_t *pszText, const char *pszIcon, int iBackgroundTeam )
{
	// Reload the base
	LoadControlSettings( "resource/UI/notifications/base_notification.res" );

	m_pIcon->SetIcon( pszIcon );
	m_pText->SetText( pszText );

	if ( iBackgroundTeam == TF_TEAM_RED )
	{
		m_pBackground->SetImage( "../hud/score_panel_red_bg" );
	}
	else if ( iBackgroundTeam == TF_TEAM_BLUE )
	{
		m_pBackground->SetImage( "../hud/score_panel_blue_bg" );
	}
	else
	{
		m_pBackground->SetImage( "../hud/notification_black" );
	}

	// set up the fade time
	m_flFadeTime = gpGlobals->curtime + tf_hud_notification_duration.GetFloat();

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::SetupNotifyCustom( const wchar_t *pszText, HudNotification_t type, float overrideDuration )
{
	float flDuration = tf_hud_notification_duration.GetFloat();

	// Reload the base
	LoadControlSettings( GetNotificationByType( type, flDuration ) );

	m_pText->SetText( pszText );

	// set up the fade time
	m_flFadeTime = gpGlobals->curtime + ( overrideDuration > 0.f ? overrideDuration : flDuration );

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::PerformLayout( void )
{
	BaseClass::PerformLayout();

	// re-layout the panel manually so we fit the length of the string!
	// **** this is super yucky, i'm going to cry myself to sleep tonight ****

	int iTextWide, iTextTall;
	m_pText->GetContentSize( iTextWide, iTextTall );

	m_pText->SetSize( iTextWide, m_pText->GetTall() );

	float flTextWide = m_pText->GetWide();
	float flIconWide = m_pIcon->GetWide();

	float flSpacer = XRES( 5 );
	float flEndSpacer = XRES( 8 ) + XRES( 3 ) * ( flTextWide / 184 );	// total hackery

	float flTotalWidth = flEndSpacer + flIconWide + flSpacer + flTextWide + flEndSpacer;

	float flLeftSide = ( GetWide() - flTotalWidth ) * 0.5f;

	// resize and position background
	m_pBackground->SetPos( flLeftSide, 0 );

	m_pBackground->SetWide( flTotalWidth );

	// reposition icon
	int iIconXPos, iIconYPos;
	m_pIcon->GetPos( iIconXPos, iIconYPos );

	m_pIcon->SetPos( flLeftSide + flEndSpacer, iIconYPos );

	// reposition text
	int iTextXPos, iTextYPos;
	m_pText->GetPos( iTextXPos, iTextYPos );

	m_pText->SetPos( flLeftSide + flEndSpacer + flIconWide + flSpacer, iTextYPos );

	const unsigned short tempBufSize = 2048;
	wchar_t tempBufIn[tempBufSize];
	wchar_t tempBufOut[tempBufSize];
	m_pText->GetText( tempBufIn, tempBufSize );
	UTIL_ReplaceKeyBindings( tempBufIn, tempBufSize, tempBufOut, tempBufSize );
	m_pText->SetText( tempBufOut );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudNotificationPanel::ShouldDraw( void )
{
	// only if we have a valid message to draw
	if ( m_flFadeTime < gpGlobals->curtime )
		return false;

	if ( IsTakingAFreezecamScreenshot() )
		return false;

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudNotificationPanel::OnTick( void )
{
	// set alpha based on time left

	// do this if we can fade the icons and the background

	/*
	float flLifeTime = m_flFadeTime - gpGlobals->curtime;

	if ( flLifeTime >= 1 )
	{
		SetAlpha( 255 );
	}
	else
	{
		SetAlpha( (float)( 255.0f * flLifeTime ) );
	}
	*/
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CHudNotificationPanel::GetNotificationByType( int iType, float& flDuration )
{
	bool bOnBlueTeam = false;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( pLocalPlayer )
	{
		bOnBlueTeam = ( pLocalPlayer->GetTeamNumber() == TF_TEAM_BLUE );
	}

	const char *pszResult = "";

	switch ( iType )
	{
	case HUD_NOTIFY_YOUR_FLAG_TAKEN:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_your_flag_taken_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_your_flag_taken_red.res";
		}
		break;
	case HUD_NOTIFY_YOUR_FLAG_DROPPED:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_your_flag_dropped_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_your_flag_dropped_red.res";
		}
		break;
	case HUD_NOTIFY_YOUR_FLAG_RETURNED:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_your_flag_returned_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_your_flag_returned_red.res";
		}
		break;
	case HUD_NOTIFY_YOUR_FLAG_CAPTURED:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_your_flag_captured_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_your_flag_captured_red.res";
		}
		break;
	case HUD_NOTIFY_ENEMY_FLAG_TAKEN:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_taken_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_taken_red.res";
		}
		break;
	case HUD_NOTIFY_ENEMY_FLAG_DROPPED:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_dropped_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_dropped_red.res";
		}
		break;
	case HUD_NOTIFY_ENEMY_FLAG_RETURNED:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_returned_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_returned_red.res";
		}
		break;
	case HUD_NOTIFY_ENEMY_FLAG_CAPTURED:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_captured_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_enemy_flag_captured_red.res";
		}
		break;

	case HUD_NOTIFY_TOUCHING_ENEMY_CTF_CAP:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_touching_enemy_ctf_cap_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_touching_enemy_ctf_cap_red.res";
		}
		break;

	case HUD_NOTIFY_NO_INVULN_WITH_FLAG:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_no_invuln_with_flag_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_no_invuln_with_flag_red.res";
		}
		break;

	case HUD_NOTIFY_NO_TELE_WITH_FLAG:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_no_tele_with_flag_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_no_tele_with_flag_red.res";
		}
		break;


	case HUD_NOTIFY_SPECIAL:
		pszResult = "resource/UI/notifications/notify_special.res";
		break;

	case HUD_NOTIFY_GOLDEN_WRENCH:
		pszResult = "resource/UI/notifications/notify_golden_wrench.res";
		break;

	case HUD_NOTIFY_RD_ROBOT_UNDER_ATTACK:
		if ( bOnBlueTeam )
		{
			pszResult = "resource/UI/notifications/notify_rd_robot_attacked_blue.res";
		}
		else
		{
			pszResult = "resource/UI/notifications/notify_rd_robot_attacked_red.res";
		}
		break;

	case HUD_NOTIFY_HOW_TO_CONTROL_GHOST:
		pszResult = "resource/UI/notifications/notify_how_to_control_ghost.res";
		flDuration = 10.f;
		break;

	case HUD_NOTIFY_HOW_TO_CONTROL_KART:
		pszResult = "resource/UI/notifications/notify_how_to_control_kart.res";
		flDuration = 10.f;
		break;

	case HUD_NOTIFY_HOW_TO_CONTROL_GHOST_NO_RESPAWN:
		pszResult = "resource/UI/notifications/notify_how_to_control_ghost_no_respawn.res";
		flDuration = 10.f;
		break;

	// Passtime
	case HUD_NOTIFY_PASSTIME_HOWTO: 
		pszResult = "resource/UI/notifications/notify_passtime_howto.res"; 
		flDuration = 10.f; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_TELE: 
		pszResult = "resource/UI/notifications/notify_passtime_no_tele.res"; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_CARRY: 
		pszResult = "resource/UI/notifications/notify_passtime_no_carry.res"; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_INVULN: 
		pszResult = "resource/UI/notifications/notify_passtime_no_invuln.res"; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_DISGUISE: 
		pszResult = "resource/UI/notifications/notify_passtime_no_disguise.res"; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_CLOAK:
		pszResult = "resource/UI/notifications/notify_passtime_no_cloak.res"; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_OOB:
		pszResult = "resource/UI/notifications/notify_passtime_no_oob.res"; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_HOLSTER: 
		pszResult = "resource/UI/notifications/notify_passtime_no_holster.res"; 
		break;

	case HUD_NOTIFY_PASSTIME_NO_TAUNT: 
		pszResult = "resource/UI/notifications/notify_passtime_no_taunt.res"; 
		break;

	// Competitive
	case HUD_NOTIFY_COMPETITIVE_GC_DOWN:
		pszResult = "resource/UI/notifications/notify_competitive_gc_down.res";
		flDuration = 20.f;
		break;

	case HUD_NOTIFY_TRUCE_START:
		pszResult = "resource/UI/notifications/notify_truce_start.res";
		flDuration = 10.f;
		break;

	case HUD_NOTIFY_TRUCE_END:
		pszResult = "resource/UI/notifications/notify_truce_end.res";
		flDuration = 10.f;
		break;

	default:
		break;
	}

	return pszResult;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHudNotificationPanel::LoadManifest( void )
{
	const char *pszManifestFile = "resource/UI/notifications/notification_manifest.txt";
	KeyValues *manifest = new KeyValues( pszManifestFile );
	if ( manifest->LoadFromFile( g_pFullFileSystem, pszManifestFile, "GAME" ) == false )
	{
		manifest->deleteThis();
		return false;
	}

	// Load each file defined in the text
	for ( KeyValues *sub = manifest->GetFirstSubKey(); sub != NULL; sub = sub->GetNextKey() )
	{
		if ( !Q_stricmp( sub->GetName(), "file" ) )
		{
			if ( BuildGroup::PrecacheResFile( sub->GetString() ) == false )
			{
				Warning("Failed to load notification res file '%s' specified in %s.\n", sub->GetString(), pszManifestFile );
			}
		}
	}

	manifest->deleteThis();
	return true;
}
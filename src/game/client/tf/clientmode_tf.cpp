//========= Copyright Valve Corporation, All rights reserved. ============//
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "clientmode_tf.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "iviewrender.h"
#include "vgui/ISurface.h"
#include "vgui/IPanel.h"
#include "GameUI/IGameUI.h"
#include <vgui_controls/AnimationController.h>
#include "ivmodemanager.h"
#include "buymenu.h"
#include "filesystem.h"
#include "vgui/IVGui.h"
#include "hud_chat.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <KeyValues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "c_tf_player.h"
#include "ienginevgui.h"
#include "in_buttons.h"
#include "voice_status.h"
#include "tf_gamerules.h"
#include "tf_hud_menu_engy_build.h"
#include "tf_hud_menu_engy_destroy.h"
#include "tf_hud_menu_spy_disguise.h"
#include "tf_statsummary.h"
#include "tf_hud_freezepanel.h"
#include "item_quickswitch.h"
#include "hud_macros.h"
#include "vgui/ILocalize.h"
#include "glow_outline_effect.h"
#include "vgui/IInput.h"
#include "tf_hud_mainmenuoverride.h"
#include "tf_controls.h"
#include "econ_notifications.h"
#include "rtime.h"
#include "econ_item_description.h"
#include "c_tf_playerresource.h"
#include "c_team.h"
#include "tf_hud_menu_eureka_teleport.h"
#include "tf_hud_menu_taunt_selection.h"
#include "tf_hud_inspectpanel.h"
#include "engine/IEngineSound.h"
#include "tf_partyclient.h"

#include "quest_objective_manager.h"
#include "econ_item_system.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_hud_mann_vs_machine_status.h"
#include "player_vs_environment/c_tf_upgrades.h"

#include "steam/isteamfriends.h"
#include "steamworks_gamestats.h"
#include "confirm_dialog.h"
#include "ServerBrowser/blacklisted_server_manager.h"
#include "tf_quickplay_shared.h"
#include "sourcevr/isourcevirtualreality.h"
#include "client_virtualreality.h"

#include "econ_gcmessages.h"

#if defined( _X360 )
#include "tf_clientscoreboard.h"
#endif

#include "gc_clientsystem.h"
#include "tf_gcmessages.h"
#include "tf_gc_client.h"

#include "tf_wardata.h"

#include "debugoverlay_shared.h"

#include "hud_vote.h"
#include "c_tf_notification.h"

#if !defined( _X360 ) && !defined( NO_STEAM )
#include "steam/isteamtimeline.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#if !defined(NO_STEAM)
extern ConVar cl_steamscreenshots;
#endif

extern ISoundEmitterSystemBase *soundemitterbase;

static Color colorEyeballBossText( 134, 80, 172, 255 );
static Color colorMerasmusText( 112, 176, 74, 255 );

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );
ConVar fov_desired( "fov_desired", "75", FCVAR_ARCHIVE | FCVAR_USERINFO, "Sets the base field-of-view.", true, 20.0, true, MAX_FOV );


#define TF_HIGHFIVE_HINT_MAXDIST		512.0f
#define TF_HIGHFIVE_HINT_MAXHINTS		3
#define TF_HIGHFIVE_HINT_MINTIMEBETWEEN	10.0f
ConVar tf_highfive_hintcount( "tf_highfive_hintcount", "0", FCVAR_CLIENTDLL | FCVAR_DONTRECORD | FCVAR_ARCHIVE, "Counts the number of times the high five hint has been displayed", true, 0, false, 0 );

ConVar tf_delete_temp_files( "tf_delete_temp_files", "1", FCVAR_CLIENTDLL | FCVAR_ARCHIVE, "Delete custom player sprays and other temp files during shutdown" );

ConVar tf_taunt_always_show_hint( "tf_taunt_always_show_hint", "1", FCVAR_CLIENTDLL );
extern ConVar tf_allow_all_team_partner_taunt;
extern ConVar tf_mvm_buybacks_method;
extern ConVar tf_autobalance_ask_candidates_maxtime;
extern ConVar tf_autobalance_dead_candidates_maxtime;
extern ConVar tf_autobalance_xp_bonus;
extern ConVar cl_notifications_show_ingame;

extern ConVar sc_look_sensitivity_scale;

extern bool TournamentHudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
extern bool ArenaClassLayoutKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
extern bool CoachingHandlesKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
extern bool ItemTestHandlesKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );
extern bool ShouldScoreBoardHandleKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

static bool TrainingHandlesKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( TFGameRules() != NULL && TFGameRules()->IsInTraining() && TFGameRules()->IsWaitingForTrainingContinue() )
	{
		if ( down && keynum == KEY_SPACE )
		{								  
			engine->ClientCmd_Unrestricted( "training_continue" );
			return true;
		}
	}
	return false;
}


static bool HalloweenHandlesKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( pPlayer )
	{
		// don't do anything while dancing
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_THRILLER ) )
		{
			return true;
		}

		// only allow +attack
		if ( pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
		{
			if ( pszCurrentBinding )
			{
				if ( FStrEq( pszCurrentBinding, "+attack" ) )
				{
					engine->ServerCmd( "boo" );
					return true;
				}
				else if ( FStrEq( pszCurrentBinding, "+attack2" ) || FStrEq( pszCurrentBinding, "+attack3" ) )
				{
					return true;
				}
			}
		}
	}

	return false;
}


static bool TauntHandlesKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	static const char *pszStopTauntKeys[] =
	{
		"+jump",
		"+taunt",
		"weapon_taunt",
	};

	C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		{
			for ( int i=0; i<ARRAYSIZE( pszStopTauntKeys ); ++i )
			{
				if ( down && pszCurrentBinding && FStrEq( pszCurrentBinding, pszStopTauntKeys[i] ) )
				{
					// Halloween Hackery
					if ( i == 0 && pPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_KART ) )
					{
						continue;
					}

					engine->ClientCmd( "stop_taunt" );
					return true;
				}
			}
		}
	}

	return false;
}


// Sets convars to tag the current mapname and the player in your crosshairs.
// The player tagged will be overridden for killcam shots to be the killer
static void ScreenshotTaggingKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	if ( pszCurrentBinding && ( FStrEq( pszCurrentBinding, "screenshot" ) || FStrEq( pszCurrentBinding, "jpeg" ) ) )
	{
		// Tag the player in the crosshairs
		C_TFPlayer *pPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );
		if ( pPlayer )
		{
			C_TFPlayer *pCrosshairs = ToTFPlayer( UTIL_PlayerByIndex( pPlayer->GetIDTarget() ) );
			if ( pCrosshairs )
			{
				CSteamID steamID;
				if ( pCrosshairs->GetSteamID( &steamID ) )
				{
					ConVarRef cl_screenshotusertag( "cl_screenshotusertag" );
					if ( cl_screenshotusertag.IsValid() )
					{
						cl_screenshotusertag.SetValue( (int)steamID.GetAccountID() );
					}
				}
			}
		}

		// Tag the current map
		ConVarRef cl_screenshotlocation( "cl_screenshotlocation" );
		if ( cl_screenshotlocation.IsValid() )
		{
			char szMapName[MAX_MAP_NAME];
			Q_FileBase( engine->GetLevelName(), szMapName, sizeof(szMapName) );
			Q_strlower( szMapName );
			cl_screenshotlocation.SetValue( GetMapDisplayName( szMapName ) );
		}
	}
}


static void EnableSteamScreenshots( bool bEnable )
{
#if !defined(NO_STEAM)
	if ( steamapicontext && steamapicontext->SteamScreenshots() )
	{
		ConVarRef cl_savescreenshotstosteam( "cl_savescreenshotstosteam" );
		if ( cl_savescreenshotstosteam.IsValid() )
		{
			cl_savescreenshotstosteam.SetValue( bEnable );
			steamapicontext->SteamScreenshots()->HookScreenshots( bEnable );
		}
	}
#endif
}

#if !defined(NO_STEAM)
void SteamScreenshotsCallBack( IConVar *var, const char *pOldString, float flOldValue )
{
	EnableSteamScreenshots( cl_steamscreenshots.GetBool() );
}
ConVar cl_steamscreenshots( "cl_steamscreenshots", "1", FCVAR_ARCHIVE, "Enable/disable saving screenshots to Steam", SteamScreenshotsCallBack );
#endif


void HUDMinModeChangedCallBack( IConVar *var, const char *pOldString, float flOldValue )
{
	engine->ExecuteClientCmd( "hud_reloadscheme" );
}
ConVar cl_hud_minmode( "cl_hud_minmode", "0", FCVAR_ARCHIVE, "Set to 1 to turn on the advanced minimalist HUD mode.", HUDMinModeChangedCallBack );

IClientMode *g_pClientMode = NULL;

// --------------------------------------------------------------------------------- //
// CTFModeManager.
// --------------------------------------------------------------------------------- //

class CTFModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CTFModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;


// --------------------------------------------------------------------------------- //
// CTFModeManager implementation.
// --------------------------------------------------------------------------------- //

#define SCREEN_FILE		"scripts/vgui_screens.txt"

void CTFModeManager::Init()
{
	g_pClientMode = GetClientModeNormal();
	
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	// Load the objects.txt file.
	LoadObjectInfos( ::filesystem );

	GetClientVoiceMgr()->SetHeadLabelOffset( 40 );

	EnableSteamScreenshots( true );
}

void CTFModeManager::LevelInit( const char *newmap )
{
	g_pClientMode->LevelInit( newmap );

	ConVarRef voice_steal( "voice_steal" );

	if ( voice_steal.IsValid() )
	{
		voice_steal.SetValue( 1 );
	}
}

void CTFModeManager::LevelShutdown( void )
{
	g_pClientMode->LevelShutdown();

	extern void CL_Training_LevelShutdown();
	extern void CL_Coaching_LevelShutdown();
	extern void CL_Consumables_LevelShutdown();
	extern void CL_Halloween_LevelShutdown();
	CL_Training_LevelShutdown();
	CL_Coaching_LevelShutdown();
	CL_Consumables_LevelShutdown();
	CL_Halloween_LevelShutdown();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeTFNormal::ClientModeTFNormal()
{
	m_pMenuEngyBuild = NULL;
	m_pMenuEngyDestroy = NULL;
	m_pMenuSpyDisguise = NULL;
	m_pEurekaTeleportMenu = NULL;
	m_pMenuTauntSelection = NULL;
	m_pGameUI = NULL;
	m_pFreezePanel = NULL;
	m_pQuickSwitch = NULL;
	m_pInspectPanel = NULL;
	m_wasConnectedLastUpdate = false;
	m_lastServerIP = 0;
	m_lastServerPort = 0;
	m_lastServerName = NULL;
	m_lastServerConnectTime = 0;
	m_pTeamGoalTournament = NULL;

#if defined( _X360 )
	m_pScoreboard = NULL;
#endif
	
#if !defined(NO_STEAM)
	m_CallbackScreenshotRequested.Register( this, &ClientModeTFNormal::OnScreenshotRequested );
#endif

}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeTFNormal::~ClientModeTFNormal()
{
}

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUI( "GameUI" );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Init()
{
	m_pMenuEngyBuild = ( CHudMenuEngyBuild * )GET_HUDELEMENT( CHudMenuEngyBuild );
	Assert( m_pMenuEngyBuild );

	m_pMenuEngyDestroy = ( CHudMenuEngyDestroy * )GET_HUDELEMENT( CHudMenuEngyDestroy );
	Assert( m_pMenuEngyDestroy );

	m_pMenuSpyDisguise = ( CHudMenuSpyDisguise * )GET_HUDELEMENT( CHudMenuSpyDisguise );
	Assert( m_pMenuSpyDisguise );

	m_pFreezePanel = ( CTFFreezePanel * )GET_HUDELEMENT( CTFFreezePanel );
	Assert( m_pFreezePanel );

	m_pQuickSwitch = ( CItemQuickSwitchPanel * )GET_HUDELEMENT( CItemQuickSwitchPanel );
	Assert( m_pQuickSwitch );

	m_pMenuTauntSelection = ( CHudMenuTauntSelection * )GET_HUDELEMENT( CHudMenuTauntSelection );
	Assert( m_pMenuTauntSelection );

	m_pMenuUpgradePanel = ( CHudUpgradePanel* )GET_HUDELEMENT( CHudUpgradePanel );


	m_pMenuSpell = ( CHudSpellMenu * )GET_HUDELEMENT( CHudSpellMenu);
	Assert( m_pMenuSpell );

	m_pEurekaTeleportMenu = ( CHudEurekaEffectTeleportMenu * )GET_HUDELEMENT( CHudEurekaEffectTeleportMenu );
	Assert( m_pEurekaTeleportMenu  );

	m_pTeamGoalTournament = (CHudTeamGoalTournament *)GET_HUDELEMENT( CHudTeamGoalTournament );
	Assert( m_pTeamGoalTournament );

	m_pInspectPanel = (CHudInspectPanel *)GET_HUDELEMENT( CHudInspectPanel );
	Assert( m_pInspectPanel );

	m_wasConnectedLastUpdate = false;

	m_lastServerIP = 0;
	m_lastServerPort = 0;
	m_lastServerName = NULL;
	m_lastServerConnectTime = 0;

	m_flNextAllowedHighFiveHintTime = 0.0f;

	m_bInfoPanelShown = false;
	m_bRestrictInfoPanel = false;

	CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
	if ( gameUIFactory )
	{
		m_pGameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL );
		if ( NULL != m_pGameUI )
		{
			// insert stats summary panel as the loading background dialog
			CTFStatsSummaryPanel *pPanel = GStatsSummaryPanel();
			pPanel->InvalidateLayout( false, true );
			pPanel->SetVisible( false );
			pPanel->MakePopup( false );
			m_pGameUI->SetLoadingBackgroundDialog( pPanel->GetVPanel() );

			IViewPortPanel *pMMOverride = ( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
			if ( pMMOverride )
			{
				((CHudMainMenuOverride*)pMMOverride)->AttachToGameUI();	
			}
		}		
	}

#if defined( _X360 )
	m_pScoreboard = (CTFClientScoreBoardDialog *)( gViewPortInterface->FindPanelByName( PANEL_SCOREBOARD ) );
	Assert( m_pScoreboard );
#endif

	ListenForGameEvent( "localplayer_changeclass" );

#ifdef TF_RAID_MODE
	ListenForGameEvent( "raid_spawn_mob" );
	ListenForGameEvent( "raid_spawn_squad" );
#endif // TF_RAID_MODE
	
	ListenForGameEvent( "player_upgraded" );
	ListenForGameEvent( "player_buyback" );
	ListenForGameEvent( "player_death" );
	ListenForGameEvent( "player_used_powerup_bottle" );

	ListenForGameEvent( "pve_win_panel" );

	ListenForGameEvent( "arena_win_panel" );
	ListenForGameEvent( "teamplay_win_panel" );
	ListenForGameEvent( "server_spawn" );

	ListenForGameEvent( "pumpkin_lord_summoned" );
	ListenForGameEvent( "pumpkin_lord_killed" );
	ListenForGameEvent( "eyeball_boss_summoned" );
	ListenForGameEvent( "eyeball_boss_stunned" );
	ListenForGameEvent( "eyeball_boss_killed" );
	ListenForGameEvent( "eyeball_boss_killer" );
	ListenForGameEvent( "eyeball_boss_escape_imminent" );
	ListenForGameEvent( "eyeball_boss_escaped" );

	ListenForGameEvent( "merasmus_summoned" );
	ListenForGameEvent( "merasmus_killed" );
	ListenForGameEvent( "merasmus_escape_warning" );
	ListenForGameEvent( "merasmus_escaped" );

	ListenForGameEvent( "player_highfive_start" );
	ListenForGameEvent( "player_highfive_cancel" );
	ListenForGameEvent( "player_highfive_success" );

	ListenForGameEvent( "client_beginconnect" );
	ListenForGameEvent( "client_disconnect" );

	ListenForGameEvent( "player_teleported" );
	ListenForGameEvent( "scorestats_accumulated_reset" );
	ListenForGameEvent( "scorestats_accumulated_update" );

	extern void Training_Init();
	Training_Init();

	BaseClass::Init();

	m_bPendingRichPresenceUpdate = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void ClientModeTFNormal::Shutdown()
{
	if ( tf_delete_temp_files.GetBool() )
	{
		RemoveFilesInPath( "materials/temp" );
		RemoveFilesInPath( "download/user_custom" );
		RemoveFilesInPath( "sound/temp" );
	}

	DestroyStatsSummaryPanel();
}

void ClientModeTFNormal::InitViewport()
{
	m_pViewport = new TFViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}


void ClientModeTFNormal::LevelInit( const char *newmap )
{
	BaseClass::LevelInit( newmap );

	m_bInfoPanelShown = false;
}

IClientMode *GetClientModeNormal()
{
	static ClientModeTFNormal g_ClientModeNormal;
	
	return &g_ClientModeNormal;
}


ClientModeTFNormal* GetClientModeTFNormal()
{
	Assert( dynamic_cast< ClientModeTFNormal* >( GetClientModeNormal() ) );

	return static_cast< ClientModeTFNormal* >( GetClientModeNormal() );
}

extern ConVar v_viewmodel_fov;
ConVar v_viewmodel_fov_demo( "viewmodel_fov_demo", "54", FCVAR_ARCHIVE );
float ClientModeTFNormal::GetViewModelFOV( void )
{
	// If we're playing back a demo, we clamp the viewmodel fov
	if ( engine->IsPlayingDemo() )
		return v_viewmodel_fov_demo.GetFloat();

	return v_viewmodel_fov.GetFloat();
}

extern ConVar r_drawviewmodel;
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::ShouldDrawViewModel()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
			return false;
	}

	if ( !r_drawviewmodel.GetBool() )
		return false;

	return true;
}

ConVar tf_hud_no_crosshair_on_scope_zoom( "tf_hud_no_crosshair_on_scope_zoom", "0", FCVAR_CLIENTDLL | FCVAR_ARCHIVE );

bool ClientModeTFNormal::ShouldDrawCrosshair()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return false;

	if ( pPlayer->GetPlayerClass() &&
		 pPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_SNIPER &&
		 pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) &&
		 tf_hud_no_crosshair_on_scope_zoom.GetBool() )
	{
		return false;
	}

	return ClientModeShared::ShouldDrawCrosshair();
}


//-----------------------------------------------------------------------------
// Purpose: Returns true if VR mode should black out everything outside the HUD.
//			This is used for things like sniper scopes and full screen UI
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::ShouldBlackoutAroundHUD()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return true;

	return ClientModeShared::ShouldBlackoutAroundHUD();
}

	
//-----------------------------------------------------------------------------
// Purpose: Allows the client mode to override mouse control stuff in sourcevr
//-----------------------------------------------------------------------------
HeadtrackMovementMode_t ClientModeTFNormal::ShouldOverrideHeadtrackControl() 
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer )
		return HMM_NOOVERRIDE;

	// TODO: check if these actually all work right.
	switch ( pPlayer->GetObserverMode() )
	{
		case OBS_MODE_DEATHCAM	:		// Turned into OBS_MODE_CHASE in VR
		case OBS_MODE_ROAMING	:
		case OBS_MODE_FIXED		:		// Checked - works.
		case OBS_MODE_CHASE		:		// Checked - works.
		case OBS_MODE_POI		:		// PASSTIME NOT CHECKED
		case OBS_MODE_FREEZECAM	:		// Turned into OBS_MODE_CHASE in VR
			return HMM_SHOOTMOVEMOUSE_LOOKFACE;
		case OBS_MODE_IN_EYE	:		// Checked - works.
			return HMM_NOOVERRIDE;
	}

	return ClientModeShared::ShouldOverrideHeadtrackControl();
}


int ClientModeTFNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

void ClientModeTFNormal::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if ( !eventname || !eventname[0] )
		return;

	CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
	if ( FStrEq( "player_changename", eventname ) )
	{
		return; // server sends a colorized text string for this
	}
	else if ( FStrEq( "localplayer_changeclass", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && pLocalPlayer->GetPlayerClass() )
		{
			int iClass = pLocalPlayer->GetPlayerClass()->GetClassIndex();

			// have the player to exec a <class>.cfg file for the class they have selected
			char szCmd[128];
			Q_snprintf( szCmd, sizeof( szCmd ), "exec %s.cfg", GetPlayerClassData( iClass )->m_szClassName );
			engine->ExecuteClientCmd( szCmd ); 
		}
	}
	
#ifdef TF_RAID_MODE
	else if ( FStrEq( "raid_spawn_mob", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Raid.MobSpawn" );
		}
	}
	else if ( FStrEq( "raid_spawn_squad", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Raid.SquadSpawn" );
		}
	}
#endif // TF_RAID_MODE
	else if ( FStrEq( "player_connect_client", eventname ) || FStrEq( "player_disconnect", eventname ) )
	{
		// ignore these
		if ( TFGameRules() && TFGameRules()->IsPVEModeActive() && event->GetInt( "bot" ) != 0 )
			return;
	}
	else if ( FStrEq( "client_disconnect", eventname ) )
	{
		m_eConnectState = k_eConnectState_Disconnected;
		m_szMapBaseName[0] = '\0';
		m_bPendingRichPresenceUpdate = true;
#if !defined( _X360 ) && !defined( NO_STEAM )
		if ( SteamTimeline() )
			SteamTimeline()->ClearTimelineStateDescription( 0 );
#endif
	}
	else if ( FStrEq( "server_cvar", eventname ) )
	{
		if ( TFGameRules() && TFGameRules()->IsPVEModeActive() && !Q_strcmp( event->GetString("cvarname"), "tf_bot_count" ) )
			return;
	}
	else if ( FStrEq( "player_buyback", eventname ) )
	{
		int idxPlayer = event->GetInt( "player" );
		KeyValuesAD pKeyValues( "data" );
		if ( g_TF_PR )
		{
			const char *pszString = tf_mvm_buybacks_method.GetBool() ? "#TF_PVE_Player_BuyBack_Fixed" : "#TF_PVE_Player_BuyBack";

			pKeyValues->SetString( "player", g_TF_PR->GetPlayerName( idxPlayer ) );
			pKeyValues->SetInt( "credits", event->GetInt( "cost", 0 ) );
			PrintTextToChatPlayer( idxPlayer, pszString, pKeyValues );

			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				pLocalPlayer->EmitSound( "MVM.PlayerBoughtIn" );
			}
		}
	}
	else if ( FStrEq( "player_death", eventname ) )
	{
		// Make sure they're not doing a dead ringer fake death
		if ( ( event->GetInt( "death_flags" ) & TF_DEATH_FEIGN_DEATH ) == 0 )
		{
			if ( TFGameRules() && ( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING ) && ( TFGameRules()->IsMannVsMachineMode() || TFGameRules()->IsCompetitiveMode() ) )
			{
				C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
				if ( pLocalPlayer )
				{
					int nVictimIndex = event->GetInt( "victim_entindex" );
					int nVictimTeam = g_TF_PR->GetTeam( nVictimIndex );

					// See if there are any other players still alive
					bool bSomeAlive = false;
					for ( int playerIndex = 1; playerIndex <= MAX_PLAYERS; playerIndex++ )
					{
						if ( !g_TF_PR->IsConnected( playerIndex ) )
							continue;

						if ( nVictimIndex == playerIndex )
							continue;

						if ( nVictimTeam != g_TF_PR->GetTeam( playerIndex ) )
							continue;

						if ( g_TF_PR->IsAlive( playerIndex ) )
						{
							// Found one
							bSomeAlive = true;
							break;
						}
					}

					if ( !bSomeAlive )
					{
						if ( TFGameRules()->IsMannVsMachineMode() )
						{
							if ( nVictimTeam == TF_TEAM_PVE_DEFENDERS )
							{
								// Inform the team that everyone is dead
								pLocalPlayer->EmitSound( "Announcer.MVM_All_Dead" );
							}
						}
						else
						{
							const char *pszSound = NULL;

							if ( ( RandomInt( 1, 100 ) <= 20 ) || ( pLocalPlayer->GetTeamNumber() < FIRST_GAME_TEAM ) )
							{
								pszSound = ( nVictimTeam == TF_TEAM_RED ) ? "Announcer.TeamWipeRed" : "Announcer.TeamWipeBlu";
							}
							else if ( pLocalPlayer->GetTeamNumber() == nVictimTeam )
							{
								pszSound = "Announcer.YourTeamWiped";
							}
							else
							{
								pszSound = "Announcer.TheirTeamWiped";
							}

							if ( pszSound )
							{
								pLocalPlayer->EmitSound( pszSound );
							}
						}
					}
				}
			}
		}
	}
	else if ( FStrEq( "player_used_powerup_bottle", eventname ) )
	{
		int idxPlayer = event->GetInt( "player" );
		KeyValuesAD pKeyValues( "data" );
		if ( g_TF_PR )
		{
			pKeyValues->SetString( "player", g_TF_PR->GetPlayerName( idxPlayer ) );
			const char *pText = NULL;
			switch ( event->GetInt( "type" ) )
			{
			case POWERUP_BOTTLE_CRITBOOST: pText = "#TF_PVE_Player_UsedCritsBottle"; break;
			case POWERUP_BOTTLE_UBERCHARGE: pText = "#TF_PVE_Player_UsedUberBottle"; break;
			case POWERUP_BOTTLE_RECALL: pText = "#TF_PVE_Player_UsedRecallBottle"; break;
			case POWERUP_BOTTLE_REFILL_AMMO: pText = "#TF_PVE_Player_UsedRefillAmmoBottle"; break;
			case POWERUP_BOTTLE_BUILDINGS_INSTANT_UPGRADE: pText = "#TF_PVE_Player_UsedBuildingUpgrade"; break;
			case POWERUP_BOTTLE_RADIUS_STEALTH: pText = "#TF_PVE_Player_UsedRadiusStealth"; break;
			}
			if ( pText != NULL )
			{
				PrintTextToChatPlayer( idxPlayer, pText, pKeyValues );
			}

			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				pLocalPlayer->EmitSound( "MVM.PlayerUsedPowerup" );
			}
		}
	}
	else if ( 
			  FStrEq( "pve_win_panel", eventname ) ||
			  FStrEq( "arena_win_panel", eventname ) ||
			  FStrEq( "teamplay_win_panel", eventname )  )
	{
#if defined( REPLAY_ENABLED )
		DisplayReplayReminder();
#endif
	}
	else if ( FStrEq( "server_spawn", eventname ) )
	{
		m_eConnectState = k_eConnectState_Connected;
		V_strncpy( m_szMapBaseName, event->GetString( "mapname" ), sizeof( m_szMapBaseName ) );

		uint32 newServerIP = 0;
		int newServerPort = event->GetInt( "port" );

		const char *pzAddress = event->GetString( "address" );
		if ( pzAddress )
		{
			CUtlStringList IPs;
			V_SplitString( pzAddress, ".", IPs );

			if ( IPs.Count() == 4 )
			{
				byte ip[4] = { 0 };
				for ( int i=0; i<IPs.Count() && i<4; ++i )
				{
					ip[i] = (byte) Q_atoi( IPs[i] );
				}
				newServerIP = (ip[0]<<24) + (ip[1]<<16) + (ip[2]<<8) + ip[3];
			}
		}

		if ( m_lastServerConnectTime == 0 || newServerIP != m_lastServerIP || newServerPort != m_lastServerPort )
		{
			// we are connecting, or have connected to a different server
			m_lastServerIP = newServerIP;
			m_lastServerPort = newServerPort;

			const char *hostname = event->GetString( "hostname" );
			if ( hostname )
			{
				if ( m_lastServerName )
				{
					delete [] m_lastServerName;
					m_lastServerName = NULL;
				}

				int hostnameLength = V_strlen( hostname )+1;

				m_lastServerName = new char[ hostnameLength ];

				V_strncpy( m_lastServerName, hostname, hostnameLength );
			}

			m_lastServerConnectTime = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch();
		}

		m_flNextAllowedHighFiveHintTime = 0.0f;

		// Play Sound and flash window if joining a game from a lobby
		CTFGSLobby *pLobby = GTFGCClientSystem()->GetLobby();
		if ( pLobby )
		{
			engine->FlashWindow();

			{
				// If minimized, Blink and play noise
				if ( engine->IsActiveApp() )
				{
					vgui::surface()->PlaySound( "ui/vote_started.wav" );
				}
				else
				{
					char fullpath[ 512 ];
					g_pFullFileSystem->RelativePathToFullPath( "sound/ui/vote_started.wav", "GAME", fullpath, sizeof( fullpath ) );
					PlayOutOfGameSound( fullpath );
				}
			}
		}

		m_bPendingRichPresenceUpdate = true;
	}
	else if ( FStrEq( "pumpkin_lord_summoned", eventname ) )
	{
		if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				pLocalPlayer->EmitSound( "Halloween.HeadlessBossSpawn" );
			}

			CEconNotification *pNotification = new CEconNotification();
			pNotification->SetText( "#TF_Halloween_Boss_Appeared" );
			pNotification->SetLifetime( 5.0f );
			pNotification->SetSoundFilename( "vo/null.mp3" );
			NotificationQueue_Add( pNotification );
		}
	}
	else if ( FStrEq( "pumpkin_lord_killed", eventname ) )
	{
		if ( !TFGameRules()->IsHalloweenScenario( CTFGameRules::HALLOWEEN_SCENARIO_DOOMSDAY ) )
		{
			C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
			if ( pLocalPlayer )
			{
				pLocalPlayer->EmitSound( "Halloween.HeadlessBossDeath" );
			}

			CEconNotification *pNotification = new CEconNotification();
			pNotification->SetText( "#TF_Halloween_Boss_Killed" );
			pNotification->SetLifetime( 5.0f );
			pNotification->SetSoundFilename( "vo/null.mp3" );
			NotificationQueue_Add( pNotification );
		}
	}
	else if ( FStrEq( "eyeball_boss_summoned", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Halloween.MonoculusBossSpawn" );
		}

		KeyValuesAD keyValues( "eyeball_boss" );
		keyValues->SetColor( "custom_color", colorEyeballBossText );

		CEconNotification *pNotification = new CEconNotification();

		const int iLevel = event->GetInt( "level" );

		if ( iLevel > 1 )
		{
			wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "TF_Halloween_Eyeball_Boss_LevelUp_Appeared" );
			if ( pszBaseString )
			{
				KeyValuesAD pKeyValues( "data" );			
				pKeyValues->SetInt( "level", iLevel );

				wchar_t	wTemp[256];
				g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, pKeyValues );

				static char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wTemp, szAnsi, sizeof(szAnsi) );

				pNotification->SetText( szAnsi );
			}
		}
		else
		{
			pNotification->SetText( "#TF_Halloween_Eyeball_Boss_Appeared" );
		}

		pNotification->SetLifetime( 5.0f );
		pNotification->SetSoundFilename( "vo/null.mp3" );
		pNotification->SetKeyValues( keyValues );
		NotificationQueue_Add( pNotification );
	}
	else if ( FStrEq( "eyeball_boss_stunned", eventname ) )
	{
		int iPlayerIndex = event->GetInt( "player_entindex" );
		if ( pHUDChat )
		{
			KeyValuesAD pKeyValues( "data" );
			if ( g_TF_PR )
			{
				pKeyValues->SetString( "player", g_TF_PR->GetPlayerName( iPlayerIndex ) );

				pHUDChat->SetCustomColor( colorEyeballBossText );

				const int iLevel = event->GetInt( "level" );
				if ( iLevel > 1 )
				{
					pKeyValues->SetInt( "level", iLevel );
					PrintTextToChatPlayer( iPlayerIndex, "#TF_Halloween_Eyeball_Boss_LevelUp_Stun", pKeyValues );
				}
				else
				{
					PrintTextToChatPlayer( iPlayerIndex, "#TF_Halloween_Eyeball_Boss_Stun", pKeyValues );
				}
			}
		}
	}
	else if ( FStrEq( "eyeball_boss_killed", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Halloween.MonoculusBossDeath" );
		}

		KeyValuesAD keyValues( "eyeball_boss" );
		keyValues->SetColor( "custom_color", colorEyeballBossText );

		CEconNotification *pNotification = new CEconNotification();

		const int iLevel = event->GetInt( "level" );

		if ( iLevel > 1 )
		{
			wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "TF_Halloween_Eyeball_Boss_LevelUp_Killed" );
			if ( pszBaseString )
			{
				KeyValuesAD pKeyValues( "data" );			
				pKeyValues->SetInt( "level", iLevel );

				wchar_t	wTemp[256];
				g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, pKeyValues );

				static char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wTemp, szAnsi, sizeof(szAnsi) );

				pNotification->SetText( szAnsi );
			}
		}
		else
		{
			pNotification->SetText( "#TF_Halloween_Eyeball_Boss_Killed" );
		}

		pNotification->SetLifetime( 5.0f );
		pNotification->SetSoundFilename( "vo/null.mp3" );
		pNotification->SetKeyValues( keyValues );
		NotificationQueue_Add( pNotification );
	}
	else if ( FStrEq( "eyeball_boss_killer", eventname ) )
	{
		int iPlayerIndex = event->GetInt( "player_entindex" );
		if ( pHUDChat )
		{
			KeyValuesAD pKeyValues( "data" );
			if ( g_TF_PR )
			{
				pKeyValues->SetString( "player", g_TF_PR->GetPlayerName( iPlayerIndex ) );

				pHUDChat->SetCustomColor( colorEyeballBossText );

				const int iLevel = event->GetInt( "level" );

				if ( iLevel > 1 )
				{
					pKeyValues->SetInt( "level", iLevel );

					PrintTextToChatPlayer( iPlayerIndex, "#TF_Halloween_Eyeball_Boss_LevelUp_Killers", pKeyValues );
				}
				else
				{
					PrintTextToChatPlayer( iPlayerIndex, "#TF_Halloween_Eyeball_Boss_Killers", pKeyValues );
				}
			}
		}
	}
	else if ( FStrEq( "eyeball_boss_escape_imminent", eventname ) )
	{
		int nSecondsRemaining = event->GetInt( "time_remaining" );

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			if ( nSecondsRemaining <= 10 )
				pLocalPlayer->EmitSound( "Halloween.EyeballBossEscapeImminent" );
			else
				pLocalPlayer->EmitSound( "Halloween.EyeballBossEscapeSoon" );
		}

		if ( pHUDChat )
		{
			char szEyeballBossEscaping[128];
			pHUDChat->SetCustomColor( colorEyeballBossText );

			const int iLevel = event->GetInt( "level" );

			if ( iLevel > 1 )
			{
				KeyValuesAD pKeyValues( "data" );
				pKeyValues->SetInt( "level", iLevel );

				V_sprintf_safe( szEyeballBossEscaping, "#TF_Halloween_Eyeball_Boss_LevelUp_Escaping_In_%i", nSecondsRemaining );

				PrintTextToChat( szEyeballBossEscaping, pKeyValues );
			}
			else
			{
				V_sprintf_safe( szEyeballBossEscaping, "#TF_Halloween_Eyeball_Boss_Escaping_In_%i", nSecondsRemaining );

				PrintTextToChat( szEyeballBossEscaping );
			}
		}
	}
	else if ( FStrEq( "eyeball_boss_escaped", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Halloween.EyeballBossEscaped" );
		}

		KeyValuesAD keyValues( "eyeball_boss" );
		keyValues->SetColor( "custom_color", colorEyeballBossText );

		CEconNotification *pNotification = new CEconNotification();

		const int iLevel = event->GetInt( "level" );

		if ( iLevel > 1 )
		{
			wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "TF_Halloween_Eyeball_Boss_LevelUp_Escaped" );
			if ( pszBaseString )
			{
				KeyValuesAD pKeyValues( "data" );			
				pKeyValues->SetInt( "level", iLevel );

				wchar_t	wTemp[256];
				g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, pKeyValues );

				static char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wTemp, szAnsi, sizeof(szAnsi) );

				pNotification->SetText( szAnsi );
			}
		}
		else
		{
			pNotification->SetText( "#TF_Halloween_Eyeball_Boss_Escaped" );
		}

		pNotification->SetLifetime( 5.0f );
		pNotification->SetSoundFilename( "vo/null.mp3" );
		pNotification->SetKeyValues( keyValues );
		NotificationQueue_Add( pNotification );
	}
	else if ( FStrEq( "merasmus_summoned", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Halloween.MerasmusBossSpawn" );
		}

		KeyValuesAD keyValues( "merasmus" );
		keyValues->SetColor( "custom_color", colorMerasmusText );

		CEconNotification *pNotification = new CEconNotification();

		const int iLevel = event->GetInt( "level" );

		if ( iLevel > 1 )
		{
			wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "TF_Halloween_Merasmus_LevelUp_Appeared" );
			if ( pszBaseString )
			{
				KeyValuesAD pKeyValues( "data" );			
				pKeyValues->SetInt( "level", iLevel );

				wchar_t	wTemp[256];
				g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, pKeyValues );

				static char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wTemp, szAnsi, sizeof(szAnsi) );

				pNotification->SetText( szAnsi );
			}
		}
		else
		{
			pNotification->SetText( "#TF_Halloween_Merasmus_Appeared" );
		}

		pNotification->SetLifetime( 5.0f );
		pNotification->SetSoundFilename( "vo/null.mp3" );
		pNotification->SetKeyValues( keyValues );
		NotificationQueue_Add( pNotification );
	}
	else if ( FStrEq( "merasmus_killed", eventname ) )
	{
		KeyValuesAD keyValues( "merasmus" );
		keyValues->SetColor( "custom_color", colorMerasmusText );

		CEconNotification *pNotification = new CEconNotification();

		const int iLevel = event->GetInt( "level" );

		if ( iLevel > 1 )
		{
			wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "TF_Halloween_Merasmus_LevelUp_Killed" );
			if ( pszBaseString )
			{
				KeyValuesAD pKeyValues( "data" );			
				pKeyValues->SetInt( "level", iLevel );

				wchar_t	wTemp[256];
				g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, pKeyValues );

				static char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wTemp, szAnsi, sizeof(szAnsi) );

				pNotification->SetText( szAnsi );
			}
		}
		else
		{
			pNotification->SetText( "#TF_Halloween_Merasmus_Killed" );
		}

		pNotification->SetLifetime( 5.0f );
		pNotification->SetSoundFilename( "vo/null.mp3" );
		pNotification->SetKeyValues( keyValues );
		NotificationQueue_Add( pNotification );
	}
	else if ( FStrEq( "merasmus_escape_warning", eventname ) )
	{
		int nSecondsRemaining = event->GetInt( "time_remaining" );

		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			if ( nSecondsRemaining <= 10 )
				pLocalPlayer->EmitSound( "Halloween.EyeballBossEscapeImminent" );
			else
				pLocalPlayer->EmitSound( "Halloween.EyeballBossEscapeSoon" );
		}

		if ( pHUDChat )
		{
			char szEyeballBossEscaping[128];
			pHUDChat->SetCustomColor( colorMerasmusText );

			const int iLevel = event->GetInt( "level" );

			if ( iLevel > 1 )
			{
				KeyValuesAD pKeyValues( "data" );
				pKeyValues->SetInt( "level", iLevel );

				V_sprintf_safe( szEyeballBossEscaping, "#TF_Halloween_Merasmus_LevelUp_Escaping_In_%i", nSecondsRemaining );

				PrintTextToChat( szEyeballBossEscaping, pKeyValues );
			}
			else
			{
				V_sprintf_safe( szEyeballBossEscaping, "#TF_Halloween_Merasmus_Escaping_In_%i", nSecondsRemaining );

				PrintTextToChat( szEyeballBossEscaping );
			}
		}
	}
	else if ( FStrEq( "merasmus_escaped", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			pLocalPlayer->EmitSound( "Halloween.EyeballBossEscaped" );
		}

		KeyValuesAD keyValues( "merasmus" );
		keyValues->SetColor( "custom_color", colorMerasmusText );

		CEconNotification *pNotification = new CEconNotification();

		const int iLevel = event->GetInt( "level" );

		if ( iLevel > 1 )
		{
			wchar_t	*pszBaseString = g_pVGuiLocalize->Find( "TF_Halloween_Merasmus_LevelUp_Escaped" );
			if ( pszBaseString )
			{
				KeyValuesAD pKeyValues( "data" );			
				pKeyValues->SetInt( "level", iLevel );

				wchar_t	wTemp[256];
				g_pVGuiLocalize->ConstructString_safe( wTemp, pszBaseString, pKeyValues );

				static char szAnsi[1024];
				g_pVGuiLocalize->ConvertUnicodeToANSI( wTemp, szAnsi, sizeof(szAnsi) );

				pNotification->SetText( szAnsi );
			}
		}
		else
		{
			pNotification->SetText( "#TF_Halloween_Merasmus_Escaped" );
		}

		pNotification->SetLifetime( 5.0f );
		pNotification->SetSoundFilename( "vo/null.mp3" );
		pNotification->SetKeyValues( keyValues );
		NotificationQueue_Add( pNotification );
	}
	else if ( FStrEq( "player_highfive_start", eventname ) )
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

		// Don't show a hint if we're dead, we've already done it the maximum amount of times, or if it's too soon.
		if ( pLocalPlayer && pLocalPlayer->IsAlive() &&
				( tf_taunt_always_show_hint.GetBool() ||
				( tf_highfive_hintcount.GetInt() < TF_HIGHFIVE_HINT_MAXHINTS && gpGlobals->curtime > m_flNextAllowedHighFiveHintTime ) ) )
		{
			int entindex = event->GetInt( "entindex" );
			C_BasePlayer *pHighFiveInitiator = UTIL_PlayerByIndex( entindex );
			if ( pHighFiveInitiator && pHighFiveInitiator != pLocalPlayer && ( pHighFiveInitiator->GetTeamNumber() == pLocalPlayer->GetTeamNumber() || tf_allow_all_team_partner_taunt.GetBool() ) )
			{
				// check that this player isn't too far away
				Vector vecStart = pLocalPlayer->EyePosition();
				Vector vecEnd = pHighFiveInitiator->EyePosition();
				float flLength = (vecEnd - vecStart).Length();

				if ( flLength < TF_HIGHFIVE_HINT_MAXDIST )
				{
					// check that we have line of sight to this player
					trace_t tr;
					UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, pLocalPlayer, COLLISION_GROUP_NONE, &tr );

					if ( !tr.startsolid && tr.m_pEnt != NULL && tr.m_pEnt == pHighFiveInitiator )
					{
						IGameEvent *pEvent = gameeventmanager->CreateEvent( "show_annotation" );
						if ( pEvent )
						{
							Vector location = pHighFiveInitiator->GetAbsOrigin();

							pEvent->SetString( "text", "#TF_HighFive_Hint" );
							pEvent->SetInt( "id", TF_HIGHFIVE_HINT_MASK | entindex );
							pEvent->SetFloat( "worldPosX", location.x );
							pEvent->SetFloat( "worldPosY", location.y );
							pEvent->SetFloat( "worldPosZ", location.z + 48.0f );
							pEvent->SetFloat( "lifetime", 10.0f );
							pEvent->SetInt( "follow_entindex", entindex );

							gameeventmanager->FireEventClientSide( pEvent );
							tf_highfive_hintcount.SetValue( tf_highfive_hintcount.GetInt() + 1 );

							m_flNextAllowedHighFiveHintTime = gpGlobals->curtime + TF_HIGHFIVE_HINT_MINTIMEBETWEEN;
						}
					}
				}
			}
		}
	}
	else if ( FStrEq( "player_highfive_cancel", eventname ) )
	{
		int entindex = event->GetInt( "entindex" );
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "hide_annotation" );
		if ( pEvent )
		{
			pEvent->SetInt( "id", TF_HIGHFIVE_HINT_MASK | entindex );
			gameeventmanager->FireEventClientSide( pEvent );
		}
	}
	else if ( FStrEq( "player_highfive_success", eventname ) )
	{
		int entindex = event->GetInt( "initiator_entindex" );
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "hide_annotation" );
		if ( pEvent )
		{
			pEvent->SetInt( "id", TF_HIGHFIVE_HINT_MASK | entindex );
			gameeventmanager->FireEventClientSide( pEvent );
		}
	}
	else if ( FStrEq( "client_beginconnect", eventname ) )
	{
		m_eConnectState = k_eConnectState_Connecting;
		m_bPendingRichPresenceUpdate = true;
		const char *pchSource = event->GetString( "source" );
		m_bRestrictInfoPanel = pchSource && ( FStrEq( "matchmaking", pchSource ) || !Q_strncmp( pchSource, "quickplay_", 10 ) );

		m_bInfoPanelShown = false;
	}
	else if ( FStrEq( "player_teleported", eventname ) )
	{
		int iUserID = event->GetInt( "userid" );
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if( pLocalPlayer && pLocalPlayer->GetUserID() == iUserID && UseVR() )
		{
			g_ClientVirtualReality.AlignTorsoAndViewToWeapon();
		}
	}
	else if ( FStrEq( "scorestats_accumulated_reset", eventname ) )
	{
		if ( g_TF_PR && TFGameRules() && !TFGameRules()->IsMannVsMachineMode() )
		{
			g_TF_PR->ResetPlayerScoreStats();
		}
	}
	else if ( FStrEq( "scorestats_accumulated_update", eventname ) )
	{
		if ( g_TF_PR && TFGameRules() && !TFGameRules()->IsMannVsMachineMode() )
		{
			g_TF_PR->UpdatePlayerScoreStats();
		}
	}

	BaseClass::FireGameEvent( event );
}

void ClientModeTFNormal::PostRenderVGui()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::CreateMove( float flInputSampleTime, CUserCmd *cmd )
{
	return BaseClass::CreateMove( flInputSampleTime, cmd );
}

//-----------------------------------------------------------------------------
// Purpose: See if hud elements want key input. Return 0 if the key is swallowed
//-----------------------------------------------------------------------------
int	ClientModeTFNormal::HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
	// Let scoreboard handle input first because on X360 we need gamertags and
	// gamercards accessible at all times when gamertag is visible.
#if defined( _X360 )
	if ( m_pScoreboard )
	{
		if ( !m_pScoreboard->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}
#endif

	// Applies basic tags if we're going to take a screenshot
	ScreenshotTaggingKeyInput( down, keynum, pszCurrentBinding );

	if ( TrainingHandlesKeyInput( down, keynum, pszCurrentBinding ) )
	{
		return 0;
	}

	if ( CoachingHandlesKeyInput( down, keynum, pszCurrentBinding ) )
	{
		return 0;
	}

	if ( ItemTestHandlesKeyInput( down, keynum, pszCurrentBinding ) )
	{
		return 0;
	}

	if ( HalloweenHandlesKeyInput( down, keynum, pszCurrentBinding ) )
	{
		return 0;
	}

	if ( TauntHandlesKeyInput( down, keynum, pszCurrentBinding ) )
	{
		return 0;
	}

	// check for hud menus
	if ( m_pMenuEngyBuild )
	{
		if ( !m_pMenuEngyBuild->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pMenuEngyDestroy )
	{
		if ( !m_pMenuEngyDestroy->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pMenuSpyDisguise )
	{
		if ( !m_pMenuSpyDisguise->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pFreezePanel )
	{
		m_pFreezePanel->HudElementKeyInput( down, keynum, pszCurrentBinding );
	}

	if ( m_pQuickSwitch )
	{
		if ( !m_pQuickSwitch->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pMenuTauntSelection )
	{
		if ( !m_pMenuTauntSelection->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}


	if ( m_pEurekaTeleportMenu )
	{
		if ( !m_pEurekaTeleportMenu->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pInspectPanel )
	{
		if ( !m_pInspectPanel->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( m_pTeamGoalTournament )
	{
		if ( !m_pTeamGoalTournament->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
		{
			return 0;
		}
	}

	if ( TournamentHudElementKeyInput( down, keynum, pszCurrentBinding ) == true )
		return 0;

	if ( ArenaClassLayoutKeyInput( down, keynum, pszCurrentBinding ) == true )
		return 0;

#ifndef _X360
	if ( ShouldScoreBoardHandleKeyInput( down, keynum, pszCurrentBinding ) )
		return 0;
#endif // !360

	return BaseClass::HudElementKeyInput( down, keynum, pszCurrentBinding );
}

//-----------------------------------------------------------------------------
// Purpose: See if spectator input occurred. Return 0 if the key is swallowed.
//-----------------------------------------------------------------------------
int ClientModeTFNormal::HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
{
#if defined( _X360 )
	// On X360 when we have scoreboard up in spectator menu we cannot
	// steal any input because gamertags must be selectable and gamercards
	// must be accessible.
	// We cannot rely on any keybindings in this case since user could have
	// remapped everything.
	if ( m_pScoreboard && m_pScoreboard->IsVisible() )
	{
		return 1;
	}
#endif

	// @note Tom Bui: Coaching, so override all input
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();	
	if ( pLocalPlayer && pLocalPlayer->m_bIsCoaching  )
	{
		if ( down && pszCurrentBinding && Q_strcmp( pszCurrentBinding, "+jump" ) == 0 )
		{
			engine->ClientCmd( "spec_mode" );
			return 0;
		}
		return 1;
	}
	else
	{
		return BaseClass::HandleSpectatorKeyInput( down, keynum, pszCurrentBinding );
	}
}

bool ClientModeTFNormal::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	if ( !BaseClass::DoPostScreenSpaceEffects( pSetup ) )
		return false;
	
	if( IsInFreezeCam() )
		return false;

	g_GlowObjectManager.RenderGlowEffects( pSetup, 0 );

	return true;
}

#if !defined(NO_STEAM)
void ClientModeTFNormal::OnScreenshotRequested( ScreenshotRequested_t *pParam )
{
	// Steam has requested a screenshot, act as if the key currently bound to screenshots
	// has been pressed (we want tagging and the killcam screenshot behavior if applicable)
	HudElementKeyInput( 0, BUTTON_CODE_INVALID, "screenshot" );
	engine->ClientCmd( "screenshot" );
}
#endif



//-----------------------------------------------------------------------------------------
ConVar cl_ask_favorite_min_session_duration( "cl_ask_favorite_min_session_duration", "600", 0, "If player stays on a server for longer than this time (in seconds) prompt to add server to favorites" );
ConVar cl_ask_favorite_opt_out( "cl_ask_favorite_opt_out", "0", FCVAR_ARCHIVE, "If nonzero, don't auto-ask to favorite servers" );
ConVar cl_ask_favorite_for_any_server( "cl_ask_favorite_for_any_server", "0", 0, "If nonzero, auto-ask for local/LAN servers (for debugging)" );


ConVar cl_ask_blacklist_max_session_duration( "cl_ask_blacklist_max_session_duration", "60", 0, "If player stays on a server for less than this time (in seconds) prompt to add server to blacklist" );
ConVar cl_ask_blacklist_opt_out( "cl_ask_blacklist_opt_out", "0", FCVAR_ARCHIVE, "If nonzero, don't auto-ask to blacklist servers" );
ConVar cl_ask_blacklist_for_any_server( "cl_ask_blacklist_for_any_server", "0", 0, "If nonzero, auto-ask for local/LAN servers (for debugging)" );


//----------------------------------------------------------------------------
void OnAskFavoriteDialogButtonPressed( bool bConfirm, void *pContext )
{
	if ( bConfirm )
	{
		// add last server to our favorites
		steamapicontext->SteamMatchmaking()->AddFavoriteGame( steamapicontext->SteamUtils()->GetAppID(),
															  GetClientModeTFNormal()->GetLastConnectedServerIP(),
															  GetClientModeTFNormal()->GetLastConnectedServerPort(),
															  GetClientModeTFNormal()->GetLastConnectedServerPort(),
															  k_unFavoriteFlagFavorite, 
															  CRTime::RTime32TimeCur() );

		// Send it to the GC
		GCSDK::CGCMsg< MsgGCServerBrowser_Server_t > msg( k_EMsgGCServerBrowser_FavoriteServer );
		msg.Body().m_unIP = GetClientModeTFNormal()->GetLastConnectedServerIP();
		msg.Body().m_usPort = GetClientModeTFNormal()->GetLastConnectedServerPort();
		msg.Body().m_ubSource = k_EGCMsgServerBrowser_FromAutoAskDialog;
		GCClientSystem()->BSendMessage( msg );
	}
}


//----------------------------------------------------------------------------
void OnAskBlacklistDialogButtonPressed( bool bConfirm, void *pContext )
{
	if ( bConfirm )
	{
		// add last server to our blacklist
		CBlacklistedServerManager blackList;
		blackList.LoadServersFromFile( BLACKLIST_DEFAULT_SAVE_FILE, false );

		blackList.AddServer( GetClientModeTFNormal()->GetLastConnectedServerName(),
							 GetClientModeTFNormal()->GetLastConnectedServerIP(),
							 GetClientModeTFNormal()->GetLastConnectedServerPort() );

		blackList.SaveToFile( BLACKLIST_DEFAULT_SAVE_FILE );

		// Send it to the GC
		GCSDK::CGCMsg< MsgGCServerBrowser_Server_t > msg( k_EMsgGCServerBrowser_BlacklistServer );
		msg.Body().m_unIP = GetClientModeTFNormal()->GetLastConnectedServerIP();
		msg.Body().m_usPort = GetClientModeTFNormal()->GetLastConnectedServerPort();
		msg.Body().m_ubSource = k_EGCMsgServerBrowser_FromAutoAskDialog;
		GCClientSystem()->BSendMessage( msg );
	}
}


//----------------------------------------------------------------------------
// If conditions are right, prompt the user to either favorite or blacklist
// the last server they had connected to.
void ClientModeTFNormal::AskFavoriteOrBlacklist() const
{
	// based on the time we spent on our last server, ask to favorite or blacklist it
	int sessionDuration = GetSteamWorksSGameStatsUploader().GetTimeSinceEpoch() - m_lastServerConnectTime;

	if ( sessionDuration > 0 )
	{
		// favorite?
		if ( !cl_ask_favorite_opt_out.GetBool() && sessionDuration > cl_ask_favorite_min_session_duration.GetFloat() )
		{
			// don't ask to favorite reserved addresses
			if ( !cl_ask_favorite_for_any_server.GetBool() )
			{
				netadr_t netAdr( GetLastConnectedServerIP(), GetLastConnectedServerPort() );

				if ( !netAdr.IsValid() || netAdr.IsReservedAdr() )
					return;

				// Don't offer this for Valve servers, either
				if ( GTFGCClientSystem()->BIsIPRecentMatchServer( netAdr ) )
				{
					return;
				}
			}

			// is this server already a favorite?
			for( int i=0; i<steamapicontext->SteamMatchmaking()->GetFavoriteGameCount(); ++i )
			{
				AppId_t appID = steamapicontext->SteamUtils()->GetAppID();
				uint32 IP;
				uint16 connPort;
				uint16 queryPort;
				uint32 flags;
				uint32 time32LastPlayedOnServer;

				if ( steamapicontext->SteamMatchmaking()->GetFavoriteGame( i, &appID, &IP, &connPort, &queryPort, &flags, &time32LastPlayedOnServer ) )
				{
					if ( ( flags & k_unFavoriteFlagFavorite ) == false )
					{
						// not a favorite
						continue;
					}

					if ( ( appID == 0 || appID == steamapicontext->SteamUtils()->GetAppID() ) && 
						 IP == GetLastConnectedServerIP() &&
						 connPort == GetLastConnectedServerPort() )
					{
						// already have this server in our favorites - don't ask again
						return;
					}
				}
			}

			// new potential favorite - ask
			ShowConfirmOptOutDialog( "#TF_Serverlist_Ask_Favorite_Title", "#TF_Serverlist_Ask_Favorite_Text", 
				"#TF_Serverlist_Ask_Yes", "#TF_Serverlist_Ask_No", 
				"#TF_ServerList_Ask_Favorite_Opt_Out", "cl_ask_favorite_opt_out", 
				OnAskFavoriteDialogButtonPressed, NULL );
		}

		// blacklist?
		if ( !cl_ask_blacklist_opt_out.GetBool() && sessionDuration < cl_ask_blacklist_max_session_duration.GetFloat() )
		{
			// is this server already blacklisted?
			CBlacklistedServerManager blackList;
			blackList.LoadServersFromFile( BLACKLIST_DEFAULT_SAVE_FILE, false );

			netadr_t lastServer( GetLastConnectedServerIP(), (unsigned short)GetLastConnectedServerPort() );
			if ( ( !blackList.CanServerBeBlacklisted( lastServer.GetIPHostByteOrder(), lastServer.GetPort(), GetLastConnectedServerName() )
			       || GTFGCClientSystem()->BIsIPRecentMatchServer( lastServer ) )
			     && !cl_ask_blacklist_for_any_server.GetBool() )
			{
				// don't bother - this server is not blacklistable
				return;
			}

			if ( blackList.IsServerBlacklisted( GetLastConnectedServerIP(), GetLastConnectedServerPort(), GetLastConnectedServerName() ) )
			{
				// already marked as bad
				return;
			}

			// new potential blacklist - ask
			ShowConfirmOptOutDialog( "#TF_Serverlist_Ask_Blacklist_Title", "#TF_Serverlist_Ask_Blacklist_Text", 
				"#TF_Serverlist_Ask_Yes", "#TF_Serverlist_Ask_No",
				"#TF_ServerList_Ask_Blacklist_Opt_Out", "cl_ask_blacklist_opt_out", 
				OnAskBlacklistDialogButtonPressed, NULL );
		}
	}
}


//----------------------------------------------------------------------------
void ClientModeTFNormal::RemoveFilesInPath( const char *pszPath ) const
{
	FileFindHandle_t hFind = NULL;

	const char *pszSearch = CFmtStr( "%s/*", pszPath );
	char const *szFileName = g_pFullFileSystem->FindFirstEx( pszSearch, "MOD", &hFind );
	while ( szFileName )
	{
		if ( szFileName[ 0 ] != '.' )
		{
			CFmtStr fmtFilename( "%s/%s", pszPath, szFileName );

			if ( g_pFullFileSystem->IsDirectory( fmtFilename, "MOD" ) )
			{
				RemoveFilesInPath( fmtFilename );
			}
			else
			{
				g_pFullFileSystem->RemoveFile( fmtFilename, "MOD" );
			}
		}

		szFileName = g_pFullFileSystem->FindNext( hFind );
	}

	g_pFullFileSystem->FindClose( hFind );
}


//----------------------------------------------------------------------------
void ClientModeTFNormal::Update()
{
	BaseClass::Update();

	if ( m_bPendingRichPresenceUpdate )
	{
		m_bPendingRichPresenceUpdate = false;
		UpdateSteamRichPresence();
	}

	TFModalStack()->Update();

	NotificationQueue_Update();

	// CHudVote *pHudVote = GET_HUDELEMENT( CHudVote );
	// CTFHudMannVsMachineStatus *pMannVsMachineStatus = GET_HUDELEMENT( CTFHudMannVsMachineStatus );
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();

	// Update steam controller stuff if one is active
	if ( ::input->IsSteamControllerActive() )
	{
		// Walk through all the panels and HUD elements, see what kind of action set each one requests.
		bool bNeedMenu = false;
		bool bNeedHUD = false;
		bool bNeedSpectator = false;
		m_pViewport->ForEachPanel( [&] (IViewPortPanel* pPanel) {
			if ( pPanel && pPanel->IsVisible() )
			{
				auto actionset = pPanel->GetPreferredActionSet();
				if ( actionset == GAME_ACTION_SET_MENUCONTROLS )
				{
					bNeedMenu = true;
				}
				else if ( actionset == GAME_ACTION_SET_IN_GAME_HUD )
				{
					bNeedHUD = true;
				}
				else if ( actionset == GAME_ACTION_SET_SPECTATOR )
				{
					bNeedSpectator = true;
				}
			}
		} );

		gHUD.ForEachHudElement( [&]( CHudElement* pElement ) {
			if ( pElement )
			{
				auto actionset = pElement->GetPreferredActionSet();
				if ( actionset == GAME_ACTION_SET_MENUCONTROLS )
				{
					bNeedMenu = true;
				}
				else if ( actionset == GAME_ACTION_SET_IN_GAME_HUD )
				{
					bNeedHUD = true;
				}
				else if ( actionset == GAME_ACTION_SET_SPECTATOR )
				{
					bNeedSpectator = true;
				}
			}
		} );

		// See if there's a modal dialog which wants to change the action set
		if ( !TFModalStack()->IsEmpty() )
		{
			vgui::VPANEL panelTop = TFModalStack()->Top().Get();
			vgui::Panel* pPanel =  vgui::ipanel()->GetPanel( panelTop, "ClientDLL" );
			if ( pPanel )
			{
				CConfirmDialog* pDialog = dynamic_cast<CConfirmDialog*>( pPanel );
				if ( pDialog )
				{
					GameActionSet_t actionset = pDialog->GetPreferredActionSet();
					if ( actionset == GAME_ACTION_SET_MENUCONTROLS )
					{
						bNeedMenu = true;
					}
					else if ( actionset == GAME_ACTION_SET_IN_GAME_HUD )
					{
						bNeedMenu = true;
					}
					else if ( actionset == GAME_ACTION_SET_SPECTATOR )
					{
						bNeedSpectator = true;
					}
				}
			}
		}
		
		// Set the preferred action set. Requesting menu trumps hud, which trumps spectator, which trumps fps.
		if ( !engine->IsInGame() || !engine->IsConnected() || enginevgui->IsGameUIVisible() || bNeedMenu )
		{
			::input->SetPreferredGameActionSet( GAME_ACTION_SET_MENUCONTROLS );
		}
		else if ( bNeedHUD )
		{
			::input->SetPreferredGameActionSet( GAME_ACTION_SET_IN_GAME_HUD );
		}
		else if ( bNeedSpectator || (pLocalPlayer && pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR) )
		{
			::input->SetPreferredGameActionSet( GAME_ACTION_SET_SPECTATOR );
		}
		else
		{
			::input->SetPreferredGameActionSet( GAME_ACTION_SET_FPSCONTROLS );
		}

		// Adjust look sensitivity down if the player is current using a zoomed-in weapon (typically sniper rifle).
		float look_sensitivity = 0.125f;
		if ( pLocalPlayer && pLocalPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
		{
			look_sensitivity = 0.035f;
		}

		// Set the flag for special action handling if we're taunting
		if ( pLocalPlayer && pLocalPlayer->m_Shared.InCond( TF_COND_TAUNTING ) )
		{
			::input->SetGameActionSetFlags( GAME_ACTION_SET_FLAGS_TAUNTING );
		}
		else
		{
			::input->SetGameActionSetFlags( GAME_ACTION_SET_FLAGS_NONE );
		}

		sc_look_sensitivity_scale.SetValue( look_sensitivity );
	}

	if ( !engine->IsInGame() )
	{
		// @note Tom Bui: we want this thing to always run, so we get animations at the main menu
		g_pClientMode->GetViewportAnimationController()->UpdateAnimations( gpGlobals->curtime );
	}

	if ( !engine->IsConnected() )
	{
		if ( m_wasConnectedLastUpdate )
		{
			// just disconnected from a game
			m_wasConnectedLastUpdate = false;

			AskFavoriteOrBlacklist();

			// Reset our sound mixed in case we were in a freeze cam when we
			// disconnected, which would cause the snd_soundmixer to be left modified.
			ConVar *pVar = ( ConVar * ) cvar->FindVar( "snd_soundmixer" );
			pVar->Revert();

			m_lastServerConnectTime = 0;
		}
	}
	else
	{
		m_wasConnectedLastUpdate = true;
	}
}


//----------------------------------------------------------------------------
void	ClientModeTFNormal::ComputeVguiResConditions( KeyValues *pkvConditions ) 
{
	BaseClass::ComputeVguiResConditions( pkvConditions );
}


//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsInfoPanelAllowed()
{
	if ( !BaseClass::IsInfoPanelAllowed() )
		return false;

	return !m_bRestrictInfoPanel || !m_bInfoPanelShown;
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsHTMLInfoPanelAllowed()
{
	if ( !BaseClass::IsHTMLInfoPanelAllowed() )
		return false;

	return !m_bRestrictInfoPanel;
}


//----------------------------------------------------------------------------
void ClientModeTFNormal::InfoPanelDisplayed()
{
	BaseClass::InfoPanelDisplayed();

	m_bInfoPanelShown = true;
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsEngyBuildVisible() const
{
	return m_pMenuEngyBuild && m_pMenuEngyBuild->IsVisible();
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsEngyDestroyVisible() const
{
	return m_pMenuEngyDestroy && m_pMenuEngyDestroy->IsVisible();
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsEngyEurekaTeleportVisible() const
{
	return m_pEurekaTeleportMenu && m_pEurekaTeleportMenu->IsVisible();
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsSpyDisguiseVisible() const
{
	return m_pMenuSpyDisguise && m_pMenuSpyDisguise->IsVisible();
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsUpgradePanelVisible() const
{
	return m_pMenuUpgradePanel && m_pMenuUpgradePanel->IsVisible();
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::IsTauntSelectPanelVisible() const
{
	return m_pMenuTauntSelection && m_pMenuTauntSelection->IsVisible();
}

//----------------------------------------------------------------------------
void ClientModeTFNormal::UpdateSteamRichPresence() const
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Update our steam rich presence keys when some event changes that would require it
	// We don't explicitly flush keys, so this (and callees) should always touch/clear all keys they manage

	ISteamFriends *pSteamFriends = steamapicontext->SteamFriends();
	if ( !pSteamFriends )
		{ return; }

	bool bConnected = ( m_eConnectState == k_eConnectState_Connected );
	bool bConnecting = ( m_eConnectState == k_eConnectState_Connecting );
	// BConnectedToMatchServer includes the connecting state
	bool bInMatch = GTFGCClientSystem()->BConnectedToMatchServer( false );

	//
	// Set 'currentmap'
	//
	const char *pszPrettyMap = nullptr;
	if ( m_szMapBaseName[0] )
		{ pszPrettyMap = GetMapDisplayName( m_szMapBaseName, /* bTitleCase */ true ); }

	if ( bConnected )
		{ pSteamFriends->SetRichPresence( "currentmap", pszPrettyMap );}
	else
		{ pSteamFriends->SetRichPresence( "currentmap", nullptr ); }

	//
	// Set 'connect'
	//
	//   The underlying engine calls AdvertiseGame(), so setting it to null is sufficient to get a "+connect <ip>"
	//   action, but in some cases we may want to direct joiners to the player's party instead (such as main menu, in a
	//   MM match you cannot directly join)
	//
	// Note that AdvertiseGame() happens as soon as we begin connecting
	if ( ( bInMatch || ( !bConnected && !bConnecting ) ) && steamapicontext->SteamUser() )
	{
		// If they have an MM match, or if they're just on the menus, direct joiners to join their party, they cannot
		// join the server directly.
		CFmtStr strConnect( "+tf_party_request_join_user %llu",
		                    steamapicontext->SteamUser()->GetSteamID().ConvertToUint64() );

		engine->SetRichPresenceConnect( strConnect );
	}
	else
	{
		// Otherwise, no connect string.  If they're connected, the engine will handle updating this
		// correctly.
		engine->SetRichPresenceConnect( nullptr );
	}

	//
	// Set 'steam_player_group' and 'steam_player_group_size'
	//
	if ( GTFPartyClient()->BHaveActiveParty() )
	{
		pSteamFriends->SetRichPresence( "steam_player_group",
		                                CFmtStr( "party_%llu", GTFPartyClient()->GetActivePartyID() ) );
		// Only tell steam about online party members, since offline members may still be on steam, but their rich
		// presence won't attest to their membership in this party.
		pSteamFriends->SetRichPresence( "steam_player_group_size",
		                                CFmtStr( "%d", GTFPartyClient()->CountNumOnlinePartyMembers() ) );
	}
	else
	{
		pSteamFriends->SetRichPresence( "steam_player_group", nullptr );
		pSteamFriends->SetRichPresence( "steam_player_group_size", nullptr );
	}

	//
	// Set 'state'
	// Set 'matchgrouploc' if used by given state
	//

	// Used below for building 'status'
	const char *pszState = nullptr;
	const char *pszMatchGroupLoc = nullptr;

	// Playing -- MM Match
	if ( bInMatch )
	{
		ETFMatchGroup eMatchGroup = GTFGCClientSystem()->GetLiveMatchGroup();
		auto *pDesc = GetMatchGroupDescription( eMatchGroup );
		const char *pLoc = pDesc ? pDesc->GetRichPresenceLocToken() : nullptr;
		if ( pLoc )
		{
			pszMatchGroupLoc = pLoc;
			if ( pszPrettyMap && pszPrettyMap[0] )
				{ pszState = "PlayingMatchGroup"; }
			else
				{ pszState = "LoadingMatchGroup"; }
		}
		else
		{
			// Nameless match group
			if ( pszPrettyMap && pszPrettyMap[0] )
				{ pszState = "PlayingGeneric"; }
			else
				{ pszState = "LoadingGeneric"; }
		}
	}
	// Playing -- community server
	else if ( bConnecting || bConnected )
	{
		// In a server, but partyclient doesn't know about it -- set community server
		if ( pszPrettyMap && pszPrettyMap[0] )
			{ pszState = "PlayingCommunity"; }
		else
			{ pszState = "LoadingCommunity"; }
	}
	// Main menu -- searching for a match
	else if ( GTFPartyClient()->BInStandbyQueue() || GTFPartyClient()->BInAnyMatchQueue() )
	{
		// If we're searching for just one group, use the more specific message
		bool bSpecificQueue = ( !GTFPartyClient()->BInStandbyQueue() &&
		                        GTFPartyClient()->GetNumQueuedMatchGroups() == 1 );

		const char *pMatchLoc = nullptr;
		if ( bSpecificQueue )
		{
			// Set 'matchgrouploc' used for this field
			ETFMatchGroup eMatchGroup = GTFPartyClient()->GetQueuedMatchGroupByIdx( 0 );
			auto *pDesc = GetMatchGroupDescription( eMatchGroup );
			pMatchLoc = pDesc ? pDesc->GetRichPresenceLocToken() : nullptr;

			if ( pMatchLoc )
				{ pszMatchGroupLoc = pMatchLoc; }
			else
				{ bSpecificQueue = false; }
		}

		if ( bSpecificQueue )
			{ pszState = "SearchingMatchGroup"; }
		else
			{ pszState = "SearchingGeneric"; }
	}
	// Main menu -- not searching
	else
	{
		pszState = "MainMenu";
	}

	pSteamFriends->SetRichPresence( "state", pszState );
	pSteamFriends->SetRichPresence( "matchgrouploc", pszMatchGroupLoc );

	//
	// 'steam_display' embeds our state and matchgrouploc set above.
	//
	pSteamFriends->SetRichPresence( "steam_display", "#TF_RichPresence_Display" );

	//
	// 'status' field -- used by legacy steam client only right now
	//
	// If we're connecting or connected, the source engine called AdvertiseGame() which shows a this-server status we
	// don't want to override -- except if we're in a match which cannot be ad-hoc joined.
	wchar_t wzStatus[256] = { 0 };
	if ( ( bInMatch || ( !bConnecting && !bConnected ) ) &&
	     BuildRichPresenceStatus( wzStatus, pszState, pszMatchGroupLoc, pszPrettyMap ))
	{
			char szStatus[256] = { 0 };
			V_UnicodeToUTF8( wzStatus, szStatus, sizeof( szStatus ) );
			pSteamFriends->SetRichPresence( "status", szStatus );
	}
	else
	{
		pSteamFriends->SetRichPresence( "status", nullptr );
	}
}

//----------------------------------------------------------------------------
bool ClientModeTFNormal::BuildRichPresenceStatusDirect( wchar_t *pwzOutStatus, size_t uOutSizeBytes,
                                                        const char *pszState,
                                                        const char *pszMatchGroupLocTokenSuffix,
                                                        const char *pszPrettyMapName )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// We don't support the {#TF_SomeToken} recursion steam does in our localize library, but we only use a few specific
	// pieces of indirection
	KeyValues *pKV = new KeyValues( "loc" );
	KeyValuesAD kvAutoDestruct( pKV );

	wchar_t *pwzStateToken = g_pVGuiLocalize->Find( CFmtStr( "#TF_RichPresence_State_%s", pszState ) );
	if ( !pwzStateToken )
		{ return false; }

	wchar_t *pwzMatchGroupLocToken = nullptr;
	if ( pszMatchGroupLocTokenSuffix && pszMatchGroupLocTokenSuffix[0] )
	{
		pwzMatchGroupLocToken = g_pVGuiLocalize->Find( CFmtStr( "#TF_RichPresence_MatchGroup_%s",
		                                                        pszMatchGroupLocTokenSuffix ) );
		if ( !pwzMatchGroupLocToken )
			{ return false; }
	}

	pKV->SetWString( "matchgrouploc_token", pwzMatchGroupLocToken ? pwzMatchGroupLocToken : L"" );
	pKV->SetString( "matchgrouploc", pszMatchGroupLocTokenSuffix ? pszMatchGroupLocTokenSuffix : "" );
	pKV->SetString( "state", pszState ? pszState : "" );
	pKV->SetString( "currentmap", pszPrettyMapName ? pszPrettyMapName : "" );

	g_pVGuiLocalize->ConstructString( pwzOutStatus, uOutSizeBytes, pwzStateToken, pKV );
	return true;
}

//----------------------------------------------------------------------------
void ClientModeTFNormal::PrintTextToChat( const char *pText, KeyValues *pKeyValues )
{
	CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
	if ( pHUDChat )
	{
		wchar_t wszText[1024]=L"";
		g_pVGuiLocalize->ConstructString_safe( wszText, pText, pKeyValues );
		char szAnsi[1024];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszText, szAnsi, sizeof(szAnsi) );
		pHUDChat->Printf( CHAT_FILTER_NONE, "%s", szAnsi );
	}
}

void ClientModeTFNormal::PrintTextToChatPlayer( int iPlayerIndex, const char *pText, KeyValues *pKeyValues )
{
	CBaseHudChat *pHUDChat = (CBaseHudChat *)GET_HUDELEMENT( CHudChat );
	if ( pHUDChat )
	{
		wchar_t wszText[1024]=L"";
		g_pVGuiLocalize->ConstructString_safe( wszText, pText, pKeyValues );
		char szAnsi[1024];
		g_pVGuiLocalize->ConvertUnicodeToANSI( wszText, szAnsi, sizeof(szAnsi) );
		pHUDChat->ChatPrintf( iPlayerIndex, CHAT_FILTER_NONE, "%s", szAnsi );
	}
}

void ClientModeTFNormal::OnDemoRecordStart( char const* pDemoBaseName ) 
{
	BaseClass::OnDemoRecordStart( pDemoBaseName );	
}

void ClientModeTFNormal::OnDemoRecordStop()
{
	IGameEvent *event = gameeventmanager->CreateEvent( "ds_stop" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}

	BaseClass::OnDemoRecordStop();
}

bool ClientModeTFNormal::BCanSendPartyChatMessages() const
{
	return GTFPartyClient()->BHaveActiveParty();
}

bool ClientModeTFNormal::BIsFriendOrPartyMember( C_TFPlayer *pPlayer )
{
	player_info_t pi;
	if ( !pPlayer || !engine->GetPlayerInfo( pPlayer->entindex(), &pi ) || !pi.friendsID )
		return false;

	CSteamID targetSteamID( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );
	EFriendRelationship eRelationship = steamapicontext->SteamFriends()->GetFriendRelationship( targetSteamID );
	if ( eRelationship == k_EFriendRelationshipFriend )
		return true;

	if ( GTFPartyClient()->BHaveActiveParty() )
	{
		for ( int nSlot = 0; nSlot < GTFPartyClient()->GetNumPartyMembers(); ++nSlot )
		{
			if ( targetSteamID == GTFPartyClient()->GetPartyMember( nSlot ) )
				return true;
		}
	}

	return false;
}

USER_MESSAGE( PlayerBonusPoints )
{
	int nPoints = (int) msg.ReadByte();
	int iPlayerEntIndex = (int) msg.ReadByte();
	int iSourceEntIndex = (int) msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "player_bonuspoints" );
	if ( event )
	{
		event->SetInt( "points", nPoints );
		event->SetInt( "player_entindex", iPlayerEntIndex );
		event->SetInt( "source_entindex", iSourceEntIndex );
		gameeventmanager->FireEventClientSide( event );
	}
}

USER_MESSAGE( PlayerGodRayEffect )
{
	int iPlayerEntIndex = (int)msg.ReadByte();

	CBasePlayer *player = UTIL_PlayerByIndex( iPlayerEntIndex );
	if ( player )
	{
		player->ParticleProp()->Create( "god_rays", PATTACH_ABSORIGIN_FOLLOW );
	}
}

USER_MESSAGE( PlayerTeleportHomeEffect )
{
	int iPlayerEntIndex = (int)msg.ReadByte();

	CBasePlayer *pPlayer = UTIL_PlayerByIndex( iPlayerEntIndex );
	if ( pPlayer )
	{
		pPlayer->ParticleProp()->Create( "drg_wrenchmotron_teleport", PATTACH_ABSORIGIN );
	}
}

USER_MESSAGE( RDTeamPointsChanged )
{
	int nPoints = (int)msg.ReadShort();
	int nTeam = (int)msg.ReadByte();
	int nMethod = (int)msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "rd_team_points_changed" );
	if ( event )
	{
		event->SetInt( "points", nPoints );
		event->SetInt( "team", nTeam );
		event->SetInt( "method", nMethod );
		gameeventmanager->FireEventClientSide( event );
	}
}

USER_MESSAGE( PlayerLoadoutUpdated )
{
	int iPlayerEntIndex = (int)msg.ReadByte();
	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerEntIndex ) );
	if ( pPlayer )
	{
		CTFPlayerInventory *pInv = pPlayer->Inventory();
		if ( pInv )
		{
			pInv->ClearClassLoadoutChangeTracking();
		}
	}
}

USER_MESSAGE( PlayerTauntSoundLoopStart )
{
	int iPlayerEntIndex = (int)msg.ReadByte();
	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerEntIndex ) );
	if ( pPlayer )
	{
		char szTauntSoundLoopName[256];
		msg.ReadString( szTauntSoundLoopName, sizeof(szTauntSoundLoopName) );
		pPlayer->PlayTauntSoundLoop( szTauntSoundLoopName );
	}
}

USER_MESSAGE( PlayerTauntSoundLoopEnd )
{
	int iPlayerEntIndex = (int)msg.ReadByte();
	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerEntIndex ) );
	if ( pPlayer )
	{
		pPlayer->StopTauntSoundLoop();
	}
}


USER_MESSAGE( ForcePlayerViewAngles )
{
	// Read flag byte. Should be 1 right now.
	int iFlags = (int)msg.ReadByte();
	NOTE_UNUSED( iFlags );
	Assert( iFlags == 0x01 );

	int iPlayerEntIndex = (int)msg.ReadByte();
	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerEntIndex ) );
	if ( pPlayer )
	{
		QAngle viewangles;
		msg.ReadBitAngles( viewangles );

		// Set the magic angles here!
		pPlayer->SetLocalAngles( viewangles );
		pPlayer->SetAbsAngles( viewangles );
		pPlayer->SetTauntYaw( viewangles[YAW] );
		pPlayer->m_Shared.SetVehicleMoveAngles( viewangles );
	}
}

ConVar tf_halloween_bonus_ducks_cooldown( "tf_halloween_bonus_ducks_cooldown", "20", FCVAR_ARCHIVE );
USER_MESSAGE( BonusDucks )
{
	static float sflNextBonusDucks = 0.f;

	int iPlayerEntIndex = (int)msg.ReadByte();
	int iIgnoreTimer = (int)msg.ReadByte();

	if ( Plat_FloatTime() < sflNextBonusDucks && !iIgnoreTimer )
		return;

	sflNextBonusDucks = Plat_FloatTime() + tf_halloween_bonus_ducks_cooldown.GetFloat();

	
	C_TFPlayer *pPlayer = ToTFPlayer( UTIL_PlayerByIndex( iPlayerEntIndex ) );
	if ( pPlayer )
	{
		pPlayer->EmitSound( "sf14.Merasmus.DuckHunt.BonusDucks" );
	}
}

USER_MESSAGE( PlayerPickupWeapon )
{
	IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_pickup_weapon" );
	if ( event )
	{
		gameeventmanager->FireEventClientSide( event );
	}
}

USER_MESSAGE( QuestObjectiveCompleted )
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	itemid_t itemID = (itemid_t)msg.ReadLongLong();
	uint16 nPoints0 = msg.ReadByte();
	uint16 nPoints1 = msg.ReadByte();
	uint16 nPoints2 = msg.ReadByte();
	uint32 nObjectiveDefIndex = msg.ReadWord();
	uint16 nScorerUserID = msg.ReadByte();

	QuestObjectiveManager()->UpdateFromServer( itemID, nPoints0, nPoints1, nPoints2 );
	QuestObjectiveManager()->EnsureTrackersForPlayer( steamapicontext->SteamUser()->GetSteamID() );

	// Passing a -1 means this is a ninja update where we don't want the fanfare
	if ( nObjectiveDefIndex != (uint32)-1 )
	{
		IGameEvent *pEvent = gameeventmanager->CreateEvent( "quest_objective_completed" );
		if ( pEvent )
		{
			pEvent->SetInt( "quest_item_id_low", itemID & 0xFFFFFFFF );
			pEvent->SetInt( "quest_item_id_hi", itemID >> 32 );
			pEvent->SetInt( "quest_objective_id", nObjectiveDefIndex );
			pEvent->SetInt( "scorer_user_id", nScorerUserID );
			gameeventmanager->FireEventClientSide( pEvent );
		}	
	}
}

USER_MESSAGE( BuiltObject )
{
	int nObjType = (int)msg.ReadByte();
	int nObjMode = (int)msg.ReadByte();
	int nObjIndex = (int)msg.ReadByte();

	IGameEvent *event = gameeventmanager->CreateEvent( "localplayer_builtobject" );
	if ( event )
	{
		event->SetInt( "object", nObjType );
		event->SetInt( "object_mode", nObjMode );
		event->SetInt ( "index", nObjIndex );
		gameeventmanager->FireEventClientSide( event );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Hook for servers requesting our current loadout
// (SDK mods do not connect to the gc and get subscriptions)
//-----------------------------------------------------------------------------
USER_MESSAGE( SdkRequestEquipment )
{
	GTFGCClientSystem()->ServerRequestEquipment();
}

float PlaySoundEntry( const char* pszSoundEntryName )
{
	const char *pszSoundName = UTIL_GetRandomSoundFromEntry( pszSoundEntryName );
	if ( pszSoundName && pszSoundName[0] )
	{
		vgui::surface()->PlaySound( pszSoundName );
		return enginesound->GetSoundDuration( pszSoundName );
	}

	return 0.f;
}

#if defined( STAGING_ONLY ) || defined( _DEBUG )
CON_COMMAND( tf_rich_presence_set, "Set rich presence key" )
{
	if ( args.ArgC() != 3 )
	{
		ConMsg( "Usage: tf_rich_presence_set - <key> <value>\n" );
		return;
	}

	const char *pKey = args[1];
	const char *pValue = args[2];

	auto *pSteamFriends = steamapicontext->SteamFriends();
	if ( !pSteamFriends )
	{
		ConMsg( "Set rich presence failed - No SteamFriends API\n" );
		return;
	}

	pSteamFriends->SetRichPresence( pKey, pValue );
	ConMsg( "Set rich presence key \"%s\" -> \"%s\"\n", pKey, pValue );
}

CON_COMMAND( tf_rich_presence_clear, "Clear all rich presence" )
{
	if ( args.ArgC() != 1 )
	{
		ConMsg( "Usage: tf_rich_presence_clear - no arguments, clears all rich presence\n" );
		return;
	}

	auto *pSteamFriends = steamapicontext->SteamFriends();
	if ( !pSteamFriends )
	{
		ConMsg( "Clearing rich presence failed - No SteamFriends API\n" );
		return;
	}

	pSteamFriends->ClearRichPresence();
	ConMsg( "Clear all rich presence keys\n" );
}

CON_COMMAND( tf_rich_presence_force_update, "Force a rich presence update" )
{
	if ( args.ArgC() != 1 )
	{
		ConMsg( "Usage: tf_rich_presence_force_update - - no arguments, forces a rich presence update\n" );
		return;
	}

	auto *pSteamFriends = steamapicontext->SteamFriends();
	if ( !pSteamFriends )
	{
		ConMsg( "Updating rich presence failed - No SteamFriends API\n" );
		return;
	}

	GetClientModeTFNormal()->UpdateSteamRichPresence();
	ConMsg( "Forced rich presence update\n" );
}
#endif // defined( STAGING_ONLY ) || defined( _DEBUG )

#ifdef _WIN32

#include "winlite.h"
#pragma warning (push)
#pragma warning(disable:4201)	// nameless struct/union
#include <mmsystem.h>
#pragma warning (pop) 
#endif
//
void PlayOutOfGameSound( const char *pszSound )
{
#ifdef _WIN32
	PlaySound( pszSound, NULL, SND_FILENAME | SND_ASYNC );
#endif
}

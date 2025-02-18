//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_team.h"
#include "tf_playerpanel.h"
#include "tf_spectatorgui.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "tf_hud_objectivestatus.h"
#include "tf_hud_statpanel.h"
#include "iclientmode.h"
#include "c_playerresource.h"
#include "tf_hud_building_status.h"
#include "tf_hud_tournament.h"
#include "tf_hud_winpanel.h"
#include "tf_tips.h"
#include "tf_mapinfomenu.h"
#include "econ_wearable.h"
#include "c_tf_playerresource.h"
#include "playerspawncache.h"
#include "econ_notifications.h"
#include "tf_hud_item_progress_tracker.h"
#include "tf_hud_target_id.h"
#include "c_baseobject.h"
#include "inputsystem/iinputsystem.h"

#if defined( REPLAY_ENABLED )
#include "replay/replay.h"
#include "replay/ireplaysystem.h"
#include "replay/ireplaymanager.h"
#endif // REPLAY_ENABLED

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include "vgui_avatarimage.h"

using namespace vgui;

extern ConVar _cl_classmenuopen;
extern ConVar tf_max_health_boost;
extern const char *g_pszItemClassImages[];
extern int g_ClassDefinesRemap[];
extern ConVar tf_mvm_buybacks_method;

static const wchar_t* GetSCGlyph( const char* action )
{
	auto origin = g_pInputSystem->GetSteamControllerActionOrigin( action, GAME_ACTION_SET_SPECTATOR );
	return g_pInputSystem->GetSteamControllerFontCharacterForActionOrigin( origin );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFSpectatorGUI *GetTFSpectatorGUI()
{
	extern CSpectatorGUI *g_pSpectatorGUI;
	return dynamic_cast< CTFSpectatorGUI * >( g_pSpectatorGUI );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ConVar cl_spec_carrieditems( "cl_spec_carrieditems", "1", FCVAR_ARCHIVE, "Show non-standard items being carried by player you're spectating." );

void HUDTournamentSpecChangedCallBack( IConVar *var, const char *pOldString, float flOldValue )
{
	CTFSpectatorGUI *pPanel = (CTFSpectatorGUI*)gViewPortInterface->FindPanelByName( PANEL_SPECGUI );
	if ( pPanel )
	{
		pPanel->InvalidateLayout( true, true );
	}
}
ConVar cl_use_tournament_specgui( "cl_use_tournament_specgui", "0", FCVAR_ARCHIVE, "When in tournament mode, use the advanced tournament spectator UI.", HUDTournamentSpecChangedCallBack );

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFSpectatorGUI::CTFSpectatorGUI(IViewPort *pViewPort) : CSpectatorGUI(pViewPort)
{
	m_flNextTipChangeTime = 0;
	m_iTipClass = TF_CLASS_UNDEFINED;

	m_nEngBuilds_xpos = m_nEngBuilds_ypos = 0;
	m_nSpyBuilds_xpos = m_nSpyBuilds_ypos = 0;

	m_nMannVsMachineStatus_xpos = m_nMannVsMachineStatus_ypos = 0;
	m_pBuyBackLabel = new CExLabel( this, "BuyBackLabel", "" );
	m_pReinforcementsLabel = new Label( this, "ReinforcementsLabel", "" );
	m_pClassOrTeamLabel = new Label( this, "ClassOrTeamLabel", "" );
	// m_pSwitchCamModeKeyLabel = new Label( this, "SwitchCamModeKeyLabel", "" );
	m_pSwitchCamModeKeyLabel = nullptr;
	m_pClassOrTeamKeyLabel = nullptr;
	m_pCycleTargetFwdKeyLabel = new Label( this, "CycleTargetFwdKeyLabel", "" );
	m_pCycleTargetRevKeyLabel = new Label( this, "CycleTargetRevKeyLabel", "" );
	m_pMapLabel = new Label( this, "MapLabel", "" );
	m_pItemPanel = new CItemModelPanel( this, "itempanel" );
	m_pCycleTargetRevHintIcon = m_pCycleTargetFwdHintIcon = nullptr;
	m_pClassOrTeamHintIcon = nullptr;

	m_pStudentHealth = new CTFSpectatorGUIHealth( this, "StudentGUIHealth" );
	m_pAvatar = NULL;

	m_flNextItemPanelUpdate = 0;
	m_flNextPlayerPanelUpdate = 0;
	m_iPrevItemShown = 0;
	m_iFirstItemShown = 0;
	m_bShownItems = false;
	m_hPrevItemPlayer = NULL;
	m_pPlayerPanelKVs = NULL;
	m_bReapplyPlayerPanelKVs = false;
	m_bCoaching = false;

	ListenForGameEvent( "spec_target_updated" );
	ListenForGameEvent( "player_death" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFSpectatorGUI::~CTFSpectatorGUI()
{
	if ( m_pPlayerPanelKVs )
	{
		m_pPlayerPanelKVs->deleteThis();
		m_pPlayerPanelKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::Reset( void )
{
	BaseClass::Reset();

	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		m_PlayerPanels[i]->Reset();
	}

	m_pStudentHealth->Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CTFSpectatorGUI::GetTopBarHeight()
{
	int iPlayerPanelHeight = 0;
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		if ( GetLocalPlayerTeam() == TF_TEAM_PVE_DEFENDERS )
		{
			if ( m_PlayerPanels.Count() > 0 )
			{
				iPlayerPanelHeight = m_PlayerPanels[0]->GetTall();
			}
		}
	}

	return m_pTopBar->GetTall() + iPlayerPanelHeight; 
}

//-----------------------------------------------------------------------------
// Purpose: makes the GUI fill the screen
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::PerformLayout( void )
{
	BaseClass::PerformLayout();

	if ( m_bReapplyPlayerPanelKVs )
	{
		m_bReapplyPlayerPanelKVs = false;

		if ( m_pPlayerPanelKVs )
		{
			for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
			{
				m_PlayerPanels[i]->ApplySettings( m_pPlayerPanelKVs );
				m_PlayerPanels[i]->InvalidateLayout( false, true );
			}
		}
	}

	if ( m_pStudentHealth )
	{
		Panel* pHealthPosPanel = FindChildByName( "HealthPositioning" );
		if ( pHealthPosPanel )
		{
			int xPos, yPos, iWide, iTall;
			pHealthPosPanel->GetBounds( xPos, yPos, iWide, iTall );
			m_pStudentHealth->SetBounds( xPos, yPos, iWide, iTall );
			m_pStudentHealth->SetZPos( pHealthPosPanel->GetZPos() );
		}
	}

	UpdatePlayerPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	bool bVisible = IsVisible();
	BaseClass::ApplySchemeSettings( pScheme );

	m_bReapplyPlayerPanelKVs = true;
	m_bPrevTournamentMode = InTournamentGUI();

	m_pSwitchCamModeKeyLabel = dynamic_cast< Label* >( FindChildByName( "SwitchCamModeKeyLabel" ) );

	m_pAvatar = dynamic_cast<CAvatarImagePanel *>( FindChildByName("AvatarImage") );
	if ( ::input->IsSteamControllerActive() )
	{
		m_pCycleTargetFwdHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "CycleTargetFwdHintIcon", true ) );
		m_pCycleTargetRevHintIcon = dynamic_cast< CSCHintIcon* >( FindChildByName( "CycleTargetRevHintIcon", true ) );
		m_pClassOrTeamHintIcon = dynamic_cast<CSCHintIcon*>( FindChildByName( "ClassOrTeamHintIcon", true ) );
	}
	else
	{
		m_pClassOrTeamHintIcon = m_pCycleTargetRevHintIcon = m_pCycleTargetFwdHintIcon = nullptr;
	}

	if ( m_bCoaching )
	{
		if ( m_pTopBar )
		{
			m_pTopBar->SetBgColor( Color( 255, 255, 255, 0 ) );
		}
		if ( m_pBottomBarBlank )
		{
			m_pBottomBarBlank->SetVisible( false );
		}
	}

	if ( m_bCoaching )
	{
		if ( m_pClassOrTeamLabel && m_pClassOrTeamLabel->IsVisible() )
		{
			m_pClassOrTeamLabel->SetVisible( false );
		}

		if ( m_pClassOrTeamKeyLabel && m_pClassOrTeamKeyLabel->IsVisible() )
		{
			m_pClassOrTeamKeyLabel->SetVisible( false );
		}
	}

	// Stay the same visibility as before the scheme reload.
	SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "playerpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pPlayerPanelKVs )
		{
			m_pPlayerPanelKVs->deleteThis();
		}
		m_pPlayerPanelKVs = new KeyValues("playerpanels_kv");
		pItemKV->CopySubkeys( m_pPlayerPanelKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSpectatorGUI::NeedsUpdate( void )
{
	if ( !C_BasePlayer::GetLocalPlayer() )
		return false;

	if( IsVisible() )
		return true;

	return BaseClass::NeedsUpdate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::Update()
{
	BaseClass::Update();

	UpdateReinforcements();
	UpdateKeyLabels();

	if ( m_flNextItemPanelUpdate < gpGlobals->curtime )
	{
		UpdateItemPanel();
	}

	// If we need to flip tournament mode, do it now
	if ( m_bPrevTournamentMode != InTournamentGUI() )
	{
		InvalidateLayout( false, true );
	}
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer && m_bCoaching != pLocalPlayer->m_bIsCoaching )
	{
		m_bCoaching = pLocalPlayer->m_bIsCoaching;
		InvalidateLayout( false, true );
	}
	if ( pLocalPlayer && pLocalPlayer->m_hStudent && m_bCoaching )
	{
		Vector vecTarget = pLocalPlayer->m_hStudent->GetAbsOrigin();		
		Vector vecDelta = pLocalPlayer->GetAbsOrigin() - vecTarget;
		float flDistance = vecDelta.Length();
		const float kInchesToMeters = 0.0254f;
		int distance = RoundFloatToInt( flDistance * kInchesToMeters );
		wchar_t wzValue[32];
		_snwprintf( wzValue, ARRAYSIZE( wzValue ), L"%u", distance );
		wchar_t wzText[256];
		g_pVGuiLocalize->ConstructString_safe( wzText, g_pVGuiLocalize->Find( "#TR_DistanceToStudent" ), 1, wzValue );
		SetDialogVariable( "student_distance", wzText );
	}

	if ( m_flNextPlayerPanelUpdate < gpGlobals->curtime )
	{
		RecalculatePlayerPanels();
		m_flNextPlayerPanelUpdate = gpGlobals->curtime + 0.1f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::UpdateReinforcements( void )
{
	if( !m_pReinforcementsLabel )
		return;

	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || pPlayer->IsHLTV() ||
		( pPlayer->GetTeamNumber() != TF_TEAM_RED && pPlayer->GetTeamNumber() != TF_TEAM_BLUE ) ||
		( pPlayer->m_Shared.GetState() != TF_STATE_OBSERVER && pPlayer->m_Shared.GetState() != TF_STATE_DYING ) ||
		( pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM ) )
	{
		m_pReinforcementsLabel->SetVisible( false );
		m_pBuyBackLabel->SetVisible( false );

		return;
	}

	bool bBuyBackVisible = false;
	wchar_t wLabel[256];
		
	if ( TFGameRules()->InStalemate() )
	{
		if ( TFGameRules()->IsInArenaMode() == true )
		{
			g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find( "#TF_Arena_NoRespawning" ), 0 );
		}
		else
		{
			g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find( "#game_respawntime_stalemate" ), 0 );
		}
	}
	else if ( TFGameRules()->State_Get() == GR_STATE_TEAM_WIN )
	{
		// a team has won the round
		g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find( "#game_respawntime_next_round" ), 0 );
	}
	else
	{
		float flNextRespawn = 0.f;
		bool bQuickSpawn = TFGameRules() && TFGameRules()->IsMannVsMachineMode() && pPlayer->IsPlayerClass( TF_CLASS_SCOUT );

		if ( g_TF_PR )
		{
			flNextRespawn = g_TF_PR->GetNextRespawnTime( pPlayer->entindex() );
		}
		else if ( !bQuickSpawn )
		{
			flNextRespawn = TFGameRules()->GetNextRespawnWave( pPlayer->GetTeamNumber(), pPlayer );
		}

		if ( !flNextRespawn )
		{
			m_pReinforcementsLabel->SetVisible( false );
			m_pBuyBackLabel->SetVisible( false );
			return;
		}

		int iRespawnWait = (flNextRespawn - gpGlobals->curtime);
		if ( iRespawnWait <= 0 )
		{
			g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find("#game_respawntime_now" ), 0 );
		}
		else if ( iRespawnWait <= 1.0 )
		{
			g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find("#game_respawntime_in_sec" ), 0 );
		}
		else
		{
			char szSecs[6];
			wchar_t wSecs[4];

			if ( TFGameRules()->IsMannVsMachineMode() )
			{
				bool bNewMethod = tf_mvm_buybacks_method.GetBool();
				bBuyBackVisible = ( bNewMethod ) ? g_TF_PR->GetNumBuybackCredits( pPlayer->entindex() ) : true;

				if ( bBuyBackVisible )
				{
					// When using the new system, we display "Hit '%use_action_slot_item%' to RESPAWN INSTANTLY! (%s1 remaining this wave)"
					// When using the old system, we display "Hit '%use_action_slot_item%' to pay %s1 credits and RESPAWN INSTANTLY!"
					int nCost = ( bNewMethod ) ? g_TF_PR->GetNumBuybackCredits( pPlayer->entindex() ) : iRespawnWait * MVM_BUYBACK_COST_PER_SEC;
					const char *pszString = ( bNewMethod ) ? "#TF_PVE_Buyback_Fixed" : "#TF_PVE_Buyback";

					Q_snprintf( szSecs, sizeof( szSecs ), "%d", nCost );
					g_pVGuiLocalize->ConvertANSIToUnicode( szSecs, wSecs, sizeof( wSecs ) );
					g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find( pszString ), 1, wSecs );

					wchar_t wBuyBack[256];
					UTIL_ReplaceKeyBindings( wLabel, 0, wBuyBack, sizeof( wBuyBack ), GAME_ACTION_SET_SPECTATOR );

					m_pBuyBackLabel->SetText( wBuyBack, true );
				}
			}

			Q_snprintf( szSecs, sizeof(szSecs), "%d", iRespawnWait );

			g_pVGuiLocalize->ConvertANSIToUnicode(szSecs, wSecs, sizeof(wSecs));
			g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find("#game_respawntime_in_secs" ), 1, wSecs );
		}
	}

	m_pReinforcementsLabel->SetVisible( true );
	m_pReinforcementsLabel->SetText( wLabel, true );
	m_pBuyBackLabel->SetVisible( bBuyBackVisible );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::UpdateKeyLabels( void )
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	bool bSteamController = ::input->IsSteamControllerActive();

	if ( InTournamentGUI() == false )
	{
		// get the desired player class	
		int iClass = TF_CLASS_UNDEFINED;
		bool bIsHLTV = engine->IsHLTV();
		
		if ( pPlayer )
		{
			iClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();
		}

		// if it's time to change the tip, or the player has changed desired class, update the tip
		if ( ( gpGlobals->curtime >= m_flNextTipChangeTime ) || ( iClass != m_iTipClass ) )
		{
			if ( bIsHLTV )
			{
				const wchar_t *wzTip = g_pVGuiLocalize->Find( "#Tip_HLTV" );

				if ( wzTip )
				{
					SetDialogVariable( "tip", wzTip );
				}
			}
			else
			{
				wchar_t wzTipLabel[512]=L"";
				const wchar_t *wzTip = g_TFTips.GetNextClassTip( iClass );
				Assert( wzTip && wzTip[0] );
				g_pVGuiLocalize->ConstructString_safe( wzTipLabel, g_pVGuiLocalize->Find( "#Tip_Fmt" ), 1, wzTip );
				SetDialogVariable( "tip", wzTipLabel );
			}
			
			m_flNextTipChangeTime = gpGlobals->curtime + 10.0f;
			m_iTipClass = iClass;
		}

		if ( m_pClassOrTeamLabel )
		{
			if ( pPlayer )
			{
				static wchar_t wzFinal[512] = L"";
				const wchar_t *wzTemp = NULL;
				const char *szAction = nullptr;

				if ( TFGameRules() && TFGameRules()->IsInTraining() )
				{
					wzTemp = L"";
				}
				else if ( bIsHLTV )
				{
					wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_AutoDirector" );
				}
				else if ( pPlayer->GetTeamNumber() == TEAM_SPECTATOR && TFGameRules()->IsInArenaMode() == false )
				{
					if ( bSteamController )
					{
						wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_ChangeTeam_NoKey" );
						szAction = "changeteam";
					}
					else
					{
						wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_ChangeTeam" );
					}
				}
				else
				{
					if ( bSteamController )
					{
						wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_ChangeClass_NoKey" );
						szAction = "changeclass";
					}
					else
					{
						wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_ChangeClass" );
					}
				}

				if ( wzTemp )
				{
					UTIL_ReplaceKeyBindings( wzTemp, 0, wzFinal, sizeof( wzFinal ) );
				}

				m_pClassOrTeamLabel->SetText( wzFinal, true );

				if ( m_pClassOrTeamHintIcon )
				{
					if ( szAction && m_pClassOrTeamLabel->IsVisible() )
					{
						m_pClassOrTeamHintIcon->SetVisible( true );
						m_pClassOrTeamHintIcon->SetAction( szAction );
					}
					else
					{
						m_pClassOrTeamHintIcon->SetVisible( false );
					}
				}
			}
		}

		static ConVarRef cl_hud_minmode( "cl_hud_minmode", true );
		if ( m_bCoaching == true || ( cl_hud_minmode.IsValid() && ( cl_hud_minmode.GetBool() == false ) ) )
		{
			if ( m_pSwitchCamModeKeyLabel )
			{
				if ( ( pPlayer && pPlayer->GetTeamNumber() > TEAM_SPECTATOR ) && ( ( mp_forcecamera.GetInt() == OBS_ALLOW_TEAM ) || ( mp_forcecamera.GetInt() == OBS_ALLOW_NONE ) || mp_fadetoblack.GetBool() ) )
				{
					if ( m_pSwitchCamModeKeyLabel->IsVisible() )
					{
						m_pSwitchCamModeKeyLabel->SetVisible( false );

						Label *pLabel = dynamic_cast<Label *>( FindChildByName( "SwitchCamModeLabel" ) );
						if ( pLabel )
						{
							pLabel->SetVisible( false );
						}
					}
				}
				else
				{
					if ( !m_pSwitchCamModeKeyLabel->IsVisible() )
					{
						m_pSwitchCamModeKeyLabel->SetVisible( true );

						Label *pLabel = dynamic_cast<Label *>( FindChildByName( "SwitchCamModeLabel" ) );
						if ( pLabel )
						{
							pLabel->SetVisible( true );
						}
					}

					wchar_t wLabel[256] = L"";
					const wchar_t *wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_SwitchCamModeKey" );
					UTIL_ReplaceKeyBindings( wzTemp, 0, wLabel, sizeof( wLabel ) );
					m_pSwitchCamModeKeyLabel->SetText( wLabel, true );
				}
			}

			bool bSuppressCycle = ( pPlayer && pPlayer->GetTeamNumber() > TEAM_SPECTATOR ) && ( mp_fadetoblack.GetBool() || ( mp_forcecamera.GetInt() == OBS_ALLOW_NONE ) );

			if ( m_pCycleTargetFwdKeyLabel )
			{
				if ( bSuppressCycle )
				{
					if ( m_pCycleTargetFwdKeyLabel->IsVisible() )
					{
						m_pCycleTargetFwdKeyLabel->SetVisible( false );

						Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetFwdLabel" ) );
						if ( pLabel )
						{
							pLabel->SetVisible( false );
						}
					}
				}
				else
				{
					if ( !m_pCycleTargetFwdKeyLabel->IsVisible() )
					{
						m_pCycleTargetFwdKeyLabel->SetVisible( true );

						Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetFwdLabel" ) );
						if ( pLabel )
						{
							pLabel->SetVisible( true );
						}
					}

					wchar_t wLabel[ 256 ] = L"";
					const wchar_t *wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_CycleTargetFwdKey" );
					UTIL_ReplaceKeyBindings( wzTemp, 0, wLabel, sizeof( wLabel ) );
					m_pCycleTargetFwdKeyLabel->SetText( wLabel, true );
				}

				if ( bSteamController )
				{
					m_pCycleTargetFwdKeyLabel->SetVisible( false );
				}
			}

			if ( m_pCycleTargetFwdHintIcon )
				m_pCycleTargetFwdHintIcon->SetVisible( bSteamController && !bSuppressCycle );


			if ( m_pCycleTargetRevKeyLabel )
			{
				if ( bSuppressCycle )
				{
					if ( m_pCycleTargetRevKeyLabel->IsVisible() )
					{
						m_pCycleTargetRevKeyLabel->SetVisible( false );

						Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetRevLabel" ) );
						if ( pLabel )
						{
							pLabel->SetVisible( false );
						}
					}
				}
				else
				{
					if ( !m_pCycleTargetRevKeyLabel->IsVisible() )
					{
						m_pCycleTargetRevKeyLabel->SetVisible( true );

						Label *pLabel = dynamic_cast<Label *>( FindChildByName( "CycleTargetRevLabel" ) );
						if ( pLabel )
						{
							pLabel->SetVisible( true );
						}
					}

					wchar_t wLabel[ 256 ] = L"";
					const wchar_t *wzTemp = g_pVGuiLocalize->Find( "#TF_Spectator_CycleTargetRevKey" );
					UTIL_ReplaceKeyBindings( wzTemp, 0, wLabel, sizeof( wLabel ) );
					m_pCycleTargetRevKeyLabel->SetText( wLabel, true );
				}

				if ( bSteamController )
				{
					m_pCycleTargetRevKeyLabel->SetVisible( false );
				}
			}

			if ( m_pCycleTargetRevHintIcon )
				m_pCycleTargetRevHintIcon->SetVisible( bSteamController && !bSuppressCycle );


			if ( m_pMapLabel )
			{
				wchar_t wMapName[32];
				wchar_t wLabel[256];
				char szMapName[32];

				char tempname[128];
				Q_FileBase( engine->GetLevelName(), tempname, sizeof( tempname ) );
				Q_strlower( tempname );

				if ( IsX360() )
				{
					char *pExt = Q_stristr( tempname, ".360" );
					if ( pExt )
					{
						*pExt = '\0';
					}
				}

				Q_strncpy( szMapName, GetMapDisplayName( tempname ), sizeof( szMapName ) );

				g_pVGuiLocalize->ConvertANSIToUnicode( szMapName, wMapName, sizeof(wMapName));
				g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find( "#Spec_Map" ), 1, wMapName );

				m_pMapLabel->SetText( wLabel ); 
			}
		}
	}

	// coaching stuff
	if ( pPlayer && pPlayer->m_hStudent )
	{
		int iHealth = 0;
		int iMaxHealth = 1;
		int iMaxBuffedHealth = 0;

		C_TFPlayer *pStudent = pPlayer->m_hStudent;
		{
			wchar_t wPlayerName[MAX_PLAYER_NAME_LENGTH];
			wchar_t wLabel[256];
			const char* pStudentName = g_TF_PR->GetPlayerName( pStudent->entindex() );

			g_pVGuiLocalize->ConvertANSIToUnicode( pStudentName, wPlayerName, sizeof(wPlayerName));
			g_pVGuiLocalize->ConstructString_safe( wLabel, g_pVGuiLocalize->Find( "#TF_Coach_Student_Prefix" ), 1, wPlayerName );
			
			SetDialogVariable( "student_name", wLabel ); 
		}
		for ( int i = 1; i <= 2; ++i )
		{
			wchar_t wLabel[256] = L"";
			const wchar_t *wzTemp = g_pVGuiLocalize->Find( CFmtStr1024( "#TF_Coach_Slot%uLabel", i ) );
			UTIL_ReplaceKeyBindings( wzTemp, 0, wLabel, sizeof( wLabel ) );
			SetDialogVariable( CFmtStr1024( "coach_command_%u", i ), wLabel );
		}
		if ( m_pAvatar )
		{
			m_pAvatar->SetShouldDrawFriendIcon( false );

			if ( steamapicontext && steamapicontext->SteamUser() )
			{
				CSteamID studentSteamID;
				if ( pStudent->GetSteamID( &studentSteamID ) )
				{
					m_pAvatar->SetPlayer( studentSteamID, k_EAvatarSize64x64 );
				}
				else
				{
					m_pAvatar->ClearAvatar();
				}				
			}
		}

		// don't show crosshair when viewing the world from the student's POV
		bool bShowCrosshair = pPlayer->GetObserverMode() != OBS_MODE_IN_EYE;
		vgui::Panel *pCrosshair = FindChildByName( "Crosshair" );
		if ( pCrosshair && pCrosshair->IsVisible() != bShowCrosshair )
		{
			pCrosshair->SetVisible( bShowCrosshair );
		}

		iHealth = pStudent->GetHealth();
		iMaxHealth = pStudent->GetMaxHealth();
		iMaxBuffedHealth = pStudent->m_Shared.GetMaxBuffedHealth();

		if ( m_pStudentHealth )
		{
			m_pStudentHealth->SetHealth( iHealth, iMaxHealth, iMaxBuffedHealth );

			if ( !m_pStudentHealth->IsVisible() )
			{
				m_pStudentHealth->SetVisible( true );
			}
		}
	}
	else
	{
		if ( m_pStudentHealth && m_pStudentHealth->IsVisible() )
		{
			m_pStudentHealth->SetVisible( false );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::ShowPanel(bool bShow)
{
	if ( bShow != IsVisible() )
	{
		CTFHudObjectiveStatus *pStatus = GET_HUDELEMENT( CTFHudObjectiveStatus );
		CHudBuildingStatusContainer_Engineer *pEngBuilds = GET_NAMED_HUDELEMENT( CHudBuildingStatusContainer_Engineer, BuildingStatus_Engineer );
		CHudBuildingStatusContainer_Spy *pSpyBuilds = GET_NAMED_HUDELEMENT( CHudBuildingStatusContainer_Spy, BuildingStatus_Spy );
		CHudItemAttributeTracker *pAttribTrackers = GET_HUDELEMENT( CHudItemAttributeTracker );

		if ( pAttribTrackers )
		{
			pAttribTrackers->InvalidateLayout();
		}

		if ( bShow )
		{
			int xPos = 0, yPos = 0;

			if ( pStatus )
			{
				pStatus->SetParent( this );
				pStatus->SetProportional( true );
			}

			if ( pEngBuilds )
			{
				pEngBuilds->GetPos( xPos, yPos );
				m_nEngBuilds_xpos = xPos;
				m_nEngBuilds_ypos = yPos;
				pEngBuilds->SetPos( xPos, GetTopBarHeight() );
			}

			if ( pSpyBuilds )
			{
				pSpyBuilds->GetPos( xPos, yPos );
				m_nSpyBuilds_xpos = xPos;
				m_nSpyBuilds_ypos = yPos;
				pSpyBuilds->SetPos( xPos, GetTopBarHeight() );
			}

#if defined( REPLAY_ENABLED )
			// We don't want to display this message the first time the spectator GUI is shown, since that is right
			// when the class menu is shown.  We use the player spawn cache here - which is a terrible name for something
			// very useful - which will nuke m_nDisplaySaveReplay every time a new map is loaded, which is exactly what
			// we want.
			int &nDisplaySaveReplay = CPlayerSpawnCache::Instance().m_Data.m_nDisplaySaveReplay;
			extern IReplayManager *g_pReplayManager;
			CReplay *pCurLifeReplay = ( g_pReplayManager ) ? g_pReplayManager->GetReplayForCurrentLife() : NULL;
			if ( g_pReplay->IsRecording() &&
				 !engine->IsPlayingDemo() &&
				 !::input->IsSteamControllerActive() &&
				 nDisplaySaveReplay &&
				 ( pCurLifeReplay && !pCurLifeReplay->m_bRequestedByUser && !pCurLifeReplay->m_bSaved ) )
			{
				wchar_t wText[256];
				wchar wKeyBind[80];
				char szText[256 * sizeof(wchar_t)];

				const char *pSaveReplayKey = engine->Key_LookupBinding( "save_replay" );
				if ( !pSaveReplayKey )
				{
					pSaveReplayKey = "< not bound >";
				}
				g_pVGuiLocalize->ConvertANSIToUnicode( pSaveReplayKey, wKeyBind, sizeof( wKeyBind ) );
				g_pVGuiLocalize->ConstructString_safe( wText, g_pVGuiLocalize->Find( "#Replay_SaveThisLifeMsg" ), 1, wKeyBind );
				g_pVGuiLocalize->ConvertUnicodeToANSI( wText, szText, sizeof( szText ) );

				g_pClientMode->DisplayReplayMessage( szText, -1.0f, false, NULL, false );
			}
			++nDisplaySaveReplay;
#endif

			m_flNextTipChangeTime = 0;	// force a new tip immediately

			InvalidateLayout();
		}
		else
		{
			if ( pStatus )
			{
				pStatus->SetParent( g_pClientMode->GetViewport() );
			}

			if ( pEngBuilds )
			{
				pEngBuilds->SetPos( m_nEngBuilds_xpos, m_nEngBuilds_ypos );
			}

			if ( pSpyBuilds )
			{
				pSpyBuilds->SetPos( m_nSpyBuilds_xpos, m_nSpyBuilds_ypos );
			}
		}

		UpdateKeyLabels();

		if ( bShow )
		{
			m_flNextPlayerPanelUpdate = 0;
			m_flNextItemPanelUpdate = 0;
			UpdateItemPanel();	
			RecalculatePlayerPanels();
		}
	}

	BaseClass::ShowPanel( bShow );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::FireGameEvent( IGameEvent *event )
{
	const char *pEventName = event->GetName();

	if ( Q_strcmp( "spec_target_updated", pEventName ) == 0 )
	{
		UpdateItemPanel();
	}
	else if ( Q_strcmp( "player_death", pEventName ) == 0 && m_bCoaching )
	{
		CBaseEntity *pVictim = ClientEntityList().GetEnt( engine->GetPlayerForUserID( event->GetInt("userid") ) );
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer && ( pVictim == pLocalPlayer->m_hStudent ) )
		{
			CEconNotification *pNotification = new CEconNotification();
			pNotification->SetText( "#TF_Coach_StudentHasDied" );
			pNotification->SetLifetime( 10.0f );
			pNotification->SetSoundFilename( "coach/coach_student_died.wav" );
			NotificationQueue_Add( pNotification );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::UpdateItemPanel( bool bForce )
{
	bool bVisible = false;

	// Stat panel prevents the item panel from showing.
	CTFStatPanel *pStatPanel = GET_HUDELEMENT( CTFStatPanel );
	if ( ( pStatPanel && pStatPanel->IsVisible() ) || ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_TEAM_WIN ) || (!cl_spec_carrieditems.GetBool() && !bForce) )
	{
		bVisible = false;
	}
	else 
	{
		C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
		if ( pLocalPlayer )
		{
			C_TFPlayer *pPlayer = ToTFPlayer( pLocalPlayer->GetObserverTarget() );
			if ( pPlayer && pPlayer != pLocalPlayer )
			{
				if ( m_flNextItemPanelUpdate && m_flNextItemPanelUpdate > gpGlobals->curtime && m_hPrevItemPlayer == pPlayer )
					return;

				if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() && ( pPlayer->GetTeamNumber() == TF_TEAM_PVE_INVADERS ) )
					return;

				if ( m_hPrevItemPlayer != pPlayer )
				{
					m_iPrevItemShown = 0;
					m_iFirstItemShown = 0;
					m_bShownItems = false;
					m_hPrevItemPlayer = pPlayer;
				}
				m_flNextItemPanelUpdate = gpGlobals->curtime + 8.0;

				// Don't reshow the items for a player unless we're being forced to
				if ( !m_bShownItems || bForce )
				{
					// If our killer is using a non-standard item, display its stats.
					// Loop through all items and pick one at random, so that we show non-active weapons as well.
					CEconItemView *pItemToShow = pPlayer->GetInspectItem( &m_iPrevItemShown );

					// If we've looped, we're done. Hide the item.
					if ( m_iFirstItemShown && m_iFirstItemShown == m_iPrevItemShown )
					{
						m_iFirstItemShown = 0;
						m_iPrevItemShown = 0;
						pItemToShow = NULL;
						m_bShownItems = true;
					}

					if ( pItemToShow && !pItemToShow->IsUndefined() )
					{
						if ( !m_iFirstItemShown )
						{
							m_iFirstItemShown = m_iPrevItemShown;
						}

						Label* pItemLabel = m_pItemPanel->FindControl<Label>( "ItemLabel" );

						CBasePlayer *pOriginalOwner = GetPlayerByAccountID( pItemToShow->GetAccountID() );

						// Change the label text depending on if the original owner is holding the weapon
						if ( pItemLabel )
						{
							CSteamID steamIDOwner;
							pPlayer->GetSteamID( &steamIDOwner );
							bool bOriginalOwner = !pOriginalOwner || steamIDOwner.GetAccountID() == pItemToShow->GetAccountID();
							pItemLabel->SetText( bOriginalOwner ? "#FreezePanel_Item" : "#FreezePanel_ItemOtherOwner" );
						}
						

						bVisible = true;
						m_pItemPanel->SetDialogVariable( "killername", g_TF_PR->GetPlayerName( pPlayer->entindex() ) );

						// Set the item owner's name
						if ( pOriginalOwner )
						{
							m_pItemPanel->SetDialogVariable( "ownername", g_TF_PR->GetPlayerName( pOriginalOwner->entindex() ) );
						}

						m_pItemPanel->SetItem( pItemToShow );

						// force update description to get the correct panel size
						m_pItemPanel->UpdateDescription();
						m_pItemPanel->SetPos( ScreenWidth() - XRES( 10 ) - m_pItemPanel->GetWide(), ScreenHeight() - YRES( 12 ) - m_pItemPanel->GetTall() );
					}	
				}
			}
		}
	}

	if ( m_pItemPanel->IsVisible() != bVisible )
	{
		m_pItemPanel->SetVisible( bVisible );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::ForceItemPanelCycle( void )
{
	m_flNextItemPanelUpdate = 0;
	UpdateItemPanel( true );		
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
const char *CTFSpectatorGUI::GetResFile( void ) 
{ 
	if ( m_bCoaching )
	{
		return "Resource/UI/SpectatorCoach.res"; 
	}
	else if ( InTournamentGUI() )
	{
		return "Resource/UI/SpectatorTournament.res"; 
	}
	else if ( ::input->IsSteamControllerActive() )
	{
		return "Resource/UI/Spectator_SC.res";
	}
	else
	{
		return "Resource/UI/Spectator.res";
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFSpectatorGUI::InTournamentGUI( void )
{
	bool bOverride = false;

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		bOverride = true;
	}

	return ( TFGameRules()->IsInTournamentMode() && !TFGameRules()->IsCompetitiveMode() && ( cl_use_tournament_specgui.GetBool() || bOverride ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::RecalculatePlayerPanels( void )
{
	if ( !InTournamentGUI() )
	{
		if ( m_PlayerPanels.Count() > 0 )
		{
			// Delete any player panels we have, we've turned off the tourney GUI.
			for ( int i = m_PlayerPanels.Count()-1; i >= 0; i-- )
			{
				m_PlayerPanels[i]->MarkForDeletion();
			}
			m_PlayerPanels.Purge();
		}

		return;
	}

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !g_TF_PR || !TFGameRules() )
		return;

	int iLocalTeam = pPlayer->GetTeamNumber();
	bool bMvM = TFGameRules()->IsMannVsMachineMode();

	// Calculate the number of players that must be shown. Spectators see all players (except in MvM), team members only see their team.
	int iPanel = 0;

	for ( int nClass = TF_FIRST_NORMAL_CLASS; nClass <= TF_LAST_NORMAL_CLASS; nClass++ )
	{
		// we want to sort the images to match the class menu selections
		int nCurrentClass = g_ClassDefinesRemap[nClass];

		for ( int i = 0; i < MAX_PLAYERS; i++ )
		{
			int iPlayer = i+1;
			if ( !g_TF_PR->IsConnected( iPlayer ) )
				continue;

			bool bHideBots = false;

	#ifndef _DEBUG 
			if ( bMvM )
			{
				bHideBots = true;
			}
	#endif
			int iTeam = g_TF_PR->GetTeam( iPlayer );
			if ( iTeam != iLocalTeam && iLocalTeam != TEAM_SPECTATOR )
				continue;
			if ( iTeam != TF_TEAM_RED && iTeam != TF_TEAM_BLUE )
				continue;
			if ( g_TF_PR->IsFakePlayer( iPlayer ) && bHideBots )
				continue;
			if ( bMvM && ( iTeam == TF_TEAM_PVE_INVADERS ) )
				continue;
			if ( g_TF_PR->GetPlayerClass( iPlayer ) != nCurrentClass )
				continue;

			if ( m_PlayerPanels.Count() <= iPanel )
			{
				CTFPlayerPanel *pPanel = new CTFPlayerPanel( this, VarArgs("playerpanel%d", i) );
				if ( m_pPlayerPanelKVs )
				{
					pPanel->ApplySettings( m_pPlayerPanelKVs );
				}
				m_PlayerPanels.AddToTail( pPanel );
			}

			m_PlayerPanels[iPanel]->SetPlayerIndex( iPlayer );
			iPanel++;
		}
	}

	for ( int i = iPanel; i < m_PlayerPanels.Count(); i++  )
	{
		m_PlayerPanels[i]->SetPlayerIndex( 0 );
	}

	UpdatePlayerPanels();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::UpdatePlayerPanels( void )
{
	if ( !g_TF_PR )
		return;

	uint nVisible = 0;
	bool bNeedsPlayerLayout = false;
	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		if ( m_PlayerPanels[i]->Update() )
		{
			bNeedsPlayerLayout = true;
		}

		if ( m_PlayerPanels[i]->IsVisible() )
		{
			nVisible++;
		}
	}

	if ( !bNeedsPlayerLayout )
		return;

	// Try and always put the local player's team on team1, if he's in a team
	int iTeam1 = TF_TEAM_BLUE;
	int iTeam2 = TF_TEAM_RED;
	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return;

	int iLocalTeam = g_TF_PR->GetTeam( pLocalPlayer->entindex() );

	if ( iLocalTeam == TF_TEAM_RED || iLocalTeam == TF_TEAM_BLUE )
	{
		iTeam1 = iLocalTeam;
		iTeam2 = ( iTeam1 == TF_TEAM_BLUE ) ? TF_TEAM_RED : TF_TEAM_BLUE;
	}

	int iTeam1Count = 0;
	int iTeam2Count = 0;
	int iCenter = GetWide() * 0.5;
	int iYPosOverride = 0;

	// We only want to draw the player panels for the defenders in MvM
	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		iTeam1 = TF_TEAM_PVE_DEFENDERS;
		iTeam2 = TF_TEAM_PVE_INVADERS;

		int iNumValidPanels = 0;
		for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
		{
			if ( m_PlayerPanels[i]->GetPlayerIndex() <= 0 )
			{
				continue;
			}

			iNumValidPanels++;
		}

		// we'll center the group of players in MvM
		m_iTeam1PlayerBaseOffsetX = ( iNumValidPanels * m_iTeam1PlayerDeltaX ) * -0.5;

		if ( iLocalTeam > LAST_SHARED_TEAM )
		{
			if ( m_pTopBar )
			{
				iYPosOverride = m_pTopBar->GetTall();
			}
		}
	}

	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		int iXPos = 0;
		int iYPos = 0;

		if ( m_PlayerPanels[i]->GetPlayerIndex() <= 0 )
		{
			m_PlayerPanels[i]->SetVisible( false );
			continue;
		}

		int iTeam = g_TF_PR->GetTeam( m_PlayerPanels[i]->GetPlayerIndex() );
		if ( iTeam == iTeam1 )
		{
			iXPos = m_iTeam1PlayerBaseOffsetX ? (iCenter + m_iTeam1PlayerBaseOffsetX) : m_iTeam1PlayerBaseX;
			iXPos += (iTeam1Count * m_iTeam1PlayerDeltaX);
			iYPos = iYPosOverride ? iYPosOverride : m_iTeam1PlayerBaseY + (iTeam1Count * m_iTeam1PlayerDeltaY);
			m_PlayerPanels[i]->SetSpecIndex( 6 - iTeam1Count );
			m_PlayerPanels[i]->SetPos( iXPos, iYPos );
			m_PlayerPanels[i]->SetVisible( true );
			iTeam1Count++;
		}
		else if ( iTeam == iTeam2 )
		{
			iXPos = m_iTeam2PlayerBaseOffsetX ? (iCenter + m_iTeam2PlayerBaseOffsetX) : m_iTeam2PlayerBaseX;
			iXPos += (iTeam2Count * m_iTeam2PlayerDeltaX);
			iYPos = iYPosOverride ? iYPosOverride : m_iTeam2PlayerBaseY + (iTeam2Count * m_iTeam2PlayerDeltaY);
			m_PlayerPanels[i]->SetSpecIndex( 7 + iTeam2Count );
			m_PlayerPanels[i]->SetPos( iXPos, iYPos );
			m_PlayerPanels[i]->SetVisible( true );
			iTeam2Count++;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFSpectatorGUI::SelectSpec( int iSlot )
{
	if ( m_bCoaching )
	{
		engine->ClientCmd_Unrestricted( CFmtStr1024( "coach_command %u", iSlot ) );
		return;
	}

	if ( !InTournamentGUI() )
		return;

	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		if ( m_PlayerPanels[i]->GetSpecIndex() == iSlot )
		{
			engine->ClientCmd_Unrestricted( VarArgs( "spec_player %d\n", m_PlayerPanels[i]->GetPlayerIndex() ) );
			return;
		}
	}
}

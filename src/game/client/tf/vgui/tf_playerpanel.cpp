//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "hud.h"
#include "c_team.h"
#include "tf_shareddefs.h"
#include "tf_gamerules.h"
#include "tf_hud_objectivestatus.h"
#include "tf_hud_statpanel.h"
#include "iclientmode.h"
#include "c_playerresource.h"
#include "tf_hud_building_status.h"
#include "tf_hud_mann_vs_machine_status.h"
#include "tf_hud_tournament.h"
#include "tf_hud_winpanel.h"
#include "tf_tips.h"
#include "tf_mapinfomenu.h"
#include "econ_wearable.h"
#include "c_tf_playerresource.h"
#include "playerspawncache.h"
#include "econ_notifications.h"
#include <spectatorgui.h>
#include "hudelement.h"
#include "tf_spectatorgui.h"
#include "tf_hud_playerstatus.h"
#include "item_model_panel.h"
#include "tf_gamerules.h"
#include "tf_playerpanel.h"

#include "tf_gc_client.h"
#include "tf_lobby_server.h"

#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <VGuiMatSurface/IMatSystemSurface.h>
#include "vgui_avatarimage.h"

using namespace vgui;

extern ConVar tf_max_health_boost;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFPlayerPanel::CTFPlayerPanel( vgui::Panel *parent, const char *name ) : vgui::EditablePanel( parent, name )
{
	m_pHealthIcon = new CTFPlayerPanelGUIHealth( this, "HealthIcon" );
	m_pClassImage = NULL;
	m_bPlayerReadyModeActive = false;
	m_pReadyBG = new ScalableImagePanel( this , "ReadyBG" );
	m_pReadyImage = new ImagePanel( this, "ReadyImage" );

	SetDialogVariable( "chargeamount", "" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerPanel::Reset( void )
{
	m_pHealthIcon->Reset();
	m_iPrevHealth = -999;
	m_iPrevClass = -999;
	m_bPrevAlive = false;
	m_iPrevRespawnWait = -999;
	m_iPrevCharge = -1;
	m_bPrevReady = true;
	m_iPrevState = GR_STATE_PREGAME;
	m_bPlayerReadyModeActive = false;
	m_nGCTeam = TEAM_INVALID;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFPlayerPanel::Update( void )
{
	if ( !g_TF_PR || !TFGameRules() )
		return false;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer )
		return false;

	bool bChanged = false;
	bool bObserver = pLocalPlayer->GetObserverMode() != OBS_MODE_NONE;
	bool bVisible = GetTeam() >= FIRST_GAME_TEAM;
	int iRespawnWait = -1;
	m_bPlayerReadyModeActive = ( !bObserver &&
								 TFGameRules()->UsePlayerReadyStatusMode() &&
								 TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS );

	CTFGSLobby *pLobby = GTFGCClientSystem()->GetLobby();
	if ( pLobby )
	{
		int idxMember = pLobby->GetMemberIndexBySteamID( m_steamID );
		if ( idxMember >= 0 )
		{
			ConstTFLobbyPlayer member = pLobby->GetMemberDetails( idxMember );
			// Keep this updated
			m_nGCTeam = member.GetTeam();

			RTime32 rtLastConnect = member.GetLastConnectTime();
			if ( !m_iPlayerIndex && rtLastConnect != 0 )
			{
				iRespawnWait = CRTime::RTime32DateAdd( rtLastConnect, 180, k_ETimeUnitSecond ) - CRTime::RTime32TimeCur();
				if ( iRespawnWait <= 0 )
					iRespawnWait = -1;
			}
		}
	}

	if ( IsVisible() != bVisible )
	{
		SetVisible( bVisible );
		bChanged = true;
	}

	if ( bVisible )
	{
		if ( g_TF_PR )
		{
			// Are we connected with a class?
			Assert( TF_CLASS_UNDEFINED == 0 );
			int iClass = -1;
			bool bAlive = false;
			const int k_Health_Dead = -1;
			const int k_Health_NotINGame = -2;
			int iHealth = k_Health_NotINGame;
			if ( m_iPlayerIndex > 0 )
			{
				iClass = g_TF_PR->GetPlayerClass( m_iPlayerIndex );

				if ( iClass != TF_CLASS_UNDEFINED )
				{
					bAlive = g_TF_PR->IsAlive( m_iPlayerIndex );
					iHealth = k_Health_Dead;
				}
				if ( bAlive )
					iHealth = g_TF_PR->GetHealth( m_iPlayerIndex );

				// Calc respawn time remaining
				if ( iClass != TF_CLASS_UNDEFINED && !bAlive )
				{
					float flRespawnAt = g_TF_PR->GetNextRespawnTime( m_iPlayerIndex );
					iRespawnWait = (flRespawnAt - gpGlobals->curtime);
					if ( iRespawnWait <= 0 )
						iRespawnWait = -1;
				}

				// Hide class info from the other team?
				if ( !bObserver && 
					 TFGameRules()->IsCompetitiveMode() && 
					 GetTeam() != g_TF_PR->GetTeam( pLocalPlayer->entindex() ) )
				{
					iClass = TF_CLASS_UNDEFINED;
				}
			}

			// Update live state
			if ( m_pClassImage )
			{
				if ( m_bPrevAlive != bAlive )
				{
					bChanged = true;
					m_bPrevAlive = bAlive;
					if ( bAlive )
					{
						m_pClassImage->SetDrawColor( Color( 255, 255, 255, 255 ) );
					}
					else
					{
						m_pClassImage->SetDrawColor( Color( 96, 96, 96, 255 ) );
					}

					UpdateBorder();
				}

				// Update class image
				if ( m_iPrevClass != iClass )
				{
					bChanged = true;
					m_iPrevClass = iClass;
					if ( iClass < 0 )
					{
						m_pClassImage->SetImage( "hud_connecting" );
					}
					else if ( iClass == TF_CLASS_UNDEFINED )
					{
						m_pClassImage->SetImage( "hud_class_not_chosen" );
					}
					else
					{
						m_pClassImage->SetImage( VarArgs( "%s_alpha", ( GetTeam() == TF_TEAM_RED ) ? g_pszItemClassImagesRed[iClass] : g_pszItemClassImagesBlue[iClass] ) );
					}
				}
			}

			// update health indicator
			if ( iHealth != m_iPrevHealth && m_pHealthIcon )
			{
				m_iPrevHealth = iHealth;
				if ( iHealth == k_Health_NotINGame )
				{
					m_pHealthIcon->SetVisible( false );
				}
				else
				{
					m_pHealthIcon->SetVisible( true );
					if ( iHealth < 0 )
					{
						Assert( iHealth == k_Health_Dead );
						m_pHealthIcon->SetHealth( -1, 1, 1 );
					}
					else
					{
						float flMaxHealth = g_TF_PR->GetMaxHealth( m_iPlayerIndex );
						float iMaxBuffedHealth = flMaxHealth * tf_max_health_boost.GetFloat();	// Hacky, but it'll work.
						m_pHealthIcon->SetHealth( iHealth, flMaxHealth, iMaxBuffedHealth );
						bChanged = true;
					}
				}
			}

			// Update respawn time text
			if ( iRespawnWait != m_iPrevRespawnWait )
			{
				m_iPrevRespawnWait = iRespawnWait;
				if ( iRespawnWait < 0 )
				{
					SetDialogVariable( "respawntime", "" );
				}
				else
				{
					SetDialogVariable( "respawntime", VarArgs( "%d s", iRespawnWait ) );
					bChanged = true;
				}
			}

			bool bReadyMode = TFGameRules()->UsePlayerReadyStatusMode();

			int iCharge = ( iClass == TF_CLASS_MEDIC ) ? g_TF_PR->GetChargeLevel( m_iPlayerIndex ) : 0;
			if ( iCharge != m_iPrevCharge )
			{
				if ( iCharge > 0 && !( bReadyMode && !bObserver ) )
				{
					SetDialogVariable( "chargeamount", VarArgs( "%d%%", iCharge ) );
					bChanged = true;
				}
				else
				{
					SetDialogVariable( "chargeamount", "" );
				}
				m_iPrevCharge = iCharge;
			}

			if ( bReadyMode )
			{
				bool bPlayerReady = false;

				if ( m_bPlayerReadyModeActive )
				{
					if ( m_iPlayerIndex && g_TF_PR->IsConnected( m_iPlayerIndex ) )
					{
						bPlayerReady = TFGameRules()->IsPlayerReady( m_iPlayerIndex );
					}
				}

				if ( m_pReadyImage && ( m_pReadyImage->IsVisible() != bPlayerReady ) )
				{
					m_pReadyImage->SetVisible( bPlayerReady );
				}

				if ( m_pHealthIcon && ( m_pHealthIcon->IsVisible() == m_bPlayerReadyModeActive ) )
				{
					m_pHealthIcon->SetVisible( !m_bPlayerReadyModeActive );
				}

				if ( m_pReadyBG && ( m_pReadyBG->IsVisible() != m_bPlayerReadyModeActive ) )
				{
					m_pReadyBG->SetVisible( m_bPlayerReadyModeActive );
				}

				if ( TFGameRules()->State_Get() != m_iPrevState )
				{
					bChanged = true;
				}
				m_iPrevState = TFGameRules()->State_Get();
			}
		}
	}

	return bChanged;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_pClassImage = dynamic_cast<CTFClassImage*>( FindChildByName( "classimage" ) );

	Reset();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerPanel::SetPlayerIndex( int iIndex ) 
{ 
	if ( iIndex <= 0 )
	{
		Setup( iIndex, CSteamID(), "" );
	}
	else
	{
		Setup( iIndex, GetSteamIDForPlayerIndex( iIndex ), g_TF_PR->GetPlayerName( iIndex ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerPanel::Setup( int iPlayerIndex, CSteamID steamID, const char *pszPlayerName, int nLobbyTeam /*= TEAM_INVALID*/ )
{
	if ( pszPlayerName == NULL )
		pszPlayerName = "";
	if ( m_iPlayerIndex != iPlayerIndex
		|| m_steamID != steamID
		|| Q_strcmp( m_sPlayerName, pszPlayerName ) )
	{
		Reset();
		m_iPlayerIndex = iPlayerIndex;
		m_steamID = steamID;
		m_sPlayerName = pszPlayerName;
		SetDialogVariable( "playername", m_sPlayerName );
		m_nGCTeam = nLobbyTeam;
	}

	if ( m_iPlayerIndex > 0 || m_steamID.IsValid() )
	{
		UpdateBorder();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerPanel::SetSpecIndex( int iIndex ) 
{ 
	m_iSpecIndex = iIndex; 

	if ( m_iSpecIndex > 0 && m_iSpecIndex <= 12 && !m_bPlayerReadyModeActive )
	{
		SetDialogVariable( "specindex", VarArgs( "%d", m_iSpecIndex ) );
	}
	else
	{
		SetDialogVariable( "specindex", "" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFPlayerPanel::UpdateBorder( void )
{
	if ( !g_TF_PR )
		return;
		
	vgui::IScheme *pScheme = vgui::scheme()->GetIScheme( GetScheme() );
	if ( !m_bPrevAlive )
	{
		SetBorder( pScheme->GetBorder("TFFatLineBorder") );
	}
	else
	{
		if ( GetTeam() == TF_TEAM_RED )
		{
			SetBorder( pScheme->GetBorder( "TFFatLineBorderRedBG" ) );
		}
		else
		{
			SetBorder( pScheme->GetBorder( "TFFatLineBorderBlueBG" ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFPlayerPanel::GetTeam( void )
{
	if ( m_nGCTeam != TEAM_INVALID && TFGameRules() )
	{
		return TFGameRules()->GetGameTeamForGCTeam( (TF_GC_TEAM)m_nGCTeam );
	}
	else if ( GetPlayerIndex() && g_TF_PR )
	{
		return  g_TF_PR->GetTeam( GetPlayerIndex() );
	}

	return TEAM_INVALID;
}



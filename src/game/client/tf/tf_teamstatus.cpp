//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ProgressBar.h>

#include "tf_gamerules.h"
#include "tf_logic_halloween_2014.h"
#include "c_tf_playerresource.h"
#include "tf_playerpanel.h"
#include "tf_teamstatus.h"
#include "tf_matchmaking_shared.h"
#include "tf_match_description.h"
#include "tf_hud_match_status.h"

using namespace vgui;

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CTFTeamStatusPlayerPanel::CTFTeamStatusPlayerPanel( vgui::Panel *parent, const char *name ) : CTFPlayerPanel( parent, name )
{
	m_pHealthBar = new vgui::ContinuousProgressBar( this, "healthbar" );
	m_pOverhealBar = new vgui::ContinuousProgressBar( this, "overhealbar" );
	m_pClassImageBG = new vgui::Panel( this, "classimagebg" );
	m_pDeathFlag = new vgui::ImagePanel( this, "DeathPanel" );

	m_iTeam = TEAM_UNASSIGNED;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatusPlayerPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatusPlayerPanel::UpdateBorder( void )
{
	// do nothing
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTeamStatusPlayerPanel::Update( void )
{
	if ( !g_TF_PR || !TFGameRules() )
		return false;

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pLocalPlayer || ( pLocalPlayer->GetTeamNumber() < FIRST_GAME_TEAM ) )
		return false;

	bool bChanged = false;
	bool bVisible = GetTeam() >= FIRST_GAME_TEAM;
	int iRespawnWait = -1;

	if ( IsVisible() != bVisible )
	{
		SetVisible( bVisible );
		bChanged = true;
	}

	if ( bVisible && g_TF_PR )
	{
		bool bSameTeamAsLocalPlayer = ( GetTeam() == g_TF_PR->GetTeam( pLocalPlayer->entindex() ) );

		// are we connected with a class?
		Assert( TF_CLASS_UNDEFINED == 0 );
		int iClass = -1;
		bool bAlive = false;
		bool bFeigned = false;
		int iHealth = -1;
		bool bIsLocalPlayer = false;
		if ( m_iPlayerIndex > 0 )
		{
			bIsLocalPlayer = pLocalPlayer->entindex() == m_iPlayerIndex;

			iClass = g_TF_PR->GetPlayerClass( m_iPlayerIndex );

			if ( iClass != TF_CLASS_UNDEFINED )
			{
				bAlive = g_TF_PR->IsAlive( m_iPlayerIndex );
			}

			C_TFPlayer* pTFPlayer = ToTFPlayer( UTIL_PlayerByIndex( m_iPlayerIndex ) );

			// Josh: Not sure if this halloween logic can ever trigger, but it was missing
			// replication from the scoreboard either way.
			if ( TFGameRules() && TFGameRules()->IsHolidayActive( kHoliday_Halloween ) && TFGameRules()->ArePlayersInHell() )
			{
				if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_HALLOWEEN_GHOST_MODE ) )
				{
					bAlive = false;
					bFeigned = true;
				}
			}

			// Josh: Are they a Spy that's feigning death? Mark them as dead on the status UI.
			if ( g_TF_PR->GetPlayerClass( m_iPlayerIndex ) == TF_CLASS_SPY )
			{
				if ( pTFPlayer && pTFPlayer->m_Shared.InCond( TF_COND_FEIGN_DEATH ) )
				{
					bAlive = false;
					bFeigned = true;
				}
			}

			if ( bAlive )
			{
				iHealth = g_TF_PR->GetHealth( m_iPlayerIndex );
			}

			// calc respawn time remaining
			if ( !bAlive && ( iClass != TF_CLASS_UNDEFINED ) )
			{
				float flRespawnAt = g_TF_PR->GetNextRespawnTime( m_iPlayerIndex );
				iRespawnWait = ( flRespawnAt - gpGlobals->curtime );
				if ( iRespawnWait <= 0 )
					iRespawnWait = -1;
			}

			// hide class info from the other team?
			if ( !bSameTeamAsLocalPlayer )
			{
				iClass = TF_CLASS_UNDEFINED;
			}
		}

		if ( m_iTeam != GetTeam() )
		{
			m_iTeam = GetTeam();
			bChanged = true;
		}

		if ( m_pClassImageBG )
		{
			if ( m_iTeam == TF_TEAM_RED )
			{
				Color bgColor( m_ColorPortraitBGRedLocalPlayer );
				if ( !bIsLocalPlayer )
				{
					bgColor = bAlive ? m_ColorPortraitBGRed : m_ColorPortraitBGRedDead;
				}
				
				m_pClassImageBG->SetBgColor( bgColor );
			}
			else
			{
				Color bgColor( m_ColorPortraitBGBlueLocalPlayer );
				if ( !bIsLocalPlayer )
				{
					bgColor = bAlive ? m_ColorPortraitBGBlue : m_ColorPortraitBGBlueDead;
				}
				m_pClassImageBG->SetBgColor( bgColor );
			}
		}

		// update live state
		if ( m_pClassImage )
		{
			if ( m_bPrevAlive != bAlive )
			{
				bChanged = true;
				m_bPrevAlive = bAlive;
				if ( !bAlive )
				{
					m_pClassImage->SetDrawColor( ( m_iTeam == TF_TEAM_RED ) ? m_ColorPortraitBlendDeadRed : m_ColorPortraitBlendDeadBlue );
				}

				m_pDeathFlag->SetImage( ( m_iTeam == TF_TEAM_RED ) ? "../HUD/comp_player_status" : "../HUD/comp_player_status_blue" );

				if ( bAlive )
				{
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "TeamStatus_PlayerAlive", false );
				}
				else
				{
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( this, "TeamStatus_PlayerDead", false );
				}
			}

			// update class image
			if ( ( m_iPrevClass != iClass ) || bChanged )
			{
				bChanged = true;
				m_iPrevClass = iClass;
				if ( iClass < 0 )
				{
					m_pClassImage->SetImage( "hud_connecting" );
				}
				else if ( iClass == TF_CLASS_UNDEFINED )
				{
					int iDeadClass = bFeigned ? g_TF_PR->GetPlayerClass( m_iPlayerIndex ) : g_TF_PR->GetPlayerClassWhenKilled( m_iPlayerIndex );
					if ( !bAlive && !bSameTeamAsLocalPlayer && ( m_iTeam >= FIRST_GAME_TEAM ) && ( iDeadClass > TF_CLASS_UNDEFINED ) )
					{
						m_pClassImage->SetImage( VarArgs( "%s_alpha", ( m_iTeam == TF_TEAM_RED ) ? g_pszItemClassImagesRed[iDeadClass + 9] : g_pszItemClassImagesBlue[iDeadClass + 9] ) );
					}
					else
					{
						m_pClassImage->SetImage( "class_portraits/silhouette_alpha" );
					}
				}
				else
				{
					if ( bAlive )
					{
						m_pClassImage->SetImage( VarArgs( "%s_alpha", ( m_iTeam == TF_TEAM_RED ) ? g_pszItemClassImagesRed[iClass] : g_pszItemClassImagesBlue[iClass] ) );
					}
					else
					{
						m_pClassImage->SetImage( VarArgs( "%s_alpha", ( m_iTeam == TF_TEAM_RED ) ? g_pszItemClassImagesRed[iClass + 9] : g_pszItemClassImagesBlue[iClass + 9] ) );

					}
				}
			}
		}

		// update health indicator
		if ( m_pHealthBar && m_pOverhealBar )
		{
			if ( iHealth != m_iPrevHealth )
			{
				m_iPrevHealth = iHealth;

				if ( ( iHealth > 0 ) && bSameTeamAsLocalPlayer )
				{
					float flMaxHealth = g_TF_PR->GetMaxHealth( m_iPlayerIndex );
					float flProgress = iHealth / flMaxHealth;

					// health bar
					if ( !m_pHealthBar->IsVisible() )
					{
						m_pHealthBar->SetVisible( true );
					}
					m_pHealthBar->SetProgress( ( flProgress >= 1.0f ) ? 1.0f : flProgress );

					if ( flProgress > m_flPercentageHealthMed )
					{
						m_pHealthBar->SetFgColor( m_ColorBarHealthHigh );
					}
					else if ( flProgress > m_flPercentageHealthLow )
					{
						m_pHealthBar->SetFgColor( m_ColorBarHealthMed );
					}
					else
					{
						m_pHealthBar->SetFgColor( m_ColorBarHealthLow );
					}

					// overheal bar
					if ( flProgress > 1.0f )
					{
						if ( !m_pOverhealBar->IsVisible() )
						{
							m_pOverhealBar->SetVisible( true );
						}
						m_pOverhealBar->SetProgress( flProgress - 1.0f );
					}
					else
					{
						if ( m_pOverhealBar->IsVisible() )
						{
							m_pOverhealBar->SetVisible( false );
						}
					}
				}
				else
				{
					if ( m_pHealthBar->IsVisible() )
					{
						m_pHealthBar->SetVisible( false );
					}

					if ( m_pOverhealBar->IsVisible() )
					{
						m_pOverhealBar->SetVisible( false );
					}
				}

				bChanged = true;
			}
		}

		// update respawn time
		if ( iRespawnWait != m_iPrevRespawnWait )
		{
			m_iPrevRespawnWait = iRespawnWait;
			if ( ( iRespawnWait < 0 ) || !bSameTeamAsLocalPlayer )
			{
				SetDialogVariable( "respawntime", "" );
			}
			else
			{
				SetDialogVariable( "respawntime", VarArgs( "%d", iRespawnWait ) );
			}

			bChanged = true;
		}

		// gamerules state
		if ( TFGameRules()->State_Get() != m_iPrevState )
		{
			m_iPrevState = TFGameRules()->State_Get();
			bChanged = true;
		}
	}

	return bChanged;
}

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CTFTeamStatus::CTFTeamStatus( Panel *parent, const char *panelName ) : BaseClass( parent, panelName )
{
	m_pPlayerPanelKVs = NULL;
	m_bReapplyPlayerPanelKVs = false;

	Reset();

	ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeamStatus::~CTFTeamStatus()
{
	if ( m_pPlayerPanelKVs )
	{
		m_pPlayerPanelKVs->deleteThis();
		m_pPlayerPanelKVs = NULL;
	}
}

void SetGrowDir( CTFTeamStatus::EGrowDir* pGrowDir, const char* pszString )
{
	if ( FStrEq( pszString, "east" ) )
	{
		(*pGrowDir) = CTFTeamStatus::EGrowDir::GROW_EAST;
	}
	else if ( FStrEq( pszString, "west" ) )
	{
		(*pGrowDir) = CTFTeamStatus::EGrowDir::GROW_WEST;
	}
	else
	{
		Assert( !"Invalid direction string for team status team grow direction" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatus::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	SetGrowDir( &m_eTeam1GrowDir, inResourceData->GetString( "team1_grow_dir" ) );
	SetGrowDir( &m_eTeam2GrowDir, inResourceData->GetString( "team2_grow_dir" ) );

	KeyValues *pItemKV = inResourceData->FindKey( "playerpanels_kv" );
	if ( pItemKV )
	{
		if ( m_pPlayerPanelKVs )
		{
			m_pPlayerPanelKVs->deleteThis();
		}
		m_pPlayerPanelKVs = new KeyValues( "playerpanels_kv" );
		pItemKV->CopySubkeys( m_pPlayerPanelKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	m_bReapplyPlayerPanelKVs = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatus::PerformLayout( void )
{
	BaseClass::PerformLayout();

	int iTeam1Count = 0;
	int iTeam2Count = 0;

	// Tally up how many are on each team so we can scale them appropriately down below
	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		if ( m_PlayerPanels[i]->GetTeam() == TF_TEAM_BLUE )
		{
			++iTeam1Count;
		}
		else if ( m_PlayerPanels[i]->GetTeam() == TF_TEAM_RED )
		{
			++iTeam2Count;
		}
	}

	C_BasePlayer *pLocalPlayer = C_BasePlayer::GetLocalPlayer();

	// Snag local player info
	int nLocalPlayerTeam = TF_TEAM_COUNT;
	int nLocalPlayerIndex = -1;
	if ( pLocalPlayer )
	{
		nLocalPlayerIndex = pLocalPlayer->entindex();
		nLocalPlayerTeam = pLocalPlayer->GetTeamNumber();
	}

	int iTeam1Processed = 0;
	int iTeam2Processed = 0;

	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		if ( m_PlayerPanels[i]->GetPlayerIndex() <= 0 )
		{
			m_PlayerPanels[i]->SetVisible( false );
			continue;
		}

		bool bIsLocalPlayerPanel = nLocalPlayerIndex == m_PlayerPanels[i]->GetPlayerIndex();
		int iTeam = m_PlayerPanels[i]->GetTeam();
		int iXPos = 0;

		// Setup vars
		EGrowDir& eGrowDir	= iTeam == TF_TEAM_BLUE ? m_eTeam1GrowDir	: m_eTeam2GrowDir;
		int& iTeamCount		= iTeam == TF_TEAM_BLUE ? iTeam1Count		: iTeam2Count;
		int& iBaseX			= iTeam == TF_TEAM_BLUE ? m_iTeam1BaseX		: m_iTeam2BaseX;
		int& iProcessed		= iTeam == TF_TEAM_BLUE ? iTeam1Processed	: iTeam2Processed;
		int& iMaxExpand		= iTeam == TF_TEAM_BLUE ? m_iTeam1MaxExpand	: m_iTeam2MaxExpand;
		const int iGap		= RemapValClamped( iTeamCount, 6, 12, m_i6v6Gap, m_i12v12Gap );

		// Local player is always the innermost panel
		int nTeamPanelIndex = bIsLocalPlayerPanel ? 0
							: iTeam == nLocalPlayerTeam ? iProcessed + 1 
							: iProcessed;

		// Setup X-position and widths
		// Use the max width if less than 6 (to fill out the space)
		int nNewWide = iTeamCount <= 6 ? m_iMaxSize : ( iMaxExpand - ( iGap * ( iTeamCount - 1 ) ) ) / iTeamCount;

		// How far each panel is apart from each other
		const int iXStep = iGap + nNewWide;
		// How many steps out this panel should be
		const int iXOffset = iXStep * nTeamPanelIndex;
		// Step out the panels by the step
		iXPos = eGrowDir == GROW_EAST ? iBaseX + iXOffset				// Align the left edge of the left-most panel on the specified point
			  : eGrowDir == GROW_WEST ? iBaseX - iXOffset - nNewWide	// Align the right edge of the right-most panel on the specified point
			  : 0;

		// This is expensive, so only do it if we need to
		if ( nNewWide != m_PlayerPanels[i]->GetWide() )
		{
			// We change the width in the KV's then reapply settings so the children will do their
			// magical proportionaltoparent layout.
			if ( m_pPlayerPanelKVs )
			{
				m_pPlayerPanelKVs->SetInt( "wide", scheme()->GetProportionalNormalizedValueEx( GetScheme(), nNewWide ) );
				m_PlayerPanels[i]->ApplySettings( m_pPlayerPanelKVs );
				m_PlayerPanels[i]->InvalidateLayout( false, true );
				m_PlayerPanels[i]->Update();
			}
		}

		if ( !bIsLocalPlayerPanel )
		{
			++iProcessed;
		}

		m_PlayerPanels[i]->SetPos( iXPos, m_PlayerPanels[i]->GetYPos() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatus::Reset()
{
	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		m_PlayerPanels[i]->Reset();
	}

	InvalidateLayout();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFTeamStatus::ShouldDraw( void )
{
	// Get the player and active weapon.
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();

	if ( !pPlayer )
		return false;

	int iLocalTeam = g_TF_PR->GetTeam( pPlayer->entindex() );
	if ( iLocalTeam < FIRST_GAME_TEAM )
		return false;

	if ( TFGameRules() )
	{
		const IMatchGroupDescription* pMatchDesc = GetMatchGroupDescription( TFGameRules()->GetCurrentMatchGroup() );
		if ( ( pMatchDesc && !pMatchDesc->BUsesMatchHUD() ) || !ShouldUseMatchHUD() )
			return false;

		if ( TFGameRules()->ShowMatchSummary() )
			return false; 
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatus::OnTick()
{
	if ( IsVisible() != ShouldDraw() )
	{
		SetVisible( ShouldDraw() );
	}

	if ( IsVisible() )
	{
		RecalculatePlayerPanels();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFTeamStatusPlayerPanel *CTFTeamStatus::GetOrAddPanel( int iPanelIndex )
{
	if ( iPanelIndex < m_PlayerPanels.Count() )
	{
		return m_PlayerPanels[iPanelIndex];
	}
	Assert( iPanelIndex == m_PlayerPanels.Count() );
	CTFTeamStatusPlayerPanel *pPanel = new CTFTeamStatusPlayerPanel( this, VarArgs( "playerpanel%d", iPanelIndex ) );
	if ( m_pPlayerPanelKVs )
	{
		pPanel->ApplySettings( m_pPlayerPanelKVs );
		pPanel->InvalidateLayout( false, true );
		InvalidateLayout();
	}
	m_PlayerPanels.AddToTail( pPanel );
	return pPanel;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatus::RecalculatePlayerPanels( void )
{
	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
	if ( !pPlayer || !g_TF_PR )
		return;

	int iPanel = 0;
	bool bNeedsLayout = false;
	int iLocalTeam = g_TF_PR->GetTeam( pPlayer->entindex() );
	if ( iLocalTeam >= FIRST_GAME_TEAM )
	{
		for ( int i = 1; i <= MAX_PLAYERS; i++ )
		{
			if ( !g_TF_PR->IsConnected( i ) )
				continue;

			int iTeam = g_TF_PR->GetTeam( i );
			if ( iTeam < FIRST_GAME_TEAM )
				continue;

			// Add an entry
			CTFTeamStatusPlayerPanel *pPanel = GetOrAddPanel( iPanel );

			if ( pPanel->GetPlayerIndex() != i )
			{
				bNeedsLayout = true;
			}

			pPanel->SetPlayerIndex( i );

			if ( pPanel->GetPreviousTeam() != pPanel->GetTeam() )
			{
				bNeedsLayout = true;
			}

			++iPanel;
		}
	}

	// Clear out any extra panels
	for ( int i = iPanel; i < m_PlayerPanels.Count(); i++ )
	{
		if ( m_PlayerPanels[i]->GetPlayerIndex() != 0 )
		{
			bNeedsLayout = true;
		}

		m_PlayerPanels[i]->SetPlayerIndex( 0 );
	}

	UpdatePlayerPanels();

	if ( bNeedsLayout )
	{
		InvalidateLayout();	
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFTeamStatus::UpdatePlayerPanels( void )
{
	if ( !g_TF_PR )
		return;

	for ( int i = 0; i < m_PlayerPanels.Count(); i++ )
	{
		m_PlayerPanels[i]->Update();
	}
}
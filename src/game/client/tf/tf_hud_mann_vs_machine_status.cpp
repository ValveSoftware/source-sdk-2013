//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: HUD Target ID element
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_hud_mann_vs_machine_status.h"

#include <vgui_controls/AnimationController.h>
#include "iclientmode.h"
#include "c_tf_objective_resource.h"
#include "tf_gamerules.h"
#include "tf_mann_vs_machine_stats.h"
#include "spectatorgui.h"
#include "engine/IEngineSound.h"
#include "c_tf_mvm_boss_progress_user.h"
#include "usermessages.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;

extern void AddSubKeyNamed( KeyValues *pKeys, const char *pszName );
extern const ConVar *sv_cheats;
extern ConVar cl_hud_minmode;


#define VICTORY_SPLASH_TIME			1.0f
#define CREDITS_COLLECTED_TIME		2.0f
#define CREDITS_MISSED_TIME			1.0f
#define CREDITS_BONUS_TIME			0.5f
#define RATING_LABEL_TIME			0.5f
#define RATING_SCORE_TIME			0.5f
#define WAIT_TIME					12.0f

// String constants that match variable names in .res files
#define CREDITS_COLLECTED_STR		"creditscollected"
#define CREDITS_MISSED_STR			"creditsmissed"
#define CREDITS_BONUS_STR			"creditbonus"
#define RESPEC_COUNT_STR			"respeccount"
#define RATING_LABEL_STR			"ratinglabel"
#define RATING_SCORE_STR			"ratingscore"


ConVar cl_mvm_wave_status_visible_during_wave( "cl_mvm_wave_status_visible_during_wave", "0", FCVAR_ARCHIVE, "Display full wave contents while a wave is active in MvM." );


//-----------------------------------------------------------------------------
// User Message Callbacks for CTFHudMannVsMachineStatus
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Purpose: Restore Checkpoint status message
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMWaveFailed )
{
	CTFHudMannVsMachineStatus *pMannVsMachineStatus = GET_HUDELEMENT( CTFHudMannVsMachineStatus );
	if ( pMannVsMachineStatus )
	{
		pMannVsMachineStatus->WaveFailed();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Announce a MVM message on the HUD
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMAnnouncement )
{
	CTFHudMannVsMachineStatus *pMannVsMachineStatus = GET_HUDELEMENT( CTFHudMannVsMachineStatus );
	if ( pMannVsMachineStatus )
	{
		uint8 nType = msg.ReadByte();
		uint8 nCount = msg.ReadByte();

		switch ( nType )
		{
		case TF_MVM_ANNOUNCEMENT_WAVE_COMPLETE:
			pMannVsMachineStatus->ShowWaveSummary( nCount );
			break;
		default:
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Players have won the MvM Pop File
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMVictory )
{
	CTFHudMannVsMachineStatus *pMannVsMachineStatus = GET_HUDELEMENT( CTFHudMannVsMachineStatus );
	if ( !pMannVsMachineStatus )
		return;
	
	bool bIsKicking = (bool)msg.ReadByte();
	int nTime = (int)msg.ReadByte();

	pMannVsMachineStatus->MVMVictory( bIsKicking, nTime );
}
//-----------------------------------------------------------------------------
// Update the time to disconnect / kick
//-----------------------------------------------------------------------------
USER_MESSAGE( MVMServerKickTimeUpdate )
{
	CTFHudMannVsMachineStatus *pMannVsMachineStatus = GET_HUDELEMENT( CTFHudMannVsMachineStatus );
	if ( !pMannVsMachineStatus )
		return;

	int nTime = (int)msg.ReadByte();

	pMannVsMachineStatus->MVMServerKickTimeUpdate( nTime );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CEnemyCountPanel );

CEnemyCountPanel::CEnemyCountPanel( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	m_bFlashing = false;

	ListenForGameEvent( "localplayer_respawn" );

	m_pEnemyCountImage = NULL;
	m_pEnemyCountImageBG = NULL;
	m_pEnemyCountCritBG = NULL;
}

//-----------------------------------------------------------------------------
void CEnemyCountPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/EnemyCountPanel.res" );

	// Save refs to images
	m_pEnemyCountImage = dynamic_cast< CTFImagePanel* >( FindChildByName( "EnemyCountImage" ) );
	m_pEnemyCountImageBG = dynamic_cast< Panel* >( FindChildByName( "EnemyCountImageBG" ) );
	m_pEnemyCountCritBG = dynamic_cast< CTFImagePanel* >( FindChildByName( "EnemyCountCritImageBG" ) );
}

//-----------------------------------------------------------------------------
void CEnemyCountPanel::FireGameEvent( IGameEvent * event )
{
	if ( FStrEq( event->GetName(), "localplayer_respawn" ) )
	{
		if ( m_bFlashing )
		{
			// when the player respawns all the animation events
			// are cleared so we need to restart them if necessary
			SetFlashing( true );
		}
	}
}

//-----------------------------------------------------------------------------
void CEnemyCountPanel::SetFlashing( bool bState )
{
	if ( m_bFlashing != bState )
	{
		m_bFlashing = bState;

		if (m_bFlashing)
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "SpyWarningFlash");
		}
		else
		{
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence(this, "SpyWarningFlashEnd");
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMBossProgressBar );

CMvMBossProgressBar::CMvMBossProgressBar( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	m_flPercentage = m_flOldPercentage = 1.0f;
	m_pProgressBar = new ScalableImagePanel( this, "ProgressBar" );
	m_pProgressBarBG = new ScalableImagePanel( this, "ProgressBarBG" );
	m_pBossImage = new CTFImagePanel( this, "TankImage" );

	m_nBarOrgX = m_nBarOrgY = m_nBarOrgW = m_nBarOrgT = 0;
	m_nBgOrgX = m_nBgOrgY = m_nBgOrgW = m_nBgOrgT = 0;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
void CMvMBossProgressBar::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/TankProgressBar.res" );

	if ( m_pProgressBar )
	{
		m_pProgressBar->GetBounds( m_nBarOrgX, m_nBarOrgY, m_nBarOrgW, m_nBarOrgT );
	}

	if ( m_pProgressBarBG )
	{
		m_pProgressBarBG->GetBounds( m_nBgOrgX, m_nBgOrgY, m_nBgOrgW, m_nBgOrgT );
	}
}

//-----------------------------------------------------------------------------
void CMvMBossProgressBar::SetPercentage( float flPercentage )
{ 
	m_flPercentage = flPercentage;
	if ( m_flPercentage < 0.0f )
	{
		m_flPercentage = 0.0f;
	}
	else if ( m_flPercentage > 1.0f )
	{
		m_flPercentage = 1.0f;
	}
}

//-----------------------------------------------------------------------------
void CMvMBossProgressBar::SetImage( const char* pszImageName )
{
	if ( m_pBossImage )
	{
		m_pBossImage->SetImage( pszImageName );
	}
}

//-----------------------------------------------------------------------------
void CMvMBossProgressBar::OnTick( void )
{	
	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	if ( m_flOldPercentage != m_flPercentage )
	{
		m_flOldPercentage = m_flPercentage;
				
		if ( m_pProgressBar )
		{
			m_pProgressBar->SetBounds(m_nBarOrgX, m_nBarOrgY, (int)(m_nBarOrgW * m_flPercentage) + m_nWidthSpacer, m_nBarOrgT );
		}

		if ( m_pProgressBarBG )
		{
			m_pProgressBarBG->SetBounds( m_nBgOrgX, m_nBgOrgY, m_nBgOrgW + m_nWidthSpacer, m_nBgOrgT );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMBossStatusPanel );

CMvMBossStatusPanel::CMvMBossStatusPanel( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	m_pBackground = new ScalableImagePanel( this, "Background" );

	for ( int i = 0; i < MAX_TANK_PROGRESS_BARS; ++i )
	{
		m_ProgressBars.AddToTail();
		m_ProgressBars[ i ] = new CMvMBossProgressBar( this, "TankProgressBar" );
		m_ProgressBars[ i ]->SetVisible( false );
	}	

	m_nBackGroundTall = 0;
	m_bPanelDirty = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
void CMvMBossStatusPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/TankStatusPanel.res" );

	if ( m_pBackground )
	{
		m_pBackground->GetBounds( m_nBackgroundOriginalX, m_nBackgroundOriginalY, m_nBackgroundOriginalW, m_nBackgroundOriginalT );
	}

	m_bPanelDirty = true;
}

//-----------------------------------------------------------------------------
void CMvMBossStatusPanel::OnTick( void )
{	
	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	CUtlVector< C_TFMvMBossProgressUser* > activeBosses;
	for ( int i = 0; i < ITFMvMBossProgressUserAutoList::AutoList().Count(); ++i )
	{
		C_TFMvMBossProgressUser *pBossProgressUser = static_cast< C_TFMvMBossProgressUser* >( ITFMvMBossProgressUserAutoList::AutoList()[i] );
		if ( pBossProgressUser->GetBossProgressImageName() != NULL )
		{
			C_BaseEntity *pEnt = dynamic_cast< C_BaseEntity* >( pBossProgressUser );
			if ( pEnt && !pEnt->IsDormant() )
			{
				activeBosses.AddToTail( pBossProgressUser );
			}
		}
	}

	// setup the background
	bool bBackgroundVisible = activeBosses.Count() > 0;
	if ( m_pBackground && ( m_pBackground->IsVisible() != bBackgroundVisible ) )
	{
		m_pBackground->SetVisible( bBackgroundVisible );
	}

	int nBackgroundTall = 0;
	int nHeightPerPanel = m_ProgressBars[0]->GetTall(); 
	int nTotalPanelHeight = ( activeBosses.Count() * nHeightPerPanel );// + ( ( nNumBosses - 1 ) * m_nSpaceBetweenPanels );
	if ( m_pBackground && bBackgroundVisible )
	{
		nBackgroundTall =  nTotalPanelHeight + ( m_nSpaceBetweenPanels * 2 );
		if ( ( m_nBackGroundTall != nBackgroundTall ) || m_bPanelDirty )
		{
			m_bPanelDirty = false;
			m_nBackGroundTall = nBackgroundTall;
			m_pBackground->SetBounds( m_nBackgroundOriginalX, m_nBackgroundOriginalY, m_nBackgroundOriginalW, m_nBackGroundTall );
		}
	}
	 
	// setup the tank progress bars
	int iPanelIndex = 0;
	int nYPos = 0;
	if ( nBackgroundTall > 0 )
	{
		nYPos = ( nBackgroundTall * 0.5 ) - ( nTotalPanelHeight * 0.5 );
	}

	for ( int i = 0 ; i < activeBosses.Count() && iPanelIndex < MAX_TANK_PROGRESS_BARS ; i++, iPanelIndex++ )
	{
		m_ProgressBars[ iPanelIndex ]->SetVisible( true );
		m_ProgressBars[ iPanelIndex ]->SetPercentage( activeBosses[i]->GetBossStatusProgress() );
		char szImg[128];
		V_snprintf( szImg, sizeof( szImg ), "../HUD/leaderboard_class_%s", activeBosses[i]->GetBossProgressImageName() );
		m_ProgressBars[ iPanelIndex ]->SetImage( szImg );
		m_ProgressBars[ iPanelIndex ]->SetPos( m_nXOffset, nYPos );

		nYPos += nHeightPerPanel;// + m_nSpaceBetweenPanels;
	}

	// turn off any unused panels
	while ( iPanelIndex < MAX_TANK_PROGRESS_BARS )
	{
		m_ProgressBars[ iPanelIndex ]->SetVisible( false );
		iPanelIndex++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CWaveStatusPanel );

CWaveStatusPanel::CWaveStatusPanel( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	m_pSeparatorBar = new vgui::Panel( this, "SeparatorBar" );
	m_pSupportLabel = new CExLabel( this, "SupportLabel", L"" );
	m_pProgressBar = new ScalableImagePanel( this, "ProgressBar" );
	m_pProgressBarBG = new ScalableImagePanel( this, "ProgressBarBG" );
	m_pBackground = new ScalableImagePanel( this, "Background" );

	for ( int i = 0; i < MVM_CLASS_TYPES_PER_WAVE_MAX_NEW; ++i )
	{
		m_EnemyCountPanels.AddToTail();
		m_EnemyCountPanels[ i ] = new CEnemyCountPanel( this, "EnemyCountPanel" );
		m_EnemyCountPanels[ i ]->SetVisible( false );
	}

	m_nBarOrgX = m_nBarOrgY = m_nBarOrgW = m_nBarOrgT = 0;
	m_nBgOrgX = m_nBgOrgY = m_nBgOrgW = m_nBgOrgT = 0;

	m_nWaveCount = -1;
	m_nMaxWaveCount = -1;
	m_nEnemyRemainingNoSupport = 0;
	m_bPanelDirty = false;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
void CWaveStatusPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues *pConditions = NULL;

	if ( m_bVerbose )
	{
		pConditions = new KeyValues( "conditions" );
		AddSubKeyNamed( pConditions, "if_verbose" );
	}

	LoadControlSettings( "resource/UI/WaveStatusPanel.res", NULL, NULL, pConditions );

	if ( pConditions )
	{
		pConditions->deleteThis();
	}

	m_bPanelDirty = true;

	if ( m_pProgressBar )
	{
		m_pProgressBar->GetBounds( m_nBarOrgX, m_nBarOrgY, m_nBarOrgW, m_nBarOrgT );
	}

	if ( m_pProgressBarBG )
	{
		m_pProgressBarBG->GetBounds( m_nBgOrgX, m_nBgOrgY, m_nBgOrgW, m_nBgOrgT );
	}
}

//-----------------------------------------------------------------------------
void CWaveStatusPanel::OnTick( void )
{	
	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	if ( !TFObjectiveResource() )
		return;

	if ( m_nWaveCount != TFObjectiveResource()->GetMannVsMachineWaveCount() || 
		m_nMaxWaveCount != TFObjectiveResource()->GetMannVsMachineMaxWaveCount() || m_bPanelDirty )
	{
		m_nWaveCount = TFObjectiveResource()->GetMannVsMachineWaveCount();
		m_nMaxWaveCount = TFObjectiveResource()->GetMannVsMachineMaxWaveCount();

		char szbuf[32];
		if ( TFObjectiveResource()->GetMvMEventPopfileType() == MVM_EVENT_POPFILE_HALLOWEEN )
		{
			V_strcpy_safe( szbuf, "666" );
		}
		else if ( m_nMaxWaveCount == 0 )
		{
			V_snprintf( szbuf, sizeof(szbuf), "%d", m_nWaveCount );
		}
		else
		{
			V_snprintf( szbuf, sizeof(szbuf), "%d / %d", m_nWaveCount, m_nMaxWaveCount );
		}

		wchar_t wszFinal[256];
		wchar_t wszCount[32];

		g_pVGuiLocalize->ConvertANSIToUnicode( szbuf, wszCount, sizeof(wszCount) );
		g_pVGuiLocalize->ConstructString_safe( wszFinal, g_pVGuiLocalize->Find( "#TF_PVE_WaveCount" ), 1, wszCount );
		SetDialogVariable( "wave_count", wszFinal );

		m_bPanelDirty = false;
	}

	UpdateEnemyCounts();

	C_BasePlayer *pLocalPlayer = CBasePlayer::GetLocalPlayer();
	if ( pLocalPlayer )
	{
		bool bVisible = m_bVerbose || pLocalPlayer->IsAlive() || ( pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR ) || ( !m_bVerbose && !pLocalPlayer->IsAlive() );
		
		// turn off if game is over
		bVisible &= TFGameRules()->State_Get() != GR_STATE_GAME_OVER;
		if ( IsVisible() != bVisible )
		{
			SetVisible( bVisible );
		}
	}
}

//-----------------------------------------------------------------------------
void CWaveStatusPanel::AddClassIconBeingUsed( CUtlVector< const char* > &vector, const char *pchIcon ) const
{
	if ( pchIcon && pchIcon[0] && !IsClassIconBeingUsed( vector, pchIcon ) )
	{
		vector.AddToHead( pchIcon );
	}
}

//-----------------------------------------------------------------------------
bool CWaveStatusPanel::IsClassIconBeingUsed( CUtlVector< const char* > &vector, const char *pchIcon ) const
{
	if ( pchIcon && pchIcon[0] )
	{
		for ( int i = 0 ; i < vector.Count() ; i++ )
		{
			if ( FStrEq( vector[i], pchIcon )  )
			{
				return true;
			}
		}
	}

	return false;
}
 
//-----------------------------------------------------------------------------
void CWaveStatusPanel::UpdateEnemyCounts( void )
{
	if ( !TFGameRules() || !TFObjectiveResource() )
		return;

	bool bBetweenWaves = TFGameRules()->InSetup() || TFObjectiveResource()->GetMannVsMachineIsBetweenWaves();
	if ( bBetweenWaves )
	{
		m_nEnemyRemainingNoSupport = 0;
	}

	int nMaxEnemyCountNoSupport = TFObjectiveResource()->GetMannVsMachineWaveEnemyCount();

	// Loop through all the class types to find which enemy counts to display
	CUtlVector< hud_enemy_data_t > miniboss;
	CUtlVector< hud_enemy_data_t > normal;
	CUtlVector< hud_enemy_data_t > support;
	CUtlVector< hud_enemy_data_t > mission;
	int nNumEnemyRemaining = 0;
	int nNumEnemyTypes = 0;
	int nNumNonVerboseTypes = 0;

	for ( int i = 0; i < MVM_CLASS_TYPES_PER_WAVE_MAX_NEW; ++i )
	{
		int nClassCount = TFObjectiveResource()->GetMannVsMachineWaveClassCount( i );
		const char *pchClassIconName = TFObjectiveResource()->GetMannVsMachineWaveClassName( i );
		unsigned int iFlags = TFObjectiveResource()->GetMannVsMachineWaveClassFlags( i );

		if ( pchClassIconName[ 0 ] != '\0' )
		{
			if ( iFlags & MVM_CLASS_FLAG_SUPPORT )
			{
				int index = support.AddToTail();
				support[index].nCount = nClassCount;
				support[index].pchClassIconName = pchClassIconName;
				support[index].iFlags = iFlags;
				support[index].bActive = TFObjectiveResource()->GetMannVsMachineWaveClassActive( i );
				nNumEnemyTypes++;

				// Show support spies
				if ( ( iFlags & MVM_CLASS_FLAG_SUPPORT_LIMITED ) && support[index].bActive )
				{
 					nNumNonVerboseTypes++;
				}
			}
			else if ( iFlags & MVM_CLASS_FLAG_MISSION )
			{
				if ( ( ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING ) && !( FStrEq( "sentry_buster", pchClassIconName ) || FStrEq( "teleporter", pchClassIconName ) ) ) || ( nClassCount > 0 ) )
				{
					int index = mission.AddToTail();
					mission[index].nCount = nClassCount;
					mission[index].pchClassIconName = pchClassIconName;
					mission[index].iFlags = iFlags;
					mission[index].bActive = TFObjectiveResource()->GetMannVsMachineWaveClassActive( i );
					nNumEnemyTypes++;
					nNumNonVerboseTypes++;
				}
			}
			else if ( iFlags & MVM_CLASS_FLAG_MINIBOSS )
			{
				if ( nClassCount > 0 )
				{
					int index = miniboss.AddToTail();
					miniboss[index].nCount = nClassCount;
					miniboss[index].pchClassIconName = pchClassIconName;
					miniboss[index].iFlags = iFlags;
					miniboss[index].bActive = TFObjectiveResource()->GetMannVsMachineWaveClassActive( i );
					nNumEnemyTypes++;
					nNumEnemyRemaining += nClassCount;
				}
			}
			else if ( iFlags & MVM_CLASS_FLAG_NORMAL )
			{
				// only show classes with > 0 remaining
				if ( nClassCount > 0 )
				{
					int index = normal.AddToTail();
					normal[index].nCount = nClassCount;
					normal[index].pchClassIconName = pchClassIconName;
					normal[index].iFlags = iFlags;
					normal[index].bActive = TFObjectiveResource()->GetMannVsMachineWaveClassActive( i );
					nNumEnemyTypes++;
					nNumEnemyRemaining += nClassCount;

					if ( FStrEq( "spy", pchClassIconName ) )
					{
						nNumNonVerboseTypes++;
					}
				}
			}
		}
	}

	// update the progress bar
	if ( nMaxEnemyCountNoSupport > 0 )
	{
		if ( m_nEnemyRemainingNoSupport != nNumEnemyRemaining )
		{
			m_nEnemyRemainingNoSupport = nNumEnemyRemaining;

			if ( m_pProgressBar )
			{
				float flPercent = (float)m_nEnemyRemainingNoSupport / (float)nMaxEnemyCountNoSupport;
				m_pProgressBar->SetBounds( m_nBarOrgX, m_nBarOrgY, (int)(m_nBarOrgW * flPercent) + m_nWidthSpacer, m_nBarOrgT );
			}
			
			if ( m_pProgressBarBG )
			{
				m_pProgressBarBG->SetBounds( m_nBgOrgX, m_nBgOrgY, m_nBgOrgW + m_nWidthSpacer, m_nBgOrgT );
			}
		}
	}

	int nXPos = 0;
	int nBgX = 0, nBgY = 0, nBgWide = 0, nBgTall = 0;
	int nEnemyCountWide = m_EnemyCountPanels[0]->GetWide();

	CTFPlayer *pLocalPlayer = ToTFPlayer( C_TFPlayer::GetLocalPlayer() );
	bool bVerbose = ( m_bVerbose || 
		            ( TFGameRules()->State_Get() != GR_STATE_RND_RUNNING ) || 
					( pLocalPlayer && ( pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR ) ) ||
					  cl_mvm_wave_status_visible_during_wave.GetBool() );
	if ( !bVerbose )
	{
		nNumEnemyTypes = nNumNonVerboseTypes;
	}

	int nSupportLabelRightSide = 0;
	int nBackGroundRightSide = 0;
	int nUpdatedBgWidth = 0;
	int nMinModeReduction = ( cl_hud_minmode.GetBool() ? -YRES(12) : 0 );

	if ( m_pBackground )
	{
		int nNumEnemyWidth = nNumEnemyTypes * nEnemyCountWide;
		int nSpacerWidth = ( nNumEnemyTypes - 1 ) * m_nWaveCountOffset;

		int nSeparatorBarWidth = 0;
		if ( support.Count() > 0 || mission.Count() > 0 )
		{
			nSeparatorBarWidth = m_pSeparatorBar ? m_pSeparatorBar->GetWide() + m_nWaveCountOffset : 0;
		}
		int nTotalEnemyWidth = nNumEnemyWidth + nSpacerWidth + nSeparatorBarWidth;

		m_pBackground->GetBounds( nBgX, nBgY, nBgWide, nBgTall );

		nUpdatedBgWidth = m_nWaveCountBGMinWidth;
		int nNeededBgWidth = nTotalEnemyWidth + ( m_nWaveCountOffset * 3 ); // add little buffer to both ends of the background
		if ( nNeededBgWidth > m_nWaveCountBGMinWidth )
		{
			nUpdatedBgWidth = nNeededBgWidth;
		}

		nBgX = ( GetWide() * 0.5 ) - ( nUpdatedBgWidth * 0.5 );
		
		nBgTall = m_nNormalHeight + nMinModeReduction;

		if ( nNumEnemyTypes > 0 )
		{
			if ( !bVerbose )
			{
				nBgTall = m_nVerboseHeightNoNumbers + nMinModeReduction;
			}
			else
			{
				nBgTall = m_nVerboseHeight + nMinModeReduction;
			}
		}

		SetTall( nBgTall + YRES( 2 ) );

		m_pBackground->SetBounds( nBgX, nBgY, nUpdatedBgWidth, nBgTall );
		nXPos = nBgX + ( nUpdatedBgWidth * 0.5 ) - ( nTotalEnemyWidth * 0.5 );

		nBackGroundRightSide = nBgX	+ nUpdatedBgWidth;
	}

	int iPanelIndex = 0;

	if ( bVerbose )
	{
		// miniboss	enemies
		for ( int i = 0 ; i < miniboss.Count() && iPanelIndex < m_EnemyCountPanels.Count() ; i++, iPanelIndex++ )
		{
			CEnemyCountPanel *pPanel = m_EnemyCountPanels[ iPanelIndex ];

			pPanel->SetVisible( true );
			pPanel->SetDialogVariable( "enemy_count", miniboss[i].nCount );
			pPanel->SetPos( nXPos, m_nWaveCountYPos + nMinModeReduction );
			pPanel->SetFlashing( false );

			if ( pPanel->m_pEnemyCountImage )
			{
				pPanel->m_pEnemyCountImage->SetImage( VarArgs( "../hud/leaderboard_class_%s", miniboss[i].pchClassIconName ) );
				pPanel->m_pEnemyCountImage->SetVisible( true );
			}
			
			if ( pPanel->m_pEnemyCountImageBG  )
			{
				pPanel->m_pEnemyCountImageBG->SetBgColor( m_clrMiniBoss );
			}
			if ( pPanel->m_pEnemyCountCritBG )
			{
				pPanel->m_pEnemyCountCritBG->SetVisible( miniboss[i].iFlags & MVM_CLASS_FLAG_ALWAYSCRIT );
			}

			nXPos += nEnemyCountWide + m_nWaveCountOffset;
		}
	}

	// normal enemies
	for ( int i = 0 ; i < normal.Count() && iPanelIndex < m_EnemyCountPanels.Count() ; i++ )
	{
		bool bNonVerboseSpy = !bVerbose && FStrEq( "spy", normal[i].pchClassIconName );
		if ( bVerbose || bNonVerboseSpy )
		{
			CEnemyCountPanel *pPanel = m_EnemyCountPanels[ iPanelIndex ];

			pPanel->SetVisible( true );
			if ( bNonVerboseSpy )
			{
				pPanel->SetDialogVariable( "enemy_count", "" );
			}
			else
			{
				pPanel->SetDialogVariable( "enemy_count", normal[i].nCount );
			}
			pPanel->SetPos( nXPos, m_nWaveCountYPos + nMinModeReduction );

			if ( pPanel->m_pEnemyCountImage )
			{
				pPanel->m_pEnemyCountImage->SetImage( VarArgs( "../hud/leaderboard_class_%s", normal[i].pchClassIconName ) );
				pPanel->m_pEnemyCountImage->SetVisible( true );
			}

			bool bResetBG = true;

			if ( bNonVerboseSpy )
			{
				if ( pPanel->IsFlashing() )
				{
					// we're already flashing so don't reset the background
					bResetBG = false;
				}
				else
				{
					// start flashing
					pPanel->SetFlashing( true );
				}
			}
			else
			{
				pPanel->SetFlashing( false );
			}

			if ( bResetBG )
			{
				if ( pPanel->m_pEnemyCountImageBG  )
				{
					pPanel->m_pEnemyCountImageBG->SetBgColor( m_clrNormal );
				}
				if ( pPanel->m_pEnemyCountCritBG )
				{
					pPanel->m_pEnemyCountCritBG->SetVisible( normal[i].iFlags & MVM_CLASS_FLAG_ALWAYSCRIT );
				}
			}

			nXPos += nEnemyCountWide + m_nWaveCountOffset;
			iPanelIndex++;
		}
	}

	// bar and label
	if ( bVerbose && ( support.Count() > 0 || mission.Count() > 0 ) )
	{
		if ( m_pSeparatorBar && m_pSupportLabel )
		{
			if ( !m_pSeparatorBar->IsVisible() )
			{
				m_pSeparatorBar->SetVisible( true );
			}

			m_pSeparatorBar->SetPos( nXPos, m_nWaveCountYPos + nMinModeReduction );
			nXPos += m_pSeparatorBar->GetWide() + m_nWaveCountOffset;

			if ( !m_pSupportLabel->IsVisible() )
			{
				m_pSupportLabel->SetVisible( true );
			}

			m_pSupportLabel->SizeToContents();
			m_pSupportLabel->SetPos( nXPos, m_nWaveCountYPos + nMinModeReduction + m_nSupportLabelYOffset );
			nSupportLabelRightSide = nXPos + m_pSupportLabel->GetWide();
		}
	}
	else
	{
		if ( m_pSeparatorBar )
		{
			if ( m_pSeparatorBar->IsVisible() )
			{
				m_pSeparatorBar->SetVisible( false );
			}
		}

		if ( m_pSupportLabel )
		{
			if ( m_pSupportLabel->IsVisible() )
			{
				m_pSupportLabel->SetVisible( false );
			}
		}
	}
	
  	CUtlVector< const char* > classIconsBeingUsed; // used temporarily to track the icons we're showing

	// support
	for ( int i = 0 ; i < support.Count() && iPanelIndex < m_EnemyCountPanels.Count() ; i++ )
	{
		bool bActive = !bVerbose && support[i].bActive && ( support[i].iFlags & MVM_CLASS_FLAG_SUPPORT_LIMITED );
		if ( bVerbose || bActive )
		{
			if ( !IsClassIconBeingUsed( classIconsBeingUsed, support[i].pchClassIconName ) )
			{
				CEnemyCountPanel *pPanel = m_EnemyCountPanels[ iPanelIndex ];

				pPanel->SetVisible( true );
				pPanel->SetDialogVariable( "enemy_count", "" );
				pPanel->SetPos( nXPos, m_nWaveCountYPos + nMinModeReduction );
				pPanel->SetFlashing( false );

				if ( pPanel->m_pEnemyCountImage )
				{
					pPanel->m_pEnemyCountImage->SetImage( VarArgs( "../hud/leaderboard_class_%s", support[i].pchClassIconName ) );
					pPanel->m_pEnemyCountImage->SetVisible( true );
				}

				AddClassIconBeingUsed( classIconsBeingUsed, support[i].pchClassIconName );

				if ( pPanel->m_pEnemyCountImageBG  )
				{
					pPanel->m_pEnemyCountImageBG->SetBgColor( m_clrNormal );
				}

				if ( pPanel->m_pEnemyCountCritBG )
				{
					pPanel->m_pEnemyCountCritBG->SetVisible( support[i].iFlags & MVM_CLASS_FLAG_ALWAYSCRIT );
				}

				nXPos += nEnemyCountWide + m_nWaveCountOffset;
				iPanelIndex++;
			}
		}
	}

	// missions
	for ( int i = 0 ; i < mission.Count() && iPanelIndex < m_EnemyCountPanels.Count() ; i++, iPanelIndex++ )
	{
		if ( !IsClassIconBeingUsed( classIconsBeingUsed, mission[i].pchClassIconName ) )
		{
			CEnemyCountPanel *pPanel = m_EnemyCountPanels[ iPanelIndex ];

			pPanel->SetVisible( true );
			pPanel->SetDialogVariable( "enemy_count", "" );
			pPanel->SetPos( nXPos, m_nWaveCountYPos + nMinModeReduction );

			if ( pPanel->m_pEnemyCountImage )
			{
				const char* pchMissionClassIconName = mission[i].pchClassIconName;
				pPanel->m_pEnemyCountImage->SetImage( VarArgs( "../hud/leaderboard_class_%s", pchMissionClassIconName ) );
				pPanel->m_pEnemyCountImage->SetVisible( true );

				bool bResetBG = true;

				if ( !bBetweenWaves && ( FStrEq( "spy", pchMissionClassIconName ) || FStrEq( "sentry_buster", pchMissionClassIconName ) || FStrEq( "engineer", pchMissionClassIconName ) ) )
				{
					if ( pPanel->IsFlashing() )
					{
						// we're already flashing so don't reset the background
						bResetBG = false;
					}
					else
					{
						// start flashing
						pPanel->SetFlashing( true );
					}
				}
				else
				{
					pPanel->SetFlashing( false );
				}
			
				if ( pPanel->m_pEnemyCountCritBG )
				{
					pPanel->m_pEnemyCountCritBG->SetVisible( mission[i].iFlags & MVM_CLASS_FLAG_ALWAYSCRIT );
				}

				if ( bResetBG )
				{
					if ( pPanel->m_pEnemyCountImageBG )
					{
						pPanel->m_pEnemyCountImageBG->SetBgColor( m_clrNormal );
					}
					if ( pPanel->m_pEnemyCountCritBG )
					{
						pPanel->m_pEnemyCountCritBG->SetVisible( false );
					}
				}
			}
			
			AddClassIconBeingUsed( classIconsBeingUsed, mission[i].pchClassIconName );

			nXPos += nEnemyCountWide + m_nWaveCountOffset;
		}
	}
	
	// final check to make sure the background covers the support label
	if ( nSupportLabelRightSide > nBackGroundRightSide )
	{
		if ( m_pBackground )
		{
			int nDelta = nSupportLabelRightSide - nBackGroundRightSide + ( m_nWaveCountOffset * 1.5 );
			m_pBackground->SetBounds( nBgX - nDelta, nBgY, nUpdatedBgWidth + ( nDelta * 2 ), nBgTall );
		}
	}

	// turn off any unused panels
	while ( iPanelIndex < m_EnemyCountPanels.Count() )
	{
		CEnemyCountPanel *pPanel = m_EnemyCountPanels[ iPanelIndex ];

		pPanel->SetVisible( false );
		pPanel->SetFlashing( false );
		iPanelIndex++;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_HUDELEMENT( CCurrencyStatusPanel );

CCurrencyStatusPanel::CCurrencyStatusPanel( const char *pElementName )
	: CHudElement( pElementName )
	, vgui::EditablePanel( NULL, "CurrencyStatusPanel" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_nCurrency = 0;
	m_nTargetCurrency = 0;
	SetDialogVariable( "currency", "$0" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );
}

//-----------------------------------------------------------------------------
void CCurrencyStatusPanel::OnTick( void )
{
	BaseClass::OnTick();

	if ( !IsVisible() )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pLocalPlayer )
	{
		m_nTargetCurrency = pLocalPlayer->GetCurrency();

		if ( UpdateHUD() )
		{
			pLocalPlayer->EmitSound( "Credits.Updated" );
		}
	}
}

//-----------------------------------------------------------------------------
bool CCurrencyStatusPanel::ShouldDraw( void )
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || pLocalPlayer->GetObserverMode() == OBS_MODE_FREEZECAM || !TFGameRules() || !TFGameRules()->GameModeUsesCurrency() )
	{
		return false;
	}

	if ( TFGameRules()->IsPVEModeActive() )
	{
		// Josh: Seems like an intentional design decision that
		// in MvM the money panel stays up when you are dead.
		// So I am not mirroring the IsAlive change for the non-MvM stuff.
		if ( pLocalPlayer->GetTeamNumber() != TF_TEAM_PVE_DEFENDERS || 
			 pLocalPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		{
			return false;
		}
	}
	else
	{
		if ( !pLocalPlayer->IsAlive() ||
			 pLocalPlayer->GetTeamNumber() == TEAM_UNASSIGNED ||
			 pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR ||
			 pLocalPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED )
		{
			return false;
		}
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
void CCurrencyStatusPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/HudCurrencyAccount.res" );
}

//-----------------------------------------------------------------------------
bool CCurrencyStatusPanel::UpdateHUD( void )
{
	if ( m_nTargetCurrency == m_nCurrency )
	{
		return false;
	}

	int delta = m_nTargetCurrency - m_nCurrency;

	if ( delta > 0 )
	{
		if ( delta > 100 )
		{
			delta = 100;
		}
		else if ( delta > 10 )
		{
			delta = 10;
		}
		else
		{
			delta = 1;
		}
	}
	else
	{
		if ( delta < -100 )
		{
			delta = -100;
		}
		else if ( delta < -10 )
		{
			delta = -10;
		}
		else
		{
			delta = -1;
		}
	}

	m_nCurrency += delta;

	char szTmp[16];
	Q_snprintf( szTmp, ARRAYSIZE( szTmp ), "$%d", m_nCurrency );
	SetDialogVariable( "currency", szTmp );
	
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CInWorldCurrencyStatus );

CInWorldCurrencyStatus::CInWorldCurrencyStatus( Panel *parent, const char *name ) : vgui::EditablePanel( parent, name )
{
	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );

	SetDialogVariable( "currency", "" );

	m_pGood = NULL;
	m_pBad = NULL;
}

//-----------------------------------------------------------------------------
void CInWorldCurrencyStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/MvMInWorldCurrency.res" );

	m_pGood = dynamic_cast<vgui::Label*>( FindChildByName( "CurrencyGood" ) );
	m_pBad = dynamic_cast<vgui::Label*>( FindChildByName( "CurrencyBad" ) );
}

//-----------------------------------------------------------------------------
void CInWorldCurrencyStatus::OnTick( void )
{
	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( !pLocalPlayer || pLocalPlayer->GetPlayerClass()->GetClassIndex() == TF_CLASS_UNDEFINED ||
		pLocalPlayer->GetTeamNumber() == TEAM_SPECTATOR )
	{
		if ( IsVisible() )
		{
			SetVisible( false );
		}
		return;
	}

	if ( !IsVisible() )
	{
		SetVisible( true );
	}

	int nWorldMoney = TFObjectiveResource()->GetMvMInWorldMoney();
	int iWaveIndex = TFObjectiveResource()->GetMannVsMachineWaveCount();
	if ( TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() && iWaveIndex > 1 )
	{
		iWaveIndex--;
	}

	if ( m_pGood && m_pBad )
	{
		C_MannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
		int nMissed = pStats ? (int)pStats->GetMissedCredits( iWaveIndex - 1 ) : 0;
		if ( (pStats && nMissed > nWorldMoney) || ( TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() && nWorldMoney == 0 ))
		{
			m_pBad->SetVisible( true );
			m_pGood->SetVisible( false );
		}
		else
		{
			m_pBad->SetVisible( false );
			m_pGood->SetVisible( true );
		}
	}

	char szTmp[16];
	Q_snprintf( szTmp, ARRAYSIZE( szTmp ), "$%d", nWorldMoney );
	SetDialogVariable( "currency", szTmp );
}
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CWarningSwoop );

CWarningSwoop::CWarningSwoop( Panel *parent, const char *name ) : vgui::ImagePanel( parent, name )
{

}

//-----------------------------------------------------------------------------
void CWarningSwoop::PaintBackground( void )
{
	float flElapsedTime = ( gpGlobals->curtime - m_flStartCapAnimStart );

	if ( GetImage() )
	{
		surface()->DrawSetColor( 255, 255, 255, 255 );
		int iYPos = RemapValClamped( flElapsedTime, 0, m_flSwoopTime, 0, GetTall() );
		GetImage()->SetPos( 0, GetTall() - iYPos );
		GetImage()->SetSize( GetWide(), GetTall() );
		GetImage()->Paint();
	}

	if ( flElapsedTime >= m_flSwoopTime )
	{
		SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
bool CWarningSwoop::IsVisible( void )
{
	if ( IsInFreezeCam() == true )
		return false;

	return BaseClass::IsVisible();
}

//-----------------------------------------------------------------------------
void CWarningSwoop::StartSwoop( void )
{
	SetVisible( true );
	m_flStartCapAnimStart = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CWaveCompleteSummaryPanel );

CWaveCompleteSummaryPanel::CWaveCompleteSummaryPanel( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	m_eState = FINISHED;

	m_pWaveCompleteContainer = NULL;
	m_pCreditContainerPanel = NULL;
	m_pRatingContainerPanel = NULL;

	m_pCreditBonusTextLabel = NULL;
	m_pCreditBonusCountLabel = NULL;

	m_pRespecBackground = NULL;
	m_pRespecContainerPanel = NULL;
	m_pRespecTextLabel = NULL;
	m_pRespecCountLabel = NULL;
}

//-----------------------------------------------------------------------------
void CWaveCompleteSummaryPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/WaveCompleteSummaryPanel.res" );

	m_pWaveCompleteContainer = dynamic_cast<vgui::EditablePanel*>( FindChildByName("WaveCompleteContainer") );
	m_pCreditContainerPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("CreditContainer") );
	m_pRatingContainerPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("RatingContainer") );

	if ( m_pCreditContainerPanel )
	{
		m_pCreditBonusTextLabel = dynamic_cast<vgui::Label*>( m_pCreditContainerPanel->FindChildByName("CreditBonusTextLabel") );
		m_pCreditBonusCountLabel = dynamic_cast<vgui::Label*>( m_pCreditContainerPanel->FindChildByName("CreditBonusCountLabel") );
	}
	
	m_pRespecBackground = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName("RespecBackground") );
	m_pRespecContainerPanel = dynamic_cast<vgui::EditablePanel*>( FindChildByName("RespecContainer") );

	if ( m_pRespecContainerPanel )
	{
		m_pRespecTextLabel = dynamic_cast<vgui::Label*>( m_pRespecContainerPanel->FindChildByName("RespecTextLabelWin") );
		m_pRespecCountLabel = dynamic_cast<vgui::Label*>( m_pRespecContainerPanel->FindChildByName("RespecCountLabel") );
	}
}

//-----------------------------------------------------------------------------
void CWaveCompleteSummaryPanel::ShowWaveSummary( int nWaveNumber )
{
	// if in progress just add time (reset it)
	if ( m_eState != FINISHED ) 
	{
		m_fStateRunningTime = 0;
		return;
	}

	m_nWaveNumber = nWaveNumber;

	if ( m_pWaveCompleteContainer )
	{
		// Set all the values to empty strings
		if ( nWaveNumber == -1 )
		{
			m_pWaveCompleteContainer->SetDialogVariable( "titletext", g_pVGuiLocalize->Find( "#Winpanel_PVE_Evil_Wins" ) );
		}
		else
		{
			m_pWaveCompleteContainer->SetDialogVariable( "titletext", g_pVGuiLocalize->Find( "#TF_PVE_WaveComplete" ) );
		}
	}

	if ( m_pCreditContainerPanel )
	{
		m_pCreditContainerPanel->SetDialogVariable( CREDITS_COLLECTED_STR, "" );
		m_pCreditContainerPanel->SetDialogVariable( CREDITS_MISSED_STR, "" );
		m_pCreditContainerPanel->SetDialogVariable( CREDITS_BONUS_STR, "" );
	}

	if ( m_pRatingContainerPanel )
	{
		m_pRatingContainerPanel->SetDialogVariable( RATING_LABEL_STR, "" );
		m_pRatingContainerPanel->SetDialogVariable( RATING_SCORE_STR, "" );
	}

	if ( m_pRespecContainerPanel )
	{
		m_pRespecContainerPanel->SetDialogVariable( RESPEC_COUNT_STR, "" );
	}

	m_eState = CREDITS_COLLECT;
	m_fStateRunningTime = 0;
	m_fPreviousTick = gpGlobals->curtime;

	int nAcquired = MannVsMachineStats_GetAcquiredCredits( nWaveNumber, false );
	int nDropped = MannVsMachineStats_GetDroppedCredits( nWaveNumber );
	int nPickedUp = MIN( nAcquired, nDropped );

	int nMissed = nDropped - nPickedUp;
	int nBonus = Max( (int)MannVsMachineStats_GetAcquiredCredits( m_nWaveNumber, true ) - nAcquired, 0 );

	m_nCreditsCollected = nPickedUp;
	m_nCreditsMissed = nMissed;
	m_nCreditBonus = nBonus;

	if ( m_pCreditBonusTextLabel )
	{
		m_pCreditBonusTextLabel->SetVisible( false );
	}

	if ( m_pCreditBonusCountLabel )
	{
		m_pCreditBonusCountLabel->SetVisible( false );
	}

	if ( m_pRespecBackground && m_pRespecTextLabel && m_pRespecCountLabel )
	{
		m_pRespecBackground->SetVisible( false );
		m_pRespecTextLabel->SetVisible( false );
		m_pRespecCountLabel->SetVisible( false );
	}

	SetVisible( true );
}

//-----------------------------------------------------------------------------
void CWaveCompleteSummaryPanel::OnTick( void )
{	
	if ( TFGameRules()->State_Get() != GR_STATE_BETWEEN_RNDS && TFGameRules()->State_Get() != GR_STATE_GAME_OVER && TFGameRules()->State_Get() != GR_STATE_TEAM_WIN )
	{
		SetVisible( false );
		return;
	}

	if ( TFObjectiveResource() && TFObjectiveResource()->GetMannVsMachineWaveCount() == 0 )
	{
		SetVisible( false );
		return;
	}

	if ( m_eState == FINISHED )
		return;
	
	m_fStateRunningTime += gpGlobals->curtime - m_fPreviousTick;
	m_fPreviousTick = gpGlobals->curtime;

	CheckCredits();

	// Run through animation loop
	switch ( m_eState )
	{
	case CREDITS_COLLECT:
		StateUpdateValue ( m_pCreditContainerPanel, CREDITS_COLLECTED_STR, CREDITS_COLLECTED_TIME, m_fStateRunningTime, CREDITS_MISSED, m_nCreditsCollected );
		break;
	case CREDITS_MISSED:
		StateUpdateValue ( m_pCreditContainerPanel, CREDITS_MISSED_STR, CREDITS_MISSED_TIME, m_fStateRunningTime, RATING_LABEL, m_nCreditsMissed );
		break;
	case RATING_LABEL:
		RatingLabelUpdate();
		CheckState( RATING_LABEL_TIME, m_fStateRunningTime, RATING_SCORE );
		break;
	case RATING_SCORE:
		RatingScoreUpdate();
		CheckState( RATING_SCORE_TIME, m_fStateRunningTime, WAIT );
		break;
	case WAIT:
		if ( CheckState( WAIT_TIME, m_fStateRunningTime, FINISHED ) )
		{
			SetVisible( false );
		}
		break;
	case FINISHED:
		SetVisible( false );
		break;
	default:
		SetVisible( false );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: updates the target field based on the input args.  Returns TRUE if transitioning to new state
//-----------------------------------------------------------------------------
bool CWaveCompleteSummaryPanel::StateUpdateValue( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int endValue )
{
	float fPercent = currentTime / targetTime;
	fPercent = 1.0 < fPercent ? 1.0f : fPercent;

	if ( parent )
	{
		parent->SetDialogVariable( field, (int)(endValue * fPercent) );
	}

	// transition to next state
	return CheckState( targetTime, currentTime, nextState );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWaveCompleteSummaryPanel::RatingLabelUpdate( void )
{
	if ( m_pRatingContainerPanel )
	{
		m_pRatingContainerPanel->SetDialogVariable( RATING_LABEL_STR, g_pVGuiLocalize->Find( "#TF_PVE_CreditRating" ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWaveCompleteSummaryPanel::RatingScoreUpdate( )
{
	//calc a score
	const char* pletterScore = "F";

	int nAcquired = MannVsMachineStats_GetAcquiredCredits( m_nWaveNumber, false );
	int nDropped = MannVsMachineStats_GetDroppedCredits( m_nWaveNumber );

	float fPercent = (float)nAcquired / (float)nDropped;

	if ( fPercent >= 1.0 )
	{
		pletterScore = "A+";
	}
	else if ( fPercent >= 0.9 )
	{
		pletterScore = "A";
	}
	else if ( fPercent >= 0.8 )
	{
		pletterScore = "B";
	}
	else if ( fPercent >= 0.7 )
	{
		pletterScore = "C";
	}
	else if ( fPercent >= 0.6 )
	{
		pletterScore = "D";
	}

	if ( m_pRatingContainerPanel )
	{
		m_pRatingContainerPanel->SetDialogVariable( RATING_SCORE_STR, pletterScore );
	}
}
//-----------------------------------------------------------------------------
bool CWaveCompleteSummaryPanel::CheckState( float targetTime, float currentTime, int nextState )
{
	if ( currentTime >= targetTime )
	{
		m_fStateRunningTime = 0;
		m_eState = nextState;
		return true;
	}
	return false;
}
//-----------------------------------------------------------------------------
// Purpose: Check CreditCounts incase they have changed after Summary first showed on screen.
//-----------------------------------------------------------------------------
void CWaveCompleteSummaryPanel::CheckCredits() 
{
	int nAcquired = MannVsMachineStats_GetAcquiredCredits( m_nWaveNumber, false );
	int nDropped = MannVsMachineStats_GetDroppedCredits( m_nWaveNumber );
	int nPickedUp = Min( nAcquired, nDropped );
	int nMissed = nDropped - nPickedUp;
	int nBonus = Max( (int)MannVsMachineStats_GetAcquiredCredits( m_nWaveNumber, true ) - nAcquired, 0 );

	if ( m_eState > CREDITS_COLLECT )
	{
		if ( m_nCreditsCollected != nPickedUp )
		{
			if ( m_pCreditContainerPanel )
			{
				m_pCreditContainerPanel->SetDialogVariable( CREDITS_COLLECTED_STR, nPickedUp );
			}
			m_nCreditsCollected = nPickedUp;
		}
	}

	if ( m_eState > CREDITS_MISSED )
	{
		if ( m_nCreditsMissed != nMissed)
		{
			if ( m_pCreditContainerPanel )
			{
				m_pCreditContainerPanel->SetDialogVariable( CREDITS_MISSED_STR, nMissed );
			}
			m_nCreditsMissed = nMissed;
		}

		if ( m_nCreditBonus != nBonus )
		{
			if ( m_pCreditContainerPanel )
			{
				m_pCreditContainerPanel->SetDialogVariable( CREDITS_BONUS_STR, nBonus );
			}
			m_nCreditBonus = nBonus;
		}

		if ( m_nCreditBonus > 0 )
		{
			if ( m_pCreditBonusCountLabel )
			{
				m_pCreditBonusCountLabel->SetVisible( true );
			}

			if ( m_pCreditBonusTextLabel )
			{
				m_pCreditBonusTextLabel->SetVisible( true );
			}
		}
	}

	// Respec
	if ( TFGameRules()->IsMannVsMachineRespecEnabled() )
	{
		CMannVsMachineStats *pStats = MannVsMachineStats_GetInstance();
		if ( pStats )
		{
			uint16 nRespecs = pStats->GetNumRespecsEarnedInWave();
			bool bVisible = nRespecs > 0;
			if ( bVisible )
			{
				if ( m_pRespecContainerPanel )
				{
					m_pRespecContainerPanel->SetDialogVariable( RESPEC_COUNT_STR, nRespecs );
				}

				if ( m_pRespecBackground && m_pRespecCountLabel && m_pRespecTextLabel )
				{
					if ( !m_pRespecBackground->IsVisible() && !m_pRespecCountLabel->IsVisible() && !m_pRespecTextLabel->IsVisible() )
					{
						C_TFPlayer *pLocalTFPlayer = C_TFPlayer::GetLocalTFPlayer();
						if ( pLocalTFPlayer )
						{
							pLocalTFPlayer->EmitSound( "MVM.RespecAwarded" );
						}
						g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "RespecEarnedPulse" );
					}

					if ( m_pRespecBackground->IsVisible() != bVisible )
					{
						m_pRespecBackground->SetVisible( bVisible );
					}
					if ( m_pRespecCountLabel->IsVisible() != bVisible )
					{
						m_pRespecCountLabel->SetVisible( bVisible );
					}
					if ( m_pRespecTextLabel->IsVisible() != bVisible )
					{
						m_pRespecTextLabel->SetVisible( bVisible );
					}					
				}
			}
		}
	}

	if ( m_eState > RATING_SCORE )
	{
		RatingLabelUpdate();
		RatingScoreUpdate();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CVictorySplash );

CVictorySplash::CVictorySplash( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	
}

//-----------------------------------------------------------------------------
void CVictorySplash::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/MvMVictorySplash.res" );
}

//-----------------------------------------------------------------------------
void CVictorySplash::OnTick( void )
{
	
}

//-----------------------------------------------------------------------------
// CMvMBombCarrierProgress
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CMvMBombCarrierProgress );

CMvMBombCarrierProgress::CMvMBombCarrierProgress( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{

}

//-----------------------------------------------------------------------------
void CMvMBombCarrierProgress::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "resource/UI/MvMBombCarrierProgressPanel.res" );
}

//-----------------------------------------------------------------------------
// CTFHudMannVsMachineStatus 
//-----------------------------------------------------------------------------
DECLARE_HUDELEMENT( CTFHudMannVsMachineStatus );

CTFHudMannVsMachineStatus::CTFHudMannVsMachineStatus( const char *pElementName ) :
CHudElement( pElementName ), BaseClass( NULL, "HudMannVsMachineStatus" )
{
	Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	SetHiddenBits( HIDEHUD_MISCSTATUS );

	m_pWarningSwoop = new CWarningSwoop( this, "WarningSwoop" );
	m_pWaveStatusPanel = new CWaveStatusPanel( this, "WaveStatusPanel" );
	m_pWaveCompletePanel = new CWaveCompleteSummaryPanel( this, "WaveCompleteSummaryPanel" );

	m_pVictorySplash = new CVictorySplash( this, "VictorySplash" );
	m_pVictoryContainer = new CMvMVictoryPanelContainer( this, "VictoryPanelContainer" );
	
	m_pWaveLossPanel = new CMvMWaveLossPanel ( this, "WaveLossPanel" );

	m_nFlagCarrierUpgradeLevel = -1;
	m_pUpgradeLevelContainer = new vgui::EditablePanel( this, "UpgradeLevelContainer" );
	m_pUpgradeLevel1 = new vgui::ImagePanel( m_pUpgradeLevelContainer, "UpgradeLevel1" );
	m_pUpgradeLevel2 = new vgui::ImagePanel( m_pUpgradeLevelContainer, "UpgradeLevel2" );
	m_pUpgradeLevel3 = new vgui::ImagePanel( m_pUpgradeLevelContainer, "UpgradeLevel3" );
	m_pUpgradeLevelBoss = new vgui::ImagePanel( m_pUpgradeLevelContainer, "UpgradeLevelBoss" );

	m_nNextWaveTime = 0;
	m_nSpyMissionCount = 0;

 	m_bSpecPanelVisible = false;

	m_bInVictorySplash = false;

	m_bAdjustWaveStatusPanel = false;

	ListenForGameEvent( "mvm_mission_update" );
	ListenForGameEvent( "localplayer_changeteam" );
	ListenForGameEvent( "mvm_begin_wave" );

	vgui::ivgui()->AddTickSignal( GetVPanel(), 100 );
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/HudMannVsMachineStatus.res" );

	m_bSpecPanelVisible = false;

	m_pWaveCompletePanel->SetVisible( false );

	CMvMBombCarrierProgress *parent = dynamic_cast<CMvMBombCarrierProgress*>( m_pUpgradeLevelContainer->FindChildByName( "UpgradeProgressTrack" ) );
	if ( parent )
	{
		parent->ApplySchemeSettings( pScheme );
		m_pBombUpgradeMeterMask = dynamic_cast<vgui::EditablePanel*>( parent->FindChildByName( "FillContainer" ) );
		m_nUpgradeMaskBaseWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 20 );
		m_nUpgradeMaskMaxWidth = scheme()->GetProportionalScaledValueEx( GetScheme(), 60 );
	}

	m_pServerChangeMessage = dynamic_cast<vgui::EditablePanel*>( FindChildByName( "ServerChangeMessage" ) );
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::FireGameEvent( IGameEvent * event )
{
	if ( !ShouldDraw() )
		return;

	C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();

	const char *type = event->GetName();

	if ( Q_strcmp( type, "mvm_mission_update" ) == 0 )
	{
		int nClass = event->GetInt( "class", 0 );
		if ( nClass == TF_CLASS_SPY )
		{
			int nCount = event->GetInt( "count", 0 );

			if ( nCount > 0 )
			{
				if ( m_nSpyMissionCount == 0 )
				{
					if ( pPlayer )
					{
						pPlayer->EmitSound( "Announcer.MVM_Spy_Alert" );
					}

					if ( m_pWarningSwoop )
					{
						m_pWarningSwoop->StartSwoop();
					}
				}
// 				else if ( m_nSpyMissionCount > nCount )
// 				{
// 					if ( pPlayer )
// 					{
// 						switch ( nCount )
// 						{
// 						case 6: 
// 							pPlayer->EmitSound( "Announcer.mvm_spybot_death_six" );
// 							break;
// 						case 5:
// 							pPlayer->EmitSound( "Announcer.mvm_spybot_death_five" );
// 							break;
// 						case 4:
// 							pPlayer->EmitSound( "Announcer.mvm_spybot_death_four" );
// 							break;
// 						case 3:
// 							pPlayer->EmitSound( "Announcer.mvm_spybot_death_three" );
// 							break;
// 						case 2:
// 							pPlayer->EmitSound( "Announcer.mvm_spybot_death_two" );
// 							break;
// 						case 1:
// 							pPlayer->EmitSound( "Announcer.mvm_spybot_death_one" );
// 							break;
// 						}
// 					}
// 				}

				m_nSpyMissionCount = nCount;
			}
			else
			{
				if ( m_nSpyMissionCount != 0 )
				{
					m_nSpyMissionCount = 0;

					if ( pPlayer )
					{
						pPlayer->EmitSound( "Announcer.mvm_spybot_death_all" );
					}
				}
			}
		}
	}
	else if ( Q_strcmp( type, "localplayer_changeteam" ) == 0 )
	{
		m_bAdjustWaveStatusPanel = true;
	}
	else if ( Q_strcmp( type, "mvm_begin_wave" ) == 0 )
	{
		m_nSpyMissionCount = 0;
	}
}

//-----------------------------------------------------------------------------
bool CTFHudMannVsMachineStatus::ShouldDraw( void )
{
	// Don't draw in freezecam
	C_TFPlayer *pPlayer = CTFPlayer::GetLocalTFPlayer();
	if ( !pPlayer || pPlayer->GetObserverMode() == OBS_MODE_FREEZECAM || !TFGameRules()->IsMannVsMachineMode() )
	{
		return false;
	}

	return CHudElement::ShouldDraw();
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::OnTick( void )
{	
	if ( !TFGameRules() || !TFGameRules()->IsMannVsMachineMode() )
		return;

	if ( m_pVictoryContainer->IsVisible() )
	{
		m_pVictoryContainer->OnTick();
	}

	if ( !IsVisible() || !TFObjectiveResource() )
		return;

	m_pWaveCompletePanel->OnTick();
	
	m_pWaveLossPanel->OnTick();
	
	if ( g_pSpectatorGUI && m_pWaveStatusPanel )
	{
		if ( ( m_bSpecPanelVisible != g_pSpectatorGUI->IsVisible() ) || m_bAdjustWaveStatusPanel )
		{
			int xPos, yPos;
			m_pWaveStatusPanel->GetPos( xPos, yPos );

			m_bSpecPanelVisible = g_pSpectatorGUI->IsVisible();

			if ( m_bSpecPanelVisible )
			{
				m_pWaveStatusPanel->SetPos( xPos, g_pSpectatorGUI->GetTopBarHeight() );
			}
			else
			{
				m_pWaveStatusPanel->SetPos( xPos, 0 );
			}

			m_bAdjustWaveStatusPanel = false;
		}
	}

	int nTime = 0;

	if ( TFObjectiveResource()->GetMannVsMachineNextWaveTime() > 0.0f )
	{
		nTime = TFObjectiveResource()->GetMannVsMachineNextWaveTime() - gpGlobals->curtime;
	}

	if ( TFGameRules()->InSetup() && ObjectiveResource() && !TFGameRules()->IsInTournamentMode() )
	{
		CTeamRoundTimer *pTimer = dynamic_cast< CTeamRoundTimer* >( ClientEntityList().GetEnt( ObjectiveResource()->GetTimerToShowInHUD() ) );
		if ( pTimer )
		{
			nTime = pTimer->GetTimeRemaining();
		}
	}

	if ( nTime < 0 )
	{
		nTime = 0;
	}

	if ( TFGameRules()->State_Get() > GR_STATE_PREGAME )
	{
		if ( m_nNextWaveTime != nTime )
		{
			m_nNextWaveTime = nTime;

			// announcer countdown
			C_BasePlayer *pPlayer = C_BasePlayer::GetLocalPlayer();
			if ( pPlayer )
			{
				switch( nTime )
				{
				case 10:
					if ( TFObjectiveResource()->GetMannVsMachineWaveCount() + 1 >= TFObjectiveResource()->GetMannVsMachineMaxWaveCount() )
					{
						pPlayer->EmitSound( "Announcer.MVM_Final_Wave_Start" );
					}
					else if ( TFObjectiveResource()->GetMannVsMachineWaveCount() == 0 )
					{
						pPlayer->EmitSound( "Announcer.MVM_First_Wave_Start" );
					}
					else
					{
						pPlayer->EmitSound( "Announcer.MVM_Wave_Start" );
					}
					break;
				case 5:		pPlayer->EmitSound( "Announcer.RoundBegins5Seconds" );	break;
				case 4:		pPlayer->EmitSound( "Announcer.RoundBegins4Seconds" );	break;
				case 3:		pPlayer->EmitSound( "Announcer.RoundBegins3Seconds" );	break;
				case 2:		pPlayer->EmitSound( "Announcer.RoundBegins2Seconds" );	break;
				case 1:		pPlayer->EmitSound( "Announcer.RoundBegins1Seconds" );	break;
				}
			}
		}
	}

	// Bomb Carrier Indictators
	UpdateBombCarrierProgress();

	if ( m_bInVictorySplash && gpGlobals->curtime > m_flVictoryTimer )
	{
		m_bInVictorySplash = false;
		m_pVictorySplash->SetVisible( false );
		m_pVictoryContainer->ShowVictoryPanel( false );
	}

	m_pVictorySplash->SetVisible( m_bInVictorySplash && TFGameRules()->State_Get() == GR_STATE_GAME_OVER );

	// Check to see if we need to display server message
	if ( TFGameRules()->State_Get() == GR_STATE_GAME_OVER && !m_bInVictorySplash)
	{
		//calculate seconds
		m_pServerChangeMessage->SetVisible( true );
		int seconds = MAX(0, (int)(m_flServerEndTime - gpGlobals->curtime) );
		wchar_t wszTime[16];
		_snwprintf( wszTime, ARRAYSIZE( wszTime ), L"%d", seconds );
		wchar_t wszLocalizedMessage[512];

		if ( !m_bIsServerKicking )
		{
			if ( seconds > 1 )
			{
				g_pVGuiLocalize->ConstructString_safe( wszLocalizedMessage, g_pVGuiLocalize->Find( "#TF_PVE_Server_Message_Reset" ), 1, wszTime );
			}
			else
			{
				g_pVGuiLocalize->ConstructString_safe( wszLocalizedMessage, g_pVGuiLocalize->Find( "#TF_PVE_Server_Message_ResetNoS" ), 1, wszTime );
			}
			m_pServerChangeMessage->SetDialogVariable( "servermessage", wszLocalizedMessage);
		}
		else
		{
			if ( seconds > 1 )
			{
				g_pVGuiLocalize->ConstructString_safe( wszLocalizedMessage, g_pVGuiLocalize->Find( "#TF_PVE_Server_Message_Kick" ), 1, wszTime );
			}
			else
			{
				g_pVGuiLocalize->ConstructString_safe( wszLocalizedMessage, g_pVGuiLocalize->Find( "#TF_PVE_Server_Message_KickNoS" ), 1, wszTime );
			}
			m_pServerChangeMessage->SetDialogVariable( "servermessage", wszLocalizedMessage);
		}
	}
	else
	{
		m_pServerChangeMessage->SetVisible( false );
	}
	
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::WaveFailed( void )
{
	if ( TFObjectiveResource() && TFObjectiveResource()->GetMannVsMachineWaveCount() > 1 )
	{
		m_pWaveLossPanel->ShowPanel();
	}
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::ShowWaveSummary( int nWaveNumber) 
{
	m_pWaveCompletePanel->ShowWaveSummary( nWaveNumber );
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::MVMVictory( bool bIsKicking, int nTime )
{
	m_bInVictorySplash = true;
	m_flVictoryTimer = gpGlobals->curtime + VICTORY_SPLASH_TIME;
	m_pVictorySplash->SetVisible( true );

	m_bIsServerKicking = bIsKicking;
	m_flServerEndTime = gpGlobals->curtime + (float)nTime;
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::MVMServerKickTimeUpdate( int nTime )
{
	m_flServerEndTime = gpGlobals->curtime + (float)nTime;
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::MVMVictoryGCResponse( CMsgMvMVictoryInfo &pData )
{
	m_pVictoryContainer->MannUpServerResponse( pData );
}

void CTFHudMannVsMachineStatus::ForceVictoryRefresh()
{
	InvalidateLayout( false, true );
}

void CTFHudMannVsMachineStatus::ReopenVictoryPanel( void )
{
	m_pVictoryContainer->ShowVictoryPanel( true );
}

//-----------------------------------------------------------------------------
// Private
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::UpdateBombCarrierProgress ( void )
{
	m_pUpgradeLevelContainer->SetVisible( TFGameRules()->State_Get() == GR_STATE_RND_RUNNING );

	if ( !m_pUpgradeLevel1 || !m_pUpgradeLevel2 || !m_pUpgradeLevel3 || !m_pUpgradeLevelBoss )
		return;

	// Width of Meter
	float flBase = TFObjectiveResource()->GetNextMvMBombUpgradeTime() - TFObjectiveResource()->GetBaseMvMBombUpgradeTime();
	float flCurr = TFObjectiveResource()->GetNextMvMBombUpgradeTime() - gpGlobals->curtime;
	
	float flPercent = 0;
	int nFlagLevel = TFObjectiveResource()->GetFlagCarrierUpgradeLevel();
	if ( flBase <= 0 || nFlagLevel >= 3 )
	{
		flPercent = nFlagLevel >= 3 ? 1.0 : 0;
	}
	else 
	{
		flPercent = MIN( 1.0f, 1.0f - (flCurr / flBase) );
	}
	m_pBombUpgradeMeterMask->SetWide( ( m_nUpgradeMaskBaseWidth * ( nFlagLevel + 1 ) ) + flPercent * m_nUpgradeMaskBaseWidth );

	// Updating Image
	if ( m_nFlagCarrierUpgradeLevel == nFlagLevel )
		return;

	m_pUpgradeLevel1->SetImage( "../hud/hud_mvm_bomb_upgrade_1_disabled" );
	m_pUpgradeLevel2->SetImage( "../hud/hud_mvm_bomb_upgrade_2_disabled" );
	m_pUpgradeLevel3->SetImage( "../hud/hud_mvm_bomb_upgrade_3_disabled" );

	m_nFlagCarrierUpgradeLevel = nFlagLevel;

	switch ( m_nFlagCarrierUpgradeLevel )
	{
	case 4:
		m_pUpgradeLevelBoss->SetVisible( true );
		m_pUpgradeLevel1->SetVisible( false );
		m_pUpgradeLevel2->SetVisible( false );
		m_pUpgradeLevel3->SetVisible( false );
		break;

	case 3:
		m_pUpgradeLevel3->SetImage( "../hud/hud_mvm_bomb_upgrade_3" );
		// Intentionally fall through
	case 2:
		m_pUpgradeLevel2->SetImage( "../hud/hud_mvm_bomb_upgrade_2" );
		// Intentionally fall through
	case 1:
		m_pUpgradeLevel1->SetImage( "../hud/hud_mvm_bomb_upgrade_1" );
		// Intentionally fall through

	default:
		m_pUpgradeLevelBoss->SetVisible( false );
		m_pUpgradeLevel1->SetVisible( true );
		m_pUpgradeLevel2->SetVisible( true );
		m_pUpgradeLevel3->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineStatus::UpdateServerMessage( void )
{
	
}
//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Scoreboard for MvM
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_hud_mann_vs_machine_scoreboard.h"
#include <filesystem.h>
#include <time.h>
#include "tf_lobby_server.h"

using namespace vgui;


extern ConVar tf_mvm_respec_credit_goal;
extern ConVar tf_mvm_respec_limit;
extern ConVar tf_scoreboard_alt_class_icons;

DECLARE_BUILD_FACTORY( CMvMScoreboardEnemyInfo );

//-----------------------------------------------------------------------------
CMvMScoreboardEnemyInfo::CMvMScoreboardEnemyInfo( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{

}

//-----------------------------------------------------------------------------
void CMvMScoreboardEnemyInfo::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/MvMScoreboardEnemyInfo.res" );
}
//-----------------------------------------------------------------------------
void CMvMScoreboardEnemyInfo::FireGameEvent( IGameEvent *event )
{

}
//-----------------------------------------------------------------------------
void CMvMScoreboardEnemyInfo::UpdateEntry( char* icon, bool bIsGiant )
{
	const wchar_t *pwchHint = g_pVGuiLocalize->Find( VarArgs( "#TF_PVE_ROBOT_%s", icon ) );
	if ( pwchHint && pwchHint[ 0 ] != L'\0' )
	{
		SetDialogVariable( "description", pwchHint );
	}
	else 
	{
		SetDialogVariable( "description", VarArgs( "NYI: #TF_PVE_ROBOT_%s", icon ) );
	}

	CTFImagePanel *pImage = dynamic_cast< CTFImagePanel* >( FindChildByName( "EnemyIcon" ) );
	if ( pImage )
	{
		pImage->SetImage( VarArgs( "../hud/leaderboard_class_%s", icon ) );
		pImage->SetVisible( true );
	}

	Panel *pBGPanel = dynamic_cast< Panel* >( FindChildByName( "Background" ) );
	if ( pBGPanel )
	{
		if ( bIsGiant )	
		{
			pBGPanel->SetBgColor( m_clrMiniBoss );
		}
		else
		{
			pBGPanel->SetBgColor( m_clrNormal );
		}
	}
}

//-----------------------------------------------------------------------------
// CTFHudMannVsMachineScoreboard
//-----------------------------------------------------------------------------
DECLARE_BUILD_FACTORY( CTFHudMannVsMachineScoreboard );

CTFHudMannVsMachineScoreboard::CTFHudMannVsMachineScoreboard( Panel *parent, const char *pName ): vgui::EditablePanel( parent, pName )
{
	m_pPlayerList = new SectionedListPanel( this, "MvMPlayerList" );

	m_pImageList = NULL;
	m_mapAvatarsToImageList.SetLessFunc( DefLessFunc( CSteamID ) );
	m_mapAvatarsToImageList.RemoveAll();

	m_pLocalPlayerStatsPanel = new vgui::EditablePanel( this, "LocalPlayerStatsPanel" );
	m_pCreditStatsPanel = new vgui::EditablePanel( this, "CreditStatsContainer");

	m_pDifficultyContainer = new vgui::EditablePanel( this, "DifficultyContainer");

	m_iDisplayedWave = -1;
	m_bInitialized = false;

	m_iSquadSurplusTexture = 0;

	Q_memset( m_iImageClass, NULL, sizeof( m_iImageClass ) );
	Q_memset( m_iImageClassAlt, NULL, sizeof( m_iImageClassAlt ) );

	ListenForGameEvent( "mvm_wave_complete" );
	ListenForGameEvent( "mvm_reset_stats" );
	ListenForGameEvent( "player_connect_client" );
}

//-----------------------------------------------------------------------------
CTFHudMannVsMachineScoreboard::~CTFHudMannVsMachineScoreboard()
{
	if ( NULL != m_pImageList )
	{
		delete m_pImageList;
		m_pImageList = NULL;
	}
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	// load control settings...
	LoadControlSettings( "resource/UI/MvMScoreboard.res" );

	InitPlayerList( pScheme );
	m_iDisplayedWave = -1;

	if ( m_pCreditStatsPanel )
	{
		m_pPreviousWaveCreditsInfo = dynamic_cast<CCreditDisplayPanel*>( m_pCreditStatsPanel->FindChildByName("PreviousWaveCreditInfoPanel") );
		m_pTotalCreditsInfo = dynamic_cast<CCreditDisplayPanel*>( m_pCreditStatsPanel->FindChildByName("TotalGameCreditInfoPanel") );	
		m_pPreviousWaveCreditsSpend = dynamic_cast<CCreditSpendPanel*>( m_pCreditStatsPanel->FindChildByName("PreviousWaveCreditSpendPanel") );
		m_pTotalCreditsSpend = dynamic_cast<CCreditSpendPanel*>( m_pCreditStatsPanel->FindChildByName("TotalGameCreditSpendPanel") );
		m_pRespecStatusLabel = dynamic_cast< CExLabel* >( m_pCreditStatsPanel->FindChildByName( "RespecStatusLabel" ) );
	}

	// Last
	m_bInitialized = true;
	m_popfile[0] = '\0'; // clear the popfile name so everything will be reset again in the UpdatePopFile() call in OnTick()
	OnTick();
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::FireGameEvent( IGameEvent * event )
{
	const char *pszEvent = event->GetName();

	if ( !Q_strcmp( pszEvent, "mvm_wave_complete" ) )
	{
		if ( !g_TF_PR )
			return;


		// Adds current stats to "prev" containers (i.e. current wave's stats added to total of all previous wave stats)
		g_TF_PR->UpdatePlayerScoreStats();
	}
	else if ( !Q_strcmp( pszEvent, "mvm_reset_stats" ) )
	{
		if ( !g_TF_PR )
			return;

		g_TF_PR->ResetPlayerScoreStats();
	}
	else if ( !Q_strcmp( pszEvent, "player_connect_client" ) )
	{
		if ( !g_TF_PR )
			return;

		g_TF_PR->ResetPlayerScoreStats( event->GetInt( "index" ) + 1 );
	}
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::OnTick ()
{
	// Ensure the panels and everything else have been initialized
	if (!m_bInitialized
		|| !TFGameRules() 
		|| !TFGameRules()->IsMannVsMachineMode()
		|| !TFObjectiveResource() )
	{
		return;
	}

	UpdatePlayerList();
	UpdateCreditStats();
	UpdatePopFile();
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::InitPlayerList ( IScheme *pScheme ) 
{
	// Scoreboard
	if ( m_pImageList )
	{
		delete m_pImageList;
	}
	m_pImageList = new ImageList( false );
	m_mapAvatarsToImageList.RemoveAll();

	m_pPlayerListBackground = dynamic_cast<vgui::ScalableImagePanel*>( FindChildByName("PlayerListBackground") );
	
	m_iImageDead = m_pImageList->AddImage( scheme()->GetImage( "../hud/leaderboard_dead", true ) );
	m_iSquadSurplusTexture = m_pImageList->AddImage( scheme()->GetImage( "pve/mvm_squad_surplus_small", true ) );

	for(int i = 1 ; i < SCOREBOARD_CLASS_ICONS ; i++)
	{
		m_iImageClass[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszClassIcons[i], true ) );
		m_iImageClassAlt[i] = m_pImageList->AddImage( scheme()->GetImage( g_pszClassIconsAlt[i], true ) );
	}

	// resize the images to our resolution
	for (int i = 1 ; i < m_pImageList->GetImageCount(); i++ )
	{
		int wide = 14, tall = 14;
		m_pImageList->GetImage(i)->SetSize(scheme()->GetProportionalScaledValueEx( GetScheme(), wide ), scheme()->GetProportionalScaledValueEx( GetScheme(),tall ) );
	}

	m_pPlayerList->SetVerticalScrollbar( false );
	m_pPlayerList->RemoveAll();
	m_pPlayerList->RemoveAllSections();
	m_pPlayerList->AddSection( 0, "Players" );
	m_pPlayerList->SetSectionAlwaysVisible( 0, true );
	m_pPlayerList->SetSectionFgColor( 0, Color( 255, 255, 255, 255 ) );
	m_pPlayerList->SetBgColor( Color( 0, 0, 0, 0 ) );
	m_pPlayerList->SetBorder( NULL );

	m_hScoreFont = pScheme->GetFont( "ScoreboardVerySmall", true );	
	m_pPlayerList->AddColumnToSection( 0, "medal", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iMedalWidth );
	m_pPlayerList->AddColumnToSection( 0, "spacer1", "", 0, m_iMedalSpacerWidth );
	m_pPlayerList->AddColumnToSection( 0, "avatar", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_RIGHT, m_iAvatarWidth );
	m_pPlayerList->AddColumnToSection( 0, "spacer0", "", 0, m_iSpacerWidth );
	m_pPlayerList->AddColumnToSection( 0, "name", "#TF_Scoreboard_Name", 0, m_iNameWidth );
	m_pPlayerList->AddColumnToSection( 0, "class", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iClassWidth );
	m_pPlayerList->AddColumnToSection( 0, "tour_no", "#TF_MvMScoreboard_Tour", SectionedListPanel::COLUMN_RIGHT, m_iClassWidth );
	m_pPlayerList->AddColumnToSection( 0, "score", "#TF_Scoreboard_Score", SectionedListPanel::COLUMN_RIGHT, m_iStatWidth );
	m_pPlayerList->AddColumnToSection( 0, "damage", "#TF_MvMScoreboard_Damage", SectionedListPanel::COLUMN_RIGHT, m_iStatWidth );
	m_pPlayerList->AddColumnToSection( 0, "tank", "#TF_MvMScoreboard_Tank", SectionedListPanel::COLUMN_RIGHT, m_iStatWidth );
	m_pPlayerList->AddColumnToSection( 0, "healing", "#TF_MvMScoreboard_Healing", SectionedListPanel::COLUMN_RIGHT, m_iStatWidth );
	m_pPlayerList->AddColumnToSection( 0, "support", "#TF_MvMScoreboard_Support", SectionedListPanel::COLUMN_RIGHT, m_iStatWidth );
	// m_pPlayerList->AddColumnToSection( 0, "blocked", "Blocked", 0, m_iStatWidth );
	// m_pPlayerList->AddColumnToSection( 0, "bonus", "Bonus", 0, m_iStatWidth );
	m_pPlayerList->AddColumnToSection( 0, "credits", "#TF_MvMScoreboard_Money", SectionedListPanel::COLUMN_RIGHT, m_iStatWidth );
	m_pPlayerList->AddColumnToSection( 0, "squad_surplus", "", SectionedListPanel::COLUMN_IMAGE | SectionedListPanel::COLUMN_CENTER, m_iStatWidth );
	m_pPlayerList->AddColumnToSection( 0, "ping", "#TF_Scoreboard_Ping", SectionedListPanel::COLUMN_RIGHT, m_iPingWidth );

	m_pPlayerList->SetImageList( m_pImageList, false );
	m_pPlayerList->SetVisible( true );
}

//-----------------------------------------------------------------------------
char *ConvertScoreboardValueToString( int iValue )
{
	static char szConversion[32];
	szConversion[0] = '\0';

	if ( iValue >= 1000000 )
	{
		V_sprintf_safe( szConversion, "%d%s%d%d%d%s%d%d%d", iValue / 1000000, ",", ( iValue % 1000000 ) / 100000, ( iValue % 100000 ) / 10000, ( iValue % 10000 ) / 1000, ",", ( iValue % 1000 ) / 100, ( iValue % 100 ) / 10, iValue % 10 );
	}
	else if ( iValue >= 1000 )
	{
		V_sprintf_safe( szConversion, "%d%s%d%d%d", iValue / 1000, ",", ( iValue % 1000 ) / 100, ( iValue % 100 ) / 10, iValue % 10 );
	}
	else if ( iValue >= 0 )
	{
		V_sprintf_safe( szConversion, "%d", iValue  );
	}

	return szConversion;
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::UpdatePlayerList () 
{
	m_pPlayerList->ClearSelection();
	m_pPlayerList->RemoveAll();

	if ( !g_TF_PR )
		return;

	for( int playerIndex = 1 ; playerIndex <= MAX_PLAYERS; playerIndex++ )
	{
		if( !g_PR->IsConnected( playerIndex ) )
		{
			continue;
		}

		if ( g_PR->GetTeam( playerIndex ) != TF_TEAM_PVE_DEFENDERS ) 
		{
			continue;
		}

		const char *szName = g_TF_PR->GetPlayerName( playerIndex );
		KeyValues *pKeyValues = new KeyValues( "data" );

		pKeyValues->SetInt( "playerIndex", playerIndex );
		pKeyValues->SetString( "name", szName );
			
		bool bAlive = g_TF_PR->IsAlive( playerIndex );

		if( g_PR->IsConnected( playerIndex ) )
		{
			int iClass = g_TF_PR->GetPlayerClass( playerIndex );
			if ( GetLocalPlayerIndex() == playerIndex && !bAlive )
			{
				// If this is local player and he is dead, show desired class (which he will spawn as) rather than current class.
				C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
				int iDesiredClass = pPlayer->m_Shared.GetDesiredPlayerClassIndex();
				// use desired class unless it's random -- if random, his future class is not decided until moment of spawn
				if ( TF_CLASS_RANDOM != iDesiredClass )
				{
					iClass = iDesiredClass;
				}
			} 
			else 
			{
				// for non-local players, show the current class
				iClass = g_TF_PR->GetPlayerClass( playerIndex );
			}

			if ( iClass >= TF_FIRST_NORMAL_CLASS && iClass <= TF_LAST_NORMAL_CLASS )
			{
				if ( bAlive )
				{
					pKeyValues->SetInt( "class", tf_scoreboard_alt_class_icons.GetBool() ? m_iImageClassAlt[iClass] : m_iImageClass[iClass] );
				}
				else
				{
					pKeyValues->SetInt( "class", tf_scoreboard_alt_class_icons.GetBool() ? m_iImageClassAlt[iClass + 9] : m_iImageClass[iClass + 9] ); // +9 is to jump ahead to the darker dead icons
				}
			}
			else
			{
				pKeyValues->SetInt( "class", 0 );
			}
		}

		// check for bots first, so malicious server operators can't fake a ping and stuff their server with bots that look like players
		if ( g_PR->IsFakePlayer( playerIndex ) )
		{
			pKeyValues->SetString( "ping", "#TF_Scoreboard_Bot" );
		}
		else
		{
			if ( g_PR->GetPing( playerIndex ) < 1 )
			{
				pKeyValues->SetString( "ping", "" );
			}
			else
			{
				pKeyValues->SetString( "ping", ConvertScoreboardValueToString( g_PR->GetPing( playerIndex ) ) );
			}
		}

		Color fgClr;
		Color bgClr;
		// change color based off alive or dead status. Also slightly different if its local player since the name is highlighted.
		if ( bAlive )
		{
			UpdatePlayerAvatar( playerIndex, pKeyValues );
			fgClr = g_PR->GetTeamColor( TF_TEAM_RED );
			bgClr = Color( 0, 0, 0, 0 );
		}
		else
		{
			pKeyValues->SetInt( "avatar", m_iImageDead );
			fgClr = Color( 117, 107, 94, 255 );
			bgClr = Color( 78, 78, 78, 150 );
		}

		// the medal column is just a place holder for the images that are displayed later
		pKeyValues->SetInt( "medal", 0 );

		if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
		{
			CTFGSLobby *pLobby = GTFGCClientSystem()->GetLobby();
			if ( pLobby )
			{
				int nTourNo = 0;
				int bSurplusEnabled = false;
				int idxMember = pLobby->GetMemberIndexBySteamID( GetSteamIDForPlayerIndex( playerIndex ) );
				if ( idxMember >= 0 )
				{
					ConstTFLobbyPlayer member = pLobby->GetMemberDetails( idxMember );
					// I guess they could be on standby to join the match while being themselves ad-hoc or something
					// bizarre?
					bSurplusEnabled = member.BMatchPlayer() && member.GetSquadSurplus();
					nTourNo = member.GetBadgeLevel();
				}

				if ( bSurplusEnabled )
				{
					pKeyValues->SetInt( "squad_surplus", m_iSquadSurplusTexture );
				}
				else
				{
					pKeyValues->SetInt( "squad_surplus", 0 );
				}

				if ( nTourNo > 0 )
				{
					pKeyValues->SetString( "tour_no", ConvertScoreboardValueToString( nTourNo ) );
				}
				else
				{
					pKeyValues->SetString( "tour_no", "" );
				}
			}

			// int nTeam = g_PR->GetTeam( playerIndex );
			// int nTeamDamage = g_TF_PR->GetTeamDamage( nTeam );
			// int nTeamTankDamage = g_TF_PR->GetTeamDamageBoss( nTeam );
			// int nTeamAssist = g_TF_PR->GetTeamDamageAssist( nTeam ) + g_TF_PR->GetTeamHealingAssist( nTeam );
			// int nTeamHealing = g_TF_PR->GetTeamHealingAssist( nTeam );

			// Note: Inflate bonus points when bucketing with healing and damage assist.
			// We do this because each have different weights.  Example: Every 250 points
			// of healing or damage assist results in 1 point of Score.  Bonus requires
			// only 10.  This makes it easier for players to evaluate their "support" value.
			int nSupport = g_TF_PR->GetDamageAssist( playerIndex ) + 
						   g_TF_PR->GetHealingAssist( playerIndex ) +
						   g_TF_PR->GetDamageBlocked( playerIndex ) +
						   ( g_TF_PR->GetBonusPoints( playerIndex ) * 25 );

			pKeyValues->SetString( "score", ConvertScoreboardValueToString( g_TF_PR->GetTotalScore( playerIndex ) ) );
			pKeyValues->SetString( "damage", ConvertScoreboardValueToString( g_TF_PR->GetDamage( playerIndex ) ) );
			pKeyValues->SetString( "tank", ConvertScoreboardValueToString( g_TF_PR->GetDamageBoss( playerIndex ) ) );
			pKeyValues->SetString( "healing", ConvertScoreboardValueToString( g_TF_PR->GetHealing( playerIndex ) ) );
			pKeyValues->SetString( "support", ConvertScoreboardValueToString( nSupport ) );
			//pKeyValues->SetString( "blocked", ConvertScoreboardValueToString( g_TF_PR->GetDamageBlocked( playerIndex ) ) );
			//pKeyValues->SetString( "bonus", ConvertScoreboardValueToString( g_TF_PR->GetBonusPoints( playerIndex ) ) );
			pKeyValues->SetString( "credits", ConvertScoreboardValueToString( g_TF_PR->GetCurrencyCollected( playerIndex ) ) );
		}	

		int itemID = m_pPlayerList->AddItem( 0, pKeyValues );
			
		m_pPlayerList->SetItemFgColor( itemID, fgClr );
		m_pPlayerList->SetItemBgColor( itemID, bgClr );
		m_pPlayerList->SetItemFont( itemID, m_hScoreFont );
			
		pKeyValues->deleteThis();
	}

	// force the list to PerformLayout() now so we can update our medal images after we return
	m_pPlayerList->InvalidateLayout( true );
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::UpdatePlayerAvatar( int playerIndex, KeyValues *kv )
{
	// Update their avatar
	if ( kv && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() )
	{
		player_info_t pi;
		if ( engine->GetPlayerInfo( playerIndex, &pi ) )
		{
			if ( pi.friendsID )
			{
				CSteamID steamIDForPlayer( pi.friendsID, 1, GetUniverse(), k_EAccountTypeIndividual );

				// See if we already have that avatar in our list
				int iMapIndex = m_mapAvatarsToImageList.Find( steamIDForPlayer );
				int iImageIndex;
				if ( iMapIndex == m_mapAvatarsToImageList.InvalidIndex() )
				{
					CAvatarImage *pImage = new CAvatarImage();
					pImage->SetAvatarSteamID( steamIDForPlayer );
					pImage->SetAvatarSize( 32, 32 );	// Deliberately non scaling
					pImage->SetDrawFriend( false );
					iImageIndex = m_pImageList->AddImage( pImage );

					m_mapAvatarsToImageList.Insert( steamIDForPlayer, iImageIndex );
				}
				else
				{
					iImageIndex = m_mapAvatarsToImageList[ iMapIndex ];
				}

				kv->SetInt( "avatar", iImageIndex );

				CAvatarImage *pAvIm = (CAvatarImage *)m_pImageList->GetImage( iImageIndex );
				pAvIm->UpdateFriendStatus();
			}
		}
	}
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::UpdateCreditStats()
{
	int iWaveNumber = TFObjectiveResource()->GetMannVsMachineWaveCount() - 1;
	if ( ( TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() == true ) && ( iWaveNumber > 0 ) )
	{
		iWaveNumber--; // Examine Previous wave credits
	}

	if ( !m_pPreviousWaveCreditsInfo || !m_pTotalCreditsInfo )
	{
		return;
	}

	CMannVsMachineStats *pMVMStats = MannVsMachineStats_GetInstance();
	if ( pMVMStats == NULL )
	{
		return;
	}

	// Previous or current
	int nAcquired = pMVMStats->GetAcquiredCredits( iWaveNumber, false );
	int nMissed = pMVMStats->GetMissedCredits( iWaveNumber );
	int nBonus = pMVMStats->GetBonusCredits( iWaveNumber );

	wchar_t wszWaveNumber[16];
	_snwprintf( wszWaveNumber, ARRAYSIZE( wszWaveNumber ), L"%d", MAX( 1, iWaveNumber + 1 ) );

	wchar_t wszLocalizedWave[512];
	g_pVGuiLocalize->ConstructString_safe( wszLocalizedWave, g_pVGuiLocalize->Find( "#TF_PVE_WaveCount" ), 1, wszWaveNumber );
	m_pPreviousWaveCreditsInfo->SetDialogVariable( "header", wszLocalizedWave );
	UpdateCreditPanel( m_pPreviousWaveCreditsInfo, nAcquired, nMissed, nBonus );

	// Total
	nAcquired = pMVMStats->GetAcquiredCredits( -1, false );
	nMissed = pMVMStats->GetMissedCredits( -1 );
	nBonus = pMVMStats->GetBonusCredits( -1 );
	m_pTotalCreditsInfo->SetDialogVariable( "header", g_pVGuiLocalize->Find( "#TF_PVE_GameTotal" ) );

	UpdateCreditPanel( m_pTotalCreditsInfo, nAcquired, nMissed, nBonus );

	// Credit Spend
	if ( m_pPreviousWaveCreditsSpend )
	{
		m_pPreviousWaveCreditsSpend->SetDialogVariable( "header", "" );
	}
		
	CPlayerWaveSpendingStats *pWaveStats = pMVMStats->GetLocalSpending( iWaveNumber );
	int nUpgradeSpending = pWaveStats ? pWaveStats->nCreditsSpentOnUpgrades : 0;
	int nBuyBackSpending = pWaveStats ? pWaveStats->nCreditsSpentOnBuyBacks : 0;
	int nBottleSpending = pWaveStats ? pWaveStats->nCreditsSpentOnBottles : 0;
	UpdateCreditSpend( m_pPreviousWaveCreditsSpend, nUpgradeSpending, nBuyBackSpending, nBottleSpending );

	if ( m_pTotalCreditsSpend )
	{
		m_pTotalCreditsSpend->SetDialogVariable( "header", "" );
	}

	// Get the current WaveStats, have to re-add 1 from above
	if ( ( TFObjectiveResource()->GetMannVsMachineIsBetweenWaves() == true ) && ( TFObjectiveResource()->GetMannVsMachineWaveCount() > 1 ) )
	{
		pWaveStats = pMVMStats->GetLocalSpending( iWaveNumber + 1 );
	}
	else
	{
		pWaveStats = pMVMStats->GetLocalSpending( iWaveNumber );
	}

	nUpgradeSpending = pWaveStats ? pWaveStats->nCreditsSpentOnUpgrades : 0;
	nBuyBackSpending = pWaveStats ? pWaveStats->nCreditsSpentOnBuyBacks : 0;
	nBottleSpending = pWaveStats ? pWaveStats->nCreditsSpentOnBottles : 0;

	pWaveStats = pMVMStats->GetLocalSpending( -1 );
	nUpgradeSpending += pWaveStats ? pWaveStats->nCreditsSpentOnUpgrades : 0;
	nBuyBackSpending += pWaveStats ? pWaveStats->nCreditsSpentOnBuyBacks : 0;
	nBottleSpending += pWaveStats ? pWaveStats->nCreditsSpentOnBottles : 0;

	UpdateCreditSpend( m_pTotalCreditsSpend, nUpgradeSpending, nBuyBackSpending, nBottleSpending );

	if ( m_pRespecStatusLabel )
	{
		// Respec progress.  Don't show if we aren't limiting respecs.
		bool bShowRespec = TFGameRules() && TFGameRules()->IsMannVsMachineRespecEnabled() && tf_mvm_respec_limit.GetInt();
		if ( m_pRespecStatusLabel->IsVisible() != bShowRespec )
		{
			m_pRespecStatusLabel->SetVisible( bShowRespec );
		}
		if ( bShowRespec )
		{
			int nCredits = pMVMStats->GetAcquiredCreditsForRespec();
			int nCreditGoal = tf_mvm_respec_credit_goal.GetInt();
			wchar_t wzRespecProg[256];
			wchar_t wzCredits[32];
			wchar_t wzGoal[32];
			g_pVGuiLocalize->ConvertANSIToUnicode( CFmtStr( "%d", nCredits ), wzCredits, sizeof( wzCredits ) );
			g_pVGuiLocalize->ConvertANSIToUnicode( CFmtStr( "%d", nCreditGoal ), wzGoal, sizeof( wzGoal ) );
			g_pVGuiLocalize->ConstructString_safe( wzRespecProg, g_pVGuiLocalize->Find( "#TF_PVE_RespecsProgress" ), 2, wzCredits, wzGoal );
			m_pRespecStatusLabel->SetText( wzRespecProg );
		}
	}
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::UpdateCreditPanel( CCreditDisplayPanel *panel, int nAcquired, int nMissed, int nBonus )
{
	panel->SetDialogVariable( "creditscollected", nAcquired );
	panel->SetDialogVariable( "creditsmissed", nMissed );
	panel->SetDialogVariable( "creditbonus", nBonus );

	int nDropped = nAcquired + nMissed;

	if ( ( TFGameRules() && TFGameRules()->State_Get() == GR_STATE_BETWEEN_RNDS && TFObjectiveResource()->GetMannVsMachineWaveCount() == 1 ) || nDropped == 0 )
	{
		panel->SetDialogVariable( "rating", "" );
		panel->SetDialogVariable( "ratingshadow", "" );
		return;
	}

	//calc a score
	const char* pletterScore = "F";

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

	panel->SetDialogVariable( "rating", pletterScore );
	panel->SetDialogVariable( "ratingshadow", pletterScore );
}

//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::UpdateCreditSpend ( CCreditSpendPanel *panel, int nUpgrades, int nBuybacks, int nBottles )
{
	if ( panel )
	{
		panel->SetDialogVariable( "upgrades", nUpgrades );
		panel->SetDialogVariable( "buybacks", nBuybacks );
		panel->SetDialogVariable( "bottles", nBottles );
	}
}
//-----------------------------------------------------------------------------
void CTFHudMannVsMachineScoreboard::UpdatePopFile( void )
{
	if ( TFObjectiveResource() )
	{
		if ( Q_strcmp( m_popfile, TFObjectiveResource()->GetMvMPopFileName() ) != 0 )
		{
			V_strcpy_safe( m_popfile, TFObjectiveResource()->GetMvMPopFileName() );
			char szTempName[MAX_PATH];
			V_FileBase( m_popfile, szTempName, sizeof( szTempName ) );
			int iChallengeIndex = GetItemSchema()->FindMvmMissionByName( szTempName );

			if ( GetItemSchema()->GetMvmMissions().IsValidIndex( iChallengeIndex ) )
			{
				const MvMMission_t &mission = GetItemSchema()->GetMvmMissions()[ iChallengeIndex ];
				wchar_t wszChallengeName[ 256 ];
				g_pVGuiLocalize->ConstructString_safe( wszChallengeName, L"%s1 (%s2)", 2, 
					g_pVGuiLocalize->Find( mission.m_sDisplayName.Get() ), g_pVGuiLocalize->Find( mission.m_sMode.Get() ) );

				SetDialogVariable( "popfile", wszChallengeName );
				
				m_pDifficultyContainer->SetVisible( true );
				m_pDifficultyContainer->SetDialogVariable( "difficultyvalue", g_pVGuiLocalize->Find( GetMvMChallengeDifficultyLocName( mission.m_eDifficulty ) ) );
			}
			else 
			{
				SetDialogVariable( "popfile", GetMapDisplayName(szTempName) );
				// Hide Difficulty Panel since we dont know what it is
				m_pDifficultyContainer->SetVisible( false );
			}
		}
	}
}


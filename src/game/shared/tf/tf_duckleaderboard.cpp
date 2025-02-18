//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"
#include "tf_duckleaderboard.h"
#ifdef CLIENT_DLL
	#include "vgui_avatarimage.h"
	#include "tf_item_inventory.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

//-------------------------------
const char *g_szDuckLeaderboardNames[] =
{
	"TF_DUCK_SCORING_OVERALL_RATING",				// TF_DUCK_SCORING_OVERALL_RATING
	"TF_DUCK_SCORING_PERSONAL_GENERATION",			// TF_DUCK_SCORING_PERSONAL_GENERATION
	"TF_DUCK_SCORING_PERSONAL_PICKUP_OFFENSE",		// TF_DUCK_SCORING_PERSONAL_PICKUP_OFFENSE
	"TF_DUCK_SCORING_PERSONAL_PICKUP_DEFENDED",		// TF_DUCK_SCORING_PERSONAL_PICKUP_DEFENDED
	"TF_DUCK_SCORING_PERSONAL_PICKUP_OBJECTIVE",	// TF_DUCK_SCORING_PERSONAL_PICKUP_OBJECTIVE
	"TF_DUCK_SCORING_TEAM_PICKUP_MY_DUCKS",			// TF_DUCK_SCORING_TEAM_PICKUP_MY_DUCKS
	"TF_DUCK_SCORING_PERSONAL_BONUS_PICKUP",		// TF_DUCK_SCORING_PERSONAL_BONUS_PICKUP
};
COMPILE_TIME_ASSERT( ARRAYSIZE( g_szDuckLeaderboardNames ) == DUCK_NUM_LEADERBOARDS );

#ifdef CLIENT_DLL

//-----------------------------------------------------------------------------
CDucksLeaderboard::CDucksLeaderboard( Panel *parent, const char *panelName, const char *pszDuckLeaderboardname )
	: CTFLeaderboardPanel( parent, panelName )
	, m_pszDuckLeaderboardName( pszDuckLeaderboardname )
	, m_pToolTip( NULL )
{
	m_pToolTip = new CTFTextToolTip( this );
	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );
}

//-----------------------------------------------------------------------------
CDucksLeaderboard::~CDucksLeaderboard()
{}

//-----------------------------------------------------------------------------
void CDucksLeaderboard::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/DucksLeaderboardPanel.res" );
}

//-----------------------------------------------------------------------------
bool CDucksLeaderboard::GetLeaderboardData( CUtlVector< LeaderboardEntry_t* > &scores )
{
	return Leaderboards_GetDuckLeaderboard( scores, m_pszDuckLeaderboardName );
}

//-----------------------------------------------------------------------------
bool CDucksLeaderboard::UpdateLeaderboards()
{
	CUtlVector< LeaderboardEntry_t* > scores;
	if ( !GetLeaderboardData( scores ) )
		return false;

	CSteamID localSteamID;
	if ( steamapicontext && steamapicontext->SteamUser() )
	{
		localSteamID = steamapicontext->SteamUser()->GetSteamID();
	}

	// Scores were empty but the leaderboard query was OK?  This will happen while the user
	// and their friends has no scores.  For now, insert a dummy value for the local player.
	LeaderboardEntry_t dummyentry;
	if ( scores.IsEmpty() )
	{
		dummyentry.m_nScore = 0;
		dummyentry.m_steamIDUser = localSteamID;
		scores.AddToTail( &dummyentry );
	}

	int nStartingIndex = 0;
	FOR_EACH_VEC( scores, i )
	{
		if ( scores[ i ]->m_steamIDUser == localSteamID )
		{
			// Try to go 3 past where the player is if we can, then go back 6
			nStartingIndex = Max( Min( i + 3, scores.Count() ) - 6, 0 );
			break;
		}
	}

	int x=0,y=0;
	FOR_EACH_VEC( m_vecLeaderboardEntries, i )
	{
		Color colorToUse = i % 2 == 1 ? m_OddTextColor : m_EvenTextColor;
		EditablePanel *pContainer = dynamic_cast< EditablePanel* >( m_vecLeaderboardEntries[i] );
		int nScoreIndex = nStartingIndex + i;
		if ( pContainer )
		{
			bool bIsEntryVisible = nScoreIndex < scores.Count();
			pContainer->SetVisible( bIsEntryVisible );
			pContainer->SetPos( x, y );
			y += m_yEntryStep;
			if ( bIsEntryVisible )
			{
				const LeaderboardEntry_t *leaderboardEntry = scores[nScoreIndex];
				const CSteamID &steamID = leaderboardEntry->m_steamIDUser;
				bool bIsLocalPlayer = steamapicontext && steamapicontext->SteamUser() && steamapicontext->SteamUser()->GetSteamID() == steamID;
				pContainer->SetDialogVariable( "username", InventoryManager()->PersonaName_Get( steamID.GetAccountID() ) );
				float flXPToLevel = DUCK_XP_SCALE;
				const float flPreciseLevel = leaderboardEntry->m_nScore / flXPToLevel;
				const int nCurrentLevel = floor( flPreciseLevel );
				const float flPercentToNextLevel = flPreciseLevel - nCurrentLevel;
				pContainer->SetDialogVariable( "score", nCurrentLevel );
				ProgressBar* pProgressBar = pContainer->FindControl<ProgressBar>( "ProgressToNextLevel", true );
				if ( pProgressBar )
				{
					pProgressBar->SetProgress( 1.f - flPercentToNextLevel );
					pProgressBar->SetProgressDirection( ProgressBar::PROGRESS_WEST );
					//const int nNextLevelXP = ( nCurrentLevel + 1 ) * DUCK_XP_SCALE;
					pProgressBar->SetTooltip( m_pToolTip, CFmtStr( "%d / %d", leaderboardEntry->m_nScore % DUCK_XP_SCALE, DUCK_XP_SCALE ) );
				}
				
				CExLabel *pText = pContainer->FindControl< CExLabel >( "UserName" );
				if ( pText )
				{			
					pText->SetColorStr( bIsLocalPlayer ? m_LocalPlayerTextColor : colorToUse );
				}

				pText = pContainer->FindControl< CExLabel >( "Score" );
				if ( pText )
				{
					pText->SetColorStr( bIsLocalPlayer ? m_LocalPlayerTextColor : colorToUse );
				}

				CAvatarImagePanel *pAvatar = dynamic_cast< CAvatarImagePanel* >( pContainer->FindChildByName( "AvatarImage" ) );
				if ( pAvatar )
				{
					pAvatar->SetShouldDrawFriendIcon( false );
					pAvatar->SetPlayer( steamID, k_EAvatarSize32x32 );
				}
			}								
		}			
	}

	return true;
}



//-----------------------------------------------------------------------------
CDucksLeaderboardManager::CDucksLeaderboardManager( Panel *parent, const char *panelName )
	: EditablePanel( parent, panelName )
	, m_nCurrentPage( 0 )
	, m_flFadeStartTime( Plat_FloatTime() )
	, m_pDimmer( NULL )
{
	ListenForGameEvent( "gameui_hidden" );

	m_pToolTip = new CTFTextToolTip( this );
	m_pToolTipEmbeddedPanel = new vgui::EditablePanel( this, "TooltipPanel" );
	m_pToolTipEmbeddedPanel->SetKeyBoardInputEnabled( false );
	m_pToolTipEmbeddedPanel->SetMouseInputEnabled( false );
	m_pToolTip->SetEmbeddedPanel( m_pToolTipEmbeddedPanel );
	m_pToolTip->SetTooltipDelay( 0 );

	static CSchemaItemDefHandle pDuckBadgeDef( "Duck Badge" );
	// Prevent users who don't own the badge from opening the duck leaderboards
	if( CTFPlayerInventory::GetFirstItemOfItemDef( pDuckBadgeDef->GetDefinitionIndex() ) == NULL )
	{
		SetVisible( false );
		MarkForDeletion();
		return;
	}
}

//-----------------------------------------------------------------------------
void CDucksLeaderboardManager::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/econ/DucksLeaderboards.res" );

	m_pDimmer = FindControl<EditablePanel>( "Dimmer" );

	ShowPage( 0 );
}

//-----------------------------------------------------------------------------
void CDucksLeaderboardManager::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_vecLeaderboards.PurgeAndDeleteElements();

	EditablePanel* pBackgroundPanel = FindControl< EditablePanel >( "Background", true );
	if ( pBackgroundPanel )
	{
		EDuckLeaderboardTypes eShowLeaderboard = TF_DUCK_SCORING_OVERALL_RATING;
		CDucksLeaderboard *pLeaderboard = new CDucksLeaderboard( pBackgroundPanel, "DuckLeaderboard", g_szDuckLeaderboardNames[eShowLeaderboard] );
		pLeaderboard->SetDialogVariable( "title", g_pVGuiLocalize->Find( CFmtStr( "#%s", g_szDuckLeaderboardNames[eShowLeaderboard] ) ) );
		pLeaderboard->SetDialogVariable( "description", g_pVGuiLocalize->Find( CFmtStr( "#%s_Desc", g_szDuckLeaderboardNames[eShowLeaderboard] ) ) );
		pLeaderboard->InvalidateLayout( true, true );
		m_vecLeaderboards.AddToTail( pLeaderboard );

		EditablePanel *pStatsPanel = pBackgroundPanel->FindControl<EditablePanel>( "SecondaryStatsContainer", true );
		if ( pStatsPanel )
		{
			m_vecLeaderboards.AddToTail( pStatsPanel );

			Panel* pScoresContainer = pStatsPanel->FindChildByName( "ScoresContainer", true );

			if ( pScoresContainer )
			{
				CSteamID localSteamID;
				if ( steamapicontext && steamapicontext->SteamUser() )
				{
					localSteamID= steamapicontext->SteamUser()->GetSteamID();
				}

				KeyValues* pScoreEntryKVs = inResourceData->FindKey( "ScoreEntryKVs" );
				if ( pScoreEntryKVs )
				{
					for( int i = 1; i < DUCK_NUM_LEADERBOARDS; ++i )
					{
						EditablePanel *pNewEntry = new EditablePanel( pScoresContainer , "Score%d" );

						pNewEntry->ApplySettings( pScoreEntryKVs );
						pNewEntry->SetDialogVariable( "name", g_pVGuiLocalize->Find( CFmtStr( "#%s", g_szDuckLeaderboardNames[i] ) ) );
						pNewEntry->SetTooltip( m_pToolTip, CFmtStr( "#%s_Desc", g_szDuckLeaderboardNames[i] ) );
						pNewEntry->SetVisible( true );
						pNewEntry->SetPos( 0, m_iScoreStep * i );	// This is off by 1, but that's what we want.  It starts at 1, but we want it to start lower
																	// so it matches the leaderboard entries

						CUtlVector< LeaderboardEntry_t* > scores;
						Leaderboards_GetDuckLeaderboard( scores, g_szDuckLeaderboardNames[i] );

						int nScore = 0;
						FOR_EACH_VEC( scores, j )
						{
							if ( scores[j]->m_steamIDUser == localSteamID )
							{
								nScore = scores[j]->m_nScore;
								break;
							}
						}

						pNewEntry->SetDialogVariable( "score", nScore );
					}
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void CDucksLeaderboardManager::OnCommand( const char *command )
{
	if ( FStrEq( command, "close" ) )
	{
		SetVisible( false );
		MarkForDeletion();
		return;
	}
	else if ( FStrEq( command, "nextpage" ) )
	{
		NextPage();
	}
	else if ( FStrEq( command, "prevpage" ) ) 
	{
		PrevPage();
	}
}

//-----------------------------------------------------------------------------
void CDucksLeaderboardManager::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "gameui_hidden" ) )
	{
		SetVisible( false );
		MarkForDeletion();
		return;
	}
}


void CDucksLeaderboardManager::OnThink()
{
	if ( m_pDimmer )
	{
		float flDelta = Plat_FloatTime() - m_flFadeStartTime;
		float flAlpha = RemapValClamped( flDelta, 0.f, 0.2f, 0.f, 253.f );
		m_pDimmer->SetAlpha( flAlpha );
	}
}

//-----------------------------------------------------------------------------
void CDucksLeaderboardManager::NextPage()
{
	++m_nCurrentPage;
	if ( m_nCurrentPage == m_vecLeaderboards.Count() )
	{
		m_nCurrentPage = 0;
	}

	ShowPage( m_nCurrentPage );
}

//-----------------------------------------------------------------------------
void CDucksLeaderboardManager::PrevPage()
{
	--m_nCurrentPage;
	if ( m_nCurrentPage < 0 )
	{
		m_nCurrentPage = m_vecLeaderboards.Count() - 1;
	}

	ShowPage( m_nCurrentPage );
}

//-----------------------------------------------------------------------------
void CDucksLeaderboardManager::ShowPage( int nPage )
{
	for( int i=0; i < m_vecLeaderboards.Count(); ++i )
	{
		m_vecLeaderboards[i]->SetVisible( i == nPage );
	}

	EditablePanel* pBackgroundPanel = dynamic_cast< EditablePanel* >( FindChildByName( "Background", true ) );
	if ( pBackgroundPanel )
	{
		pBackgroundPanel->SetDialogVariable( "pagenumber", CFmtStr( "%d/%d", nPage + 1, m_vecLeaderboards.Count() ) );
	}
}


#endif

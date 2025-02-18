//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_matchmaking_dashboard_popup.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_gamerules.h"
#include "tf_gc_client.h"
#include "clientmode_tf.h"
#include <vgui_controls/AnimationController.h>
#include <vgui_controls/CircularProgressBar.h>

using namespace vgui;
using namespace GCSDK;

extern ConVar tf_mm_next_map_vote_time;



class CNextMapVotingDashboardState : public CTFMatchmakingPopup
{
	DECLARE_CLASS_SIMPLE( CNextMapVotingDashboardState, CTFMatchmakingPopup );
public:
	CNextMapVotingDashboardState( const char* pszName )
		: CTFMatchmakingPopup( pszName )
		, m_pTimerProgressBar( NULL )
	{
		memset( m_arMapPanels, 0, sizeof( m_arMapPanels ) );
		ListenForGameEvent( "player_next_map_vote_change" );
		ListenForGameEvent( "vote_maps_changed" );
	}

	virtual void ApplySchemeSettings( IScheme *pScheme )
	{
		SetHidden( false );
		LoadControlSettings( "resource/UI/MatchMakingDashboardPopup_NextMapVoting.res" );

		CTFMatchmakingPopup::ApplySchemeSettings( pScheme );

		m_pTimerProgressBar = FindControl< CircularProgressBar >( "TimeRemainingProgressBar", true );
		if ( m_pTimerProgressBar )
		{
			m_pTimerProgressBar->SetProgressDirection( CircularProgressBar::PROGRESS_CCW );
			m_pTimerProgressBar->SetFgImage( GetLocalPlayerTeam() == TF_TEAM_RED ? "progress_bar_red" : "progress_bar_blu" );
		}

		for ( int i=0; i < NEXT_MAP_VOTE_OPTIONS; ++i )
		{
			EditablePanel* pMapChoice = FindControl< EditablePanel >( CFmtStr( "MapChoice%d", i ), true );
			if ( pMapChoice )
			{
				pMapChoice->LoadControlSettings( "resource/UI/MatchMakingDashboardPopup_MapVotePanel.res" );
			}
		}

		m_nOriginalExpandedHeight = GetExpandedHeight();
	}

	virtual void PerformLayout() OVERRIDE
	{
		CTFMatchmakingPopup::PerformLayout();

		SetMapChoiceSettings();
		UpdateVoteCounts();
	}

	virtual void OnUpdate() OVERRIDE	
	{
		CTFMatchmakingPopup::OnUpdate();

		// Default to looping 30 sec cycle for debugging
		float flVoteEndTime = ( 30 + ( ( int( Plat_FloatTime() ) / 30 ) * 30 ) - Plat_FloatTime() ) / 30.f;

		if ( TFGameRules() )
		{
			// Get the actual countdown if we have gamerules
			flVoteEndTime = ( tf_mm_next_map_vote_time.GetInt() - ( gpGlobals->curtime - TFGameRules()->GetLastRoundStateChangeTime() ) ) / tf_mm_next_map_vote_time.GetFloat();
		}

		if ( m_pTimerProgressBar )
		{
			m_pTimerProgressBar->SetProgress( flVoteEndTime );
		}
	}

	virtual bool ShouldBeActve() const OVERRIDE
	{

		if ( engine->IsInGame() &&
			 BInEndOfMatch() &&
			 TFGameRules() &&
			 TFGameRules()->GetCurrentNextMapVotingState() == CTFGameRules::NEXT_MAP_VOTE_STATE_WAITING_FOR_USERS_TO_VOTE &&
			 GTFGCClientSystem()->BConnectedToMatchServer( false ) )
		{
			return true;
		}

		return false;
	}

	virtual void OnCommand( const char *pszCommand )
	{
		if ( Q_strnicmp( pszCommand, "choice", 6 ) == 0 &&
			GetPlayerVoteState() == CTFGameRules::USER_NEXT_MAP_VOTE_UNDECIDED )
		{
			int nIndex = atoi( pszCommand + 6 );
			Assert( nIndex >= 0 && nIndex <= 2 );
			if ( nIndex < 0 || nIndex > 2 )
				return;

			engine->ClientCmd( CFmtStr( "next_map_vote %d", nIndex ) );
		}
		else if ( FStrEq( pszCommand, "toggle_hide" ) ) 
		{
			SetHidden( !m_bHidden );
		}
	}

	virtual void FireGameEvent( IGameEvent *pEvent )
	{
		if ( FStrEq( pEvent->GetName(), "player_next_map_vote_change" ) &&
			 TFGameRules()->GetCurrentNextMapVotingState() == CTFGameRules::NEXT_MAP_VOTE_STATE_WAITING_FOR_USERS_TO_VOTE )
		{
			ShowVoteByOtherPlayer( pEvent->GetInt( "map_index" ) );
			InvalidateLayout();
			surface()->PlaySound( UTIL_GetRandomSoundFromEntry( "Vote.Cast.Yes" ) );

			return;
		}
		else if ( FStrEq( pEvent->GetName(), "vote_maps_changed" ) )
		{
			InvalidateLayout( false, true );
		}
	}

	virtual void OnEnter() OVERRIDE
	{
		// To get the voting options setup how they're supposed to be
		InvalidateLayout( true, false);

		SetHidden( false );

		CTFMatchmakingPopup::OnEnter();
	}

private:

	void SetHidden( bool bSetHidden )
	{
		m_bHidden = bSetHidden;
		SetExpandedHeight( m_bHidden ? YRES( 20 ) : m_nOriginalExpandedHeight );
		SetControlVisible( "ShowButton", m_bHidden, true );
		SetControlVisible( "HideButton", !m_bHidden, true );

	}

	void SetMapChoiceSettings()
	{
		for ( int nIndex = 0; nIndex < NEXT_MAP_VOTE_OPTIONS; ++nIndex )
		{
			const MapDef_t* pMapDef = NULL;

			if ( TFGameRules() )
			{
				pMapDef = GetItemSchema()->GetMasterMapDefByIndex( TFGameRules()->GetNextMapVoteOption( nIndex ) );
			}
			else
			{
				pMapDef = GetItemSchema()->GetMasterMapDefByIndex( RandomInt( 1, GetItemSchema()->GetMapCount() - 1 ) );
			}
		
			Assert( pMapDef );
			if ( !pMapDef )
				return;

			EditablePanel* pMapChoice = FindControl< EditablePanel >( CFmtStr( "MapChoice%d", nIndex ), true );
			if ( pMapChoice )
			{
				ScalableImagePanel* pMapImage = pMapChoice->FindControl< ScalableImagePanel >( "MapImage", true );
			
				// The image
				if ( pMapImage )
				{
					m_arMapPanels[ nIndex ].pMapImage = pMapImage;
					char imagename[ 512 ];
					Q_snprintf( imagename, sizeof( imagename ), "..\\vgui\\maps\\menu_thumb_%s", pMapDef->pszMapName );
					pMapImage->SetImage( imagename );
				}

				// Label text
				pMapChoice->SetDialogVariable( "mapname", g_pVGuiLocalize->Find( pMapDef->pszMapNameLocKey ) );
				m_arMapPanels[ nIndex ].pMapNameLabel = pMapChoice->FindControl< Label >( "NameLabel" );

				// Fixup the button
				Button* pButton = pMapChoice->FindControl< Button >( "SelectButton" );
				if ( pButton )
				{
					m_arMapPanels[ nIndex ].pChooseButton = pButton;
					pButton->SetCommand( CFmtStr( "choice%d", nIndex ) );
					// Dont let people click anymore if the've already voted
					pButton->SetEnabled( GetPlayerVoteState() == CTFGameRules::USER_NEXT_MAP_VOTE_UNDECIDED );
					pButton->SetMouseInputEnabled( GetPlayerVoteState() == CTFGameRules::USER_NEXT_MAP_VOTE_UNDECIDED );

					// Give the one the user selected a green border
					if ( GetPlayerVoteState() == nIndex )
					{
						pButton->SetArmed( true );
						pButton->MakeReadyForUse();
						pButton->SetArmedColor( pButton->GetButtonArmedFgColor(), scheme()->GetIScheme( GetScheme() )->GetColor( "CreditsGreen", Color( 94, 150, 49, 255 ) ) );
					}
				}
			}
		}
	}

	void ShowVoteByOtherPlayer( int nIndex )
	{
		EditablePanel* pMapChoice = FindControl< EditablePanel >( CFmtStr( "MapChoice%d", nIndex ), true );
		if ( pMapChoice )
		{
			// Play animation on the map that got voted on
			g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pMapChoice, "MapVoted" );
		}
	}

	void UpdateVoteCounts()
	{
		int nVotes[ CTFGameRules::EUserNextMapVote::NUM_VOTE_STATES ];
		memset( nVotes, 0, sizeof( nVotes ) );
		int nTotalVotes = 0;
		
		CTFGameRules::EUserNextMapVote eWinningVote = CTFGameRules::USER_NEXT_MAP_VOTE_UNDECIDED;
		if ( TFGameRules() )
		{
			TFGameRules()->GetWinningVote( nVotes );
		}
		else
		{
			// For testing on the main menu
			for ( int i=0; i < NEXT_MAP_VOTE_OPTIONS; ++i )
			{
				nVotes[ i ] += RandomInt( 0, 10 );
				eWinningVote = (CTFGameRules::EUserNextMapVote)( nVotes[ i ] >= nVotes[ eWinningVote ] ? i : eWinningVote );
			}
		}

		// Calculate the total so we can do a % breakdown
		for ( int i=0; i < NEXT_MAP_VOTE_OPTIONS; ++i )
		{
			nTotalVotes += nVotes[ i ];
		}

		for ( int i=0; i < NEXT_MAP_VOTE_OPTIONS; ++i )
		{
			float flPercent = nTotalVotes ? (float)nVotes[ i ] / nTotalVotes * 100.f : 0.f;
			EditablePanel* pMapChoicePanel = FindControl< EditablePanel >( CFmtStr( "MapChoice%d", i ), true );
			if ( pMapChoicePanel )
			{
				// Update the label with the % total
				pMapChoicePanel->SetDialogVariable( "votes", CFmtStr( "%3.0f%%", flPercent ) );
				// Do a color change animation
				if ( g_pClientMode && g_pClientMode->GetViewport() )
				{
					g_pClientMode->GetViewportAnimationController()->StopAnimationSequence( pMapChoicePanel, i == eWinningVote ? "LosingNextMapVote" : "WinningNextMapVote" );
					g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pMapChoicePanel, i == eWinningVote ? "WinningNextMapVote" : "LosingNextMapVote" );
				}
			}
		}
	}

	CTFGameRules::EUserNextMapVote GetPlayerVoteState()
	{
		if ( TFGameRules() )
		{
			int nPlayerIndex = GetLocalPlayerIndex();
			return TFGameRules()->PlayerNextMapVoteState( nPlayerIndex );
		}

		return CTFGameRules::USER_NEXT_MAP_VOTE_UNDECIDED;
	}

	CircularProgressBar*	m_pTimerProgressBar;

	struct MapChoice_t
	{
		ScalableImagePanel* pMapImage;
		Label*				pMapNameLabel;
		Button*				pChooseButton;
	};
	MapChoice_t	m_arMapPanels[3];

	int m_nOriginalExpandedHeight;
	bool m_bHidden = false;
};

REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( []() -> Panel* { return new CNextMapVotingDashboardState( "NextMapVoting" ); }, k_eNextMapVotePopup );
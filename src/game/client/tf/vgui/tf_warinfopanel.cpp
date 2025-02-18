#include "cbase.h"
#include "tf_warinfopanel.h"
#include "econ_controls.h"
#include "econ_item_inventory.h"
#include "vgui_avatarimage.h"
#include "vgui_controls/ProgressBar.h"
#include <vgui_controls/HTML.h>
#include "c_tf_player.h"
#include "econ_ui.h"
#include "tf_asyncpanel.h"
#include "item_model_panel.h"
#include "tf_wardata.h"
#include "iclientmode.h"
#include <vgui_controls/AnimationController.h>
#include "vgui/ISystem.h"


bool IsWarActive( war_definition_index_t nDefIndex )
{

	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( nDefIndex );
	Assert( pWarDef );
	if ( !pWarDef )
		return false;

	return pWarDef->IsActive();
}

template < typename T >
void SetNestedDialogVariable( Panel *pRoot, const char *pszPanel, const char *pszVariable, T val )
{
	EditablePanel *pPanel = pRoot->FindControl<EditablePanel>( pszPanel, true );
	if ( pPanel )
	{
		pPanel->SetDialogVariable( pszVariable, val );
	}
}

DECLARE_BUILD_FACTORY( CWarStandingPanel );
CWarStandingPanel::CWarStandingPanel( Panel* pParent, const char* pszPanelName )
	: BaseClass( pParent, pszPanelName )
	, m_flLastUpdateTime( 0.f )
	, m_pTeam0ProgressBar( NULL )
	, m_pTeam1ProgressBar( NULL )
	, m_bNeedsLerp( false )
{
	ListenForGameEvent( "global_war_data_updated" );
}

void CWarStandingPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	LoadControlSettings( "Resource/UI/econ/WarStandingPanel.res"  );

	for( int i=0; i<2; ++i )
	{
		m_Scores[i].m_pTeamProgressBar = FindControl< ContinuousProgressBar >( CFmtStr( "Team%dProgressBar", i ), true );
		m_Scores[i].m_pContainerPanel  = FindControl< EditablePanel >( CFmtStr( "Team%dContainer", i ), true );
		m_Scores[i].m_pScoreLabel	   = FindControl< CExLabel >( CFmtStr( "Team%dScore", i ), true );
		m_Scores[i].m_pTeamLabel	   = FindControl< CExLabel >( CFmtStr( "Team%dName", i ), true );
	}
}

void CWarStandingPanel::ApplySettings( KeyValues *inResourceData ) 
{
	BaseClass::ApplySettings( inResourceData );

	m_strWarName = inResourceData->GetString( "war_name", NULL );
}

void CWarStandingPanel::OnThink()
{
	BaseClass::OnThink();

	if ( Plat_FloatTime() - m_flLastUpdateTime > 30.f )
	{
		InvalidateLayout();
	}

	if ( m_bNeedsLerp )
	{
		int nTotal = 0;
		if ( m_Scores[ 0 ].m_nNewScore != 0 || m_Scores[ 1 ].m_nNewScore != 0 )
		{
			nTotal = m_Scores[ 0 ].m_nNewScore + m_Scores[ 1 ].m_nNewScore;
		}

		float flPercent = GetPercentAnimated();

		for( int i=0; i<2; ++i )
		{
			// Lerp flPercent from [0,1] -> [StartScore,EndScore]
			float flCurrentScore = RemapValClamped( flPercent, 0.f, 1.f, (float)m_Scores[i].m_nLastScore, (float)m_Scores[i].m_nNewScore );
			
			float flProgressPercent = 0.f;
			if ( nTotal != 0 )
			{
				flProgressPercent = flCurrentScore / (float)nTotal;
			}

			m_Scores[i].m_pTeamProgressBar->SetProgress( flProgressPercent );
			m_Scores[i].m_pContainerPanel->SetDialogVariable( CFmtStr( "team%dscore", i ), CFmtStr( "%.0f%%", flProgressPercent * 100.f ) );

			int nScoreXpos = RemapValClamped( flProgressPercent
											, 0.f
											, 1.f
											, m_Scores[i].m_pTeamProgressBar->GetXPos()
											, m_Scores[i].m_pTeamProgressBar->GetXPos() + m_Scores[i].m_pTeamProgressBar->GetWide() );

			m_Scores[i].m_pScoreLabel->SetPos( nScoreXpos, m_Scores[i].m_pScoreLabel->GetYPos() );
		}

		if ( flPercent == 1.f )
		{
			m_bNeedsLerp = false;
		}
	}
}

void CWarStandingPanel::PerformLayout()
{
	BaseClass::PerformLayout();

	if ( GetPercentAnimated() == 1.f )
	{
		m_flLastUpdateTime = Plat_FloatTime();
	}

	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByName( m_strWarName );
	Assert( pWarDef->GetSides().Count() == 2 ); // Needs to be 2 sides to the war for this panel to work!
	CWarData *pWarData = GetLocalPlayerWarData( pWarDef->GetDefIndex() );
	war_side_t nAffiliation = INVALID_WAR_SIDE;
	if ( pWarData )
	{
		nAffiliation = pWarData->Obj().affiliation();
	}

	FOR_EACH_MAP_FAST( pWarDef->GetSides(), i )
	{
		uint64 nScore = GetWarData().GetGlobalSideScore( pWarDef->GetDefIndex(), pWarDef->GetSide( i )->m_nSideIndex );
				
		m_Scores[ i ].m_nLastScore = m_Scores[ i ].m_nNewScore;
		m_Scores[ i ].m_nNewScore = nScore;

		if ( m_Scores[ i ].m_pTeamLabel )
		{
			m_Scores[ i ].m_pTeamLabel->SetText( pWarDef->GetSide( i )->m_pszLocalizedName );
		}

		// Check if there's a change to show
		m_bNeedsLerp |= m_Scores[ i ].m_nLastScore != m_Scores[ i ].m_nNewScore;

		if ( !m_bNeedsLerp )
		{
			m_Scores[i].m_pContainerPanel->SetDialogVariable( CFmtStr( "team%dscore", i ), CFmtStr( "%.0f%%", m_Scores[i].m_pTeamProgressBar->GetProgress() * 100.f ) );
		}

		// Set the "(Your side)" labels
		SetControlVisible( CFmtStr( "Team%dYourSide", i ), i == nAffiliation, true );
	}
}

float CWarStandingPanel::GetPercentAnimated() const
{
	float flTimeSinceLastUpdate = Plat_FloatTime() - m_flLastUpdateTime;
	float flPercent = Bias( RemapValClamped( flTimeSinceLastUpdate, 0.f, 1.2f, 0.f, 1.f ), 0.8f );
	return flPercent;
}

void CWarStandingPanel::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "global_war_data_updated" ) )
	{
		InvalidateLayout();
	}
}


DECLARE_BUILD_FACTORY( CWarLandingPanel );

CWarLandingPanel::CWarLandingPanel( Panel *pParent, const char *pszPanelName )
	: BaseClass( pParent, pszPanelName )
	, m_flChoseTeamTime( 0.f )
	, m_nLastKnownSide( INVALID_WAR_SIDE )
	, m_nPendingSide( INVALID_WAR_SIDE )
	, m_eJoiningState( NO_ACTION )
{
}

void CWarLandingPanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValues* pKVConditions = new KeyValues( "conditions" );

	if ( IsWarActive( PYRO_VS_HEAVY_WAR_DEF_INDEX ) )
	{
		pKVConditions->AddSubKey( new KeyValues( "if_war_active" ) );
	}
	else
	{
		pKVConditions->AddSubKey( new KeyValues( "if_war_over" ) );
	}

	LoadControlSettings( "Resource/UI/econ/WarJoinPanel.res", NULL, NULL, pKVConditions );
}

void CWarLandingPanel::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	m_strSceneAnimName = inResourceData->GetString( "scene_anim_name", NULL );
	Assert( m_strSceneAnimName.Length() );
}

void CWarLandingPanel::OnCommand( const char *pCommand )
{
	if ( FStrEq( pCommand, "close" ) )
	{
		if ( m_eJoiningState == NO_ACTION )
		{
			SetVisible( false );
		}
	}
	else if ( V_strncasecmp( pCommand, "join_war", V_strlen( "join_war" ) ) == 0 )
	{
		int nSide = V_atoi( &pCommand[ V_strlen( "join_war" ) ] );

		const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( PYRO_VS_HEAVY_WAR_DEF_INDEX );

		if ( !IsWarActive( PYRO_VS_HEAVY_WAR_DEF_INDEX ) )
		{
			DevMsg( "War is inactive" );
			return;
		}

		if ( !pWarDef->IsValidSide( nSide ) )
		{
			DevMsg( "%d is not a valid side", nSide );
			return;
		}
	
		m_nPendingSide = nSide;
		Assert( m_eJoiningState == NO_ACTION );
		m_eJoiningState = CONFIRM_SIDE_SELECTION;
		
		UpdateUIState();
		return;
	}
	else if ( FStrEq( pCommand, "confirm_team" ) )
	{
		if ( !IsWarActive( PYRO_VS_HEAVY_WAR_DEF_INDEX ) )
		{
			DevMsg( "War is inactive" );
			return;
		}

		// Join the war!
		GCSDK::CProtoBufMsg< CGCMsgGC_War_JoinWar > msg( k_EMsgGC_War_JoinWar );

		msg.Body().set_affiliation( m_nPendingSide );
		msg.Body().set_war_id( PYRO_VS_HEAVY_WAR_DEF_INDEX );

		GCClientSystem()->BSendMessage( msg );
		m_flChoseTeamTime = Plat_FloatTime();
		m_eJoiningState = ATTEMPTING_TO_JOIN_AND_WAITING_FOR_RESPONSE;

		UpdateUIState();
		return;
	}
	else if ( FStrEq( pCommand, "dismiss_joining_result" ) )
	{
		m_eJoiningState = NO_ACTION;
		m_nPendingSide = INVALID_WAR_SIDE;
		UpdateUIState();

		return;
	}
	else if ( FStrEq( "view_update_comic", pCommand ) )
	{
		const char* pszComicURL = "http://www.teamfortress.com/theshowdown/";

		if ( steamapicontext && steamapicontext->SteamFriends() && steamapicontext->SteamUtils() && steamapicontext->SteamUtils()->IsOverlayEnabled() )
		{
			steamapicontext->SteamFriends()->ActivateGameOverlayToWebPage( pszComicURL );
		}
		else
		{
			vgui::system()->ShellExecute( "open", pszComicURL );
		}

		return;
	}

	BaseClass::OnCommand( pCommand );
}

void CWarLandingPanel::OnThink()
{
	BaseClass::OnThink();
	
	if ( ( Plat_FloatTime() - m_flChoseTeamTime ) > 5.f && m_eJoiningState == ATTEMPTING_TO_JOIN_AND_WAITING_FOR_RESPONSE )
	{
		m_eJoiningState = FAILED_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION;
		UpdateUIState();
	}
}


void CWarLandingPanel::PerformLayout()
{
	BaseClass::PerformLayout();


	UpdateUIState();
}


void CWarLandingPanel::UpdateWarStatus( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent )
{
	if ( pObject->GetTypeID() != CWarData::k_nTypeID )
	{
		return;
	}

	if ( !steamapicontext || !steamapicontext->SteamUser() )
	{
		return;
	}

	// Make sure this is for us
	CSteamID steamID = steamapicontext->SteamUser()->GetSteamID();
	if ( steamIDOwner != steamID )
	{
		return;
	}

	const CWarData* pWarData = assert_cast< const CWarData* >( pObject );
	if ( pWarData->Obj().affiliation() != m_nLastKnownSide )
	{
		if ( m_eJoiningState == ATTEMPTING_TO_JOIN_AND_WAITING_FOR_RESPONSE )
		{
			m_eJoiningState = SUCCESS_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION;
		}

		m_nLastKnownSide = pWarData->Obj().affiliation();
	}

	UpdateUIState();
}

void CWarLandingPanel::SetVisible( bool bVisible )
{
	BaseClass::SetVisible( bVisible );

	EditablePanel* pSceneContainer = FindControl< EditablePanel >( "SceneContainer", true );
	if ( pSceneContainer )
	{
		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pSceneContainer, m_strSceneAnimName, false );
	}
}

void CWarLandingPanel::UpdateUIState()
{
	const CWarDefinition* pWarDef = GetItemSchema()->GetWarDefinitionByIndex( PYRO_VS_HEAVY_WAR_DEF_INDEX ); // SLAM
	if ( !pWarDef )
		return;

	EditablePanel* pMainContainer = FindControl< EditablePanel >( "MainContainer" );
	if ( pMainContainer )
	{
		bool bWarActive = IsWarActive( PYRO_VS_HEAVY_WAR_DEF_INDEX );
		pMainContainer->SetControlVisible( "WarOverContainer", !bWarActive );

		bool bNeedsToJoin = m_nLastKnownSide == INVALID_WAR_SIDE;
		pMainContainer->SetControlVisible( "AffiliatedContainer", bWarActive && !bNeedsToJoin && m_eJoiningState == NO_ACTION );
		pMainContainer->SetControlVisible( "UnaffiliatedContainer", bWarActive && bNeedsToJoin && m_eJoiningState == NO_ACTION );

		bool bHasGCConnection = GCClientSystem()->BConnectedtoGC();
		pMainContainer->SetControlVisible( "JoinPyroButton", bHasGCConnection, true );
		pMainContainer->SetControlVisible( "JoinHeavyButton", bHasGCConnection, true );
		pMainContainer->SetControlVisible( "NoGContainer", !bHasGCConnection, true );
	}

	static wchar_t wszEndDateOutString[ 128 ];
	char time_buf[k_RTimeRenderBufferSize];
	CRTime timeExpire( pWarDef->GetEndDate() );
	timeExpire.SetToGMT( false );
	timeExpire.Render( time_buf );
	wchar_t wszTemp[ 1024 ];
	V_UTF8ToUnicode( time_buf, wszTemp, sizeof(wszTemp) );

	const wchar_t *wpszEndDateFormat = g_pVGuiLocalize->Find( IsWarActive( PYRO_VS_HEAVY_WAR_DEF_INDEX ) ? "#TF_War_EndFutureDate" : "#TF_War_EndPastDate" );

	g_pVGuiLocalize->ConstructString_safe( wszEndDateOutString, wpszEndDateFormat, 1, wszTemp );
	CExLabel* pSceneContainer = FindControl< CExLabel >( "EndDateLabel", true );
	if ( pSceneContainer )
	{
		pSceneContainer->SetText( wszEndDateOutString );
	}


	EditablePanel* pJoiningPopup = FindControl< EditablePanel >( "CommunicatingWithGCPopup", true );
	if ( !pJoiningPopup )
		return;

	switch ( m_eJoiningState )
	{
	case NO_ACTION:
		break;
	case CONFIRM_SIDE_SELECTION:
	{
		Assert( m_nPendingSide != INVALID_WAR_SIDE );
		const CWarDefinition::CWarSideDefinition_t* pChosenSide = pWarDef->GetSide( m_nPendingSide ); // Can be NULL
		Assert( pChosenSide );
		if ( !pChosenSide )
			return;

		static wchar_t wszOutString[ 128 ];
		const wchar_t *wpszFormat = g_pVGuiLocalize->Find( "#TF_War_ConfirmSideSelection" );
		g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 1, g_pVGuiLocalize->Find( pChosenSide->m_pszLocalizedName ) );
		
		EditablePanel* pJoining = FindControl< EditablePanel >( "ConfirmSelectionContainer", true );
		if ( pJoining )
		{
			pJoining->SetDialogVariable( "confirm_selection", wszOutString );
		}
	}
		break;
	case ATTEMPTING_TO_JOIN_AND_WAITING_FOR_RESPONSE:
	{
		Assert( m_nPendingSide != INVALID_WAR_SIDE );
		const CWarDefinition::CWarSideDefinition_t* pChosenSide = pWarDef->GetSide( m_nPendingSide ); // Can be NULL
		Assert( pChosenSide );
		if ( !pChosenSide )
			return;

		static wchar_t wszOutString[ 128 ];
		const wchar_t *wpszFormat = g_pVGuiLocalize->Find( "#TF_War_JoiningTeam" );
		g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 1, g_pVGuiLocalize->Find( pChosenSide->m_pszLocalizedName ) );
		
		EditablePanel* pJoining = FindControl< EditablePanel >( "JoiningContainer", true );
		if ( pJoining )
		{
			pJoining->SetDialogVariable( "joining_team", wszOutString );
		}
	}

		break;

	case SUCCESS_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION:
	{
		const CWarDefinition::CWarSideDefinition_t* pChosenSide = pWarDef->GetSide( m_nLastKnownSide ); // Can be NULL
		Assert( pChosenSide );
		if ( !pChosenSide )
			return;

		static wchar_t wszOutString[ 128 ];
		const wchar_t *wpszFormat = g_pVGuiLocalize->Find( "#TF_War_JoinedTeam" );
		g_pVGuiLocalize->ConstructString_safe( wszOutString, wpszFormat, 1, g_pVGuiLocalize->Find( pChosenSide->m_pszLocalizedName ) );
		EditablePanel* pJoined = FindControl< EditablePanel >( "TeamJoinedContainer", true );
		if ( pJoined )
		{
			pJoined->SetDialogVariable( "team_joined", wszOutString );
		}
	}
		break;

	case FAILED_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION:

		break;

	default:
		Assert( false );
		return;
	}

	pJoiningPopup->SetVisible( m_eJoiningState != NO_ACTION );

	pJoiningPopup->SetControlVisible( "ConfirmSelectionContainer", m_eJoiningState == CONFIRM_SIDE_SELECTION, true );
	pJoiningPopup->SetControlVisible( "JoiningContainer", m_eJoiningState == ATTEMPTING_TO_JOIN_AND_WAITING_FOR_RESPONSE, true );
	pJoiningPopup->SetControlVisible( "TeamJoinedContainer", m_eJoiningState == SUCCESS_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION, true );
	pJoiningPopup->SetControlVisible( "FailedToJoinContainer", m_eJoiningState == FAILED_RESPONSE_RECIEVED_WAITING_FOR_USER_CONFIRMATION, true );
}


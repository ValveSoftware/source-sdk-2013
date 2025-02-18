//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include <vgui_controls/AnimationController.h>

#include "tf_hud_freezepanel.h"
#include "clientmode_shared.h"
#include "tf_hud_robot_destruction_status.h"
#include "tf_logic_player_destruction.h"
#include "c_tf_objective_resource.h"
#include "c_func_capture_zone.h"

#define ATTACK_BLINK_TIME 2.f

using namespace vgui;

extern ConVar tf_rd_min_points_to_steal;
extern ConVar tf_rd_steal_rate;
extern ConVar tf_rd_points_per_steal;
extern ConVar tf_rd_points_approach_interval;
extern ConVar tf_rd_points_per_approach;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_StateImage::CTFHudRobotDestruction_StateImage( Panel *parent, const char *name, const char *pszResFile  )
	: vgui::EditablePanel( parent, name )
	, m_pImage( NULL )
	, m_pRobotImage( NULL )
	, m_pszResFile( pszResFile )
{
	m_pImage = new ImagePanel( this, "Image" );
	m_pRobotImage = new ImagePanel( this, "RobotImage" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_StateImage::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( m_pszResFile );

	ImagePanel* pGlow = dynamic_cast<ImagePanel*>( FindChildByName( "GlowImage", true ) );
	if ( pGlow )
	{
		pGlow->SetAlpha( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_StateImage::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	int nTeam = TF_TEAM_RED;
	const CTFHudRobotDestruction_RobotIndicator *pRobotImageParent = dynamic_cast< const CTFHudRobotDestruction_RobotIndicator* >( GetParent()->GetParent() );
	if ( pRobotImageParent )
	{
		nTeam = pRobotImageParent->GetTeamNumber();
	}

	const char *pszKeyName = nTeam == TF_TEAM_RED ? "redimage" : "blueimage";
	const char *pszImageName = inResourceData->GetString( pszKeyName );
	if ( pszImageName && pszImageName[0] )
	{
		m_pImage->SetImage( pszImageName );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_DeadImage::CTFHudRobotDestruction_DeadImage( Panel *parent, const char *name, const char *pszResFile )
	: CTFHudRobotDestruction_StateImage( parent, name, pszResFile )
	, m_pRespawnProgressBar( NULL )
{
	m_pRespawnProgressBar = new CTFProgressBar( this, "RespawnProgressBar" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_DeadImage::SetProgress( float flProgress )
{
	m_pRespawnProgressBar->SetPercentage( flProgress );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_ActiveImage::CTFHudRobotDestruction_ActiveImage( Panel *parent, const char *name, const char *pszResFile )
	: CTFHudRobotDestruction_StateImage( parent, name, pszResFile )
{}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_ActiveImage::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	const CTFHudRobotDestruction_RobotIndicator *pRobotImageParent = dynamic_cast< const CTFHudRobotDestruction_RobotIndicator* >( GetParent()->GetParent() );
	if ( pRobotImageParent && pRobotImageParent->GetGroup() )
	{
		m_pRobotImage->SetImage( pRobotImageParent->GetGroup()->GetHUDIcon() );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHudRobotDestruction_RobotIndicator::CTFHudRobotDestruction_RobotIndicator( vgui::Panel *pParent, const char *pszName, CTFRobotDestruction_RobotGroup *pGroup )
	: EditablePanel( pParent, pszName )
	, m_hGroup( pGroup )
	, m_pNextRobotIndicator( NULL )
	, m_pPrevRobotIndicator( NULL )
{
	Assert( pGroup );

	m_pSwoop = new CControlPointIconSwoop( this, "Swoop" );
	m_pSwoop->SetVisible( false );
	m_pRobotStateContainer = new EditablePanel( this, "RobotStateContainer" );
	m_pDeadPanel = new CTFHudRobotDestruction_DeadImage( m_pRobotStateContainer, "DeadState", "resource/UI/TFHudRobotDestruction_DeadState.res" );
	m_pActivePanel = new CTFHudRobotDestruction_ActiveImage( m_pRobotStateContainer, "ActiveState", "resource/UI/TFHudRobotDestruction_ActiveState.res" );
	m_pShieldedPanel = new CTFHudRobotDestruction_StateImage( m_pRobotStateContainer, "ShieldedState", "resource/UI/TFHudRobotDestruction_ShieldedState.res" );
	m_flHealthPercentage = 0.f;
	m_eState = ROBOT_STATE_DEAD;

	vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "resource/UI/TFHudRobotDestruction_RobotIndicator.res" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::PerformLayout()
{
	BaseClass::PerformLayout();

	m_pSwoop->SetBounds( 0, 0, GetWide(), GetTall() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::OnTick()
{
	if ( !m_hGroup )
	{
		SetVisible( false );
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}

	UpdateState();
	DoUnderAttackBlink();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::DoUnderAttackBlink()
{
	if ( !m_hGroup.Get() )
		return;

	if ( gpGlobals->curtime < ( m_hGroup->GetLastAttackedTime() + ATTACK_BLINK_TIME ) && ( GetLocalPlayerTeam() == m_hGroup->GetTeamNumber() ) )
	{
		// Pulse red
		ImagePanel* pGlow = dynamic_cast<ImagePanel*>( FindChildByName( "GlowImage", true ) );
		if ( pGlow )
		{
			float flAlpha = fabs(sin( gpGlobals->curtime * 10.f )) * 255;
			pGlow->SetAlpha( flAlpha );
		}
	}
	else
	{
		// Stop pulsing, stop ticking
		ImagePanel* pGlow = dynamic_cast<ImagePanel*>( FindChildByName( "GlowImage", true ) );
		if ( pGlow )
		{
			pGlow->SetAlpha( 0 );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFHudRobotDestruction_RobotIndicator::GetGroupNumber() const 
{ 
	Assert( m_hGroup );
	if ( !m_hGroup )
	{
		return 0;
	}
	return m_hGroup->GetGroupNumber();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CTFHudRobotDestruction_RobotIndicator::GetTeamNumber() const 
{
	Assert( m_hGroup );
	if ( !m_hGroup )
	{
		return 0;
	}

	return m_hGroup->GetTeamNumber();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHudRobotDestruction_RobotIndicator::UpdateState()
{
	// Don't do anything if there's no group
	if ( !m_hGroup.Get() )
		return;

	eRobotUIState eState = (eRobotUIState)m_hGroup->GetState();

	// Get the time
	float flStartTime = m_hGroup->GetRespawnStartTime();
	float flEndTime = m_hGroup->GetRespawnEndTime();

	// Show how much time is remaining
	m_pDeadPanel->SetDialogVariable( "time", CFmtStr( "%0.0f", Max( 0.f, flEndTime - gpGlobals->curtime ) ) );

	// Figure out what percentage we're at
	float flDuration = flEndTime - flStartTime;
	float flProgress = gpGlobals->curtime - flStartTime;
	m_pDeadPanel->SetProgress( flProgress / flDuration );

	bool bStateVisibility[ NUM_ROBOT_STATES ];
	memset( bStateVisibility, false, sizeof( bStateVisibility ) );

	bool bDraw = eState != ROBOT_STATE_INACIVE;
	if ( bDraw )
	{
		m_pRobotStateContainer->SetVisible( true );
		bStateVisibility[ eState ] = true;
	}
	else
	{
		m_pRobotStateContainer->SetVisible( false );
	}

	m_eState = eState;

	bool bShowDead = m_eState == ROBOT_STATE_DEAD;
	m_pActivePanel->SetImageVisible( !bShowDead );
	
	m_pDeadPanel->SetVisible( bStateVisibility[ ROBOT_STATE_DEAD ] || bShowDead );
	m_pActivePanel->SetVisible( bStateVisibility[ ROBOT_STATE_ACTIVE ] );
	m_pShieldedPanel->SetVisible( bStateVisibility[ ROBOT_STATE_SHIELDED ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHUDRobotDestruction::CTFHUDRobotDestruction( Panel *parent, const char *name ) 
	: EditablePanel( parent, name )
	, m_bPlayingRD( false )
{
	m_pRobotIndicatorKVs = NULL;

	ReinitializeEverything();

	vgui::ivgui()->AddTickSignal( GetVPanel(), 50 );

	ListenForGameEvent( "rd_rules_state_changed" );
	ListenForGameEvent( "flagstatus_update" );
	ListenForGameEvent( "rd_team_points_changed" );
	ListenForGameEvent( "teamplay_round_start" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CTFHUDRobotDestruction::~CTFHUDRobotDestruction()
{
	if ( m_pRobotIndicatorKVs )
	{
		m_pRobotIndicatorKVs->deleteThis();
		m_pRobotIndicatorKVs = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTFHUDRobotDestruction::IsVisible( void )
{
	if( IsTakingAFreezecamScreenshot() )
		return false;

	return BaseClass::IsVisible();
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::ApplySettings( KeyValues *inResourceData )
{
	BaseClass::ApplySettings( inResourceData );

	KeyValues *pItemKV = inResourceData->FindKey( "robot_kv" );
	if ( pItemKV )
	{
		if ( m_pRobotIndicatorKVs )
		{
			m_pRobotIndicatorKVs->deleteThis();
		}
		m_pRobotIndicatorKVs = new KeyValues( "robot_kv" );
		pItemKV->CopySubkeys( m_pRobotIndicatorKVs );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int SortRobotVec( CTFHudRobotDestruction_RobotIndicator * const *p1, CTFHudRobotDestruction_RobotIndicator * const *p2 )
{
		return (*p2)->GetGroupNumber() - (*p1)->GetGroupNumber();		
}

void CTFHUDRobotDestruction::ReinitializeEverything()
{
	// misyl: Avoid custom maps' res files leaking into other maps
	// by cleaning up all the children and entirely reinitializing.

	while (vgui::ipanel()->GetChildCount(GetVPanel()))
	{
		VPANEL child = vgui::ipanel()->GetChild(GetVPanel(), 0);
		vgui::ipanel()->DeletePanel(child);
	}

	m_pPlayingTo = NULL;

	m_pCarriedContainer = new EditablePanel( this, "CarriedContainer" );
	m_pCarriedImage = new ImagePanel( m_pCarriedContainer, "CarriedImage" );
	m_pCarriedFlagProgressBar = new CProgressPanel( m_pCarriedContainer, "CarriedProgressBar" );
	m_pScoreContainer = new EditablePanel( this, "ScoreContainer" );
	m_pBlueStolenContainer = new EditablePanel( m_pScoreContainer, "BlueStolenContainer" );
	m_pBlueDroppedPanel = new EditablePanel( m_pBlueStolenContainer, "DroppedIntelContainer" );
	m_pRedStolenContainer = new EditablePanel( m_pScoreContainer, "RedStolenContainer" );
	m_pRedDroppedPanel = new EditablePanel( m_pRedStolenContainer, "DroppedIntelContainer" );

	m_pBlueScoreValueContainer = new EditablePanel( m_pScoreContainer, "BlueScoreValueContainer" );
	m_pRedScoreValueContainer = new EditablePanel( m_pScoreContainer, "RedScoreValueContainer" );

	m_pProgressBarsContainer = new EditablePanel( m_pScoreContainer, "ProgressBarContainer" );
	m_pBlueVictoryPanel = new EditablePanel( m_pProgressBarsContainer, "BlueVictoryContainer" );
	m_pBlueProgressBar = new CProgressPanel( m_pProgressBarsContainer, "BlueProgressBarFill" );
	m_pBlueProgressBarEscrow = new CProgressPanel( m_pProgressBarsContainer, "BlueProgressBarEscrow" );

	m_pRedVictoryPanel = new EditablePanel( m_pProgressBarsContainer, "RedVictoryContainer" );
	m_pRedProgressBar = new CProgressPanel( m_pProgressBarsContainer, "RedProgressBarFill" );
	m_pRedProgressBarEscrow = new CProgressPanel( m_pProgressBarsContainer, "RedProgressBarEscrow" );

	m_pCountdownContainer = NULL;

	m_pTeamLeaderImage = NULL;

	m_vecRedRobots.Purge();
	m_vecBlueRobots.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::ApplySchemeSettings( IScheme *pScheme )
{
	ReinitializeEverything();

	BaseClass::ApplySchemeSettings( pScheme );
	
	CTFRobotDestructionLogic* pRoboLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();

	if ( !pRoboLogic )
		return;

	// load control settings...
	LoadControlSettings( pRoboLogic->GetResFile() );

	CUtlVector< CTFRobotDestruction_RobotGroup * > vecSeenGroups;

	// Go through all the groups in the map, and create a UI element for each one
	for ( int i=0; i<IRobotDestructionGroupAutoList::AutoList().Count(); ++i )
	{
		CTFRobotDestruction_RobotGroup *pGroup = static_cast< CTFRobotDestruction_RobotGroup* >( IRobotDestructionGroupAutoList::AutoList()[i] );

		if ( pGroup->IsDormant() )
			continue;

		if ( vecSeenGroups.Find( pGroup ) != vecSeenGroups.InvalidIndex() )
		{
			Assert( 0 );
			DevMsg( "[RD HUD]: %p seen multiple times!\n", pGroup);
			continue;
		}
		vecSeenGroups.AddToTail( pGroup );
		RobotVector_t &robotVec = pGroup->GetTeamNumber() == TF_TEAM_RED ? m_vecRedRobots : m_vecBlueRobots;
		const char* pszPanelName = pGroup->GetTeamNumber() == TF_TEAM_RED ? "red" : "blue";

		robotVec[ robotVec.AddToTail() ] = new CTFHudRobotDestruction_RobotIndicator( this, CFmtStr( "%s_group_%d", pszPanelName, robotVec.Count() ), pGroup );
	}

	// Sort them from lowest group to highest group
	m_vecRedRobots.Sort( &SortRobotVec );
	m_vecBlueRobots.Sort( &SortRobotVec );

	m_pCarriedContainer->SetVisible( false );

	m_pCountdownContainer = dynamic_cast<EditablePanel*>( FindChildByName( "CountdownContainer" ) );
	m_pTeamLeaderImage = dynamic_cast<CTFImagePanel*>( m_pCarriedContainer->FindChildByName( "TeamLeaderImage" ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::PerformLayout()
{
	BaseClass::PerformLayout();

	// Sort them from lowest group to highest group
	m_vecRedRobots.Sort( &SortRobotVec );
	m_vecBlueRobots.Sort( &SortRobotVec );

	PerformRobotLayout( m_vecRedRobots, TF_TEAM_RED );
	PerformRobotLayout( m_vecBlueRobots, TF_TEAM_BLUE );

	int nXPos, nYPos;
	m_pProgressBarsContainer->GetPos( nXPos, nYPos );

	// Store the edges of the score container
	m_nStealLeftEdge = nXPos + m_nStealLeftEdgeOffset - ( m_pRedStolenContainer->GetWide() / 2.f );
	m_nStealRightEdge = nXPos + m_pProgressBarsContainer->GetWide() - m_nStealRightEdgeOffset + ( m_pRedStolenContainer->GetWide() / 2.f );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::PerformRobotLayout( RobotVector_t& vecRobots, int nTeam )
{
	int nParentX = 0, nParentY = 0, nParentWide = 0, nParentTall = 0;
	GetBounds( nParentX, nParentY, nParentWide, nParentTall );

	bool bIsRed = nTeam == TF_TEAM_RED;
	const int nCenterX = nParentX + ( nParentWide * 0.5f );
	int nActiveIndex = 0;
	const int nXOffset = m_iRobotXOffset;
	const int nYOffest = nParentTall - m_iRobotYOffset;
	const int nXStep = m_iRobotXStep;
	const int nYStep = m_iRobotYStep;
	Panel *pPrevPanel = NULL;

	// Position the robot panels, spanning out from the bottom center
	FOR_EACH_VEC_BACK( vecRobots, i )
	{
		CTFHudRobotDestruction_RobotIndicator* pRobot = vecRobots[i];
		if ( pRobot )
		{
			pRobot->ApplySettings( m_pRobotIndicatorKVs );
			pRobot->UpdateState();
			pRobot->SetZPos( vecRobots.Count() - i );

			CTFHudRobotDestruction_RobotIndicator *pPrevRobot = dynamic_cast< CTFHudRobotDestruction_RobotIndicator * >( pPrevPanel );
			if ( pPrevRobot )
			{
				pRobot->SetPrevRobotIndicator( pPrevRobot );
				pPrevRobot->SetNextRobotIndicator( pRobot );
			}
			
			int nWide = pRobot->GetWide();
			// The starting offset
			int nStartPos = ( ( nXOffset ) + ( nWide * 0.5f ) ) * (bIsRed ?  1 : -1);
			// The offset between each position
			int nOffset = bIsRed ? nXStep : -nXStep;

			int nXPos = nCenterX + nStartPos + ( nOffset * nActiveIndex ) - ( nWide * 0.5 );
			pRobot->SetPos( nXPos, nYOffest - pRobot->GetTall() - ( nYStep * i ) );

			pRobot->InvalidateLayout( true, true );

			// If the state is anything but ROBOT_STATE_INACTIVE, then it's an active panel
			if ( pRobot->GetState() != ROBOT_STATE_INACIVE )
			{
				pPrevPanel = pRobot;
				++nActiveIndex;
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::Reset()
{
	g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( "FlagOutlineHide" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::SetPlayingToLabelVisible( bool bVisible )
{
	if ( m_pPlayingTo && m_pPlayingToBG )
	{
		if ( m_pPlayingTo->IsVisible() != bVisible )
		{
			m_pPlayingTo->SetVisible( bVisible );
		}

		if ( m_pPlayingToBG->IsVisible() != bVisible )
		{
			m_pPlayingToBG->SetVisible( bVisible );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::OnTick()
{
	CTFRobotDestructionLogic* pRoboLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();

	bool bPlayindRD = ( pRoboLogic != NULL );
	if ( m_bPlayingRD != bPlayindRD )
	{
		if ( bPlayindRD )
		{
			InvalidateLayout( true, true );
		}

		m_bPlayingRD = bPlayindRD;
	}

	if ( !pRoboLogic )
		return;

	m_pRedScoreValueContainer->SetDialogVariable( "score", pRoboLogic->GetScore( TF_TEAM_RED ) );
	m_pBlueScoreValueContainer->SetDialogVariable( "score", pRoboLogic->GetScore( TF_TEAM_BLUE ) );

	{
		int nBlueEscrow = 0, nRedEscrow = 0;

		if ( pRoboLogic->GetType() == CTFRobotDestructionLogic::TYPE_PLAYER_DESTRUCTION )
		{
			for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
			{
				CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );

				if ( pFlag->IsStolen() && pFlag->GetPrevOwner() )
				{
					int &nFriendScore = pFlag->GetPrevOwner()->GetTeamNumber() == TF_TEAM_RED ? nRedEscrow : nBlueEscrow;

					nFriendScore += pFlag->GetPointValue();
					m_pRedDroppedPanel->SetAlpha( 255 );
				}
			}

			if ( m_pProgressBarsContainer )
			{
				m_pProgressBarsContainer->SetDialogVariable( "red_escrow", nRedEscrow );
				m_pProgressBarsContainer->SetDialogVariable( "blue_escrow", nBlueEscrow );
			}

			// update the team leader image
			if ( m_pTeamLeaderImage )
			{
				bool bLocalplayerIsLeader = false;
				if ( CTFPlayer::GetLocalTFPlayer() && ( CTFPlayer::GetLocalTFPlayer()->GetTeamNumber() > LAST_SHARED_TEAM ) )
				{
					if ( CTFPlayer::GetLocalTFPlayer() == pRoboLogic->GetTeamLeader( GetLocalPlayerTeam() ) )
					{
						bLocalplayerIsLeader = true;
					}
				}

				if ( m_pTeamLeaderImage->IsVisible() != bLocalplayerIsLeader )
				{
					m_pTeamLeaderImage->SetVisible( bLocalplayerIsLeader );
				}
			}

			// update the countdowns if they exist
			if ( m_pCountdownContainer )
			{
				float flTimeRemaining = pRoboLogic->GetCountdownEndTime() - gpGlobals->curtime;
				if ( flTimeRemaining > -1 )
				{
					if ( !m_pCountdownContainer->IsVisible() )
					{
						m_pCountdownContainer->SetVisible( true );
					}

					ImagePanel* pCountdownImage = dynamic_cast<ImagePanel*>( m_pCountdownContainer->FindChildByName( "CountdownImage", true ) );
					if ( pCountdownImage )
					{
						bool bVisible = true;

						if ( pRoboLogic->IsUsingCustomCountdownImage() )
						{
							const char *pszImage = pRoboLogic->GetCountdownImage();
							if ( pszImage && pszImage[0] )
							{
								pCountdownImage->SetImage( pszImage );
							}
							else
							{
								bVisible = false;							
							}
						}

						if ( pCountdownImage->IsVisible() != bVisible )
						{
							pCountdownImage->SetVisible( bVisible );
						}
					}

					CExLabel* pCountdownTime = dynamic_cast<CExLabel*>( m_pCountdownContainer->FindChildByName( "CountdownTime", true ) );
					CExLabel* pCountdownTimeShadow = dynamic_cast<CExLabel*>( m_pCountdownContainer->FindChildByName( "CountdownTimeShadow", true ) );
					if ( pCountdownTime )
					{
						if ( !pCountdownTime->IsVisible() )
						{
							pCountdownTime->SetVisible( true );
						}
					}
					if ( pCountdownTimeShadow )
					{
						if ( !pCountdownTimeShadow->IsVisible() )
						{
							pCountdownTimeShadow->SetVisible( true );
						}
					}

					int nCountdownTime = (int)flTimeRemaining;
					m_pCountdownContainer->SetDialogVariable( "countdowntime", ( nCountdownTime < 0 ) ? 0 : nCountdownTime );
				}
				else
				{
					if ( m_pCountdownContainer->IsVisible() )
					{
						m_pCountdownContainer->SetVisible( false );
					}
				}
			}
		}
		else
		{
			// Find the flags if we dont have them yet
			if ( !m_hRedFlag || !m_hBlueFlag )
			{
				for ( int i=0; i<ICaptureFlagAutoList::AutoList().Count(); ++i )
				{
					CCaptureFlag *pFlag = static_cast< CCaptureFlag* >( ICaptureFlagAutoList::AutoList()[i] );
					if ( pFlag->GetTeamNumber() == TF_TEAM_BLUE )
					{
						m_hBlueFlag = pFlag;
					}
					else
					{
						m_hRedFlag = pFlag;
					}
				}

				if ( pRoboLogic->GetType() == CTFRobotDestructionLogic::TYPE_ROBOT_DESTRUCTION )
				{
					Assert( m_hBlueFlag );
					Assert( m_hRedFlag );
				}
			}

			// A held flag counts towards the stealing team's escrow.
			// A dropped flag counts towards the original team's escrow.
			if ( m_hRedFlag && m_hBlueFlag )
			{
				if ( m_hRedFlag->IsDropped() )
				{
					nRedEscrow += m_hRedFlag->GetPointValue();
					if ( m_hRedFlag->GetReturnProgress() > 0.8f )
					{
						// Blink when we're close to returning
						int nAlpha = int( gpGlobals->curtime * 10 ) % 10 < 5 ? 255 : 0;
						m_pRedDroppedPanel->SetAlpha( nAlpha );
					}
				}
				else
				{
					nBlueEscrow += m_hRedFlag->GetPointValue();
					m_pRedDroppedPanel->SetAlpha( 255 );
				}

				if ( m_hBlueFlag->IsDropped() )
				{
					nBlueEscrow += m_hBlueFlag->GetPointValue();
					if ( m_hBlueFlag->GetReturnProgress() > 0.8f )
					{
						// Blink when we're close to returning
						int nAlpha = int( gpGlobals->curtime * 10 ) % 10 < 5 ? 255 : 0;
						m_pBlueDroppedPanel->SetAlpha( nAlpha );
					}
				}
				else
				{
					nRedEscrow += m_hBlueFlag->GetPointValue();
					m_pBlueDroppedPanel->SetAlpha( 255 );
				}
			}
		}

		const float flFinaleTime = pRoboLogic->GetFinaleLength();
		// Get red finale progress.  We hide the big scores and show the finale countdown if at max score.
		float flFinaleProgress = clamp( pRoboLogic->GetFinaleWinTime( TF_TEAM_RED ) - gpGlobals->curtime, 0.f, pRoboLogic->GetFinaleLength() );
		m_pRedVictoryPanel->SetVisible( flFinaleProgress < flFinaleTime );
		m_pRedScoreValueContainer->SetVisible( flFinaleProgress >= flFinaleTime );
		if ( flFinaleProgress < flFinaleTime )
		{
			m_pRedVictoryPanel->SetDialogVariable( "victorytime", (int)flFinaleProgress );
		}
		
		// Get blue finale progress.  We hide the big scores and show the finale countdown if at max score.
		flFinaleProgress = clamp( pRoboLogic->GetFinaleWinTime( TF_TEAM_BLUE ) - gpGlobals->curtime, 0.f, pRoboLogic->GetFinaleLength() );
		m_pBlueVictoryPanel->SetVisible( flFinaleProgress < flFinaleTime );
		m_pBlueScoreValueContainer->SetVisible( flFinaleProgress >= flFinaleTime );
		if ( flFinaleProgress < flFinaleTime )
		{
			m_pBlueVictoryPanel->SetDialogVariable( "victorytime", (int)flFinaleProgress );
		}

		const float flMaxPoints = pRoboLogic->GetMaxPoints();
		int nTargetPoints = pRoboLogic->GetTargetScore( TF_TEAM_BLUE );
		m_pBlueProgressBar->SetProgress( nTargetPoints / flMaxPoints );
		m_pBlueProgressBarEscrow->SetProgress( ( nTargetPoints + nBlueEscrow ) / flMaxPoints );

		nTargetPoints = pRoboLogic->GetTargetScore( TF_TEAM_RED );
		m_pRedProgressBar->SetProgress( nTargetPoints / flMaxPoints );
		m_pRedProgressBarEscrow->SetProgress( ( nTargetPoints + nRedEscrow ) / flMaxPoints );
	}

	SetPlayingToLabelVisible( true );
	SetDialogVariable( "rounds", pRoboLogic->GetMaxPoints() );
	// HACK!  Fix the events
	UpdateCarriedFlagStatus( NULL, NULL );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::PaintBackground()
{
	UpdateStolenPoints( TF_TEAM_RED, m_pRedStolenContainer );
	UpdateStolenPoints( TF_TEAM_BLUE, m_pBlueStolenContainer );

	BaseClass::PaintBackground();
}

void CTFHUDRobotDestruction::PaintPDPlayerScore( const CTFPlayer* pPlayer )
{
	if ( !pPlayer )
		return;

	// Don't draw the number for ourselves
	if ( pPlayer == C_BasePlayer::GetLocalPlayer() )
		return;

	Vector vecPos = pPlayer->GetAbsOrigin();
	vecPos.z += VEC_HULL_MAX_SCALED( pPlayer ).z + 20;

	int iX, iY;
	Vector vecWorld( vecPos.x, vecPos.y, vecPos.z );
	if ( GetVectorInHudSpace( vecWorld, iX, iY ) )
	{
		int iCurrentLeadingPoint = 0;
		if ( pPlayer->HasItem() )
		{
			CCaptureFlag *pFlag = dynamic_cast<CCaptureFlag*>( pPlayer->GetItem() );
			if ( pFlag )
			{
				iCurrentLeadingPoint = pFlag->GetPointValue();
			}
		}

		wchar_t wszScore[3];
		V_snwprintf( wszScore, ARRAYSIZE( wszScore ), L"%d", iCurrentLeadingPoint );
		const int nWidth = V_wcslen( wszScore ) * 15;
	
 		// draw the name
 		vgui::surface()->DrawSetTextFont( m_hPDPlayerScoreFont );
		vgui::surface()->DrawSetTextPos( iX - ( nWidth / 2 ), iY );
		vgui::surface()->DrawSetTextColor( m_TextColor );

		
		vgui::surface()->DrawPrintText( wszScore, wcslen( wszScore ), vgui::FONT_DRAW_NONADDITIVE );

	}
}

void CTFHUDRobotDestruction::Paint()
{
	CTFRobotDestructionLogic* pRoboLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();
	if ( pRoboLogic && pRoboLogic->GetType() == CTFRobotDestructionLogic::TYPE_PLAYER_DESTRUCTION )
	{
		CTFPlayerDestructionLogic* pPDLogic = static_cast< CTFPlayerDestructionLogic* >( pRoboLogic );
		PaintPDPlayerScore( pPDLogic->GetRedTeamLeader() );
		PaintPDPlayerScore( pPDLogic->GetBlueTeamLeader() );
	}

	BaseClass::Paint();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::UpdateStolenPoints( int nTeam, EditablePanel* pContainer )
{
	CTFRobotDestructionLogic* pRoboLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();
	if ( pRoboLogic )
	{
		int nStolenPoints = 0;
		// Get the stolen score for this team
		CCaptureFlag* pTheirFlag = nTeam == TF_TEAM_RED ? m_hRedFlag : m_hBlueFlag;
		if ( pTheirFlag )
		{
			nStolenPoints = pTheirFlag->GetPointValue();
		}
		// Show the stolen panels if the stolen score is anything
		pContainer->SetVisible( nStolenPoints > 0 );
		pContainer->SetDialogVariable( "intelvalue", nStolenPoints );
	}

	// Find our stolen flag
	CCaptureFlag *pStolenFlag = nTeam == TF_TEAM_RED ? m_hRedFlag : m_hBlueFlag;
	if ( pStolenFlag && pStolenFlag->IsHome() )
	{
		pStolenFlag = NULL;
	}


	C_CaptureZone *pStartCaptureZone = NULL, *pEndCaptureZone = NULL;
	// Go through all the capture zones and find ours and theirs
	for ( int i = 0; i<ICaptureZoneAutoList::AutoList().Count(); i++ )
	{
		C_CaptureZone *pCaptureZone = static_cast< C_CaptureZone* >( ICaptureZoneAutoList::AutoList()[i] );
		if ( !pCaptureZone->IsDormant() && !pCaptureZone->IsDisabled() )
		{
			if ( pCaptureZone->GetTeamNumber() == nTeam )
			{
				pStartCaptureZone = pCaptureZone;
			}
			else
			{
				pEndCaptureZone = pCaptureZone;
			}
		}
	}

	if ( pStolenFlag && pStartCaptureZone && pEndCaptureZone )
	{
		// Use the player's pos if the flag is being carried
		Vector vecFlagPos = pStolenFlag->GetMoveParent() ? pStolenFlag->GetMoveParent()->GetAbsOrigin() : pStolenFlag->GetAbsOrigin();
		// Get the distance of the flag between the cap points
		const float flTotalDist = ( pEndCaptureZone->WorldSpaceCenter() - pStartCaptureZone->WorldSpaceCenter() ).Length() - pEndCaptureZone->BoundingRadius() - pStartCaptureZone->BoundingRadius();
		const float flFlagDist = ( pEndCaptureZone->WorldSpaceCenter() - vecFlagPos ).Length() - pEndCaptureZone->BoundingRadius();
		const float flLerp = clamp( flFlagDist / flTotalDist, 0.f, 1.f );
		// Flip for blue team
		const float flProgress = nTeam == TF_TEAM_BLUE ? ( 1.f - flLerp ) : flLerp;

		// Calc position
		int nWide = pContainer->GetWide();
		const int nXpos = ( ( m_nStealRightEdge - ( m_nStealLeftEdge + nWide ) ) * flProgress ) + m_nStealLeftEdge;

		// Move the panel!
		int nDummy, nYpos;
		pContainer->GetPos( nDummy, nYpos );
		pContainer->SetPos( nXpos, nYpos );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::UpdateCarriedFlagStatus( C_BasePlayer *pNewOwner /*= NULL*/, C_BaseEntity *pFlagEntity /*= NULL*/ )
{
	C_TFPlayer *pLocalPlayer = ToTFPlayer( C_BasePlayer::GetLocalPlayer() );

	// If this is about the other team, we dont care
	if ( pNewOwner && pNewOwner->GetTeamNumber() != pLocalPlayer->GetTeamNumber() )
	{
		return;
	}

	// are we carrying a flag?
	CCaptureFlag *pPlayerFlag = NULL;
	if ( pLocalPlayer && pLocalPlayer->HasItem() && pLocalPlayer->GetItem()->GetItemID() == TF_ITEM_CAPTURE_FLAG )
	{
		if ( !pNewOwner || pNewOwner == pLocalPlayer )
		{
			pPlayerFlag = dynamic_cast< CCaptureFlag* >( pLocalPlayer->GetItem() );
		}
	}

	if ( !pPlayerFlag && pLocalPlayer && pLocalPlayer == pNewOwner )
	{
		pPlayerFlag = dynamic_cast< CCaptureFlag* >( pFlagEntity );
	}

	if ( pPlayerFlag && !pPlayerFlag->IsMarkedForDeletion() && !pPlayerFlag->IsDormant() )
	{
		m_pCarriedContainer->SetVisible( true );
		m_pCarriedContainer->SetDialogVariable( "flagvalue", pPlayerFlag->GetPointValue() );
		// make sure the panels are on, set the initial alpha values, 
		// set the color of the flag we're carrying, and start the animations
		if ( m_pCarriedImage && !m_pCarriedImage->IsVisible() )
		{
			int nTeam;
			if ( pPlayerFlag->GetType() == TF_FLAGTYPE_ATTACK_DEFEND || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_TERRITORY_CONTROL || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_INVADE || 
				 pPlayerFlag->GetType() == TF_FLAGTYPE_RESOURCE_CONTROL )
			{
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_BLUE ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}
			else
			{
				// normal CTF behavior (carrying the enemy flag)
				nTeam = ( ( GetLocalPlayerTeam() == TF_TEAM_RED ) ? ( TF_TEAM_BLUE ) : ( TF_TEAM_RED ) );
			}

			m_pCarriedImage->SetVisible( true );
			m_pCarriedFlagProgressBar->SetProgress( 0.f, true ); // Slam to 0 instantly
			m_pCarriedFlagProgressBar->SetColor( pLocalPlayer->GetTeamNumber() == TF_TEAM_RED ? m_ColorRed : m_ColorBlue );
		}

		CTFRobotDestructionLogic* pRoboLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();
		if ( pRoboLogic )
		{
			int nMinToSteal = tf_rd_min_points_to_steal.GetInt();
			// Current progress
			float flProgress = float( pPlayerFlag->GetPointValue() ) / float( pRoboLogic->GetMaxPoints() );
			// What percentage needs to map to the dotted line
			const float flProgressAtDottedLine = float( nMinToSteal ) / float( pRoboLogic->GetMaxPoints() );
			// This is where in the texture the dotted line is
			const float flWhereTheDottedLineIs = 0.25f;	

			// We want the progress bar range from [0, The dotted line] map to the progress value [0, Min to steal]
			if ( flProgress <= flProgressAtDottedLine )
			{
				flProgress = RemapValClamped( flProgress, 0.f, flProgressAtDottedLine, 0.f, flWhereTheDottedLineIs );
			}
			else // Make the progress bar range from (The dotted line, 1] map to the progress value(Min to steal, 1]
			{
				flProgress = RemapValClamped( flProgress, flProgressAtDottedLine, 1.f, flWhereTheDottedLineIs, 1.f );
			}
			m_pCarriedFlagProgressBar->SetProgress( flProgress );
		}
	}
	else if ( m_pCarriedImage && m_pCarriedImage->IsVisible() )
	{
		m_pCarriedContainer->SetVisible( false );
		m_pCarriedImage->SetVisible( false );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::UpdateRobotElements()
{
	m_vecRedRobots.PurgeAndDeleteElements();
	m_vecBlueRobots.PurgeAndDeleteElements();

	InvalidateLayout( false, true );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::UpdateStolenFlagStatus( int nTeam, C_BaseEntity *pFlag )
{
	CCaptureFlag *pPlayerFlag = dynamic_cast< CCaptureFlag* >( pFlag );
	if ( pPlayerFlag )
	{
		EditablePanel *pStolenContainer = nTeam == TF_TEAM_RED ? m_pRedStolenContainer : m_pBlueStolenContainer;
		Panel* pCarriedImage = pStolenContainer->FindChildByName( "IntelImage" );
		Panel* pDownImage = pStolenContainer->FindChildByName( "DroppedIntelContainer" );
		Assert( pCarriedImage && pDownImage );

		if ( !pCarriedImage || !pDownImage )
			return;

		// Toggle the carried or dropped images
		bool bIsDropped = pPlayerFlag->IsDropped();
		pCarriedImage->SetVisible( !bIsDropped );
		pDownImage->SetVisible( bIsDropped );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CTFHUDRobotDestruction::FireGameEvent( IGameEvent * pEvent )
{
	CTFRobotDestructionLogic* pRoboLogic = CTFRobotDestructionLogic::GetRobotDestructionLogic();

	if ( !pRoboLogic )
		return;

	const char *pszEventName = pEvent->GetName();

	if ( FStrEq( "rd_rules_state_changed", pszEventName ) )
	{
		UpdateRobotElements();
	}
	else if ( FStrEq( pszEventName, "flagstatus_update" ) )
	{
		int nVictimID = pEvent->GetInt( "userid" );
		C_BasePlayer *pNewOwner = USERID2PLAYER( nVictimID );

		int nFlagEntIndex = pEvent->GetInt( "entindex" );
		C_BaseEntity *pFlagEntity = ClientEntityList().GetEnt( nFlagEntIndex );
		if ( pFlagEntity )
		{
			UpdateCarriedFlagStatus( pNewOwner, pFlagEntity );
			UpdateStolenFlagStatus( pFlagEntity->GetTeamNumber(), pFlagEntity );
		}
	}
	else if ( FStrEq( pszEventName, "rd_team_points_changed" ) )
	{
		// Extract data
		int nTeam = pEvent->GetInt( "team" );
		int nPoints = pEvent->GetInt( "points" );
		RDScoreMethod_t eMethod = RDScoreMethod_t( pEvent->GetInt( "method" ) );

		// Figure out which panel and which anim
		Panel *pPanel = nTeam == TF_TEAM_RED ? m_pRedScoreValueContainer : m_pBlueScoreValueContainer;
		bool bPositive = ( nTeam == GetLocalPlayerTeam() && nPoints > 0 ) || ( nTeam != GetLocalPlayerTeam() && nPoints < 0 );
		const char *pszAnimName = bPositive ? "RDPositiveScorePulse" : "RDNegativeScorePulse";

		g_pClientMode->GetViewportAnimationController()->StartAnimationSequence( pPanel, pszAnimName );

		// Make the progress bar blink
		CProgressPanel *pProgressBar = nTeam == TF_TEAM_RED ? m_pRedProgressBar : m_pBlueProgressBar;
		pProgressBar->Blink();

		if ( eMethod == SCORE_REACTOR_STEAL )
		{
			// Make the OTHER team's escrow progress bar blink
			CProgressPanel *pEscrowBar = nTeam != TF_TEAM_RED ? m_pRedProgressBarEscrow : m_pBlueProgressBarEscrow;
			pEscrowBar->Blink();
		}
	}
	else if ( FStrEq( pszEventName, "teamplay_round_start" ) )
	{
		// Recalculate the progress speed
		float flApproachSpeed = ( tf_rd_points_per_approach.GetInt() / tf_rd_points_approach_interval.GetFloat() ) / pRoboLogic->GetMaxPoints();
		m_pBlueProgressBar->SetApproachSpeed( flApproachSpeed );
		m_pBlueProgressBarEscrow->SetApproachSpeed( flApproachSpeed );
		m_pRedProgressBar->SetApproachSpeed( flApproachSpeed );
		m_pRedProgressBarEscrow->SetApproachSpeed( flApproachSpeed );
	}
}



CTFHUDRobotDestruction::CProgressPanel::CProgressPanel( vgui::Panel *parent, const char *name )
	: BaseClass( parent, name )
	, m_nXOrg( 0 )
	, m_nYOrg( 0 )
	, m_nWideOrg( 0 )
	, m_nTallOrg( 0 )
	, m_flLastScoreTime( 0.f )
	, m_flEndProgress( 0.f )
	, m_flCurrentProgress( 0.f )
	, m_flLastTick( 0.f )
{
	ListenForGameEvent( "teamplay_round_start" );
}

void CTFHUDRobotDestruction::CProgressPanel::CaptureBounds()
{
	GetBounds( m_nXOrg, m_nYOrg, m_nWideOrg, m_nTallOrg );
	if ( GetImage() )
	{
		GetImage()->SetSize( m_nWideOrg, m_nTallOrg );
	}
}

void CTFHUDRobotDestruction::CProgressPanel::SetProgress( float flProgress, bool bInstant /*= false*/ )
{
	if ( bInstant )
	{
		m_flEndProgress = m_flCurrentProgress = flProgress;
		CalculateSize();
	}
	else
	{
		// Start ticking if the progress is different
		if ( m_flEndProgress != flProgress )
		{
			vgui::ivgui()->AddTickSignal( GetVPanel(), 0 );
			m_flLastTick = gpGlobals->curtime;
		}

		// Set end target
		m_flEndProgress = flProgress;
	}
}

void CTFHUDRobotDestruction::CProgressPanel::OnTick()
{
	float flDelta = gpGlobals->curtime - m_flLastTick;
	m_flLastTick = gpGlobals->curtime;

	// Approach the target progress amount
	m_flCurrentProgress = Approach( m_flEndProgress, m_flCurrentProgress, flDelta * m_flApproachSpeed );

	// Stop ticking if we've met our progress
	if ( m_flCurrentProgress == m_flEndProgress )
	{
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	}

	CalculateSize();
}

void CTFHUDRobotDestruction::CProgressPanel::PaintBackground()
{
	// Resize internal image in here.  The other bars use this image too, so we have to move
	// it right before we paint or else it will be out of position.
	IImage *pImage = GetImage();
	if ( pImage )
	{
		pImage->SetPos( m_bLeftToRight ? -m_nLeftOffset : -m_flXpos, m_nYOrg );
		pImage->SetSize( m_nWideOrg, m_nTallOrg );
	}

	// Find out blink lerp time
	const float flBlinkPeriod = 0.25f;
	bool bPastBlink = gpGlobals->curtime > ( m_flLastScoreTime + flBlinkPeriod );
	// Blink if it's blink time, or else pulse if within threshold, else just be the standard color
	float flLerp = bPastBlink ? ( m_flCurrentProgress >= m_flBlinkThreshold ? ( ( sin( gpGlobals->curtime * m_flBlinkRate ) * 0.5f ) + 0.5f ) : 1.f )
									: ( (gpGlobals->curtime - m_flLastScoreTime) / flBlinkPeriod );
	flLerp = clamp( flLerp, 0.f, 1.f );
	const float flInverseLerp = 1.f - flLerp;
	// Get out lerped color
	Color drawColor( flInverseLerp * m_BrightColor.r() + flLerp * m_StandardColor.r(),
						flInverseLerp * m_BrightColor.g() + flLerp * m_StandardColor.g(),
						flInverseLerp * m_BrightColor.b() + flLerp * m_StandardColor.b(),
						255 );
	// Change color in base class (it uses it in PaintBackground)
	SetDrawColor( drawColor );

	BaseClass::PaintBackground();
}

void CTFHUDRobotDestruction::CProgressPanel::ApplySettings( KeyValues *inResourceData ) 
{
	BaseClass::ApplySettings( inResourceData );

	int nXpos, nYpos;
	GetPos( nXpos, nYpos );
	SetPos( nXpos + m_nLeftOffset, nYpos );

	CaptureBounds();
	CalculateSize();
}

void CTFHUDRobotDestruction::CProgressPanel::Blink()
{
	m_flLastScoreTime = gpGlobals->curtime;
}


void CTFHUDRobotDestruction::CProgressPanel::FireGameEvent( IGameEvent * pEvent )
{
	const char *pszEventName = pEvent->GetName();

	if ( FStrEq( pszEventName, "teamplay_round_start" ) )
	{
		// We need to reset the timers here in case we changelevel'd
		m_flCurrentProgress = 0.f;
		m_flEndProgress = 0.f;

		// Resize
		CalculateSize();
	}
}

void CTFHUDRobotDestruction::CProgressPanel::CalculateSize()
{
	// Find xpos
	int nProgressWidth = m_nWideOrg - m_nRightOffset - m_nLeftOffset;
	m_flXpos = m_bLeftToRight ? m_nXOrg
							   : ( 1.f - m_flCurrentProgress) * nProgressWidth + m_nXOrg;
							
	// Find width
	m_flWidth = m_flCurrentProgress * nProgressWidth;

	// Resize
	SetBounds( m_flXpos, m_nYOrg, m_flWidth, m_nTallOrg );
}
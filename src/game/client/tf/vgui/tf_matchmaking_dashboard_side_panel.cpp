//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "tf_shareddefs.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_gamerules.h"
#include "ienginevgui.h"
#include "clientmode_tf.h"
#include <vgui/ISurface.h>
#include "tf_matchmaking_dashboard_side_panel.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "tf_partyclient.h"
#include <vgui_controls/AnimationController.h>
#include "econ_controls.h"
#include "tf_ladder_data.h"
#include "util_misc.h"

using namespace vgui;
using namespace GCSDK;

CMatchMakingDashboardSidePanel::CMatchMakingDashboardSidePanel( Panel *parent, 
																const char *panelName,
																const char* pszResFile,
																EStackSide_t eSide )
	: BaseClass( parent, panelName )
	, m_strResFile( pszResFile )
	, m_eSide( eSide )
{
	m_pReturnButton = new CExButton( this, "returnbutton", (const char*)NULL, this );
	m_pReturnButton->PassMouseTicksTo( this, true );
	m_pShade = new Panel( this, "shade" );
	m_pInnerGradient = new ImagePanel( this, "InnerGradient" );

	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );
}

void CMatchMakingDashboardSidePanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	KeyValuesAD pConditions( "conditions" );
	if ( m_eSide == k_eSideLeft )
	{
		pConditions->AddSubKey( new KeyValues( "if_left" ) ) ;
	}

	LoadControlSettings( m_strResFile, NULL, NULL, pConditions );
}

void CMatchMakingDashboardSidePanel::OnCommand( const char *command )
{
	if ( FStrEq( command, "nav_to" ) ) 
	{
		PostActionSignal( new KeyValues( "NavigateSideStack" ) );
		return;
	}
	else if ( FStrEq( command, "nav_close" ) )
	{
		PostActionSignal( new KeyValues( "CloseSideStack", "side", GetSide() ) );
	}
}

void CMatchMakingDashboardSidePanel::SetAsActive( bool bActive )
{
	int nShadeAlpha = bActive ? 0 : 50;
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pShade, "alpha", nShadeAlpha, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );
	int nGradientAlpha = bActive ? 0 : 230;
	g_pClientMode->GetViewportAnimationController()->RunAnimationCommand( m_pInnerGradient, "alpha", nGradientAlpha, 0.0f, 0.4f, vgui::AnimationController::INTERPOLATOR_GAIN, 0.8f, true, false );

	m_pReturnButton->SetVisible( !bActive );

	PostActionSignal( new KeyValues( "SidePanelActive" ) );
}


void CMatchMakingDashboardSidePanel::SetupButtonAndTooltip( Button* pButton, EditablePanel* pTooltipPanel, wchar_t* pwszTipText )
{
	Assert( pTooltipPanel );
	if ( !pTooltipPanel )
		return;

	Assert( pButton != NULL );
	if ( !pButton )
		return;

	bool bEnabled = !pwszTipText || pwszTipText[0] == 0;
	pButton->SetEnabled( bEnabled );
	pTooltipPanel->SetDialogVariable( "tiptext", pwszTipText ); // This is how we do wchar tooltips :/
	pTooltipPanel->SetTooltip( !bEnabled ? GetDashboardTooltip( k_eMediumFont ) : NULL, NULL );
	pTooltipPanel->InstallMouseHandler( pButton, true, true ); // So the button appears to have focus
	pTooltipPanel->SetVisible( !bEnabled );
}

void CMatchMakingDashboardSidePanel::SetupQueueButton( ETFMatchGroup eGroup )
{
	auto pMatchGroup = GetMatchGroupDescription( eGroup );
	if ( !pMatchGroup )
		return;

	CUtlVector< CTFPartyClient::QueueEligibilityData_t > vecReasons;
	GTFPartyClient()->BCanQueueForMatch( eGroup, vecReasons );
	wchar_t* pwszDisabledReason = vecReasons.IsEmpty() ? nullptr : vecReasons.Head().wszCantReason;

	SetupButtonAndTooltip( FindControl< Button >( "QueueButton" ), 
						   FindControl< EditablePanel >( "ToolTipButtonHack" ),
						   pwszDisabledReason );
}


void CMatchMakingDashboardSidePanel::OnUpdateVisiblity()
{
	bool bShow = GetMMDashboard()->BIsSidePanelShowing( this );
	SetVisible( bShow );
}
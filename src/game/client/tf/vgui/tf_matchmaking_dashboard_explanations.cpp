//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//


#include "cbase.h"
#include "ienginevgui.h"
#include "tf_matchmaking_dashboard.h"
#include "tf_matchmaking_dashboard_explanations.h"
#include "clientmode_tf.h"

using namespace vgui;
using namespace GCSDK;

CExplanationPopup* ShowDashboardExplanation( const char* pszExplanation )
{
	return GetDashboardPanel().GetTypedPanel< CExplanationManager >( k_eExplanations )->ShowExplanation( pszExplanation );
}


Panel* GetExplanationManager()
{
	Panel* pPanel = new CExplanationManager();
	pPanel->MakeReadyForUse();
	return pPanel;
}
REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( GetExplanationManager, k_eExplanations );

CExplanationManager::CExplanationManager()
	: EditablePanel( NULL, "ExplanationManager" )
{
	vgui::HScheme scheme = vgui::scheme()->LoadSchemeFromFileEx( enginevgui->GetPanel( PANEL_CLIENTDLL ), "resource/ClientScheme.res", "ClientScheme");
	SetScheme(scheme);
	SetProportional( true );
}

CExplanationManager::~CExplanationManager()
{}

void CExplanationManager::SetParent( Panel *newParent )
{
	bool bMouseInput = IsMouseInputEnabled();
	// This is going to stomp our mouse input sensitivity
	BaseClass::SetParent( newParent );
	SetMouseInputEnabled( bMouseInput );
}

void CExplanationManager::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	
	m_vecPopups.Purge();
	LoadControlSettings( "resource/UI/GlobalExplanations.res" );
	// We'll toggle mouse input enabled when we have something to actually show
	SetMouseInputEnabled( false );
}

void CExplanationManager::OnChildAdded( VPANEL child )
{
	BaseClass::OnChildAdded( child );

	CExplanationPopup *pPopup = dynamic_cast< CExplanationPopup* >( ipanel()->GetPanel(child, GetControlsModuleName()) );
	if ( pPopup )
	{
		m_vecPopups.AddToTail( pPopup );
	}
}

CExplanationPopup* CExplanationManager::ShowExplanation( const char* pszExplanationName )
{
	CExplanationPopup* pDesiredPopup = NULL;
	FOR_EACH_VEC( m_vecPopups, i )
	{
		auto pPopup = m_vecPopups[ i ];
		if ( FStrEq( pPopup->GetName(), pszExplanationName ) )
		{
			vgui::ivgui()->AddTickSignal( GetVPanel() );
			pDesiredPopup = pPopup;
			m_vecQueuedPopups.AddToTail( pPopup );
			PlaySoundEntry( "Hud.Hint" );
		}
		else if ( pPopup->IsVisible() )
		{
			// Make sure everyone else is hidden
			pPopup->Hide();
		}
	}

	AssertMsg1( pDesiredPopup != NULL, "Popup '%s' not found!", pszExplanationName );
	return pDesiredPopup;
}

void CExplanationManager::OnTick()
{
	FOR_EACH_VEC( m_vecQueuedPopups, i )
	{
		SetMouseInputEnabled( true );
		m_vecQueuedPopups[ i ]->Popup();
		m_vecQueuedPopups[ i ]->SetMouseInputEnabled( true );
	}
	m_vecQueuedPopups.Purge();

	FOR_EACH_VEC( m_vecPopups, i )
	{
		if ( m_vecPopups[ i ]->IsVisible() )
		{
			return;
		}
	}

	vgui::ivgui()->RemoveTickSignal( GetVPanel() );
	SetMouseInputEnabled( false );
}
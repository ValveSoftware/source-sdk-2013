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
#include "tf_hud_disconnect_prompt.h"
#include "tf_gc_client.h"
#include "tf_party.h"
#include <vgui/ISurface.h>
#include "tf_hud_mainmenuoverride.h"
#include "tf_matchmaking_dashboard_parent_manager.h"
#include "vgui_int.h"
#include "../../vgui2/src/VPanel.h"

//-----------------------------------------------------------------------------
// Purpose: Panel that lives on the viewport that is a popup that we parent
//			the MM dashboard panels to
//-----------------------------------------------------------------------------
class CMatchMakingHUDPopupContainer : public Panel 
{
public:
	DECLARE_CLASS_SIMPLE( CMatchMakingHUDPopupContainer, Panel );
	CMatchMakingHUDPopupContainer()
		: Panel( g_pClientMode->GetViewport(), "MMDashboardPopupContainer" )
	{
		SetProportional( true );
		SetBounds( 0, 0, g_pClientMode->GetViewport()->GetWide(), g_pClientMode->GetViewport()->GetTall() );
		MakePopup();
		SetMouseInputEnabled( true );
		SetKeyBoardInputEnabled( false ); // This can never be true
		SetVisible( false );
		ivgui()->AddTickSignal( GetVPanel(), 100 );
	}

	virtual void OnTick()
	{
		BaseClass::OnThink();

		SetVisible( GetMMDashboard()->BIsExpanded() );
	/*	bool bChildrenVisible = false;
		int nCount = GetChildCount();
		for( int i=0; i < nCount && !bChildrenVisible; ++i )
		{
			CExpandablePanel* pChild = assert_cast< CExpandablePanel* >( GetChild( i ) );
			bChildrenVisible = bChildrenVisible || pChild->BIsExpanded() || ( !pChild->BIsExpanded() && pChild->GetPercentAnimated() != 1.f );
		}

		SetVisible( bChildrenVisible );*/
	}
};

CMMDashboardParentManager::CMMDashboardParentManager()
	: m_bAttachedToGameUI( false )
{
	ListenForGameEvent( "gameui_activated" );
	ListenForGameEvent( "gameui_hidden" );

	m_pHUDPopup = new CMatchMakingHUDPopupContainer();
}

void CMMDashboardParentManager::AddPanel( vgui::Panel* pChild )
{
	pChild->SetAutoDelete( false );
	m_vecPanels.Insert( pChild );
	UpdateParenting();
}

void CMMDashboardParentManager::RemovePanel( vgui::Panel* pChild )
{
	m_vecPanels.FindAndRemove( pChild );
	m_vecPanels.RedoSort();
	UpdateParenting();
}

void CMMDashboardParentManager::FireGameEvent( IGameEvent *event )
{
	if ( FStrEq( event->GetName(), "gameui_activated" ) )
	{
		m_bAttachedToGameUI = false;
	}
	else if ( FStrEq( event->GetName(), "gameui_hidden" ) )
	{
		m_bAttachedToGameUI = true;
	}
	UpdateParenting();
}

void CMMDashboardParentManager::UpdateParenting()
{
	m_bAttachedToGameUI ? AttachToGameUI() : AttachToTopMostPopup();
}

//-----------------------------------------------------------------------------
// Purpose: Parent the MM dashboard panels to the right panels
//-----------------------------------------------------------------------------
void CMMDashboardParentManager::AttachToGameUI()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	if ( !m_pHUDPopup )
	{
		return;
	}

	FOR_EACH_VEC( m_vecPanels, i )
	{
		Panel *pPanel = m_vecPanels[ i ];

		if ( pPanel->GetParent() == (Panel*)m_pHUDPopup )
			continue;

		bool bKBInput = pPanel->IsKeyBoardInputEnabled();
		bool bMouseInput = pPanel->IsMouseInputEnabled();

		pPanel->SetParent( (Panel*)m_pHUDPopup );

		// SetParent forces children to inherit the parent's mouse and keyboard settings
		pPanel->SetKeyBoardInputEnabled( bKBInput );
		pPanel->SetMouseInputEnabled( bMouseInput );
		// Don't adopt the parent's proportionalness
		pPanel->SetProportional( true );
	}
}

void CMMDashboardParentManager::AttachToTopMostPopup()
{
	tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s", __FUNCTION__ );

	// Not being used.  Hide it.
	if ( m_pHUDPopup )
	{
		m_pHUDPopup->SetVisible( false );
	}

	VPanel *top = NULL;

	if ( vgui::surface()->GetPopupCount() > 0 )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - Popup traverse", __FUNCTION__ );
		int nSurfaceWide, nSurfaceTall;
		vgui::surface()->GetScreenSize( nSurfaceWide, nSurfaceTall );

		// find the highest-level window that is both visible and a popup
		int nIndex = vgui::surface()->GetPopupCount();

		while ( nIndex )
		{			
			top = (VPanel *)vgui::surface()->GetPopup( --nIndex );

			// traverse the hierarchy and check if the popup really is visible
			if (top &&
				 // top->IsPopup() &&  // These are right out of of the popups list!!!
				 top->IsVisible() && 
				 top->IsKeyBoardInputEnabled() && 
				 !vgui::surface()->IsMinimized((VPANEL)top)  )
			{
				Panel *pPopup = ipanel()->GetPanel( (VPANEL)top, GetControlsModuleName());
				if ( pPopup && pPopup->GetParent() != g_pClientMode->GetViewport() )
				{
					int nPanelWide, nPanelTall;
					pPopup->GetSize( nPanelWide, nPanelTall );

					if ( nPanelWide == nSurfaceWide && nPanelTall == nSurfaceTall )
					{
						break;
					}
				}
			}

			top = NULL;
		} 
	}

	Panel *pPopup = ipanel()->GetPanel( (VPANEL)top, GetControlsModuleName());

	if ( !pPopup && gViewPortInterface )
	{
		pPopup = (CHudMainMenuOverride*)( gViewPortInterface->FindPanelByName( PANEL_MAINMENUOVERRIDE ) );
	}

	if ( pPopup )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s - Popup reparent", __FUNCTION__ );

		FOR_EACH_VEC( m_vecPanels, i )
		{	
			Panel *pPanel = m_vecPanels[ i ];

			if ( pPanel->GetParent() == pPopup )
				continue;

			// No longer a popup
			surface()->ReleasePanel( pPanel->GetVPanel() );
			((VPanel*)pPanel->GetVPanel())->SetPopup( false );

			bool bKeyboard = pPanel->IsKeyBoardInputEnabled();
			bool bMouse = pPanel->IsMouseInputEnabled();
			pPanel->SetParent( pPopup ); 
			// SetParent forces children to inherit the parent's mouse and keyboard settings
			pPanel->SetKeyBoardInputEnabled( bKeyboard );
			pPanel->SetMouseInputEnabled( bMouse );

			pPanel->MoveToFront();
			// Don't adopt the parent's proportionalness
			pPanel->SetProportional( true );
		}
	}

}

//-----------------------------------------------------------------------------
// Purpose: Snag the singleton CMMDashboardParentManager
//-----------------------------------------------------------------------------
CMMDashboardParentManager* GetMMDashboardParentManager()
{
	static CMMDashboardParentManager* s_pParentManager = NULL;
	if ( !s_pParentManager )
	{
		s_pParentManager = new CMMDashboardParentManager();
	}

	return s_pParentManager;
}
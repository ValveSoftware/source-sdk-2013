//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "vgui/IInput.h"
#include <vgui/IVGui.h>
#include <vgui/IScheme.h>
#include "blueprint_panel.h"
#include "vgui_controls/TextImage.h"
#include "vgui_controls/Label.h"
#include "vgui_controls/Button.h"
#include "ienginevgui.h"
#include "VGuiMatSurface/IMatSystemSurface.h"
#include "renderparm.h"

DECLARE_BUILD_FACTORY( CBlueprintPanel );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBlueprintPanel::CBlueprintPanel( vgui::Panel *parent, const char *name ) : vgui::EditablePanel( parent, name )
{
	m_bClickable = false;
	m_bMouseOver = false;

	m_bInStack = false;

	SetActAsButton( false, false );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );

	LoadControlSettings( "Resource/UI/build_menu/base_selectable.res" );

	m_pItemNameLabel = dynamic_cast<vgui::Label*>( FindChildByName("ItemNameLabel") );
	m_pItemCostLabel = dynamic_cast<vgui::Label*>( FindChildByName("CostLabel") );
	m_pIcon = dynamic_cast<CIconPanel*>( FindChildByName("BuildingIcon") );
	m_pMetalIcon = dynamic_cast<CIconPanel*>( FindChildByName("MetalIcon") );
	m_pBackground = dynamic_cast<CIconPanel*>( FindChildByName("ItemBackground") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::SetObjectInfo( const CObjectInfo* pNewInfo )
{
	m_pObjectInfo = pNewInfo;

	bool bVisible = pNewInfo != NULL;

	if ( m_pItemNameLabel )
	{
		if ( m_pObjectInfo )
		{
			m_pItemNameLabel->SetText( m_pObjectInfo->m_pBuilderWeaponName );
		}
		m_pItemNameLabel->SetVisible( bVisible );
	}

	if ( m_pItemCostLabel )
	{
		if ( m_pObjectInfo )
		{
			V_snprintf( m_pszCost, sizeof( m_pszCost ), "%i", m_pObjectInfo->m_Cost );
			m_pItemCostLabel->SetText( m_pszCost );
		}
		m_pItemCostLabel->SetVisible( bVisible );
	}

	if ( m_pIcon )
	{
		if ( m_pObjectInfo )
		{
			m_pIcon->SetIcon( m_pObjectInfo->m_pIconMenu );
		}
		m_pIcon->SetVisible( bVisible );
	}

	if ( m_pMetalIcon )
	{
		m_pMetalIcon->SetVisible( bVisible );
	}

	if ( m_pBackground )
	{
		m_pBackground->SetVisible( bVisible );
	}

	SetVisible( bVisible );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::SetActAsButton( bool bClickable, bool bMouseOver ) 
{ 
	m_bClickable = bClickable; 
	m_bMouseOver = bMouseOver; 

	SetMouseInputEnabled( m_bClickable || m_bMouseOver );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::OnCursorEntered( void )
{
	if ( !m_bMouseOver )
		return;

	PostActionSignal( new KeyValues("BlueprintPanelEntered") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::OnCursorExited( void )
{
	if ( !m_bMouseOver )
		return;

	PostActionSignal( new KeyValues("BlueprintPanelExited") );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::OnMousePressed(vgui::MouseCode code)
{
	if ( !m_bClickable || code != MOUSE_LEFT )
		return;

	PostActionSignal( new KeyValues("BlueprintPanelMousePressed") );

	vgui::surface()->PlaySound( "UI/buttonclick.wav" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::OnMouseReleased(vgui::MouseCode code)
{
	if ( !m_bClickable || code != MOUSE_LEFT )
		return;

	PostActionSignal( new KeyValues("BlueprintPanelMouseReleased") );

	vgui::surface()->PlaySound( "UI/buttonclickrelease.wav" );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBlueprintPanel::OnMouseDoublePressed(vgui::MouseCode code)
{
	if ( !m_bClickable || code != MOUSE_LEFT )
		return;

	PostActionSignal( new KeyValues("BlueprintPanelMouseDoublePressed") );

	vgui::surface()->PlaySound( "UI/buttonclickrelease.wav" );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBlueprintPanel::OnCursorMoved( int x, int y )
{
	if ( !m_bClickable )
		return;

	// Add our own xpos/ypos offset
	int iXPos;
	int iYPos;
	GetPos( iXPos, iYPos );
	PostActionSignal( new KeyValues("BlueprintPanelCursorMoved", "x", x + iXPos, "y", y + iYPos) );
}
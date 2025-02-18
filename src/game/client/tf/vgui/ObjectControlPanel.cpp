//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "ObjectControlPanel.h"
#include <vgui_controls/Controls.h>
#include <vgui_controls/Label.h>
#include "vgui_bitmapbutton.h"
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "c_tf_player.h"
#include "clientmode_tf.h"
#include <vgui/IScheme.h>
#include <vgui_controls/Slider.h>
#include "vgui_rotation_slider.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#define DISMANTLE_WAIT_TIME 5.0


//-----------------------------------------------------------------------------
// Standard VGUI panel for objects 
//-----------------------------------------------------------------------------
DECLARE_VGUI_SCREEN_FACTORY( CObjectControlPanel, "object_control_panel" );


//-----------------------------------------------------------------------------
// Constructor: 
//-----------------------------------------------------------------------------
CObjectControlPanel::CObjectControlPanel( vgui::Panel *parent, const char *panelName )
	: BaseClass( parent, panelName, NULL ) 
{
	// Make some high-level panels to group stuff we want to activate/deactivate
	m_pActivePanel = new CCommandChainingPanel( this, "ActivePanel" );

	SetCursor( vgui::dc_none ); // don't draw a VGUI cursor for this panel, and for its children

	// Make sure these are behind everything
	m_pActivePanel->SetZPos( -1 );
}


//-----------------------------------------------------------------------------
// Initialization 
//-----------------------------------------------------------------------------
bool CObjectControlPanel::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	// Make sure we get ticked...
	vgui::ivgui()->AddTickSignal( GetVPanel() );

	if (!BaseClass::Init(pKeyValues, pInitData))
		return false;

	SetCursor( vgui::dc_none ); // don't draw a VGUI cursor for this panel, and for its children

	// Make the bounds of the sub-panels match
	int x, y, w, h;
	GetBounds( x, y, w, h );
	m_pActivePanel->SetBounds( x, y, w, h );

	// Make em all invisible
	m_pActivePanel->SetVisible( false );
	m_pCurrentPanel = m_pActivePanel;

	return true;
}


//-----------------------------------------------------------------------------
// Returns the object it's attached to 
//-----------------------------------------------------------------------------
C_BaseObject *CObjectControlPanel::GetOwningObject() const
{
	C_BaseEntity *pScreenEnt = GetEntity();
	if (!pScreenEnt)
		return NULL;

	C_BaseEntity *pObj = pScreenEnt->GetOwnerEntity();
	if (!pObj)
		return NULL;

	Assert( dynamic_cast<C_BaseObject*>(pObj) );
	return static_cast<C_BaseObject*>(pObj);
}


//-----------------------------------------------------------------------------
// Ticks the panel when its in its various states
//-----------------------------------------------------------------------------
void CObjectControlPanel::OnTickActive( C_BaseObject *pObj, C_TFPlayer *pLocalPlayer )
{
	//ShowDismantleButton( !(pObj->GetFlags() & OF_CANNOT_BE_DISMANTLED) && pObj->GetOwner() == pLocalPlayer );
}

vgui::Panel* CObjectControlPanel::TickCurrentPanel()
{
	C_TFPlayer *pLocalPlayer = C_TFPlayer::GetLocalTFPlayer();
	C_BaseObject *pObj = GetOwningObject();

	m_pCurrentPanel = GetActivePanel();
	OnTickActive(pObj, pLocalPlayer);
	
	return m_pCurrentPanel;
}

void CObjectControlPanel::SendToServerObject( const char *pMsg )
{
	C_BaseObject *pObj = GetOwningObject();
	if (pObj)
	{
		pObj->SendClientCommand( pMsg );
	}
}

//-----------------------------------------------------------------------------
// Frame-based update
//-----------------------------------------------------------------------------
void CObjectControlPanel::OnTick()
{
	BaseClass::OnTick();

	C_BaseObject *pObj = GetOwningObject();
	if (!pObj)
		return;

	if ( IsVisible() )
	{
		// Update the current subpanel
		m_pCurrentPanel->SetVisible( false );
	
		m_pCurrentPanel = TickCurrentPanel();

		m_pCurrentPanel->SetVisible( true );
	}
}

//-----------------------------------------------------------------------------
// Button click handlers
//-----------------------------------------------------------------------------
void CObjectControlPanel::OnCommand( const char *command )
{
	BaseClass::OnCommand(command);
}

DECLARE_VGUI_SCREEN_FACTORY( CRotatingObjectControlPanel, "rotating_object_control_panel" );


//-----------------------------------------------------------------------------
// This is a panel for an object that has rotational controls 
//-----------------------------------------------------------------------------
CRotatingObjectControlPanel::CRotatingObjectControlPanel( vgui::Panel *parent, const char *panelName )
	: BaseClass( parent, panelName ) 
{
}

bool CRotatingObjectControlPanel::Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData )
{
	// Grab ahold of certain well-known controls
	m_pRotationSlider = new CRotationSlider( GetActivePanel(), "RotationSlider" );
	m_pRotationLabel = new vgui::Label( GetActivePanel(), "RotationLabel", "Rotation Control" );

	if (!BaseClass::Init(pKeyValues, pInitData))
		return false;

	m_pRotationSlider->SetControlledObject( GetOwningObject() );

	return true;
}

void CRotatingObjectControlPanel::OnTickActive( C_BaseObject *pObj, C_TFPlayer *pLocalPlayer )
{
	BaseClass::OnTickActive( pObj, pLocalPlayer );
	bool bEnable = (pObj->GetOwner() == pLocalPlayer);
	m_pRotationSlider->SetVisible( bEnable );
	m_pRotationLabel->SetVisible( bEnable );
}


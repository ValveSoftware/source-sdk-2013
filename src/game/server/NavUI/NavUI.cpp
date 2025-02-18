//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#ifdef SERVER_USES_VGUI

#include "NavUI.h"
#include "filesystem.h"
#include "tier0/icommandline.h"
#include "vgui_gamedll_int.h"
#include "ienginevgui.h"
#include "IGameUIFuncs.h"
#include "fmtstr.h"
#include "NavMenu.h"
#include <vgui_controls/MenuButton.h>

#include "SelectionTool.h"
#include "MeshTool.h"
#include "AttributeTool.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


using namespace vgui;
class CNavUIBasePanel;
class CNavUIToolPanel;

extern IGameUIFuncs *gameuifuncs;


//--------------------------------------------------------------------------------------------------------
static CNavUIBasePanel *s_navUIPanel = NULL;
CNavUIBasePanel *TheNavUI( void )
{
	return s_navUIPanel;
}


//--------------------------------------------------------------------------------------------------------
ConVar NavGUIRebuild( "nav_gui_rebuild", "0", FCVAR_CHEAT, "Rebuilds the nav ui windows from scratch every time they're opened" );


//--------------------------------------------------------------------------------------------------------
void CNavUIButton::LookupKey( void )
{
	if ( m_hideKey == BUTTON_CODE_INVALID )
		m_hideKey = (gameuifuncs) ? gameuifuncs->GetButtonCodeForBind( "nav_gui" ) : BUTTON_CODE_INVALID;
}



//--------------------------------------------------------------------------------------------------------
void CNavUIButton::OnKeyCodePressed( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		m_hidePressedTimer.Start();
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIButton::OnKeyCodeReleased( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		if ( m_hidePressedTimer.HasStarted() && m_hidePressedTimer.GetElapsedTime() < 0.5f )
		{
			s_navUIPanel->ToggleVisibility();
			m_hidePressedTimer.Invalidate();
		}
		return;
	}

	BaseClass::OnKeyCodeReleased( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUITextEntry::LookupKey( void )
{
	if ( m_hideKey == BUTTON_CODE_INVALID )
		m_hideKey = (gameuifuncs) ? gameuifuncs->GetButtonCodeForBind( "nav_gui" ) : BUTTON_CODE_INVALID;
}



//--------------------------------------------------------------------------------------------------------
void CNavUITextEntry::OnKeyCodePressed( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		m_hidePressedTimer.Start();
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUITextEntry::OnKeyCodeReleased( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		if ( m_hidePressedTimer.HasStarted() && m_hidePressedTimer.GetElapsedTime() < 0.5f )
		{
			s_navUIPanel->ToggleVisibility();
			m_hidePressedTimer.Invalidate();
		}
		return;
	}

	BaseClass::OnKeyCodeReleased( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIComboBox::LookupKey( void )
{
	if ( m_hideKey == BUTTON_CODE_INVALID )
		m_hideKey = (gameuifuncs) ? gameuifuncs->GetButtonCodeForBind( "nav_gui" ) : BUTTON_CODE_INVALID;
}



//--------------------------------------------------------------------------------------------------------
void CNavUIComboBox::OnKeyCodePressed( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		m_hidePressedTimer.Start();
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIComboBox::OnKeyCodeReleased( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		if ( m_hidePressedTimer.HasStarted() && m_hidePressedTimer.GetElapsedTime() < 0.5f )
		{
			s_navUIPanel->ToggleVisibility();
			m_hidePressedTimer.Invalidate();
		}
		return;
	}

	BaseClass::OnKeyCodeReleased( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUICheckButton::LookupKey( void )
{
	if ( m_hideKey == BUTTON_CODE_INVALID )
		m_hideKey = (gameuifuncs) ? gameuifuncs->GetButtonCodeForBind( "nav_gui" ) : BUTTON_CODE_INVALID;
}



//--------------------------------------------------------------------------------------------------------
void CNavUICheckButton::OnKeyCodePressed( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		m_hidePressedTimer.Start();
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUICheckButton::OnKeyCodeReleased( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		if ( m_hidePressedTimer.HasStarted() && m_hidePressedTimer.GetElapsedTime() < 0.5f )
		{
			s_navUIPanel->ToggleVisibility();
			m_hidePressedTimer.Invalidate();
		}
		return;
	}

	BaseClass::OnKeyCodeReleased( code );
}


//--------------------------------------------------------------------------------------------------------
CNavUIBasePanel::CNavUIBasePanel() : vgui::Frame( NULL, "NavUI" )
{
	m_hideKey = BUTTON_CODE_INVALID;
	SetScheme( "SourceScheme" );
	LoadControlSettings( "Resource/UI/NavUI.res" );
	SetAlpha( 0 );
	SetMouseInputEnabled( false );
	SetSizeable( false );
	SetMoveable( false );
	SetCloseButtonVisible( false );
	SetTitleBarVisible( false );

	SetLeftClickAction( "", "" );

	m_hidden = false;
	m_toolPanel = NULL;
	m_selectionPanel = NULL;

	SetTitle( "", true);

	m_dragSelecting = m_dragUnselecting = false;

	MenuButton *menuButton = dynamic_cast< MenuButton * >(FindChildByName( "FileMenuButton" ));
	if ( menuButton )
	{
		NavMenu * menu = new NavMenu( menuButton, "NavFileMenu" );
		menu->AddMenuItem( "Quit", "Quit", new KeyValues( "Command", "command", "StopEditing" ), this );
		menuButton->SetMenu( menu );
		menuButton->SetOpenDirection( Menu::DOWN );
	}

	menuButton = dynamic_cast< MenuButton * >(FindChildByName( "SelectionMenuButton" ));
	if ( menuButton )
	{
		NavMenu * menu = new NavMenu( menuButton, "NavSelectionMenu" );
		menu->AddMenuItem( "Flood Select", "Flood Select", new KeyValues( "Command", "command", "FloodSelect" ), this );
		menu->AddMenuItem( "Flood Select (fog)", "Flood Select (Fog)", new KeyValues( "Command", "command", "FloodSelect fog" ), this );
		menuButton->SetMenu( menu );
		menuButton->SetOpenDirection( Menu::DOWN );
	}
}


//--------------------------------------------------------------------------------------------------------
CNavUIBasePanel::~CNavUIBasePanel()
{
	s_navUIPanel = NULL;
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::SetLeftClickAction( const char *action, const char *text )
{
	if ( !action || !*action )
	{
		action = "Selection::Select";
	}

	if ( !text || !*text )
	{
		text = "Select";
	}

	V_strncpy( m_leftClickAction, action, sizeof( m_leftClickAction ) );
	m_performingLeftClickAction = false;

	vgui::Label *label = dynamic_cast< vgui::Label * >(FindChildByName( "LeftClick" ) );
	if ( label )
	{
		label->SetText( UTIL_VarArgs( "Left Click: %s", text ) );
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::ApplySchemeSettings( IScheme *pScheme )
{
	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor( Color( 0, 0, 0, 0 ) );

	Panel *panel = FindChildByName( "SidebarParent" );
	if ( panel )
	{
		panel->SetBgColor( Color( 128, 128, 128, 255 ) );
		panel->SetPaintBackgroundType( 2 );
	}

	panel = FindChildByName( "MenuParent" );
	if ( panel )
	{
		panel->SetBgColor( Color( 128, 128, 128, 255 ) );
		panel->SetPaintBackgroundType( 0 );
	}

	panel = FindChildByName( "ToolParent" );
	if ( panel )
	{
		panel->SetBgColor( Color( 0, 0, 0, 128 ) );
		panel->SetPaintBackgroundType( 0 );
	}

	panel = FindChildByName( "SelectionParent" );
	if ( panel )
	{
		panel->SetBgColor( Color( 0, 0, 0, 128 ) );
		panel->SetPaintBackgroundType( 0 );
	}

	panel = FindChildByName( "MouseFeedbackParent" );
	if ( panel )
	{
		panel->SetBgColor( Color( 0, 0, 0, 128 ) );
		panel->SetPaintBackgroundType( 0 );
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::PerformLayout( void )
{
	int wide, tall;
	vgui::surface()->GetScreenSize( wide, tall );
	SetBounds( 0, 0, wide, tall );

	Panel *panel = FindChildByName( "MenuParent" );
	if ( panel )
	{
		int oldWide, oldTall;
		panel->GetSize( oldWide, oldTall );
		panel->SetSize( wide, oldTall );
	}

	BaseClass::PerformLayout();
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::PaintBackground( void )
{
	BaseClass::PaintBackground();
}


//--------------------------------------------------------------------------------------------------------
const char *CNavUIBasePanel::ActiveToolName( void ) const
{
	if ( m_toolPanel )
		return m_toolPanel->GetName();

	return "";
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::ActivateTool( const char *toolName )
{
	if ( m_toolPanel && FStrEq( m_toolPanel->GetName(), toolName ) )
	{
		m_toolPanel->Shutdown();
		m_toolPanel->MarkForDeletion();
		m_toolPanel = NULL;
	}
	else
	{
		if ( m_toolPanel )
		{
			m_toolPanel->Shutdown();
			m_toolPanel->MarkForDeletion();
			m_toolPanel = NULL;
		}

		Panel *toolParent = FindChildByName( "ToolParent" );
		if ( !toolParent )
			toolParent = this;

		m_toolPanel = CreateTool( toolName, toolParent );
		if ( m_toolPanel )
		{
			m_toolPanel->Init();
			m_toolPanel->SetVisible( true );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
CNavUIToolPanel *CNavUIBasePanel::CreateTool( const char *toolName, vgui::Panel *toolParent )
{
	if ( FStrEq( toolName, "Selection" ) )
	{
		return new SelectionToolPanel( toolParent, toolName );
	}
	if ( FStrEq( toolName, "Mesh" ) )
	{
		return new MeshToolPanel( toolParent, toolName );
	}
	if ( FStrEq( toolName, "Attribute" ) )
	{
		return new AttributeToolPanel( toolParent, toolName );
	}
	return NULL;
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnCommand( const char *command )
{
	CSplitString argv( command, " " );

	if ( FStrEq( "Close", command ) )
	{
		ToggleVisibility();
	}
	else if ( FStrEq( "MeshTool", command ) )
	{
		ActivateTool( "Mesh" );
		return;
	}
	else if ( FStrEq( "AttributesTool", command ) )
	{
		ActivateTool( "Attribute" );
		return;
	}
	else if ( FStrEq( "StopEditing", command ) )
	{
		MarkForDeletion();
		engine->ServerCommand( "nav_edit 0\n" );
	}
	else
	{
		BaseClass::OnCommand( command );
	}

	// argv can't delete individual elements
}


//--------------------------------------------------------------------------------------------------------
// GameUI panels are always visible by default, so here we hide ourselves if the GameUI is up.
void CNavUIBasePanel::OnTick( void )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( !player || !player->IsConnected() )
	{
		m_hidden = true;
		SetVisible( false );
		vgui::ivgui()->RemoveTickSignal( GetVPanel() );
		return;
	}

	if ( enginevgui->IsGameUIVisible() )
	{
		if ( GetAlpha() != 0 )
		{
			SetAlpha( 0 );
			SetMouseInputEnabled( false );
		}
	}
	else
	{
		if ( m_hidden )
		{
			if ( GetAlpha() > 0 )
			{
				SetAlpha( 0 );
				SetMouseInputEnabled( false );
			}

			SetVisible( false );
			vgui::ivgui()->RemoveTickSignal( GetVPanel() );
			return;
		}
		else
		{
			if ( GetAlpha() < 255 )
			{
				SetAlpha( 255 );
				SetMouseInputEnabled( true );

				if ( !m_selectionPanel )
				{
					Panel *selectionParent = FindChildByName( "SelectionParent" );
					if ( !selectionParent )
						selectionParent = this;

					m_selectionPanel = CreateTool( "Selection", selectionParent );
					if ( m_selectionPanel )
					{
						m_selectionPanel->Init();
						m_selectionPanel->SetVisible( true );
					}
				}
			}

			CFmtStr str;
			if ( m_toolPanel )
			{
				str.sprintf( "%s - %s", STRING( gpGlobals->mapname ), m_toolPanel->GetName() );
			}
			else
			{
				str.sprintf( "%s", STRING( gpGlobals->mapname ) );
			}
			SetTitle( str.Access(), true );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::ToggleVisibility( void )
{
	m_hidden = !m_hidden;

	if ( m_hidden && NavGUIRebuild.GetBool() )
	{
		MarkForDeletion();
		s_navUIPanel = NULL;
	}
}


//--------------------------------------------------------------------------------------------------------
Panel *CNavUIBasePanel::CreateControlByName( const char *controlName )
{
	if ( FStrEq( controlName, "Button" ) )
	{
		return new CNavUIButton( this, "CNavUIButton" );
	}

	if ( FStrEq( controlName, "TextEntry" ) )
	{
		return new CNavUITextEntry( this, "CNavUITextEntry" );
	}

	if ( FStrEq( controlName, "ComboBox" ) )
	{
		return new CNavUIComboBox( this, "CNavUIComboBox", 5, false );
	}

	if ( FStrEq( controlName, "CheckButton" ) )
	{
		return new CNavUICheckButton( this, "CNavUICheckButton", "" );
	}

	return BaseClass::CreateControlByName( controlName );
}


//--------------------------------------------------------------------------------------------------------
Panel *CNavUIToolPanel::CreateControlByName( const char *controlName )
{
	if ( s_navUIPanel )
	{
		return s_navUIPanel->CreateControlByName( controlName );
	}

	return BaseClass::CreateControlByName( controlName );
}


//--------------------------------------------------------------------------------------------------------
bool CNavUIToolPanel::IsCheckButtonChecked( const char *name )
{
	vgui::CheckButton *checkButton = dynamic_cast< vgui::CheckButton * >( FindChildByName( name, true ) );
	if ( !checkButton )
		return false;

	return checkButton->IsSelected();
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::LookupKey( void )
{
	if ( m_hideKey == BUTTON_CODE_INVALID )
		m_hideKey = (gameuifuncs) ? gameuifuncs->GetButtonCodeForBind( "nav_gui" ) : BUTTON_CODE_INVALID;
}



//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnKeyCodePressed( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		m_hidePressedTimer.Start();
		return;
	}

	BaseClass::OnKeyCodePressed( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnKeyCodeReleased( KeyCode code )
{
	LookupKey();

	if ( code == m_hideKey )
	{
		if ( m_hidePressedTimer.HasStarted() && m_hidePressedTimer.GetElapsedTime() < 0.5f )
		{
			s_navUIPanel->ToggleVisibility();
			m_hidePressedTimer.Invalidate();
		}
		return;
	}

	BaseClass::OnKeyCodeReleased( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnCursorEntered( void )
{
	BaseClass::OnCursorEntered();

	if ( m_performingLeftClickAction )
	{
		if ( m_toolPanel )
		{
			m_toolPanel->FinishLeftClickAction( m_leftClickAction );
		}
		if ( m_selectionPanel )
		{
			m_selectionPanel->FinishLeftClickAction( m_leftClickAction );
		}
		m_performingLeftClickAction = false;
	}
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnCursorMoved( int x, int y )
{
	if ( m_toolPanel )
	{
		m_toolPanel->OnCursorMoved( x, y );
	}
	if ( m_selectionPanel )
	{
		m_selectionPanel->OnCursorMoved( x, y );
	}
	
	BaseClass::OnCursorMoved( x, y );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnCursorExited( void )
{
	BaseClass::OnCursorExited();

	if ( m_performingLeftClickAction )
	{
		if ( m_toolPanel )
		{
			m_toolPanel->FinishLeftClickAction( m_leftClickAction );
		}
		if ( m_selectionPanel )
		{
			m_selectionPanel->FinishLeftClickAction( m_leftClickAction );
		}
		m_performingLeftClickAction = false;
	}
	/*
	if ( m_dragSelecting || m_dragUnselecting )
	{
		PlaySound( "EDIT_END_AREA.Creating" );
	}
	m_dragSelecting = false;
	m_dragUnselecting = false;
	*/
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnMousePressed( MouseCode code )
{
	/*
	CNavArea *area = TheNavMesh->GetSelectedArea();
	*/

	switch ( code )
	{
	case MOUSE_LEFT:
		m_performingLeftClickAction = true;
		if ( m_toolPanel )
		{
			m_toolPanel->StartLeftClickAction( m_leftClickAction );
		}
		if ( m_selectionPanel )
		{
			m_selectionPanel->StartLeftClickAction( m_leftClickAction );
		}
		break;
	}

	BaseClass::OnMousePressed( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::OnMouseReleased( MouseCode code )
{
	switch ( code )
	{
	case MOUSE_LEFT:
		if ( m_performingLeftClickAction )
		{
			if ( m_toolPanel )
			{
				m_toolPanel->FinishLeftClickAction( m_leftClickAction );
			}
			if ( m_selectionPanel )
			{
				m_selectionPanel->FinishLeftClickAction( m_leftClickAction );
			}
			m_performingLeftClickAction = false;
		}
		break;
	case MOUSE_RIGHT:
		if ( m_toolPanel )
		{
			m_toolPanel->StartRightClickAction( "Selection::ClearSelection" );
		}
		if ( m_selectionPanel )
		{
			m_selectionPanel->StartRightClickAction( "Selection::ClearSelection" );
		}
		break;
	}

	BaseClass::OnMousePressed( code );
}


//--------------------------------------------------------------------------------------------------------
void CNavUIBasePanel::PlaySound( const char *sound )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( player )
	{
		player->EmitSound( sound );
	}
}


//--------------------------------------------------------------------------------------------------------
// Taken from cl_dll/view.cpp
float ScaleFOVByWidthRatio( float fovDegrees, float ratio )
{
	float halfAngleRadians = fovDegrees * ( 0.5f * M_PI / 180.0f );
	float t = tan( halfAngleRadians );
	t *= ratio;
	float retDegrees = ( 180.0f / M_PI ) * atan( t );
	return retDegrees * 2.0f;
}


//--------------------------------------------------------------------------------------------------------
// Purpose:
// Given a field of view and mouse/screen positions as well as the current
// render origin and angles, returns a unit vector through the mouse position
// that can be used to trace into the world under the mouse click pixel.
// Input : 
// mousex -
// mousey -
// fov -
// vecRenderOrigin - 
// vecRenderAngles -
// Output :
// vecPickingRay
// Adapted from cl_dll/c_vguiscreen.cpp
//--------------------------------------------------------------------------------------------------------
void ScreenToWorld( int mousex, int mousey, float fov,
				   const Vector& vecRenderOrigin,
				   const QAngle& vecRenderAngles,
				   Vector& vecPickingRay )
{
	float dx, dy;
	float c_x, c_y;
	float dist;
	Vector vpn, vup, vright;

	int wide, tall;
	vgui::surface()->GetScreenSize( wide, tall );
	c_x = wide / 2;
	c_y = tall / 2;

	float scaled_fov = ScaleFOVByWidthRatio( fov, (float)wide / (float)tall * 0.75f );

	dx = (float)mousex - c_x;
	// Invert Y
	dy = c_y - (float)mousey;

	// Convert view plane distance
	dist = c_x / tan( M_PI * scaled_fov / 360.0 );

	// Decompose view angles
	AngleVectors( vecRenderAngles, &vpn, &vright, &vup );

	// Offset forward by view plane distance, and then by pixel offsets
	vecPickingRay = vpn * dist + vright * ( dx ) + vup * ( dy );

	// Convert to unit vector
	VectorNormalize( vecPickingRay );
} 


//--------------------------------------------------------------------------------------------------------
void GetNavUIEditVectors( Vector *pos, Vector *forward )
{
	CBasePlayer *player = UTIL_GetListenServerHost();
	if ( !player )
	{
		return;
	}

	if ( !s_navUIPanel )
	{
		return;
	}

	if ( s_navUIPanel->GetAlpha() < 255 )
	{
		return;
	}

	int x, y;
	vgui::surface()->SurfaceGetCursorPos( x, y );

	float fov = player->GetFOV();
	QAngle eyeAngles = player->EyeAngles();
	Vector eyePosition = player->EyePosition();

	Vector pick;
	ScreenToWorld( x, y, fov, eyePosition, eyeAngles, pick );

	*forward = pick;
}


//--------------------------------------------------------------------------------------------------------
void NavUICommand( void )
{
	if ( engine->IsDedicatedServer() )
		return;

	if ( UTIL_GetCommandClient() != UTIL_GetListenServerHost() )
		return;

	engine->ServerCommand( "nav_edit 1\n" );
	bool created = false;
	if ( !s_navUIPanel )
	{
		created = true;
		s_navUIPanel = CreateNavUI();
	}

	ShowGameDLLPanel( s_navUIPanel );
	vgui::ivgui()->AddTickSignal( s_navUIPanel->GetVPanel() );
	if ( !created )
	{
		s_navUIPanel->ToggleVisibility();
	}
}
ConCommand nav_gui( "nav_gui", NavUICommand, "Opens the nav editing GUI", FCVAR_CHEAT );

#endif // SERVER_USES_VGUI

//--------------------------------------------------------------------------------------------------------

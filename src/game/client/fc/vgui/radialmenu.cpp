// RadialMenu.cpp
// From alien swarm and modified for TF2/Source SDK 2013 by Vvis 
// (http://steamcommunity.com/profiles/76561199004586557 ) 
// (https://github.com/Lambdagon/source-sdk-2013-radialmenu)

#include "cbase.h"
#include <string.h>
#include <stdio.h>
#include "voice_status.h"
#include "c_playerresource.h"
#include "cliententitylist.h"
#include "c_baseplayer.h"
#include "materialsystem/imesh.h"
#include "view.h"
#include "materialsystem/imaterial.h"
#include "tier0/dbg.h"
#include "cdll_int.h"
#include "menu.h" // for chudmenu defs
#include "KeyValues.h"
#include <filesystem.h>
#include "c_team.h"
#include "vgui/ISurface.h"
#include "iclientmode.h"
#include "tf_gamerules.h"
#include "c_tf_player.h"
#include "radialmenu.h"
#include "vgui/Cursor.h"
#include "fmtstr.h"
#include "vgui_int.h"
#include "in_main.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar cl_fastradial( "cl_fastradial", "1", FCVAR_DEVELOPMENTONLY, "If 1, releasing the button on a radial menu executes the highlighted button" );
ConVar RadialMenuDebug( "cl_rosetta_debug", "0" );
ConVar cl_rosetta_line_inner_radius( "cl_rosetta_line_inner_radius", "25" );
ConVar cl_rosetta_line_outer_radius( "cl_rosetta_line_outer_radius", "45" );


void FlushClientMenus( void );
#define MAX_SPLITSCREEN_PLAYERS 2
//--------------------------------------------------------------------------------------------------------
static char s_radialMenuName[ MAX_SPLITSCREEN_PLAYERS ][ 64 ];
static bool s_mouseMenuKeyHeld[ MAX_SPLITSCREEN_PLAYERS ];


//--------------------------------------------------------------------------------------------------------
/**
 * A radial menu-specific subclass of CPolygonButton
 */
class CRadialButton : public CPolygonButton
{
	DECLARE_CLASS_SIMPLE( CRadialButton, CPolygonButton );

public:

	//----------------------------------------------------------------------------------------------------
	CRadialButton( vgui::Panel *parent, const char *panelName )
		: CPolygonButton( parent, panelName )
	{
		SetCursor( vgui::dc_blank );

		m_nMainMaterial = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nMainMaterial, "vgui/white" , true, false );

		m_chosen = false;
		m_hasBorders = true;
		m_armedFont = NULL;
		m_defaultFont = NULL;

		m_unscaledSubmenuPoints.RemoveAll();
		m_submenuPoints = NULL;
		m_numSubmenuPoints = 0;
		m_hasSubmenu = false;
		m_fakeArmed = false;

		m_passthru = NULL;
		m_parent = dynamic_cast< CRadialMenu * >(parent);
	}

	//----------------------------------------------------------------------------------------------------
	void ShowSubmenuIndicator( bool state )
	{
		m_hasSubmenu = state;
	}

	//----------------------------------------------------------------------------------------------------
	void SetPassthru( CRadialButton *button )
	{
		m_passthru = button;
		if ( button )
		{
			SetZPos( -1 );
		}
		else
		{
			SetZPos( 0 );
		}
	}

	//----------------------------------------------------------------------------------------------------
	CRadialButton *GetPassthru( void )
	{
		return m_passthru;
	}

	//----------------------------------------------------------------------------------------------------
	void UpdateHotspots( KeyValues *data )
	{
		BaseClass::UpdateHotspots( data );

		// clear out our old submenu hotspot
		if ( m_submenuPoints )
		{
			delete[] m_submenuPoints;
			m_submenuPoints = NULL;
			m_numSubmenuPoints = 0;
		}
		m_unscaledSubmenuPoints.RemoveAll();

		// read in a new one
		KeyValues *points = data->FindKey( "SubmenuHotspot", false );
		if ( points )
		{
			for ( KeyValues *value = points->GetFirstValue(); value; value = value->GetNextValue() )
			{
				const char *str = value->GetString();

				float x, y;
				if ( 2 == sscanf( str, "%f %f", &x, &y ) )
				{
					m_unscaledSubmenuPoints.AddToTail( Vector2D( x, y ) );
				}
			}
		}

		if ( RadialMenuDebug.GetBool() )
		{
			InvalidateLayout( false, true );
		}
	}

	//----------------------------------------------------------------------------------------------------
	void PerformLayout( void )
	{
		int wide, tall;
		GetSize( wide, tall );

		if ( m_submenuPoints )
		{
			delete[] m_submenuPoints;
			m_submenuPoints = NULL;
			m_numSubmenuPoints = 0;
		}

		// generate scaled points
		m_numSubmenuPoints = m_unscaledSubmenuPoints.Count();
		if ( m_numSubmenuPoints )
		{
			m_submenuPoints = new vgui::Vertex_t[ m_numSubmenuPoints ];
			for ( int i=0; i<m_numSubmenuPoints; ++i )
			{
				float x = m_unscaledSubmenuPoints[i].x * wide;
				float y = m_unscaledSubmenuPoints[i].y * tall;
				m_submenuPoints[i].Init( Vector2D( x, y ), m_unscaledSubmenuPoints[i] );
			}
		}

		bool isArmed = ( IsArmed() || m_fakeArmed );
		vgui::HFont currentFont = (isArmed) ? m_armedFont : m_defaultFont;
		if ( currentFont )
		{
			SetFont( currentFont );
		}

		BaseClass::PerformLayout();
	}

	//----------------------------------------------------------------------------------------------------
	Color GetRadialFgColor( void )
	{
		Color c = BaseClass::GetButtonFgColor();
		if ( !IsEnabled() || GetPassthru() )
		{
			c = m_disabledFgColor;
		}
		else if ( m_chosen )
		{
			c = m_chosenFgColor;
		}
		else if ( m_fakeArmed )
		{
			c = m_armedFgColor;
		}
		c[3] = 64;
		return c;
	}

	//----------------------------------------------------------------------------------------------------
	Color GetRadialBgColor( void )
	{
		Color c = BaseClass::GetButtonBgColor();
		if ( GetPassthru() )
		{
			if ( IsArmed() || m_fakeArmed )
			{
				for ( int i=0; i<4; ++i )
				{
					c[i] = m_disabledBgColor[i] * 0.8f + m_armedBgColor[i] * 0.2f;
				}
			}
			else
			{
				c = m_disabledBgColor;
			}
		}
		else if ( !IsEnabled() )
		{
			c = m_disabledBgColor;
		}
		else if ( m_chosen )
		{
			c = m_chosenBgColor;
		}
		else if ( m_fakeArmed )
		{
			c = m_armedBgColor;
		}
		return c;
	}

	//----------------------------------------------------------------------------------------------------
	void GetHotspotBounds( int *minX, int *minY, int *maxX, int *maxY )
	{
		if ( minX )
		{
			*minX = (int) m_hotspotMins.x;
		}

		if ( minY )
		{
			*minY = (int) m_hotspotMins.y;
		}

		if ( maxX )
		{
			*maxX = (int) m_hotspotMaxs.x;
		}

		if ( maxY )
		{
			*maxY = (int) m_hotspotMaxs.y;
		}
	}
	
	//----------------------------------------------------------------------------------------------------
	/**
	 * Paints the polygonal background
	 */
	virtual void PaintBackground( void )
	{
		if ( RadialMenuDebug.GetBool() && (IsArmed() || m_fakeArmed) )
		{
			Color c = GetRadialFgColor();
			vgui::surface()->DrawSetColor( c );
			vgui::surface()->DrawFilledRect( m_hotspotMins.x, m_hotspotMins.y, m_hotspotMaxs.x, m_hotspotMaxs.y );
		}

		Color c = GetRadialBgColor();
		vgui::surface()->DrawSetColor( c );
		vgui::surface()->DrawSetTexture( m_nMainMaterial );

		if ( RadialMenuDebug.GetInt() == 2 )
		{
			vgui::surface()->DrawTexturedPolygon( m_numHotspotPoints, m_hotspotPoints );
		}
		else
		{
			vgui::surface()->DrawTexturedPolygon( m_numVisibleHotspotPoints, m_visibleHotspotPoints );
		}

		if ( m_numSubmenuPoints && m_hasSubmenu )
		{
			vgui::surface()->DrawTexturedPolygon( m_numSubmenuPoints, m_submenuPoints );
		}
	}

	//----------------------------------------------------------------------------------------------------
	/**
	 * Paints the polygonal border
	 */
	virtual void PaintBorder( void )
	{
		if ( !m_hasBorders )
			return;

		Color c = GetRadialFgColor();
		vgui::surface()->DrawSetColor( c );
		vgui::surface()->DrawSetTexture( m_nWhiteMaterial );

		if ( RadialMenuDebug.GetInt() == 2 )
		{
			vgui::surface()->DrawTexturedPolyLine( m_hotspotPoints, m_numHotspotPoints );
		}
		else
		{
			vgui::surface()->DrawTexturedPolyLine( m_visibleHotspotPoints, m_numVisibleHotspotPoints );
		}

		if ( m_numSubmenuPoints && m_hasSubmenu )
		{
			vgui::surface()->DrawTexturedPolyLine( m_submenuPoints, m_numSubmenuPoints );
		}
	}


	//----------------------------------------------------------------------------------------------------
	/**
	 * Hard-coding colors for now
	 */
	virtual void ApplySchemeSettings( vgui::IScheme *scheme )
	{
		BaseClass::ApplySchemeSettings( scheme );
		MEM_ALLOC_CREDIT();
		SetDefaultColor( scheme->GetColor( "Rosetta.DefaultFgColor", Color( 255, 176, 0, 255 ) ), scheme->GetColor( "Rosetta.DefaultBgColor", Color( 0, 0, 0, 128 ) ) );
		m_armedFgColor = scheme->GetColor( "Rosetta.DefaultFgColor", Color( 255, 176, 0, 255 ) );
		m_armedBgColor = scheme->GetColor( "Rosetta.ArmedBgColor", Color( 0, 28, 192, 128 ) );
		SetArmedColor( m_armedFgColor, m_armedBgColor );
		m_disabledBgColor = scheme->GetColor( "Rosetta.DisabledBgColor", Color( 0, 0, 0, 128 ) );
		m_disabledFgColor = scheme->GetColor( "Rosetta.DisabledFgColor", Color( 0, 0, 0, 128 ) );
		m_chosenFgColor = scheme->GetColor( "Rosetta.DefaultFgColor", Color( 255, 176, 0, 255 ) );
		m_chosenBgColor = scheme->GetColor( "Rosetta.ArmedBgColor", Color( 0, 28, 192, 128 ) );
		SetMouseClickEnabled( MOUSE_RIGHT, true );
		SetButtonActivationType( ACTIVATE_ONPRESSED );

		m_hasBorders = false;
		const char *borderStr = scheme->GetResourceString( "Rosetta.DrawBorder" );
		if ( borderStr && atoi( borderStr ) )
		{
			m_hasBorders = true;
		}

		const char *fontStr = scheme->GetResourceString( "Rosetta.DefaultFont" );
		if ( fontStr )
		{
			m_defaultFont = scheme->GetFont( fontStr, true );
		}
		else
		{
			m_defaultFont = scheme->GetFont( "Default", true );
		}

		fontStr = scheme->GetResourceString( "Rosetta.ArmedFont" );
		if ( fontStr )
		{
			m_armedFont = scheme->GetFont( fontStr, true );
		}
		else
		{
			m_armedFont = scheme->GetFont( "Default", true );
		}
		SetFont( m_defaultFont );

		if ( RadialMenuDebug.GetBool() )
		{
			SetCursor( vgui::dc_crosshair );
		}
		else
		{
			SetCursor( vgui::dc_blank );
		}
	}

	//----------------------------------------------------------------------------------------------------
	/**
	 * Mark this button as "chosen".  Basically, it means we draw a different color as the radial
	 * menu fades out.
	 */
	void SetChosen( bool chosen )
	{
		m_chosen = chosen;
	}

	//----------------------------------------------------------------------------------------------------
	/**
	 * Mark this button as "armed" because of a cursor pos off the screen (low FPS).
	 */
	void SetFakeArmed( bool armed )
	{
		m_fakeArmed = armed;

		bool isArmed = ( IsArmed() || m_fakeArmed );
		vgui::HFont currentFont = (isArmed) ? m_armedFont : m_defaultFont;
		if ( currentFont )
		{
			SetFont( currentFont );
			InvalidateLayout( true );
		}
	}

	//----------------------------------------------------------------------------------------------------
	/**
	 * Plays a rollover sound if the cursor has left the center
	 */
	virtual void OnCursorEntered( void )
	{
		int nSlot = vgui::ipanel()->GetMessageContextId( GetVPanel() );
		//ACTIVE_SPLITSCREEN_PLAYER_GUARD( nSlot );

		int wide, tall;
		GetSize( wide, tall );

		int centerx = wide / 2;
		int centery = tall / 2;

		int x, y;
		vgui::surface()->SurfaceGetCursorPos( x, y );
		ScreenToLocal( x, y );

		if ( x != centerx || y != centery )
		{
			C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
			if ( localPlayer )
			{
				if ( !m_fakeArmed )
				{
					localPlayer->EmitSound("MouseMenu.rollover");
				}
			}

			if ( m_parent )
			{
				m_parent->OnCursorEnteredButton( x, y, this );
			}
		}

		m_fakeArmed = false;

		BaseClass::OnCursorEntered();
	}

	//----------------------------------------------------------------------------------------------------
	/**
	 * Right-click cancels, otherwise, the normal button logic applies
	 */
	virtual void OnMousePressed( vgui::MouseCode code )
	{
		if ( code == MOUSE_RIGHT )
		{
			SetCommand( "done" );
		}
		else
		{
			SetChosen( true );
		}
		BaseClass::OnMousePressed( code );
	}

	virtual bool MouseClick(int x, int y, bool bRightClick, bool bDown)
	{
		if ( !m_fakeArmed )
			return false;

		vgui::Panel *pParent = GetParent();
		if ( !pParent )
			return false;

		if ( pParent->GetAlpha() <= 0 )
			return false;

		if ( bRightClick )
		{
			SetCommand( "done" );
		}

		BaseClass::OnMousePressed( bRightClick ? MOUSE_RIGHT : MOUSE_LEFT );

		return true;
	}

private:
	CUtlVector< Vector2D > m_unscaledSubmenuPoints;
	vgui::Vertex_t *m_submenuPoints;
	int m_numSubmenuPoints;

	int m_nMainMaterial;
	bool m_hasBorders;

	Color m_disabledBgColor;
	Color m_disabledFgColor;

	Color m_chosenBgColor;
	Color m_chosenFgColor;
	bool m_chosen;

	Color m_armedBgColor;
	Color m_armedFgColor;
	bool m_fakeArmed;

	bool m_hasSubmenu;

	vgui::HFont m_armedFont;
	vgui::HFont m_defaultFont;

	CRadialButton *m_passthru;
	CRadialMenu *m_parent;
};


DECLARE_HUDELEMENT( CRadialMenu );


//--------------------------------------------------------------------------------------------------------------
CRadialMenu::CRadialMenu( const char *pElementName ) :
CHudElement( pElementName ), BaseClass( NULL, PANEL_RADIAL_MENU )
{
	MEM_ALLOC_CREDIT();

	vgui::Panel *pParent = g_pClientMode->GetViewport();
	SetParent( pParent );

	// initialize dialog
	m_resource = new KeyValues( "RadialMenu" );
	m_resource->LoadFromFile( filesystem, "resource/UI/RadialMenu.res" );
	m_menuData = NULL;
	FlushClientMenus();

	// load the new scheme early!!
	SetScheme("ClientScheme");
	SetProportional(true);

	HandleControlSettings();

	SetCursor( vgui::dc_blank );

	m_minButtonX = 0;
	m_minButtonY = 0;
	m_maxButtonX = 0;
	m_maxButtonY = 0;

	m_bMouseActivated = true;
}


//--------------------------------------------------------------------------------------------------------
const char *CRadialMenu::ButtonNameFromDir( ButtonDir dir )
{
	switch( dir )
	{
	case CENTER:
		return "Center";
	case NORTH:
		return "North";
	case NORTH_EAST:
		return "NorthEast";
	case EAST:
		return "East";
	case SOUTH_EAST:
		return "SouthEast";
	case SOUTH:
		return "South";
	case SOUTH_WEST:
		return "SouthWest";
	case WEST:
		return "West";
	case NORTH_WEST:
		return "NorthWest";
	}

	return "None";
}


//--------------------------------------------------------------------------------------------------------
CRadialMenu::ButtonDir CRadialMenu::DirFromButtonName( const char * name )
{
	if ( FStrEq( name, "Center" ) )
		return CENTER;
	if ( FStrEq( name, "North" ) )
		return NORTH;
	if ( FStrEq( name, "NorthEast" ) )
		return NORTH_EAST;
	if ( FStrEq( name, "East" ) )
		return EAST;
	if ( FStrEq( name, "SouthEast" ) )
		return SOUTH_EAST;
	if ( FStrEq( name, "South" ) )
		return SOUTH;
	if ( FStrEq( name, "SouthWest" ) )
		return SOUTH_WEST;
	if ( FStrEq( name, "West" ) )
		return WEST;
	if ( FStrEq( name, "NorthWest" ) )
		return NORTH_WEST;

	return CENTER;
}


//--------------------------------------------------------------------------------------------------------
/**
 * Created controls from the resource file.  We know how to make a special PolygonButton :)
 */
vgui::Panel *CRadialMenu::CreateControlByName(const char *controlName)
{
    if (!Q_stricmp("PolygonButton", controlName))
        return new CRadialButton(this, NULL);
    return BaseClass::CreateControlByName(controlName);
}



//--------------------------------------------------------------------------------------------------------
CRadialMenu::~CRadialMenu()
{
	m_resource->deleteThis();
	if ( m_menuData )
	{
		m_menuData->deleteThis();
	}
}

void CRadialMenu::HandleControlSettings()
{
	LoadControlSettings("Resource/UI/RadialMenu.res");

	for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
	{
		SetMouseInputEnabled(true);
		ButtonDir dir = (ButtonDir)i;
		const char *buttonName = ButtonNameFromDir( dir );
		m_buttons[i] = dynamic_cast<CRadialButton *>(FindChildByName( buttonName ));
		if ( m_buttons[i] )
		{
			m_buttons[i]->SetMouseInputEnabled( true );
		}
	}

	m_armedButtonDir = CENTER;
}


//--------------------------------------------------------------------------------------------------------
/**
 * The radial menu should cover the entire screen to capture mouse input, so we should have a blank
 * background.
 */
void CRadialMenu::ApplySchemeSettings( vgui::IScheme *pScheme )
{
	HandleControlSettings();

	BaseClass::ApplySchemeSettings( pScheme );
	SetBgColor( Color( 0, 0, 0, 0 ) );
	m_lineColor = pScheme->GetColor( "Rosetta.LineColor", Color( 192, 192, 192, 128 ) );

	if ( RadialMenuDebug.GetBool() )
	{
		SetCursor( vgui::dc_crosshair );
	}
	else
	{
		SetCursor( vgui::dc_blank );
	}

	// Restore button names/commands
	if ( m_menuData )
	{
		SetData( m_menuData );
	}
}

void CRadialMenu::PerformLayout( void )
{
	BaseClass::PerformLayout();

	SetAlpha( 0 );
}

void CRadialMenu::InitializeInputScheme()
{
	if(!IsVisible())
		return;

	MakePopup(false,UseMouseMode());
	SetKeyBoardInputEnabled(false);
	SetMouseInputEnabled(UseMouseMode());

	for(int i=0; i<NUM_BUTTON_DIRS; ++i)
	{
		if(m_buttons[i])
			m_buttons[i]->SetMouseInputEnabled(UseMouseMode());
	}
}
//--------------------------------------------------------------------------------------------------------
void CRadialMenu::ShowPanel(bool show)
{
		if(show)
		{
			for(int i=0; i<NUM_BUTTON_DIRS; ++i)
			{
				if(!m_buttons[i])
					continue;

				m_buttons[i]->SetArmed(false);
				m_buttons[i]->SetFakeArmed(false);
				m_buttons[i]->SetChosen(false);
			}

			m_bMouseActivated = true; // unlock mouse immediately
			InitializeInputScheme();
			m_cursorX = -1;
			m_cursorY = -1;
			SetVisible(true);
			SetKeyBoardInputEnabled(false);
		} 
		else
		{
			SetVisible(false);
			SetMouseInputEnabled(false);
			m_bMouseActivated = false;
			SetKeyBoardInputEnabled(true);
		}
}



//--------------------------------------------------------------------------------------------------------
void CRadialMenu::Paint( void )
{
	const float fadeDuration = 0.5f;
	if ( m_fading )
	{
		float fadeTimeRemaining = m_fadeStart + fadeDuration - gpGlobals->curtime;
		int alpha = 255 * fadeTimeRemaining / fadeDuration;
		SetAlpha( alpha );
		if ( alpha <= 0 )
		{
			m_fadeStart = 0.0f;
			m_fading = false;
			SetVisible( false );
			//SetCameraFixed( false );
			return;
		}
	}

	int x, y, wide, tall;
	GetBounds( x, y, wide, tall );
	vgui::surface()->DrawSetColor( m_lineColor );

	int centerX = x + wide/2;
	int centerY = y + tall/2;
	float innerRadius = cl_rosetta_line_inner_radius.GetFloat();
	float outerRadius = cl_rosetta_line_outer_radius.GetFloat();
	innerRadius = YRES( innerRadius );
	outerRadius = YRES( outerRadius );

	// Draw horizontal and vertical lines
	vgui::surface()->DrawLine( centerX + innerRadius, centerY, centerX + outerRadius, centerY );
	vgui::surface()->DrawLine( centerX - innerRadius, centerY, centerX - outerRadius, centerY );
	vgui::surface()->DrawLine( centerX, centerY + innerRadius, centerX, centerY + outerRadius );
	vgui::surface()->DrawLine( centerX, centerY - innerRadius, centerX, centerY - outerRadius );

	// Draw diagonal lines
	const float scale = 0.707f; // sqrt(2) / 2
	vgui::surface()->DrawLine( centerX + innerRadius * scale, centerY + innerRadius * scale, centerX + outerRadius * scale, centerY + outerRadius * scale );
	vgui::surface()->DrawLine( centerX - innerRadius * scale, centerY - innerRadius * scale, centerX - outerRadius * scale, centerY - outerRadius * scale );
	vgui::surface()->DrawLine( centerX + innerRadius * scale, centerY - innerRadius * scale, centerX + outerRadius * scale, centerY - outerRadius * scale );
	vgui::surface()->DrawLine( centerX - innerRadius * scale, centerY + innerRadius * scale, centerX - outerRadius * scale, centerY + outerRadius * scale );

	if ( RadialMenuDebug.GetBool() )
	{
		vgui::surface()->DrawSetColor( m_lineColor );
		vgui::surface()->DrawOutlinedRect( x + m_minButtonX, y + m_minButtonY, x + m_maxButtonX, y + m_maxButtonY );
	}

	BaseClass::Paint();
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::OnCommand( const char *command )
{
	if ( RadialMenuDebug.GetBool() )
	{
		Msg( "%f: Clicked on button with command '%s'\n", gpGlobals->curtime, command );
	}

	if ( !Q_strcmp(command, "done") )
	{
		C_BasePlayer *localPlayer = C_BasePlayer::GetLocalPlayer();
		if ( localPlayer )
		{
			localPlayer->EmitSound("MouseMenu.abort");
		}
		StartFade();
	}
	else
	{
		if ( !m_fading )
		{
			StartFade();
			SendCommand( command );
		}
	}

	BaseClass::OnCommand( command );
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::SetArmedButtonDir( ButtonDir dir )
{
	if ( dir != NUM_BUTTON_DIRS )
	{
		CRadialButton *armedButton = m_buttons[dir];
		if ( m_buttons[dir]->GetPassthru() )
		{
			armedButton = m_buttons[dir]->GetPassthru();
			for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
			{
				if ( m_buttons[i] == armedButton )
				{
					dir = (ButtonDir)i;
					break;
				}
			}
		}
	}

	m_armedButtonDir = dir;

	for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
	{
		if ( !m_buttons[i] )
			continue;

		m_buttons[i]->SetFakeArmed( false );
		m_buttons[i]->SetArmed( false );
		if ( i != m_armedButtonDir )
		{
			m_buttons[i]->SetChosen( false );
		}
	}

	if ( m_armedButtonDir != NUM_BUTTON_DIRS )
	{
		if ( m_buttons[m_armedButtonDir] )
		{
			m_buttons[m_armedButtonDir]->SetFakeArmed( true );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
static const char *ButtonDirString( CRadialMenu::ButtonDir dir )
{
	switch ( dir )
	{
	case CRadialMenu::CENTER:
		return "CENTER";
	case CRadialMenu::NORTH:
		return "NORTH";
	case CRadialMenu::NORTH_EAST:
		return "NORTH_EAST";
	case CRadialMenu::EAST:
		return "EAST";
	case CRadialMenu::SOUTH_EAST:
		return "SOUTH_EAST";
	case CRadialMenu::SOUTH:
		return "SOUTH";
	case CRadialMenu::SOUTH_WEST:
		return "SOUTH_WEST";
	case CRadialMenu::WEST:
		return "WEST";
	case CRadialMenu::NORTH_WEST:
		return "NORTH_WEST";
	default:
		return "UNKNOWN";
	}
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::OnCursorEnteredButton( int x, int y, CRadialButton *button )
{
	for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
	{
		m_buttons[i]->InvalidateLayout();
		if ( m_buttons[i] == button )
		{
			m_cursorX = x;
			m_cursorY = y;
			m_armedButtonDir = (ButtonDir)i;
			if ( RadialMenuDebug.GetBool() )
			{
				Msg( "%f: Rosetta cursor entered %s at %d,%d\n", gpGlobals->curtime, ButtonDirString( m_armedButtonDir ), m_cursorX, m_cursorY );
				engine->Con_NPrintf( 20, "%d %d %s", m_cursorX, m_cursorY, ButtonDirString( m_armedButtonDir ) );
			}
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::UpdateButtonBounds( void )
{
	// Save off the extents of our child buttons so we can clip the cursor to that later

	int wide, tall;
	GetSize( wide, tall );
	m_minButtonX = wide;
	m_minButtonY = tall;
	m_maxButtonX = 0;
	m_maxButtonY = 0;

	for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
	{
		if ( !m_buttons[i] )
			continue;

		int hotspotMinX = 0;
		int hotspotMinY = 0;
		int hotspotMaxX = 0;
		int hotspotMaxY = 0;
		m_buttons[i]->GetHotspotBounds( &hotspotMinX, &hotspotMinY, &hotspotMaxX, &hotspotMaxY );

		int buttonX, buttonY;
		m_buttons[i]->GetPos( buttonX, buttonY );

		m_minButtonX = MIN( m_minButtonX, hotspotMinX + buttonX );
		m_minButtonY = MIN( m_minButtonY, hotspotMinY + buttonY );
		m_maxButtonX = MAX( m_maxButtonX, hotspotMaxX + buttonX );
		m_maxButtonY = MAX( m_maxButtonY, hotspotMaxY + buttonY );
	}

	// First frame, we won't have any hotspots set up, so we get inverted bounds from our initial setup.
	// Reverse these, so our button bounds covers our bounds.
	if ( m_minButtonX > m_maxButtonX )
	{
		V_swap( m_minButtonX, m_maxButtonX );
	}

	if ( m_minButtonY > m_maxButtonY )
	{
		V_swap( m_minButtonY, m_maxButtonY );
	}
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::OnThink( void )
{
	if ( !IsMouseInputEnabled() || GetAlpha() <= 0 || m_fading )
		return;

	bool armed = false;
	for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
	{
		if ( !m_buttons[i] )
			continue;

		if ( m_buttons[i]->IsVisible() && m_buttons[i]->IsEnabled() && m_buttons[i]->IsArmed() )
		{
			SetArmedButtonDir( (ButtonDir)i );
			armed = true;
		}
	}

	int oldX = m_cursorX;
	int oldY = m_cursorY;

	input->GetFullscreenMousePos( &m_cursorX, &m_cursorY );
	ScreenToLocal( m_cursorX, m_cursorY );

	int nSlot = vgui::ipanel()->GetMessageContextId( GetVPanel() );
	//ACTIVE_SPLITSCREEN_PLAYER_GUARD( nSlot );

	int wide, tall;
	GetSize( wide, tall );

	int centerx = wide / 2;
	int centery = tall / 2;

	UpdateButtonBounds();

	float buttonRadius = MAX( m_maxButtonX - centerx, m_maxButtonY - centery );
	if ( m_cursorX != centerx && m_cursorY != centery )
	{
		float cursorDistX = (m_cursorX - centerx);
		float cursorDistY = (m_cursorY - centery);
		float cursorDist = sqrt( Sqr(cursorDistX) + Sqr(cursorDistY) );

		if ( cursorDist > buttonRadius )
		{
			cursorDistX *= (buttonRadius/cursorDist);
			cursorDistY *= (buttonRadius/cursorDist);

			m_cursorX = centerx + cursorDistX;
			m_cursorY = centery + cursorDistY;
		}

		int screenX = m_cursorX;
		int screenY = m_cursorY;
		LocalToScreen( screenX, screenY );
		if ( RadialMenuDebug.GetBool() )
		{
			Msg( "%f: Rosetta warping cursor to %d %d\n", gpGlobals->curtime, screenX, screenY );
		}

		input->SetFullscreenMousePos( screenX, screenY );
	}

	if ( m_cursorX == centerx && m_cursorY == centery && oldX >= 0 && oldY >= 0 )
	{
		if ( m_cursorX != oldX || m_cursorY != oldY )
		{
			m_cursorX = oldX;
			m_cursorY = oldY;
			armed = false;
		}
	}

	if ( armed )
	{
		if ( RadialMenuDebug.GetBool() && ( m_cursorX != oldX || m_cursorY != oldY ) )
		{
			Msg( "%f: Rosetta cursor pos is %d,%d which is over %s\n", gpGlobals->curtime, m_cursorX, m_cursorY, ButtonDirString( m_armedButtonDir ) );
			engine->Con_NPrintf( 20, "%d %d %s", m_cursorX, m_cursorY, ButtonDirString( m_armedButtonDir ) );
		}
		return;
	}

	for ( int i=NUM_BUTTON_DIRS-1; i>=0; --i )
	{
		if ( !m_buttons[i] )
			continue;

		if ( m_buttons[i]->IsVisible() && m_buttons[i]->IsEnabled() && m_buttons[i]->IsWithinTraverse( m_cursorX, m_cursorY, false ) )
		{
			SetArmedButtonDir( (ButtonDir)i );
			if ( RadialMenuDebug.GetBool() && ( m_cursorX != oldX || m_cursorY != oldY ) )
			{
				Msg( "%f: Rosetta cursor pos is %d,%d which is over %s\n", gpGlobals->curtime, m_cursorX, m_cursorY, ButtonDirString( m_armedButtonDir ) );
				engine->Con_NPrintf( 20, "%d %d %s", m_cursorX, m_cursorY, ButtonDirString( m_armedButtonDir ) );
			}
			return;
		}
	}

	if ( RadialMenuDebug.GetBool() && ( m_cursorX != oldX || m_cursorY != oldY ) )
	{
		Msg( "%f: Rosetta cursor pos is %d,%d which defaults to CENTER\n", gpGlobals->curtime, m_cursorX, m_cursorY );
		engine->Con_NPrintf( 20, "%d %d CENTER", m_cursorX, m_cursorY );
	}
	m_armedButtonDir = CENTER;
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::SendCommand( const char *commandStr ) const
{
	engine->ClientCmd_Unrestricted( commandStr );
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::ChooseArmedButton( void )
{
	StartFade();

	CRadialButton *button = NULL;
	for ( int i=NUM_BUTTON_DIRS-1; i>=0; --i )
	{
		if ( !m_buttons[i] )
			continue;

		if ( m_buttons[i]->IsVisible() && m_buttons[i]->IsEnabled() && m_buttons[i]->IsArmed() && !m_buttons[i]->GetPassthru() )
		{
			if ( RadialMenuDebug.GetBool() )
			{
				Msg( "%f: Choosing armed button %s\n", gpGlobals->curtime, ButtonDirString( (ButtonDir)i ) );
			}
			button = m_buttons[i];
			break;
		}
	}

	if ( !button && m_armedButtonDir != NUM_BUTTON_DIRS )
	{
		if ( m_buttons[m_armedButtonDir] && m_buttons[m_armedButtonDir]->IsVisible() && m_buttons[m_armedButtonDir]->IsEnabled() )
		{
			if ( RadialMenuDebug.GetBool() )
			{
				Msg( "%f: Choosing saved armed button %s\n", gpGlobals->curtime, ButtonDirString( m_armedButtonDir ) );
			}
			button = m_buttons[m_armedButtonDir];
		}
	}

	if ( button )
	{
		KeyValues *command = button->GetCommand();
		if ( command )
		{
			const char *commandStr = command->GetString( "command", NULL );
			if ( commandStr )
			{
				button->SetChosen( true );
				SendCommand( commandStr );
			}
		}

		for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
		{
			if ( !m_buttons[i] )
				continue;

			if ( m_buttons[i] == button )
				continue;

			m_buttons[i]->SetFakeArmed( false );
			m_buttons[i]->SetChosen( false );
		}
	}
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::StartFade( void )
{
	m_fading = true;
	m_fadeStart = gpGlobals->curtime;
	input->SetFullscreenMousePos( m_cursorOriginalX, m_cursorOriginalY );
	SetMouseInputEnabled( false );
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::SetData( KeyValues *data )
{
	if ( !data )
		return;

	if ( RadialMenuDebug.GetBool() )
	{
		m_resource->deleteThis();
		m_resource = new KeyValues( "RadialMenu" );
		m_resource->LoadFromFile( filesystem, "resource/UI/RadialMenu.res" );
	}

	if ( m_menuData != data )
	{
		if ( m_menuData )
		{
			m_menuData->deleteThis();
		}

		m_menuData = data->MakeCopy();
	}

	for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
	{
		if ( !m_buttons[i] )
			continue;

		ButtonDir dir = (ButtonDir)i;
		const char *buttonName = ButtonNameFromDir( dir );
		KeyValues *buttonData = data->FindKey( buttonName, false );
		if ( !buttonData )
		{
			m_buttons[i]->SetVisible( false );
			continue;
		}

		KeyValues *resourceControl = m_resource->FindKey( buttonName, false );
		if ( resourceControl )
		{
			m_buttons[i]->UpdateHotspots( resourceControl );
			m_buttons[i]->InvalidateLayout();
		}

		m_buttons[i]->SetVisible( true );
		m_buttons[i]->SetChosen( false );

		const char *text = buttonData->GetString( "text" );
		m_buttons[i]->SetText( text );

		const char *command = buttonData->GetString( "command" );
		m_buttons[i]->SetCommand( command );

		m_buttons[i]->ShowSubmenuIndicator( Q_strncasecmp( "radialmenu ", command, Q_strlen( "radialmenu " ) ) == 0 );

		const char *owner = buttonData->GetString( "owner", NULL );
		if ( owner )
		{
			ButtonDir dir = DirFromButtonName( owner );
			m_buttons[i]->SetPassthru( m_buttons[dir] );
		}
		else
		{
			m_buttons[i]->SetPassthru( NULL );
		}

		m_buttons[i]->SetArmed( false );
	}
}


//--------------------------------------------------------------------------------------------------------
void CRadialMenu::Update( void )
{
	m_fading = false;
	m_fadeStart = 0.0f;
	SetAlpha( 255 );

	CRadialButton *firstButton = NULL;
	for ( int i=0; i<NUM_BUTTON_DIRS; ++i )
	{
		if ( !m_buttons[i] || !m_buttons[i]->IsVisible() || !m_buttons[i]->IsEnabled() )
			continue;

		if ( firstButton )
		{
			// already found another valid button.  since we have at least 2, we can show the menu.
			SetMouseInputEnabled( true );
			return;
		}

		firstButton = m_buttons[i];
	}

	// if we only found one button, close the window...
	ShowPanel( false );

	// ... and execute it's command
	if ( firstButton )
	{
		KeyValues *command = firstButton->GetCommand();
		if ( command )
		{
			const char *commandStr = command->GetString( "command", NULL );
			if ( commandStr )
			{
				engine->ClientCmd_Unrestricted( commandStr );
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------
/**
 * Helper class for managing our list of possible menus.  This means we can load RadialMenu.txt,
 * and add in additional menus from other text files (UserRadialMenu.txt?)
 */
class ClientMenuManager
{
public:
	~ClientMenuManager()
	{
		Reset();
	}

	void AddMenuFile( const char *filename )
	{
		if ( !m_menuKeys )
		{
			m_menuKeys = new KeyValues( "ClientMenu" );
		}

		KeyValues *data = new KeyValues( "ClientMenu" );
		if ( !data->LoadFromFile( filesystem, filename ) )
		{
			Warning( "Could not load client menu %s\n", filename );
			data->deleteThis();
			return;
		}

		KeyValues *menuKey = data->GetFirstTrueSubKey();
		while ( menuKey )
		{
			if ( m_menuKeys->FindKey( menuKey->GetName(), false ) == NULL )
			{
				data->RemoveSubKey( menuKey );
				m_menuKeys->AddSubKey( menuKey );
			}
			menuKey = data->GetFirstTrueSubKey();
		}
		data->deleteThis();
	}

	KeyValues *FindMenu( const char *menuName )
	{
		// do not show the menu if the player is dead or is an observer
		C_BasePlayer *player = C_BasePlayer::GetLocalPlayer();
		if ( !player )
			return NULL;

		if ( !menuName || !menuName[ 0 ] )
		{
			return m_menuKeys->FindKey( "Default", false );
		}

		// Find our menu, honoring Alive/Dead/Team suffixes
		const char *teamSuffix = player->GetTeam()->Get_Name();
		const char *lifeSuffix = player->IsAlive() ? "Alive" : "Dead";

		CFmtStr str;
		const char *fullMenuName = str.sprintf( "%s,%s,%s", menuName, teamSuffix, lifeSuffix );
		KeyValues *menuKey = m_menuKeys->FindKey( fullMenuName, false );
		if ( !menuKey )
		{
			fullMenuName = str.sprintf( "%s,%s", menuName, teamSuffix );
			menuKey = m_menuKeys->FindKey( fullMenuName, false );
		}
		if ( !menuKey )
		{
			fullMenuName = str.sprintf( "%s,%s", menuName, lifeSuffix );
			menuKey = m_menuKeys->FindKey( fullMenuName, false );
		}
		if ( !menuKey )
		{
			fullMenuName = menuName;
			menuKey = m_menuKeys->FindKey( fullMenuName, false );
		}

		return menuKey;
	}

	void Flush( void )
	{
		Reset();
		AddMenuFile( "scripts/RadialMenu.txt" );
	}

	void PrintStats( void )
	{
		DevMsg( "Client menus:\n" );
		for ( KeyValues *pKey = m_menuKeys->GetFirstTrueSubKey(); pKey; pKey = pKey->GetNextTrueSubKey() )
		{
			DevMsg( "\t%s\n", pKey->GetName() );
		}
	}

private:
	void Reset( void )
	{
		if ( m_menuKeys )
		{
			m_menuKeys->deleteThis();
		}
		m_menuKeys = NULL;
	}

	KeyValues *m_menuKeys;
};


static ClientMenuManager TheClientMenuManager;


//--------------------------------------------------------------------------------------------------------
void FlushClientMenus( void )
{
	TheClientMenuManager.Flush();
}


//--------------------------------------------------------------------------------------------------------
bool IsRadialMenuOpen( const char *menuName, bool includeFadingOut )
{
	//ASSERT_LOCAL_PLAYER_RESOLVABLE();
	int nSlot = 0;

	CRadialMenu *pMenu = GET_HUDELEMENT( CRadialMenu );
	if ( !pMenu )
		return false;

	if ( menuName == NULL || FStrEq( s_radialMenuName[ nSlot ], menuName ) )
	{
		bool wasOpen = pMenu->IsVisible() && ( !pMenu->IsFading() || includeFadingOut );
		if ( wasOpen )
		{
			return true;
		}
	}

	return false;
}


//--------------------------------------------------------------------------------------------------------
void OpenRadialMenu( const char *menuName )
{
	if ( !menuName )
		return;

	int nSlot = 0;

	CRadialMenu *pMenu = GET_HUDELEMENT( CRadialMenu );
	if ( !pMenu )
		return;

	if ( FStrEq( s_radialMenuName[ nSlot ], menuName ) )
	{
		bool wasOpen = pMenu->IsVisible() && !pMenu->IsFading();
		if ( wasOpen )
		{
			return;
		}
	}

	if ( menuName == NULL )
	{
		pMenu->ShowPanel( false );
		return;
	}

	if ( RadialMenuDebug.GetBool() )
	{
		FlushClientMenus(); // for now, reload every time
	}

	KeyValues *menuKey = TheClientMenuManager.FindMenu( NULL );
	if ( !menuKey )
	{
		//DevMsg( "No client menu currently matches %s\n", menuName );
		pMenu->ShowPanel( false );
		return;
	}

	V_snprintf( s_radialMenuName[ nSlot ], sizeof( s_radialMenuName[ nSlot ] ), menuName );
	pMenu->SetData( menuKey );

	input->GetFullscreenMousePos( &pMenu->m_cursorOriginalX, &pMenu->m_cursorOriginalY );

	int wide, tall;
	pMenu->GetSize( wide, tall );
	wide /= 2;
	tall /= 2;
	pMenu->LocalToScreen( wide, tall );
	input->SetFullscreenMousePos(wide, tall);

	pMenu->ShowPanel( true );
	pMenu->Update();
}


//--------------------------------------------------------------------------------------------------------
// there is a small issue that isn't present with the mouse_menu. 
// also l4d2 uses +mouse_menu command for all the radial menus anyways. 
// If you use a menu with the command "radialmenu" it works but you just cant open a new one using the same command
// unless you change the arg. really fucky and i really should look into fixing this properly but oh well
// mouse_menu works for now... - Vvis :3 
CON_COMMAND( radialmenu, "Opens a radial menu" )
{
	if ( args.ArgC() < 2 )
	{
		OpenRadialMenu(NULL);
	}
	else
	{
		OpenRadialMenu( args[1] );
	}
}

//--------------------------------------------------------------------------------------------------------
void openradialmenu( const CCommand &args )
{
	int nSlot = 0;

	if ( s_mouseMenuKeyHeld[ nSlot ] )
		return;

	if ( !C_TFPlayer::GetLocalTFPlayer() )
		return;

	s_mouseMenuKeyHeld[ nSlot ] = true;
	radialmenu( args );
}
static ConCommand mouse_menu_open( "+mouse_menu", openradialmenu, "Opens a menu while held" );


//--------------------------------------------------------------------------------------------------------
void closeradialmenu( void )
{
	int nSlot = 0;

	s_mouseMenuKeyHeld[ nSlot ] = false;

	if ( !C_TFPlayer::GetLocalTFPlayer() )
		return;

	if ( !cl_fastradial.GetBool() )
	{
		return;
	}

	CRadialMenu *pMenu = GET_HUDELEMENT( CRadialMenu );
	if ( !pMenu )
		return;

	bool wasOpen = pMenu->IsVisible() && !pMenu->IsFading();

	if ( wasOpen )
	{
		pMenu->ChooseArmedButton();

		int wide, tall;
		pMenu->GetSize( wide, tall );
		wide /= 2;
		tall /= 2;
		pMenu->LocalToScreen( wide, tall );
		vgui::surface()->SurfaceSetCursorPos( wide, tall );
	}
	else if ( !pMenu->IsVisible() )
	{
		pMenu->ShowPanel( false );
	}

	s_radialMenuName[ nSlot ][0] = 0;
}
static ConCommand mouse_menu_close( "-mouse_menu", closeradialmenu, "Executes the highlighted button on the radial menu (if cl_fastradial is 1)" );


//--------------------------------------------------------------------------------------------------------
void CloseRadialMenu( const char *menuName, bool sendCommand )
{
	int nSlot = 0;

	if ( FStrEq( s_radialMenuName[ nSlot ], menuName ) )
	{
		s_mouseMenuKeyHeld[ nSlot ] = false;
		CRadialMenu *pMenu = GET_HUDELEMENT( CRadialMenu );
		if ( !pMenu )
			return;

		bool wasOpen = pMenu->IsVisible() && !pMenu->IsFading();

		if ( wasOpen )
		{
			if ( sendCommand )
			{
				pMenu->ChooseArmedButton();
			}
			else
			{
				pMenu->StartFade();
			}

			int wide, tall;
			pMenu->GetSize( wide, tall );
			wide /= 2;
			tall /= 2;
			pMenu->LocalToScreen( wide, tall );
			vgui::surface()->SurfaceSetCursorPos( wide, tall );
		}
		else if ( !pMenu->IsVisible() )
		{
			pMenu->ShowPanel( false );
		}

		s_radialMenuName[ nSlot ][ 0 ] = 0;
	}
}


//--------------------------------------------------------------------------------------------------------





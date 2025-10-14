// RadialMenu.h
// Copyright (c) 2006 Turtle Rock Studios, Inc.

#ifndef RADIALMENU_H
#define RADIALMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>

#define PANEL_RADIAL_MENU			"RadialMenu"

class CRadialButton;

//--------------------------------------------------------------------------------------------------------
/**
 * Viewport panel that gives us a simple rosetta menu
 */
class CRadialMenu : public CHudElement, public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CRadialMenu, vgui::EditablePanel );

public:
	CRadialMenu( const char *pElementName );
	virtual ~CRadialMenu();

	virtual void SetData( KeyValues *data );
	virtual KeyValues *GetData( void ) { return NULL; }
	bool		 IsFading( void ) { return IsVisible() && m_fading; }
	void		 StartFade( void );
	void		 ChooseArmedButton( void );

	virtual const char *GetName( void ) { return PANEL_RADIAL_MENU; }
	virtual void Reset( void ) {}
	virtual void Update( void );
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	void InitializeInputScheme();
	virtual void Paint( void );
	void OpenMenuByName(const char * menuName);
	virtual void OnCommand( const char *command );
	virtual vgui::Panel *CreateControlByName( const char *controlName );
	virtual void OnThink( void );

	void OnCursorEnteredButton( int x, int y, CRadialButton *button );
	KeyValues *g_pAllRadialMenus = nullptr;

	void LoadAllRadialMenus()
	{
		if(g_pAllRadialMenus)
			g_pAllRadialMenus->deleteThis();

		g_pAllRadialMenus = new KeyValues("RadialMenu");
		g_pAllRadialMenus->LoadFromFile(filesystem,"resource/UI/RadialMenu.res");
	}

	enum ButtonDir
	{
		CENTER = 0,
		NORTH,
		NORTH_EAST,
		EAST,
		SOUTH_EAST,
		SOUTH,
		SOUTH_WEST,
		WEST,
		NORTH_WEST,
		NUM_BUTTON_DIRS
	};
	bool						m_bMouseActivated;

	bool UseMouseMode()
    {
        return true;
    }

protected:

	void SendCommand( const char *commandStr ) const;
	const char *ButtonNameFromDir( ButtonDir dir );
	ButtonDir DirFromButtonName( const char * name );
	CRadialButton *m_buttons[NUM_BUTTON_DIRS];	// same order as vgui::Label::Alignment
	ButtonDir m_armedButtonDir;
	void SetArmedButtonDir( ButtonDir dir );
	void UpdateButtonBounds( void );

	void HandleControlSettings();

	int m_cursorX;
	int m_cursorY;

	bool m_fading;
	float m_fadeStart;

	Color m_lineColor;

	KeyValues *m_resource;
	KeyValues *m_menuData;

	int m_minButtonX;
	int m_minButtonY;
	int m_maxButtonX;
	int m_maxButtonY;

public:

	int m_cursorOriginalX;
	int m_cursorOriginalY;
};

void OpenRadialMenu( const char *menuName );
void CloseRadialMenu( const char *menuName, bool sendCommand = false );
bool IsRadialMenuOpen( const char *menuName, bool includeFadingOut );

//--------------------------------------------------------------------------------------------------------
/**
 * Button class that clips its visible bounds to an concave polygon (defined counter-clockwise).
 * An optional second hotspot can be defined for capturing mouse input outside of the visible hotspot.
 */
class CPolygonButton : public vgui::Button
{
	DECLARE_CLASS_SIMPLE( CPolygonButton, vgui::Button );

public:

	CPolygonButton( vgui::Panel *parent, const char *panelName );

	virtual void ApplySettings( KeyValues *data );

	/**
	 * Clip out cursor positions inside our overall rectangle that are outside our hotspot.
	 */
	virtual vgui::VPANEL IsWithinTraverse( int x, int y, bool traversePopups );

	/**
	 * Perform the standard layout, and scale our hotspot points - they are specified as a 0..1 percentage
	 * of the button's full size.
	 */
	virtual void PerformLayout( void );

	/**
	 * Center the text in the extent that encompasses our hotspot.
	 * TODO: allow the res file and/or the individual menu to specify a different center for text.
	 */
	virtual void ComputeAlignment( int &tx0, int &ty0, int &tx1, int &ty1 );
	virtual void PaintBackground( void );
	virtual void PaintBorder( void );
	virtual void ApplySchemeSettings( vgui::IScheme *scheme );

	virtual void UpdateHotspots( KeyValues *data );

protected:
	int m_nWhiteMaterial;

	CUtlVector< Vector2D > m_unscaledHotspotPoints;
	CUtlVector< Vector2D > m_unscaledVisibleHotspotPoints;
	vgui::Vertex_t *m_hotspotPoints;
	int m_numHotspotPoints;
	vgui::Vertex_t *m_visibleHotspotPoints;
	int m_numVisibleHotspotPoints;

	Vector2D m_hotspotMins;
	Vector2D m_hotspotMaxs;
};

//--------------------------------------------------------------------------------------------------------
CPolygonButton::CPolygonButton( vgui::Panel *parent, const char *panelName )
	: vgui::Button( parent, panelName, L"" )
{
	m_unscaledHotspotPoints.RemoveAll();
	m_unscaledVisibleHotspotPoints.RemoveAll();
	m_hotspotPoints = NULL;
	m_visibleHotspotPoints = NULL;
	m_numHotspotPoints = 0;
	m_numVisibleHotspotPoints = 0;

	m_nWhiteMaterial = vgui::surface()->CreateNewTextureID();
	vgui::surface()->DrawSetTextureFile( m_nWhiteMaterial, "vgui/white" , true, false );
}

//--------------------------------------------------------------------------------------------------------
void CPolygonButton::ApplySettings( KeyValues *data )
{
	BaseClass::ApplySettings( data );

	// Re-read hotspot data from disk
	UpdateHotspots( data );
}

//--------------------------------------------------------------------------------------------------------
void CPolygonButton::UpdateHotspots( KeyValues *data )
{
	// clear out our old hotspot
	if ( m_hotspotPoints )
	{
		delete[] m_hotspotPoints;
		m_hotspotPoints = NULL;
		m_numHotspotPoints = 0;
	}
	if ( m_visibleHotspotPoints )
	{
		delete[] m_visibleHotspotPoints;
		m_visibleHotspotPoints = NULL;
		m_numVisibleHotspotPoints = 0;
	}
	m_unscaledHotspotPoints.RemoveAll();
	m_unscaledVisibleHotspotPoints.RemoveAll();

	// read in a new one
	KeyValues *points = data->FindKey( "Hotspot", false );
	if ( points )
	{
		for ( KeyValues *value = points->GetFirstValue(); value; value = value->GetNextValue() )
		{
			const char *str = value->GetString();

			float x, y;
			if ( 2 == sscanf( str, "%f %f", &x, &y ) )
			{
				m_unscaledHotspotPoints.AddToTail( Vector2D( x, y ) );
			}
		}
	}
	points = data->FindKey( "VisibleHotspot", false );
	if ( !points )
	{
		points = data->FindKey( "Hotspot", false );
	}
	if ( points )
	{
		for ( KeyValues *value = points->GetFirstValue(); value; value = value->GetNextValue() )
		{
			const char *str = value->GetString();

			float x, y;
			if ( 2 == sscanf( str, "%f %f", &x, &y ) )
			{
				m_unscaledVisibleHotspotPoints.AddToTail( Vector2D( x, y ) );
			}
		}
	}
}

//--------------------------------------------------------------------------------------------------------
/**
 * Clip out cursor positions inside our overall rectangle that are outside our hotspot.
 */
vgui::VPANEL CPolygonButton::IsWithinTraverse( int x, int y, bool traversePopups )
{
	if ( m_numHotspotPoints < 3 )
	{
		return NULL;
	}

	vgui::VPANEL within = BaseClass::IsWithinTraverse( x, y, traversePopups );
	if ( within == GetVPanel() )
	{
		int wide, tall;
		GetSize( wide, tall );
		ScreenToLocal( x, y );

		bool inside = true;
		for ( int i=0; i<m_numHotspotPoints; ++i )
		{
			const Vector2D& pos1 = (i==0)?m_hotspotPoints[m_numHotspotPoints-1].m_Position:m_hotspotPoints[i-1].m_Position;
			const Vector2D& pos2 = m_hotspotPoints[i].m_Position;
			Vector p1( pos1.x - x, pos1.y - y, 0 );
			Vector p2( pos2.x - x, pos2.y - y, 0 );
			Vector out = p1.Cross( p2 );
			if ( out.z < 0 )
			{
				inside = false;
			}
		}

		if ( !inside )
		{
			within = NULL;
		}
	}

	return within;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Perform the standard layout, and scale our hotspot points - they are specified as a 0..1 percentage
 * of the button's full size.
 */
void CPolygonButton::PerformLayout( void )
{
	int wide, tall;
	GetSize( wide, tall );

	if ( m_hotspotPoints )
	{
		delete[] m_hotspotPoints;
		m_hotspotPoints = NULL;
		m_numHotspotPoints = 0;
	}
	if ( m_visibleHotspotPoints )
	{
		delete[] m_visibleHotspotPoints;
		m_visibleHotspotPoints = NULL;
		m_numVisibleHotspotPoints = 0;
	}

	// generate scaled points
	m_numHotspotPoints = m_unscaledHotspotPoints.Count();
	if ( m_numHotspotPoints )
	{
		m_hotspotPoints = new vgui::Vertex_t[ m_numHotspotPoints ];
		for ( int i=0; i<m_numHotspotPoints; ++i )
		{
			float x = m_unscaledHotspotPoints[i].x * wide;
			float y = m_unscaledHotspotPoints[i].y * tall;
			m_hotspotPoints[i].Init( Vector2D( x, y ), m_unscaledHotspotPoints[i] );
		}
	}

	// track our visible extent
	m_hotspotMins.Init( wide, tall );
	m_hotspotMaxs.Init( 0, 0 );

	m_numVisibleHotspotPoints = m_unscaledVisibleHotspotPoints.Count();
	if ( m_numVisibleHotspotPoints )
	{
		m_visibleHotspotPoints = new vgui::Vertex_t[ m_numVisibleHotspotPoints ];
		for ( int i=0; i<m_numVisibleHotspotPoints; ++i )
		{
			float x = m_unscaledVisibleHotspotPoints[i].x * wide;
			float y = m_unscaledVisibleHotspotPoints[i].y * tall;
			m_visibleHotspotPoints[i].Init( Vector2D( x, y ), m_unscaledVisibleHotspotPoints[i] );

			m_hotspotMins.x = MIN( x, m_hotspotMins.x );
			m_hotspotMins.y = MIN( y, m_hotspotMins.y );
			m_hotspotMaxs.x = MAX( x, m_hotspotMaxs.x );
			m_hotspotMaxs.y = MAX( y, m_hotspotMaxs.y );
		}
	}

	BaseClass::PerformLayout();
}

//--------------------------------------------------------------------------------------------------------
/**
 * Center the text in the extent that encompasses our hotspot.
 * TODO: allow the res file and/or the individual menu to specify a different center for text.
 */
void CPolygonButton::ComputeAlignment( int &tx0, int &ty0, int &tx1, int &ty1 )
{
	Vector2D center( (m_hotspotMins + m_hotspotMaxs) * 0.5f );

	BaseClass::ComputeAlignment( tx0, ty0, tx1, ty1 );
	int textWide, textTall;
	textWide = tx1 - tx0;
	textTall = ty1 - ty0;

	tx0 = center.x - textWide/2;
	ty0 = center.y - textTall/2;
	tx1 = tx0 + textWide;
	ty1 = ty0 + textTall;
}

//--------------------------------------------------------------------------------------------------------
/**
 * Paints the polygonal background
 */
void CPolygonButton::PaintBackground( void )
{
	Color c = GetButtonBgColor();
	vgui::surface()->DrawSetColor( c );
	vgui::surface()->DrawSetTexture( m_nWhiteMaterial );
	vgui::surface()->DrawTexturedPolygon( m_numVisibleHotspotPoints, m_visibleHotspotPoints );
}

//--------------------------------------------------------------------------------------------------------
/**
 * Paints the polygonal border
 */
void CPolygonButton::PaintBorder( void )
{
	Color c = GetButtonFgColor();
	vgui::surface()->DrawSetColor( c );
	vgui::surface()->DrawSetTexture( m_nWhiteMaterial );
	vgui::surface()->DrawTexturedPolyLine( m_visibleHotspotPoints, m_numVisibleHotspotPoints );
}

//--------------------------------------------------------------------------------------------------------
void CPolygonButton::ApplySchemeSettings( vgui::IScheme *scheme )
{
	BaseClass::ApplySchemeSettings( scheme );

	InvalidateLayout(); // so we can reposition the text
}
#endif // RADIALMENU_H

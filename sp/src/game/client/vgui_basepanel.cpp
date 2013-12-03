//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_basepanel.h"
#include <KeyValues.h>
#include <vgui/IScheme.h>
#include <vgui/IVGui.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePanel::CBasePanel( vgui::Panel *pParent, const char *panelName ) : BaseClass( pParent, panelName )
{
	m_bTexturedBackground = false;
	m_nBackgroundMaterial = -1;
	m_szBgTexture[ 0 ] = 0;
	m_bTiled = false;
	m_nTextureSize[ 0 ] = 0;
	m_nTextureSize[ 1 ] = 0;

	m_bReflectMouse = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : x - 
//			y - 
//			w - 
//			h - 
//-----------------------------------------------------------------------------
CBasePanel::CBasePanel( vgui::Panel *pParent, const char *panelName, int x, int y, int w, int h ) :
  Panel( pParent, panelName )
{
	SetBounds( x, y, w, h );
	m_bTexturedBackground = false;
	m_nBackgroundMaterial = -1;
	m_szBgTexture[ 0 ] = 0;
	m_bTiled = false;
	m_nTextureSize[ 0 ] = 0;
	m_nTextureSize[ 1 ] = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CBasePanel::~CBasePanel( void )
{
	if ( vgui::surface() && m_nBackgroundMaterial != -1 )
	{
		vgui::surface()->DestroyTextureID( m_nBackgroundMaterial );
		m_nBackgroundMaterial = -1;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CBasePanel::PaintBackground( void )
{
	if ( !m_bTexturedBackground )
	{
		BaseClass::PaintBackground();
		return;
	}

	if ( m_nBackgroundMaterial == -1 )
	{
		m_nBackgroundMaterial = vgui::surface()->CreateNewTextureID();
		vgui::surface()->DrawSetTextureFile( m_nBackgroundMaterial, m_szBgTexture, true, true );
	}
	
	vgui::surface()->DrawSetColor( GetFgColor() );

	// Draw it with texture
	vgui::surface()->DrawSetTexture( m_nBackgroundMaterial );

	if ( m_bTiled && ( !m_nTextureSize[ 0 ] ) )
	{
		vgui::surface()->DrawGetTextureSize( m_nBackgroundMaterial, m_nTextureSize[ 0 ], m_nTextureSize[ 1 ] );
	}

	int wide, tall;
	GetSize( wide, tall );

	if ( m_bTiled && m_nTextureSize[ 0 ] && m_nTextureSize[ 1 ] )
	{
		int x = 0;
		int y = 0;

		while ( y < tall )
		{
			vgui::surface()->DrawTexturedRect( 0, 0, m_nTextureSize[ 0 ], m_nTextureSize[ 1 ] );

			x += m_nTextureSize[ 0 ];

			if ( x >= wide )
			{
				x = 0;
				y += m_nTextureSize[ 1 ];
			}
		}
	}
	else
	{
		vgui::surface()->DrawTexturedRect( 0, 0, wide, tall );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *texname - 
//-----------------------------------------------------------------------------
void CBasePanel::SetTexture( const char *texname, bool tiled /*=false*/ )
{
	m_bTexturedBackground = true;
	m_bTiled = tiled;
	Q_strncpy( m_szBgTexture, texname, sizeof( m_szBgTexture ) );

	if ( m_nBackgroundMaterial == -1 )
	{
		m_nBackgroundMaterial = vgui::surface()->CreateNewTextureID();
	}
	vgui::surface()->DrawSetTextureFile( m_nBackgroundMaterial, m_szBgTexture , true, true);
}

void CBasePanel::SetReflectMouse( bool reflect )
{
	m_bReflectMouse = true;
}

void CBasePanel::OnCursorMoved(int x,int y)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	LocalToScreen( x, y );

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "CursorMoved", "xpos", x, "ypos", y ), 
		GetVPanel() );
}

void CBasePanel::OnMousePressed(vgui::MouseCode code)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MousePressed", "code", code ), 
		GetVPanel() );
}

void CBasePanel::OnMouseDoublePressed(vgui::MouseCode code)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MouseDoublePressed", "code", code ), 
		GetVPanel() );
}

void CBasePanel::OnMouseReleased(vgui::MouseCode code)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MouseReleased", "code", code ), 
		GetVPanel() );
}

void CBasePanel::OnMouseWheeled(int delta)
{
	if ( !m_bReflectMouse )
		return;

	if ( !GetParent() )
		return;

	vgui::ivgui()->PostMessage( 
		GetParent()->GetVPanel(), 
		new KeyValues( "MouseWheeled", "delta", delta ), 
		GetVPanel()  );
}

void CBasePanel::OnTick( void )
{
	// Nothing
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CHudLabel::CHudLabel( vgui::Panel *parent, const char *panelName, const char *text) :
	vgui::Label( parent,panelName,text )
{
	m_bSelected = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CHudLabel::ApplySchemeSettings(vgui::IScheme *pScheme)
{
	Panel::ApplySchemeSettings(pScheme);

	if ( m_bSelected )
	{
		SetBgColor( GetSchemeColor("HudStatusSelectedBgColor", pScheme) );
	}
	else
	{
		SetBgColor( GetSchemeColor("HudStatusBgColor", pScheme) );
	}
}




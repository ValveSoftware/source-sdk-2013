//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//
#if !defined( VGUI_BASEPANEL_H )
#define VGUI_BASEPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <stdarg.h>
#include <vgui_controls/Panel.h>
#include <vgui_controls/Label.h>
#include <vgui_controls/Controls.h>
#include <vgui/ISurface.h>

//-----------------------------------------------------------------------------
// Global interface allowing various rendering
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose: Base Panel for engine vgui panels ( can handle some drawing stuff )
//-----------------------------------------------------------------------------
class CBasePanel : public vgui::Panel
{
public:
	DECLARE_CLASS_GAMEROOT( CBasePanel, vgui::Panel );

					CBasePanel( vgui::Panel *pParent, const char *panelName );
					CBasePanel( vgui::Panel *pParent, const char *panelName, int x, int y, int w, int h );
	virtual			~CBasePanel( void );

	// should this panel be drawn this frame?
	virtual bool	ShouldDraw( void ) { return true;}

	virtual void	PaintBackground( void );

	virtual	void	SetTexture( const char *texname, bool tiled = false );

	virtual void	SetReflectMouse( bool reflect );
	// If reflect mouse is true, then pass these up to parent
	virtual void	OnCursorMoved(int x,int y);
	virtual void	OnMousePressed(vgui::MouseCode code);
	virtual void	OnMouseDoublePressed(vgui::MouseCode code);
	virtual void	OnMouseReleased(vgui::MouseCode code);
	virtual void	OnMouseWheeled(int delta);

	virtual void	OnTick( void );

protected:
	bool			m_bTexturedBackground;
	int				m_nBackgroundMaterial;
	char			m_szBgTexture[ 256 ];
	bool			m_bTiled;
	int				m_nTextureSize[ 2 ];

	bool			m_bReflectMouse;
};

//-----------------------------------------------------------------------------
// Purpose: Hud labels that use HUD scheme colors
//-----------------------------------------------------------------------------
class CHudLabel : public vgui::Label
{
	typedef vgui::Label BaseClass;
public:
	CHudLabel( vgui::Panel *parent, const char *panelName, const char *text );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	
	// Selection highlight
	void	SetSelected( bool bSelected );

	bool	m_bSelected;

private:
	CHudLabel( const CHudLabel & ); // not defined, not accessible
};

#endif // VGUI_BASEPANEL_H

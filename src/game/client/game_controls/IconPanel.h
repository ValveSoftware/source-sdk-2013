//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ICONPANEL_H
#define ICONPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>

using namespace vgui;

class CIconPanel : public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CIconPanel, vgui::Panel );

public:
	CIconPanel( vgui::Panel *parent, const char *name );

	void Init( void );
	virtual void Paint();
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void SetIcon( const char *szIcon );
	void SetIconColor( Color cColor ) { m_IconColor = cColor; }

private:
	CHudTexture		*m_icon;
	char			m_szIcon[128];

	bool			m_bScaleImage;

	CPanelAnimationVar( Color, m_IconColor, "iconColor", "255 255 255 255" );
};

#endif	//ICONPANEL_H

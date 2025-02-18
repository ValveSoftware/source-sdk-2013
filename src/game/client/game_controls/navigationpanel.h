//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef NAVIGATIONPANEL_H
#define NAVIGATIONPANEL_H
#ifdef _WIN32
#pragma once
#endif

//-----------------------------------------------------------------------------

#include "vgui_controls/EditablePanel.h"

//-----------------------------------------------------------------------------

namespace vgui
{
	class Panel;
	class ImagePanel;
};
class CNavButton;
class CExImageButton;

//-----------------------------------------------------------------------------

//
// A generic panel containing a list of buttons which can be displayed vertically
// or horizontally.  One button at a time can be selected, and messages are sent
// to the parent by default.
//
class CNavigationPanel : public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CNavigationPanel, vgui::EditablePanel );

public:
	CNavigationPanel( vgui::Panel *pParent, const char *pName, bool bAddParentAsActionSignalTarget = true );
	virtual ~CNavigationPanel();

	void AddButton( int iUserData, const char *pTextToken );
	int NumButtons() const { return m_vecButtons.Count(); }
	CExImageButton *GetButton( int index );

	void UpdateButtonSelectionStates( int iButton );

protected:
	virtual void PerformLayout();
	virtual void ApplySettings( KeyValues *pInResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *pCommand );
	virtual void OnThink();

	CUtlVector< CNavButton * > m_vecButtons;
	bool		m_bAutoLayout;
	bool		m_bAutoScale;	// Auto-scale buttons to proportionally match height (for horizontal display) or width (for vertical display)
	bool		m_bDisplayVertical;
	int			m_iSelectedButton;	// The currently selected button
	KeyValues	*m_pKVButtonSettings;

	/*
	enum Alignment_t
	{
		ALIGN_WEST,	// left
		ALIGN_CENTER,
	};

	Alignment_t	m_nAlignment;
	*/

	// For auto-layout mode only
	CPanelAnimationVarAliasType( int, m_nHorizontalBuffer, "auto_layout_horizontal_buffer", "5", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nVerticalBuffer, "auto_layout_vertical_buffer", "5", "proportional_ypos" );
	CPanelAnimationVar( int, m_iSelectedButtonDefault, "selected_button_default", "-1" );
};

//-----------------------------------------------------------------------------

#endif // NAVIGATIONPANEL_H

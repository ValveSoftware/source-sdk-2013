//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef BLUEPRINT_PANEL_H
#define BLUEPRINT_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include <vgui_controls/Frame.h>
#include "tf_shareddefs.h"
#include "IconPanel.h"

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBlueprintPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBlueprintPanel, vgui::EditablePanel );
public:
	CBlueprintPanel( vgui::Panel *parent, const char *name );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

	void	SetObjectInfo( const CObjectInfo* pNewInfo );
	const CObjectInfo* GetObjectInfo( void ) { return m_pObjectInfo; }

	// Button functionality
	void	SetActAsButton( bool bClickable, bool bMouseOver );
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(vgui::MouseCode code);
	virtual void OnMouseDoublePressed(vgui::MouseCode code);
	virtual void OnMouseReleased(vgui::MouseCode code);
	MESSAGE_FUNC_INT_INT( OnCursorMoved, "OnCursorMoved", x, y );

	void	SetInStack( bool bVal ) { m_bInStack = bVal; }
	bool	IsInStack( void ) { return m_bInStack; }

	vgui::Label			*m_pItemNameLabel;
	vgui::Label			*m_pItemCostLabel;
	char				m_pszCost[8];

	CIconPanel			*m_pMetalIcon;
	CIconPanel			*m_pIcon;
	CIconPanel			*m_pBackground;

	const CObjectInfo*	m_pObjectInfo;

	bool				m_bClickable;
	bool				m_bMouseOver;

	bool				m_bInStack;
};

#endif // BLUEPRINT_PANEL_H

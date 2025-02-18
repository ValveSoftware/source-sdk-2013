//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Control for displaying a Steam Controller hint icon
//
// $NoKeywords: $
//=============================================================================//

#ifndef SC_HINTICON_H
#define SC_HINTICON_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <KeyValues.h>
#include <vgui/IVGui.h>
#include <vgui_controls/Panel.h>

class CSCHintIcon : public vgui::Panel
{
public:
	DECLARE_CLASS_SIMPLE( CSCHintIcon, vgui::Panel );

	CSCHintIcon( vgui::Panel *parent, const char *panelName );

	const char* GetActionName() const { return m_strActionName.Get(); }
	const char* GetActionSet() const { return m_strActionSet.Get(); }

	// Set the action name and/or set. Passing null for either (or both) is legal and leaves the action name and/or set unchanged. In any case, the
	// action origin is re-fetched and the displayed glyph updated as needed.
	void SetAction( const char* szActionName, const char* szActionSet = nullptr );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void PaintBackground();

private:
	CUtlString m_strActionName;
	CUtlString m_strActionSet;
	ControllerActionSetHandle_t m_actionSetHandle;
	int m_nGlyphTexture;

	static int GetVGUITextureIDForActionOrigin( EControllerActionOrigin eOrigin );
	static int s_nVGUITextureForOrigin[k_EControllerActionOrigin_Count];
};

#endif  // SC_HINTICON_H

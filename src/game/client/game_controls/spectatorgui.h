//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef SPECTATORGUI_H
#define SPECTATORGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui/KeyCode.h>
#include <vgui_controls/Frame.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/ComboBox.h>
#include <igameevents.h>
#include "GameEventListener.h"

#include <game/client/iviewport.h>

class KeyValues;

namespace vgui
{
	class TextEntry;
	class Button;
	class Panel;
	class ImagePanel;
	class ComboBox;
}

#define BLACK_BAR_COLOR	Color(0, 0, 0, 196)

class IBaseFileSystem;

//-----------------------------------------------------------------------------
// Purpose: Spectator UI
//-----------------------------------------------------------------------------
class CSpectatorGUI : public vgui::EditablePanel, public IViewPortPanel
{
	DECLARE_CLASS_SIMPLE( CSpectatorGUI, vgui::EditablePanel );

public:
	CSpectatorGUI( IViewPort *pViewPort );
	virtual ~CSpectatorGUI();

	virtual const char *GetName( void ) { return PANEL_SPECGUI; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset() {};
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return false; }
	virtual void ShowPanel( bool bShow );
	
	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }
	virtual void OnThink();

	virtual int GetTopBarHeight() { return m_pTopBar->GetTall(); }
	virtual int GetBottomBarHeight() { return m_pBottomBarBlank->GetTall(); }
	
	virtual bool ShouldShowPlayerLabel( int specmode );

	virtual Color GetBlackBarColor( void ) { return BLACK_BAR_COLOR; }

	virtual const char *GetResFile( void );

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_SPECTATOR; }
	
protected:

	void SetLabelText(const char *textEntryName, const char *text);
	void SetLabelText(const char *textEntryName, wchar_t *text);
	void MoveLabelToFront(const char *textEntryName);
	void UpdateTimer();
	void SetLogoImage(const char *image);

protected:	
	enum { INSET_OFFSET = 2 } ; 

	// vgui overrides
	virtual void PerformLayout();
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
//	virtual void OnCommand( const char *command );

	vgui::Panel *m_pTopBar;
	vgui::Panel *m_pBottomBarBlank;

	vgui::ImagePanel *m_pBannerImage;
	vgui::Label *m_pPlayerLabel;

	IViewPort *m_pViewPort;

	// bool m_bHelpShown;
	// bool m_bInsetVisible;
	bool m_bSpecScoreboard;

	int m_iWasSteamController = -1;
};


//-----------------------------------------------------------------------------
// Purpose: the bottom bar panel, this is a separate panel because it
// wants mouse input and the main window doesn't
//----------------------------------------------------------------------------
class CSpectatorMenu : public vgui::Frame, public IViewPortPanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE(  CSpectatorMenu, vgui::Frame );

public:
	CSpectatorMenu( IViewPort *pViewPort );
	~CSpectatorMenu() {}

	virtual const char *GetName( void ) { return PANEL_SPECMENU; }
	virtual void SetData(KeyValues *data) {};
	virtual void Reset( void ) { m_pPlayerList->DeleteAllItems(); }
	virtual void Update( void );
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );
	virtual void FireGameEvent( IGameEvent *event );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	virtual bool IsVisible() { return BaseClass::IsVisible(); }
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual void SetParent(vgui::VPANEL parent) { BaseClass::SetParent(parent); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_SPECTATOR; }

private:
	// VGUI2 overrides
	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void ApplySchemeSettings(vgui::IScheme *pScheme);
	virtual void PerformLayout();

	void SetViewModeText( const char *text ) { m_pViewOptions->SetText( text ); }
	void SetPlayerFgColor( Color c1 ) { m_pPlayerList->SetFgColor(c1); }

	vgui::ComboBox *m_pPlayerList;
	vgui::ComboBox *m_pViewOptions;
	vgui::ComboBox *m_pConfigSettings;

	vgui::Button *m_pLeftButton;
	vgui::Button *m_pRightButton;

	IViewPort *m_pViewPort;
	ButtonCode_t m_iDuckKey;
};

extern CSpectatorGUI * g_pSpectatorGUI;

#endif // SPECTATORGUI_H

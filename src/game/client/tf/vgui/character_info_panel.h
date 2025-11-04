//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CHARACTER_INFO_PANEL_H
#define CHARACTER_INFO_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"
#include "tf_shareddefs.h"
#include "GameEventListener.h"
#include "vgui_controls/Panel.h"
#include "vgui_controls/PHandle.h"

class CCharInfoLoadoutSubPanel;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CServerNotConnectedToSteamDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CServerNotConnectedToSteamDialog, vgui::EditablePanel );

public:
	CServerNotConnectedToSteamDialog( vgui::Panel *pParent, const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	OnCommand( const char *command );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCheatDetectionDialog : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCheatDetectionDialog, vgui::EditablePanel );

public:
	CCheatDetectionDialog( vgui::Panel *pParent, const char *pElementName );

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	OnCommand( const char *command );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCharacterInfoPanel : public vgui::PropertyDialog, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CCharacterInfoPanel, vgui::PropertyDialog );
public:
	CCharacterInfoPanel( Panel *parent );
	virtual ~CCharacterInfoPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void PerformLayout( void );
	virtual void OnCommand( const char *command );
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodeTyped(vgui::KeyCode code) OVERRIDE;
	virtual void OnKeyCodePressed(vgui::KeyCode code) OVERRIDE;
	virtual void OnThink();

	void		 OpenLoadoutToClass( int iClassIndex, bool bOpenClassLoadout );
	void		 OpenToPaintkitPreview( CEconItemView* pItem, bool bFixedItem, bool bFixedPaintkit );

	void		 FireGameEvent( IGameEvent *event );

	// Gamestats access
	virtual void		SetExperimentValue( uint64 experimentValue );

	// When the root UI is closed, send an "EconUIClosed" message to pListener.
	virtual void		AddPanelCloseListener( vgui::Panel *pListener );

	// The panel at which we want back to actually close the UI - defaults to the root panel - a negative value can be passed in for class loadout panels
	virtual void		SetClosePanel( int iPanel );

	// Call this to set which team the class loadout should display
	virtual void		SetDefaultTeam( int iTeam );

private:
	void Close();
	void NotifyListenersOfCloseEvent();

	vgui::Panel					*m_pNotificationsPresentPanel;
	CCharInfoLoadoutSubPanel	*m_pLoadoutPanel;
	bool						m_bPreventClosure;
	int							m_iClosePanel;
	int							m_iDefaultTeam;

	CUtlVector< vgui::VPanelHandle >	m_vecOnCloseListeners;
};

CCheatDetectionDialog *OpenCheatDetectionDialog( vgui::Panel *pParent, const char *pszCheatMessage );

#endif // CHARACTER_INFO_PANEL_H

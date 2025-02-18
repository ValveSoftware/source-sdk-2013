//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_CLASSMENU_H
#define TF_CLASSMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <classmenu.h>
#include <vgui_controls/EditablePanel.h>
#include "vgui_controls/KeyRepeat.h"
#include <filesystem.h>
#include <tf_shareddefs.h>
#include "cbase.h"
#include "tf_controls.h"
#include "tf_gamerules.h"
#include "basemodelpanel.h"
#include "IconPanel.h"
#include <vgui_controls/CheckButton.h>
#include "GameEventListener.h"
#include "c_tf_playerresource.h"
#include "tf_playermodelpanel.h"
#include "tf_mann_vs_machine_stats.h"

using namespace vgui;

#define CLASS_COUNT_IMAGES	11

class CTFClassTipsPanel;

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CTFClassTipsItemPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFClassTipsItemPanel, vgui::EditablePanel );

public:
	CTFClassTipsItemPanel( Panel *parent, const char *pszName, int iListItemID );
	~CTFClassTipsItemPanel();

	void			SetClassTip( const wchar_t *pwszText, const char *pszIcon );
	virtual void	ApplySchemeSettings( IScheme *pScheme );

private:
	vgui::ImagePanel		*m_pTipIcon;
	CExLabel				*m_pTipLabel;
};

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
class CTFClassMenu : public CClassMenu, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassMenu, CClassMenu );

public:
	CTFClassMenu( IViewPort *pViewPort );

	virtual void Update( void );
	virtual Panel *CreateControlByName( const char *controlName );
	virtual void OnTick( void );
	virtual void PaintBackground( void );
	virtual void SetVisible( bool state );
	virtual void PerformLayout();

	MESSAGE_FUNC_PTR_CHARPTR( OnShowPage, "ShowPage", panel, page );
	CON_COMMAND_MEMBER_F( CTFClassMenu, "join_class", Join_Class, "Send a joinclass command", 0 );

	virtual void OnCommand( const char *command );
	virtual void OnClose();
	virtual void ShowPanel( bool bShow );
	virtual void UpdateClassCounts( void ){}
	void		 SelectClass( int iClass );

	virtual int GetTeamNumber( void ) = 0;

	// IGameEventListener interface:
	virtual void FireGameEvent( IGameEvent *event );

	MESSAGE_FUNC( OnEconUIClosed, "EconUIClosed" );			// If the econ UI was opened (for editing loadout), we'll get notified when the user's done.

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_IN_GAME_HUD; }

protected:
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void OnKeyCodePressed( KeyCode code );
	CExImageButton *GetCurrentClassButton();
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnThink();
	virtual void UpdateNumClassLabels( int iTeam );

	void		 UpdateButtonSelectionStates( int iClass );
	void		 SetCancelButtonVisible( bool bVisible );
	int			 GetCurrentPlayerClass();
	void		 LoadItems();
	void		 Go();

protected:

	CExImageButton		*m_pClassButtons[TF_CLASS_MENU_BUTTONS];
	vgui::ImagePanel	*m_pMvmUpgradeImages[TF_CLASS_MENU_BUTTONS];
	CSCHintIcon			*m_pClassHintIcons[TF_CLASS_MENU_BUTTONS];

	CTFClassTipsPanel		*m_pClassTipsPanel;
	CTFPlayerModelPanel		*m_pTFPlayerModelPanel;
	CExButton				*m_pEditLoadoutButton;
	CExLabel				*m_pSelectAClassLabel;
	CExplanationPopup		*m_pClassHighlightPanel;
	CSCHintIcon				*m_pEditLoadoutHintIcon;
	CSCHintIcon				*m_pCancelHintIcon;

private:

	void CheckMvMUpgrades();

#ifdef _X360
	CTFFooter		*m_pFooter;
#endif

	ButtonCode_t	m_iClassMenuKey;
	int				m_iCurrentClassIndex;
	vgui::CKeyRepeatHandler	m_KeyRepeat;

	int				m_nBaseMusicGuid;

#ifndef _X360
	CTFImagePanel *m_ClassCountImages[CLASS_COUNT_IMAGES];
	CExLabel *m_pCountLabel;
	CTFImagePanel *m_pLocalPlayerImage;
	CTFImagePanel *m_pLocalPlayerBG;
	int m_iLocalPlayerClass;
#endif
};

//-----------------------------------------------------------------------------
// Purpose: Draws the blue class menu
//-----------------------------------------------------------------------------

class CTFClassMenu_Blue : public CTFClassMenu
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassMenu_Blue, CTFClassMenu );

public:
	CTFClassMenu_Blue( IViewPort *pViewPort ) : BaseClass( pViewPort ) {}

	virtual const char *GetName( void ) { return PANEL_CLASS_BLUE; }
	virtual int GetTeamNumber( void ) { return TF_TEAM_BLUE; }
	virtual void UpdateClassCounts( void ){ UpdateNumClassLabels( TF_TEAM_BLUE ); }
};

//-----------------------------------------------------------------------------
// Purpose: Draws the red class menu
//-----------------------------------------------------------------------------

class CTFClassMenu_Red : public CTFClassMenu
{
private:
	DECLARE_CLASS_SIMPLE( CTFClassMenu_Red, CTFClassMenu );

public:
	CTFClassMenu_Red( IViewPort *pViewPort ) : BaseClass( pViewPort ) {}

	virtual const char *GetName( void ) { return PANEL_CLASS_RED; } 
	virtual int GetTeamNumber( void ) { return TF_TEAM_RED; }
	virtual void UpdateClassCounts( void ){ UpdateNumClassLabels( TF_TEAM_RED ); }
};

#endif // TF_CLASSMENU_H


//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#if defined( REPLAY_ENABLED )

#ifndef REPLAYBROWSER_MAIN_PANEL_H
#define REPLAYBROWSER_MAIN_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/PropertyDialog.h"
#include "replay/replayhandle.h"
#include "GameEventListener.h"
#include "replaybrowseritemmanager.h"

//-----------------------------------------------------------------------------

class CReplayBrowserBasePage;
class CConfirmDeleteDialog;
class CExButton;

//-----------------------------------------------------------------------------

class CReplayBrowserPanel : public vgui::PropertyDialog,
							public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CReplayBrowserPanel, vgui::PropertyDialog );
public:
	CReplayBrowserPanel( Panel *parent );
	virtual ~CReplayBrowserPanel();

	void			OnSaveReplay( ReplayHandle_t hNewReplay );
	void			OnDeleteReplay( ReplayHandle_t hDeletedReplay );
	
	void			DeleteReplay( ReplayHandle_t hReplay );

	virtual void	CleanupUIForReplayItem( ReplayItemHandle_t hReplay );	// After a replay has been deleted - deletes all UI (thumbnail, but maybe also row and/or collection as well)

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void	PerformLayout( void );
	virtual void	OnCommand( const char *command );
	virtual void	ShowPanel( bool bShow, ReplayHandle_t hReplayDetails = REPLAY_HANDLE_INVALID, int iPerformance = -1 );
	virtual void	OnKeyCodeTyped(vgui::KeyCode code);
	virtual void	OnKeyCodePressed(vgui::KeyCode code);

	virtual void	FireGameEvent( IGameEvent *event );

	MESSAGE_FUNC_PARAMS( OnConfirmDelete, "ConfirmDlgResult", data );

	void			AttemptToDeleteReplayItem( Panel *pHandler, ReplayItemHandle_t hReplayItem, IReplayItemManager *pItemManager, int iPerformance );

	CReplayBrowserBasePage		*m_pReplaysPage;
	CConfirmDeleteDialog		*m_pConfirmDeleteDialog;

	struct DeleteInfo_t
	{
		ReplayItemHandle_t	m_hReplayItem;
		IReplayItemManager	*m_pItemManager;
		vgui::VPANEL		m_hHandler;
		int					m_iPerformance;
	};

	DeleteInfo_t			m_DeleteInfo;

	float GetTimeOpened( void ){ return m_flTimeOpened; }

private:
	void ShowDeleteReplayDenialDlg();
	void ConfirmReplayItemDelete( Panel *pHandler, ReplayItemHandle_t hReplayItem, IReplayItemManager *pItemManager, int iPerformance );

	float					m_flTimeOpened;
};

//-----------------------------------------------------------------------------

CReplayBrowserPanel		*ReplayUI_GetBrowserPanel();
void					ReplayUI_ReloadBrowser( ReplayHandle_t hReplay = REPLAY_HANDLE_INVALID, int iPerformance = -1 );
void					ReplayUI_CloseReplayBrowser();

//-----------------------------------------------------------------------------

#endif // REPLAYBROWSER_MAIN_PANEL_H

#endif
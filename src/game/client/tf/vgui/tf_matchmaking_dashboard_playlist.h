//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_PLAYLIST_H
#define TF_MATCHMAKING_DASHBOARD_PLAYLIST_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"
#include <vgui_controls/PHandle.h>
#include "local_steam_shared_object_listener.h"
#include "tf_matchmaking_dashboard_side_panel.h"

// TODO: Main menu hints need to be moved to this or the dashboard for things like
//		"Hey you should try training", and "Hey go buy stuff in the store"

class CMatchMakingDashboardSidePanel* GetPlayListPanel( bool bRecreate );

class CPlayListEntry : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CPlayListEntry, vgui::EditablePanel );
public:

	CPlayListEntry( Panel* pParent, const char* pszName );
	virtual ~CPlayListEntry();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnTick() OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	virtual void UpdateDisabledState();
	virtual void OnPlaylistActive() {};

protected:

	CUtlString m_strImageName;
	CUtlString m_strButtonCommand;
	CUtlString m_strButtonToken;
	CUtlString m_strDescToken;
	ETFMatchGroup m_eMatchGroup = k_eTFMatchGroup_Invalid;

	void SetEnabled();
	void SetDisabled( bool bUsersCanAccess, const char* pszImage, const char* pszCommand, const wchar_t* pszTooltipLocalized );

private:

	void UpdateBannedState();

	struct DisabledStateDesc_t
	{
		const char* m_pszLocToken;
		const char* m_pszButtonCommand;
		const char* m_pszImageName;
	};

	
	bool m_bDisabled;

	EditablePanel* m_pToolTipHack;
	EditablePanel* m_pToolTipButtonHack;
	CExImageButton* m_pDisabledIcon;
	CExButton* m_pModeButton;
	
};

class CEventPlayListEntry : public CPlayListEntry
						  , public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CEventPlayListEntry, CPlayListEntry );
public:
	CEventPlayListEntry( Panel* pParent, const char* pszName );

	virtual void PerformLayout() OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	void UpdateEventMatchGroup();

	virtual void UpdateDisabledState() OVERRIDE;
	virtual void OnPlaylistActive() OVERRIDE;

private:
	void UpdateExpireLabel();

	bool m_bFlashedOnce = false;
	RTime32 m_rtExpireTime = 0;
};

//-----------------------------------------------------------------------------
// Purpose: Matchmaking panel that contains controls for matchmaking
//-----------------------------------------------------------------------------
class CTFPlaylistPanel : public vgui::EditablePanel
					   , public CLocalSteamSharedObjectListener
					   , public CGameEventListener
						
{
public:
	DECLARE_CLASS_SIMPLE( CTFPlaylistPanel, vgui::EditablePanel );
	CTFPlaylistPanel( Panel *parent, const char *panelName );
	virtual ~CTFPlaylistPanel();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }
	virtual void SOUpdated( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }
	virtual void SODestroyed( const CSteamID & steamIDOwner, const GCSDK::CSharedObject *pObject, GCSDK::ESOCacheEvent eEvent ) OVERRIDE { SOEvent( pObject ); }

	virtual void FireGameEvent( IGameEvent* event ) OVERRIDE;

	void UpdatePlaylistEntries( void );

	MESSAGE_FUNC( SidePanelActive, "SidePanelActive" );
private:
	void UpdateEventStatus();
	void SOEvent( const GCSDK::CSharedObject* pObject );

	CPlayListEntry* m_pCasual = NULL;
	CPlayListEntry* m_pCompetitive = NULL;
	CPlayListEntry* m_pMvM = NULL;
	CEventPlayListEntry* m_pEvent = NULL;
	bool m_bLastSawEventActive = false;
};

class CTFDashboardPlaylistPanel : public CMatchMakingDashboardSidePanel,
								  public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFDashboardPlaylistPanel, CMatchMakingDashboardSidePanel);
	CTFDashboardPlaylistPanel( Panel *parent, const char *panelName );
	virtual ~CTFDashboardPlaylistPanel();

	virtual void AddActionSignalTarget( Panel *messageTarget ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;
private:
	CTFPlaylistPanel* m_pPlayList;
};

#endif // TF_MATCHMAKING_DASHBOARD_PLAYLIST_H

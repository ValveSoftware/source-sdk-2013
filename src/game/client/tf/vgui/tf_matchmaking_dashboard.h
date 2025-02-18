//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCHMAKING_DASHBOARD_H
#define TF_MATCHMAKING_DASHBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "tf_controls.h"
#include <vgui_controls/PHandle.h>
#include "local_steam_shared_object_listener.h"
#include "tf_matchmaking_dashboard_side_panel.h"
#include "vgui_controls/PHandle.h"
#include "econ_notifications.h"
#include "tf_party.h"
#include "tf_matchmaking_dashboard_notification.h"
#include "tf_matchmaking_party_invite_notification.h"

namespace vgui
{
	class Menu;
}

class CMatchMakingDashboardSidePanel;

enum EMMTooltips
{
	k_eSmallFont = 0,
	k_eMediumFont,
	k_eLargeFont,
	k_eTooltipsCount
};

class CTFMatchmakingDashboard* GetMMDashboard();
CTFTextToolTip* GetDashboardTooltip( EMMTooltips eTipType );
extern void PromptOrFireCommand( const char* pszCommand );


enum EMMDashboadSidePanel
{
	k_ePlayList = 0,
	k_eCasual,
	k_eCompetitive,
	k_eMvM_Mode_Select,
	k_eMvM_Mode_Configure,
	k_eChat,
	k_eBGDimmer,
	k_eMMSettings,
	k_eExplanations,
	k_eNextMapWinnerPopup,
	k_eNextMapVotePopup,
	k_eToolTipSmallFont,
	k_eToolTipMediumFont,
	k_eToolTipLargeFont,
	k_eCompAccess,
	k_eEventMatch,
	k_eToolTipCompRanks,
	k_ePanelCount,
};


//-----------------------------------------------------------------------------
// Purpose: Owns and holds all of the dashboard singleton panels
//-----------------------------------------------------------------------------
class CDashboardSingletonManager
{
public:
	CDashboardSingletonManager();

	typedef vgui::Panel* (*pFnPanelCreationFunc)();
	void RegisterFnForSingleton( EMMDashboadSidePanel ePanel, pFnPanelCreationFunc );

	vgui::Panel* GetPanel( EMMDashboadSidePanel ePanel );
	template< typename T >
	T* GetTypedPanel( EMMDashboadSidePanel ePanel ) 
	{
		return assert_cast< T* >( GetPanel( ePanel ) );
	}

	void RecreateAll();

private:

	pFnPanelCreationFunc m_pFnCreation[ k_ePanelCount ];
	vgui::Panel* m_pPanels[ k_ePanelCount ];
};

CDashboardSingletonManager& GetDashboardPanel();

// Used to register a functiom for the creation of a dashboard panel
class CDashboardSingletonPanelCreationFunc
{
public:
	CDashboardSingletonPanelCreationFunc( CDashboardSingletonManager::pFnPanelCreationFunc func, EMMDashboadSidePanel eType ) { GetDashboardPanel().RegisterFnForSingleton( eType, func ); }
};
#define REGISTER_FUNC_FOR_DASHBOARD_PANEL_TYPE( func, type ) CDashboardSingletonPanelCreationFunc g_##type##CreationFunc( func, type );

//-----------------------------------------------------------------------------
// Purpose: Matchmaking panel that contains controls for matchmaking
//-----------------------------------------------------------------------------
class CTFMatchmakingDashboard : public CExpandablePanel
							  , public CGameEventListener
{
public:
	DECLARE_CLASS_SIMPLE( CTFMatchmakingDashboard, CExpandablePanel );
	CTFMatchmakingDashboard();
	virtual ~CTFMatchmakingDashboard();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnTick() OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	const Color& GetPartyMemberColor( int nSlot ) const;

	vgui::Menu* ClearAndGetDashboardContextMenu();
	void Reload();

	bool BAnySidePanelsShowing() const;
	bool BIsSidePanelShowing( const CMatchMakingDashboardSidePanel* pSidePanel ) const;

	MESSAGE_FUNC( OnPlayCompetitive, "PlayCompetitive" );
	MESSAGE_FUNC( OnPlayCasual, "PlayCasual" );
	MESSAGE_FUNC( OnPlayMvM, "PlayMvM" );
	MESSAGE_FUNC( OnPlayMvM_MannUp, "PlayMvM_MannUp" );
	MESSAGE_FUNC( OnPlayMvM_BootCamp, "PlayMvM_BootCamp" );
	MESSAGE_FUNC( OnPlayTraining, "PlayTraining" );
	MESSAGE_FUNC( OnPlayCommunity, "PlayCommunity" );
	MESSAGE_FUNC( OnCreateServer, "CreateServer" );
	MESSAGE_FUNC( OnPlayEvent, "PlayEvent" );
	MESSAGE_FUNC( OnShowCompAccess, "ShowCompAccess" );
	MESSAGE_FUNC( OnViewMatchSettings, "ViewMatchSettings" );
	MESSAGE_FUNC_PARAMS( OnCloseSideStack, "CloseSideStack", pParams );
	MESSAGE_FUNC_PTR( OnNavigateSideStack, "NavigateSideStack", panel );

	// Context menu actions
	MESSAGE_FUNC( OnLeaveParty, "Context_LeaveParty" );
	MESSAGE_FUNC( OnOpenSettings, "Context_OpenSettings" );

private:

	// Notifications
	MESSAGE_FUNC_PTR( OnNotificationCreated, "NotificationCreated", panel );
	MESSAGE_FUNC_PTR( OnNotificationCleared, "NotificationCleared", panel );
	void PositionNotifications();

	// Side panels
	void PushSlidePanel( CMatchMakingDashboardSidePanel* pPanel );
	void PopStack( int nLevels, EStackSide_t eSide );
	void ClearAllStacks();
	void RepositionSidePanels( EStackSide_t eSide );
	void OpenPartyOptionsMenu();

	// Queue panel
	void UpdateQueuePanel();
	void UpdateJoinPartyLobbyPanel();

	void UpdateFindAGameButton();
	void UpdateDisconnectAndResume();
	void UpdateDimmer();

	// Party invites
	void UpdatePartyInvites();

	CUtlVector< vgui::DHANDLE< CMatchMakingDashboardSidePanel > >& GetStackForSide( EStackSide_t eSide ) { return m_vecSideSlideStack[ eSide ]; }

	vgui::EditablePanel* m_pTopBar;
	vgui::EditablePanel* m_pQueuePanel;
	vgui::EditablePanel* m_pJoinPartyLobbyPanel;
	CExImageButton* m_pQuitButton;
	CExImageButton* m_pDisconnectButton;
	CExImageButton* m_pPlayButton;
	CExImageButton* m_pResumeButton;
	vgui::Menu* m_pContextMenu = NULL;
	CUtlVector< vgui::DHANDLE< CInviteNotification > > m_vecInviteHandles;

	CUtlVector< vgui::DHANDLE< CMatchMakingDashboardSidePanel > > m_vecSideSlideStack[ 2 ]; // A left and right
	CUtlVector< vgui::DHANDLE< CTFDashboardNotification > > m_vecNotifications;

	Color m_colorPartyMembers[ MAX_PARTY_SIZE ];
};

#endif // TF_MATCHMAKING_DASHBOARD_H

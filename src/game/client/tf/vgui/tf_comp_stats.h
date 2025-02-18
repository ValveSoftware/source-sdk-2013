//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//
#ifndef TF_COMP_STATS_H
#define TF_COMP_STATS_H

#include "cbase.h"
#include "game/client/iviewport.h"
#include "tf_leaderboardpanel.h"
#include "local_steam_shared_object_listener.h"
#include "tf_controls.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace GCSDK;

class CBaseLobbyPanel;

namespace vgui
{
	class ScrollableEditablePanel;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CLadderLobbyLeaderboard : public CTFLeaderboardPanel
							  , public CLocalSteamSharedObjectListener
{
	DECLARE_CLASS_SIMPLE( CLadderLobbyLeaderboard, CTFLeaderboardPanel );
public:

	CLadderLobbyLeaderboard( Panel *pParent, const char *pszPanelName );
	virtual ~CLadderLobbyLeaderboard();

	//-----------------------------------------------------------------------------
	// Purpose: Create leaderboard panels
	//-----------------------------------------------------------------------------
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void OnThink() OVERRIDE;

	virtual bool GetLeaderboardData( CUtlVector< LeaderboardEntry_t* >& scores );
	virtual bool UpdateLeaderboards();

	bool IsDataValid( void ) { return m_bIsDataValid; }

	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;

private:
	const char *m_pszLeaderboardName;
	bool m_bIsDataValid;
	bool m_bDataDirty = true;

	vgui::ScrollableEditablePanel *m_pScoreListScroller;
	EditablePanel *m_pScoreList;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCompStatsPanel : public EditablePanel
					  , public CLocalSteamSharedObjectListener
					  , public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CCompStatsPanel, EditablePanel );

public:
	CCompStatsPanel( vgui::Panel *pParent, const char* pszName );
	virtual ~CCompStatsPanel();

	//
	// Panel overrides
	//
	virtual void PerformLayout() OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	virtual void SOCreated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;
	virtual void SOUpdated( const CSteamID & steamIDOwner, const CSharedObject *pObject, ESOCacheEvent eEvent ) OVERRIDE;

	virtual void OnThink() OVERRIDE;

	void WriteControls();

	//
	// CGameEventListener overrides
	//
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

private:
	
	CPanelAnimationVarAliasType( int, m_iStatMedalWidth, "stat_medal_width", "14", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMedalCountWidth, "stat_medal_count_width", "20", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iHasPassWidth, "has_pass_width", "12", "proportional_int" );

	CUtlVector<vgui::Label *> m_vecSearchCriteriaLabels;

	// leaderboards
	CLadderLobbyLeaderboard *m_pCompetitiveModeLeaderboard;

	vgui::HFont m_fontMedalsCount;

	enum EMatchHistorySortMethods_t
	{
		SORT_BY_RESULT = 0,
		SORT_BY_DATE,
		SORT_BY_MAP,
		SORT_BY_KDR,

		NUM_SORT_METHODS
	};

	CScrollableList* m_pMatchHistoryScroller;
	EMatchHistorySortMethods_t m_eMatchSortMethod;
	bool m_bDescendingMatchHistorySort;

	float m_flCompetitiveRankProgress;
	float m_flCompetitiveRankPrevProgress;
	float m_flRefreshPlayerListTime;
	bool m_bCompetitiveRankChangePlayedSound;
	bool m_bMatchHistoryLoaded;
	int m_iWritingPanel = 0;

	void UpdateMatchDataForLocalPlayer();
	bool m_bMatchDataForLocalPlayerDirty;
};

#endif //TF_COMP_STATS_H

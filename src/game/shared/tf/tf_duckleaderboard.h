//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

#ifndef TF_DUCKLEADERBOARD_H
#define TF_DUCKLEADERBOARD_H
#ifdef _WIN32
#pragma once
#endif

#ifdef CLIENT_DLL
	#include "tf_mapinfo.h"
	#include "tf_leaderboardpanel.h"
	#include "tf_controls.h"
#endif

extern const char* g_szDuckLeaderboardNames[];

#define	TF_DUCK_ID		"DUCK_ID"
#define DUCK_XP_SCALE					5000
#define DUCK_XP_WEIGHT_GENERATION		3
#define DUCK_XP_WEIGHT_OFFENSE			3
#define DUCK_XP_WEIGHT_DEFENSE			1
#define DUCK_XP_WEIGHT_OBJECTIVE		3
#define DUCK_XP_WEIGHT_TEAMMATE			3
#define DUCK_XP_WEIGHT_BONUS			50			


enum EDuckLeaderboardTypes
{
	TF_DUCK_SCORING_OVERALL_RATING = 0,
	TF_DUCK_SCORING_PERSONAL_GENERATION,
	TF_DUCK_SCORING_PERSONAL_PICKUP_OFFENSE,
	TF_DUCK_SCORING_PERSONAL_PICKUP_DEFENDED,
	TF_DUCK_SCORING_PERSONAL_PICKUP_OBJECTIVE,
	TF_DUCK_SCORING_TEAM_PICKUP_MY_DUCKS,
	TF_DUCK_SCORING_PERSONAL_BONUS_PICKUP,
	DUCK_NUM_LEADERBOARDS
};

enum EDuckEventTypes
{
	DUCK_CREATED = 1,
	DUCK_COLLECTED,
};

enum EDuckFlags
{
	DUCK_FLAG_OBJECTIVE	= 1 << 0,
	DUCK_FLAG_BONUS		= 1 << 1
};

#ifdef CLIENT_DLL

class CDucksLeaderboard : public CTFLeaderboardPanel
{
	DECLARE_CLASS_SIMPLE( CDucksLeaderboard, CTFLeaderboardPanel );
public:
	CDucksLeaderboard( Panel *parent, const char *panelName, const char *pszDuckLeaderboardname );
	virtual ~CDucksLeaderboard();

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
private:
	virtual bool UpdateLeaderboards() OVERRIDE;
	virtual bool GetLeaderboardData( CUtlVector< LeaderboardEntry_t* >& scores ) OVERRIDE;

	const char		*m_pszDuckLeaderboardName;
	CTFTextToolTip	*m_pToolTip;
	vgui::EditablePanel				*m_pToolTipEmbeddedPanel;
};


class CDucksLeaderboardManager : public EditablePanel, CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CDucksLeaderboardManager, EditablePanel );
public:
	CDucksLeaderboardManager( Panel *parent, const char *panelName );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	virtual void OnThink();
private:

	void NextPage();
	void PrevPage();
	void ShowPage( int nPage );

	int m_nCurrentPage;
	CUtlVector< EditablePanel* > m_vecLeaderboards;

	CTFTextToolTip					*m_pToolTip;
	vgui::EditablePanel				*m_pToolTipEmbeddedPanel;
	vgui::EditablePanel				*m_pDimmer;
	float							m_flFadeStartTime;

	CPanelAnimationVarAliasType( int, m_iScoreStep, "score_step", "0", "proportional_int" );
};

#endif

#endif // TF_DUCKLEADERBOARD_H

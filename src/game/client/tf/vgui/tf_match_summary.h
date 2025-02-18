//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MATCH_SUMMARY_H
#define TF_MATCH_SUMMARY_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/SectionedListPanel.h>
#include "tf_imagepanel.h"
#include "econ_controls.h"
#include "drawing_panel.h"
#include "tf_particlepanel.h"
#include "tf_matchmaking_shared.h"
#include "tf_gamerules.h"

using namespace vgui;

class CAvatarImagePanel;
class CTFBadgePanel;

enum matchsummary_displaystate_t
{
	MS_STATE_INITIAL = 0,
	MS_STATE_DRAWING,
	MS_STATE_STATS,
	MS_STATE_BRONZE_MEDALS,
	MS_STATE_SILVER_MEDALS,
	MS_STATE_GOLD_MEDALS,
	MS_STATE_FINAL,

	MS_NUM_STATES
};

enum matchsummary_columns_t
{
	MS_COLUMN_INVALID = -1,

	MS_COLUMN_MEDAL = 0,
	MS_COLUMN_AVATAR,
	MS_COLUMN_SPACER,
	MS_COLUMN_NAME,
	MS_COLUMN_CLASS,
	MS_COLUMN_SCORE,
	MS_COLUMN_SCORE_MEDAL,
	MS_COLUMN_KILLS,
	MS_COLUMN_KILLS_MEDAL,
	MS_COLUMN_DAMAGE,
	MS_COLUMN_DAMAGE_MEDAL,
	MS_COLUMN_HEALING,
	MS_COLUMN_HEALING_MEDAL,
	MS_COLUMN_SUPPORT,
	MS_COLUMN_SUPPORT_MEDAL,

	MS_NUM_COLUMNS
};

class TFSectionedListPanel : public vgui::SectionedListPanel
{
private:
	DECLARE_CLASS_SIMPLE( TFSectionedListPanel, vgui::SectionedListPanel );

public:
	TFSectionedListPanel( Panel *parent, const char *panelName ) : BaseClass( parent, panelName ){}
	virtual ~TFSectionedListPanel(){}

	CPanelAnimationVarAliasType( int, m_iMedalWidth,		"medal_width",		"s.05",	"proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAvatarWidth,		"avatar_width",		"s.1",	"proportional_width" );		// Avatar width doesn't scale with resolution
	CPanelAnimationVarAliasType( int, m_iSpacerWidth,		"spacer",			"s.1",	"proportional_width" );
	CPanelAnimationVarAliasType( int, m_iNameWidth,			"name_width",		"s.1",	"proportional_width" );
	CPanelAnimationVarAliasType( int, m_iClassWidth,		"class_width",		"s.1",	"proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAwardWidth,		"award_width",		"s.1",	"proportional_width" );
	CPanelAnimationVarAliasType( int, m_iStatsWidth,		"stats_width",		"s.1",	"proportional_width" );
	CPanelAnimationVarAliasType( int, m_iHorizFillInset,	"horiz_inset",		"5",	"proportional_int" );
};
																				   
class CTFMatchSummary : public CHudElement, public vgui::EditablePanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFMatchSummary, vgui::EditablePanel );

public:
	CTFMatchSummary( const char *pElementName );
	virtual ~CTFMatchSummary();

	virtual bool ShouldDraw( void ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void SetVisible( bool state ) OVERRIDE;
	virtual void OnTick() OVERRIDE;
	virtual void LevelInit( void ) OVERRIDE;
	virtual void LevelShutdown( void ) OVERRIDE;

	virtual GameActionSet_t GetPreferredActionSet() OVERRIDE { return IsActive() ? GAME_ACTION_SET_MENUCONTROLS : GAME_ACTION_SET_NONE; }
	virtual void FireGameEvent( IGameEvent *event ) OVERRIDE;

	bool ShowPerformanceMedals( void );
	
private:

	void Update( void );
	void InitPlayerList( TFSectionedListPanel *pPlayerList, int nTeam );
	static bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );
	void UpdateTeamInfo();
	void UpdatePlayerList();
	void UpdatePlayerAvatar( int playerIndex, KeyValues *kv );
	void RecalculateMedalCounts();
	void UpdateBadgePanels( CUtlVector<CTFBadgePanel*> &pBadgePanels, TFSectionedListPanel *pPlayerList );

	void InternalUpdateMedalCountForType( int iTeam, StatMedal_t eMedal );
	matchsummary_columns_t InternalAddMedalKeyValues( int iIndex, StatMedal_t eMedal, KeyValues *pKeyValues, int nTotalMedals = -1 );
	void FireMedalEffects( Panel *pPanel, int nPanelXPos, int nPanelYPos, int nPanelWide, int nPanelTall, StatMedal_t eParticleMedal );

private:
	EditablePanel *m_pTeamScoresPanel;

	int		m_iImageClass[SCOREBOARD_CLASS_ICONS];
	int		m_iImageClassAlt[SCOREBOARD_CLASS_ICONS];
	int		m_iImageMedals[StatMedal_Max];

	CDrawingPanel				*m_pDrawingPanel;

	vgui::EditablePanel			*m_pBlueTeamPanel;
	vgui::EditablePanel			*m_pRedTeamPanel;

	vgui::EditablePanel			*m_pMainStatsContainer;
	vgui::EditablePanel			*m_pPlayerListBlueParent;
	TFSectionedListPanel		*m_pPlayerListBlue;
	vgui::EditablePanel			*m_pPlayerListRedParent;
	TFSectionedListPanel		*m_pPlayerListRed;
	CExLabel					*m_pBlueTeamScore;
	CExLabel					*m_pBlueTeamScoreDropshadow;
	EditablePanel				*m_pBlueTeamScoreBG;
	EditablePanel				*m_pBluePlayerListBG;
	CExLabel					*m_pRedTeamScore;
	CExLabel					*m_pRedTeamScoreDropshadow;
	EditablePanel				*m_pRedTeamScoreBG;
	EditablePanel				*m_pRedPlayerListBG;
	EditablePanel				*m_pBlueMedalsPanel;
	EditablePanel				*m_pRedMedalsPanel;
	vgui::ImagePanel			*m_pRedTeamImage;
	vgui::ImagePanel			*m_pBlueTeamImage;
	CAvatarImagePanel			*m_pRedLeaderAvatarImage;
	CAvatarImagePanel			*m_pBlueLeaderAvatarImage;
	EditablePanel				*m_pRedLeaderAvatarBG;
	EditablePanel				*m_pBlueLeaderAvatarBG;
	EditablePanel				*m_pStatsLabelPanel;
	CExLabel					*m_pStatsAndMedals;
	CExLabel					*m_pStatsAndMedalsShadow;
	CExLabel					*m_pBlueTeamName;
	CExLabel					*m_pRedTeamName;
  	CExLabel					*m_pRedTeamWinner;
	CExLabel					*m_pRedTeamWinnerDropshadow;
	CExLabel					*m_pBlueTeamWinner;
	CExLabel					*m_pBlueTeamWinnerDropshadow;

	CTFParticlePanel			*m_pParticlePanel;

	vgui::EditablePanel			*m_pStatsBgPanel;

	vgui::ImageList				*m_pImageList;
	CUtlMap<CSteamID,int>		m_mapAvatarsToImageList;

	vgui::HFont					m_hFont;
 	bool						m_bLargeMatchGroup;
	bool						m_bXPShown;

	float						m_flDrawingPanelTime;

	CPanelAnimationVar( Color, m_clrGoldMedal, "GoldMedalText", "214 186 24 255" );
	CPanelAnimationVar( Color, m_clrSilverMedal, "SilverMedalText", "222 218 222 255" );
	CPanelAnimationVar( Color, m_clrBronzeMedal, "BronzeMedalText", "214 125 57 255" );

	CPanelAnimationVarAliasType( int, m_iAnimBluePlayerListParent, "AnimBluePlayerListParent", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimBlueTeamScore, "AnimBlueTeamScore", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimBlueTeamScoreDropshadow, "AnimBlueTeamScoreDropshadow", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimBlueTeamScoreBG, "AnimBlueTeamScoreBG", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimBluePlayerListBG, "AnimBluePlayerListBG", "0", "proportional_width" );

	CPanelAnimationVarAliasType( int, m_iAnimRedTeamScoreBGWide, "AnimRedTeamScoreBGWide", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimRedTeamScoreBGXPos, "AnimRedTeamScoreBGXPos", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iAnimRedTeamScoreWide, "AnimRedTeamScoreWide", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimRedTeamScoreXPos, "AnimRedTeamScoreXPos", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iAnimRedTeamScoreDropshadowWide, "AnimRedTeamScoreDropshadowWide", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimRedTeamScoreDropshadowXPos, "AnimRedTeamScoreDropshadowXPos", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iAnimRedPlayerListParentWide, "AnimRedPlayerListParentWide", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimRedPlayerListParentXPos, "AnimRedPlayerListParentXPos", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iAnimRedPlayerListBGWide, "AnimRedPlayerListBGWide", "0", "proportional_width" );
	CPanelAnimationVarAliasType( int, m_iAnimRedPlayerListBGXPos, "AnimRedPlayerListBGXPos", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_iAnimBlueMedalsYPos, "AnimBlueMedalsYPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimRedMedalsYPos, "AnimRedMedalsYPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimStatsLabelPanel6v6YPos, "AnimStatsLabelPanel6v6YPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimBlueTeamLabel6v6YPos, "AnimBlueTeamLabel6v6YPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimRedTeamLabel6v6YPos, "AnimRedTeamLabel6v6YPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimStatsLabelPanel12v12YPos, "AnimStatsLabelPanel12v12YPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimBlueTeamLabel12v12YPos, "AnimBlueTeamLabel12v12YPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimRedTeamLabel12v12YPos, "AnimRedTeamLabel12v12YPos", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_iAnimStatsContainer12v12YPos, "AnimStatsContainer12v12YPos", "0", "proportional_ypos" );

	struct MatchDataUpdate_t
	{
		int nScoreRank;
		int nKillsRank;
		int nDamageRank;
		int nHealingRank;
		int nSupportRank;
	};
	MatchDataUpdate_t m_SkillRatings[MAX_PLAYERS_ARRAY_SAFE];

	int m_iCurrentState;
	float m_flNextActionTime;

	int m_nMedalsToAward_Bronze_Blue;
	int m_nMedalsToAward_Silver_Blue;
	int m_nMedalsToAward_Gold_Blue;
	int m_nMedalsToAward_Bronze_Red;
	int m_nMedalsToAward_Silver_Red;
	int m_nMedalsToAward_Gold_Red;

	int m_nMedalsRevealed;

	int m_nNumMedalsThisUpdate;

	bool m_bBlueGoldValueRevealed;
	bool m_bBlueSilverValueRevealed;
	bool m_bBlueBronzeValueRevealed;
	bool m_bRedGoldValueRevealed;
	bool m_bRedSilverValueRevealed;
	bool m_bRedBronzeValueRevealed;
	bool m_bPlayerAbandoned;

	float m_flMedalSoundTime;

	CUtlVector< CTFBadgePanel* > m_pBlueBadgePanels;
	CUtlVector< CTFBadgePanel* > m_pRedBadgePanels;
};

#endif //TF_MATCH_SUMMARY_H

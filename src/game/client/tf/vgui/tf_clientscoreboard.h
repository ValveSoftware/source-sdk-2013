//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_SCOREBOARD_H
#define TF_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif

#include "hud.h"
#include "hudelement.h"
#include "tf_hud_playerstatus.h"
#include "clientscoreboarddialog.h"
#include "tf_hud_mann_vs_machine_scoreboard.h"

class CAvatarImagePanel;
class CTFBadgePanel;
//class CTFStatsGraph;

//-----------------------------------------------------------------------------
// Purpose: displays the scoreboard
//-----------------------------------------------------------------------------

class CTFClientScoreBoardDialog : public CClientScoreBoardDialog
{
private:
	DECLARE_CLASS_SIMPLE( CTFClientScoreBoardDialog, CClientScoreBoardDialog );

public:
	CTFClientScoreBoardDialog( IViewPort *pViewPort );
	virtual ~CTFClientScoreBoardDialog();

	virtual void Reset() OVERRIDE;
	virtual void Update() OVERRIDE;
	virtual void ShowPanel( bool bShow ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding );

	struct duel_panel_t
	{
		vgui::EditablePanel *m_pPanel;
		CAvatarImagePanel	*m_pAvatar;
		CExLabel			*m_pPlayerNameLabel;
	};
	
	MESSAGE_FUNC_PTR( OnItemSelected, "ItemSelected", panel );
	MESSAGE_FUNC_PTR( OnItemContextMenu, "ItemContextMenu", panel );
	void OnScoreBoardMouseRightRelease( void );


	MESSAGE_FUNC_PARAMS( OnVoteKickPlayer, "VoteKickPlayer", pData );

protected:
	virtual void PerformLayout();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void PostApplySchemeSettings( vgui::IScheme *pScheme ) {};

	vgui::SectionedListPanel *GetPlayerListRed( void ){ return m_pPlayerListRed; }
	vgui::SectionedListPanel *GetPlayerListBlue( void ){ return m_pPlayerListBlue; }

private:
	void InitPlayerList( vgui::SectionedListPanel *pPlayerList );
	void SetPlayerListImages( vgui::SectionedListPanel *pPlayerList );
	void UpdateTeamInfo();
	void UpdatePlayerList();
	void UpdateSpectatorList();
	void UpdatePlayerDetails();
	void UpdateServerTimeLeft();
	void UpdateArenaWaitingToPlayList( void );
	void ClearPlayerDetails();
	bool ShouldShowAsSpectator( int iPlayerIndex );
	bool ShouldShowAsArenaWaitingToPlay( int iPlayerIndex );
	void GetCameraUnderlayBounds( int *pX, int *pY, int *pWide, int *pTall );
	bool UseMouseMode( void );
	void InitializeInputScheme( void );

	void AdjustForVisibleScrollbar( void );
	void UpdateBadgePanels( CUtlVector<CTFBadgePanel*> &pBadgePanels, vgui::SectionedListPanel *pPlayerList );

	virtual void FireGameEvent( IGameEvent *event );

	static bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );

	vgui::SectionedListPanel *GetSelectedPlayerList( void );

	void UpdatePlayerModel();

	vgui::SectionedListPanel	*m_pPlayerListBlue;
	vgui::SectionedListPanel	*m_pPlayerListRed;
	CExLabel					*m_pLabelPlayerName;
	CExLabel					*m_pLabelDuelOpponentPlayerName;
	vgui::ImagePanel			*m_pImagePanelHorizLine;
	CTFClassImage				*m_pClassImage;
	vgui::EditablePanel			*m_pLocalPlayerStatsPanel;
	vgui::EditablePanel			*m_pLocalPlayerDuelStatsPanel;
	duel_panel_t			    m_duelPanelLocalPlayer;
	duel_panel_t			    m_duelPanelOpponent;
	vgui::Menu					*m_pRightClickMenu;

	CExLabel					*m_pKillsLabel;
	CExLabel					*m_pDeathsLabel;
	CExLabel					*m_pAssistLabel;
	CExLabel					*m_pDestructionLabel;
	CExLabel					*m_pCapturesLabel;
	CExLabel					*m_pDefensesLabel;
	CExLabel					*m_pDominationsLabel;
	CExLabel					*m_pRevengeLabel;
	CExLabel					*m_pHealingLabel;
	CExLabel					*m_pInvulnsLabel;
	CExLabel					*m_pTeleportsLabel;
	CExLabel					*m_pHeadshotsLabel;
	CExLabel					*m_pBackstabsLabel;
	CExLabel					*m_pBonusLabel;
	CExLabel					*m_pSupportLabel;
	CExLabel					*m_pDamageLabel;

	CExLabel					*m_pServerTimeLeftValue;
	vgui::HFont					m_pFontTimeLeftNumbers;
	vgui::HFont					m_pFontTimeLeftString;

	CTFHudMannVsMachineScoreboard *m_pMvMScoreboard;
	
	int							m_iImageDominated;
	int							m_iImageDominatedDead;
	int							m_iImageNemesis;
	int							m_iImageNemesisDead;
	int							m_iImageStreak;
	int							m_iImageStreakDead;

	int							m_iImageDom[SCOREBOARD_DOMINATION_ICONS];
	int							m_iImageDomDead[SCOREBOARD_DOMINATION_ICONS];
	int							m_iImageClass[SCOREBOARD_CLASS_ICONS];
	int							m_iImageClassAlt[SCOREBOARD_CLASS_ICONS];

	int							m_iImagePing[SCOREBOARD_PING_ICONS];
	int							m_iImagePingDead[SCOREBOARD_PING_ICONS];

	int							m_iTextureCamera;

	bool						m_bIsPVEMode;
//	bool						m_bDisplayLevel;
	bool						m_bMouseActivated;
	vgui::HFont					m_hScoreFontDefault;
	vgui::HFont					m_hScoreFontSmallest;

	CPanelAnimationVarAliasType( int, m_iSpacerWidth, "spacer", "5", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iNemesisWidth, "nemesis_width", "20", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMedalWidth, "medal_width", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMedalColumnWidth, "medal_column_width", "15", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iKillstreakWidth, "killstreak_width", "20", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iKillstreakImageWidth, "killstreak_image_width", "20", "proportional_int" );

	CTFPlayerModelPanel			*m_pPlayerModelPanel;
	int							m_nPlayerModelPanelIndex;

	bool						m_bRedScrollBarVisible;
	bool						m_bBlueScrollBarVisible;
	int							m_nExtraSpace;

	CExLabel					*m_pRedTeamName;
	CExLabel					*m_pBlueTeamName;

	CAvatarImagePanel			*m_pRedLeaderAvatarImage;
	EditablePanel				*m_pRedLeaderAvatarBG;
	vgui::ImagePanel			*m_pRedTeamImage;
	CAvatarImagePanel			*m_pBlueLeaderAvatarImage;
	EditablePanel				*m_pBlueLeaderAvatarBG;
	vgui::ImagePanel			*m_pBlueTeamImage;

	CUtlVector< CTFBadgePanel* > m_pBlueBadgePanels;
	CUtlVector< CTFBadgePanel* > m_pRedBadgePanels;

	CHandle< C_TFPlayer >		m_hSelectedPlayer;
	bool						m_bUsePlayerModel;
};

const wchar_t *GetPointsString( int iPoints );

#endif // TF_SCOREBOARD_H

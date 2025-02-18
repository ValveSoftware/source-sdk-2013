//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MANN_VS_MACHINE_SCOREBOARD_H
#define TF_HUD_MANN_VS_MACHINE_SCOREBOARD_H
#ifdef _WIN32
#pragma once
#endif


#include "hudelement.h"
#include "tf_controls.h"
#include "hud.h"
#include <vgui/IScheme.h>
#include <vgui_controls/ProgressBar.h>
#include <vgui_controls/ScalableImagePanel.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ImageList.h>
#include <vgui_controls/SectionedListPanel.h>
#include <vgui/ISurface.h>
#include "c_tf_objective_resource.h"
#include "tf_gamerules.h"
#include "c_tf_playerresource.h"
#include "vgui_avatarimage.h"
#include "tf_hud_mann_vs_machine_status.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_gc_client.h"

//=========================================================
class CMvMScoreboardEnemyInfo : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMScoreboardEnemyInfo, vgui::EditablePanel );

public:
	CMvMScoreboardEnemyInfo( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );

	void UpdateEntry( char* icon, bool bIsGiant );

private:
	CPanelAnimationVar( Color, m_clrNormal, "color_normal", "TanLight" );
	CPanelAnimationVar( Color, m_clrMiniBoss, "color_miniboss", "RedSolid" );
};

//=========================================================
class CTFHudMannVsMachineScoreboard : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudMannVsMachineScoreboard, vgui::EditablePanel );

public:
	CTFHudMannVsMachineScoreboard( Panel *parent, const char *pName );
	~CTFHudMannVsMachineScoreboard();
	
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );
	virtual void OnTick( void );

	static void UpdateCreditPanel( CCreditDisplayPanel *panel, int nAcquired, int nMissed, int nBonus );

	void UpdateCreditSpend ( CCreditSpendPanel *panel, int nUpgrades, int nBuybacks, int nBottles );

	vgui::SectionedListPanel *GetPlayerList( void ){ return m_pPlayerList; }

private:
	void InitPlayerList( vgui::IScheme *pScheme );
	void UpdatePlayerList();
	void UpdatePlayerAvatar( int playerIndex, KeyValues *kv );
	void UpdateCreditStats();
	void UpdateCreditSpend();
	void UpdatePopFile();


	bool m_bInitialized;
	char m_popfile[ MAX_PATH ];

	vgui::ScalableImagePanel	*m_pPlayerListBackground;
	vgui::SectionedListPanel	*m_pPlayerList;
	vgui::ImageList				*m_pImageList;
	CUtlMap<CSteamID,int>		m_mapAvatarsToImageList;
	int							m_iImageDead;
	int							m_iImageClass[SCOREBOARD_CLASS_ICONS];
	int							m_iImageClassAlt[SCOREBOARD_CLASS_ICONS];
	int							m_iSquadSurplusTexture;

	vgui::EditablePanel *m_pDifficultyContainer;

	vgui::EditablePanel *m_pInfoContainerPanel;
	CMvMScoreboardEnemyInfo *m_pBaseEnemyPanel;
	vgui::ScalableImagePanel *m_pInfoBackgroundPanel;

	vgui::EditablePanel *m_pCreditStatsPanel;
	CCreditDisplayPanel *m_pPreviousWaveCreditsInfo;
	CCreditDisplayPanel *m_pTotalCreditsInfo;

	CCreditSpendPanel *m_pPreviousWaveCreditsSpend;
	CCreditSpendPanel *m_pTotalCreditsSpend;

	vgui::EditablePanel *m_pLocalPlayerStatsPanel;
	
	vgui::Label			*m_pRespecStatusLabel;

	int		m_iDisplayedWave;

	vgui::HFont m_hScoreFont;	

	//380
	CPanelAnimationVarAliasType( int, m_iMedalWidth, "medal_width", "20", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iMedalSpacerWidth, "medal_spacer_width", "4", "proportional_int" );
	CPanelAnimationVar( int, m_iAvatarWidth, "avatar_width", "32" );		// Avatar width doesn't scale with resolution
	CPanelAnimationVarAliasType( int, m_iSpacerWidth, "spacer_width", "2", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iNameWidth, "name_width", "94", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iStatWidth, "stat_width", "43", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iClassWidth, "class_width", "25", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iPingWidth, "ping_width", "25", "proportional_int" );
};

#endif // TF_HUD_MANN_VS_MACHINE_SCOREBOARD_H

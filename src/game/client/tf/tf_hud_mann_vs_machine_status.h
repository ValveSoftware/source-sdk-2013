//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MANN_VS_MACHINE_STATUS_H
#define TF_HUD_MANN_VS_MACHINE_STATUS_H
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
#include <vgui/ISurface.h>
#include "tf_hud_objectivestatus.h"
#include "tf_hud_mann_vs_machine_victory.h"
#include "tf_gcmessages.h"
#include "tf_hud_mann_vs_machine_stats.h"
#include "tf_hud_mann_vs_machine_loss.h"
#include "tf_mann_vs_machine_stats.h"

#define MAX_TANK_PROGRESS_BARS 5

//=========================================================
typedef struct 
{
	int		nCount;
	const char	*pchClassIconName;
	int		iFlags;
	bool	bActive;
} hud_enemy_data_t;

class CEnemyCountPanel : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CEnemyCountPanel, vgui::EditablePanel );
public:
	CEnemyCountPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );

	void SetFlashing( bool bState );
	bool IsFlashing( void ){ return m_bFlashing; }

	CTFImagePanel *m_pEnemyCountImage;
	Panel *m_pEnemyCountImageBG;
	CTFImagePanel *m_pEnemyCountCritBG;

private:
	bool	m_bFlashing;
};

//=========================================================
class CMvMBossProgressBar : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMBossProgressBar, vgui::EditablePanel );
public:
	CMvMBossProgressBar( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );
	void SetPercentage( float flPercentage );
	void SetImage( const char* pszImageName );

private:
	float m_flPercentage;
	float m_flOldPercentage;
	int m_nBarOrgX;
	int m_nBarOrgY;
	int m_nBarOrgW;
	int m_nBarOrgT;
	int m_nBgOrgX;
	int m_nBgOrgY;
	int m_nBgOrgW;
	int m_nBgOrgT;
	vgui::ScalableImagePanel *m_pProgressBar;
	vgui::ScalableImagePanel *m_pProgressBarBG;
	CTFImagePanel *m_pBossImage;

	// keeps the two rounded ends a certain distance apart from each other so the rounded line continues to look good even at low values
	CPanelAnimationVarAliasType( int, m_nWidthSpacer, "width_spacer", "2", "proportional_xpos" );
};

//=========================================================
class CMvMBossStatusPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMBossStatusPanel, vgui::EditablePanel );
public:
	CMvMBossStatusPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );

private:
	vgui::ScalableImagePanel *m_pBackground;
	CUtlVector< CMvMBossProgressBar* > m_ProgressBars;
	int m_nBackgroundOriginalX;
	int m_nBackgroundOriginalY;
	int m_nBackgroundOriginalW;
	int m_nBackgroundOriginalT;
	int m_nBackGroundTall;
	bool m_bPanelDirty;

	CPanelAnimationVarAliasType( int, m_nSpaceBetweenPanels, "yOffset", "5", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nXOffset, "xOffset", "5", "proportional_xpos" );
};

//=========================================================
class CCurrencyStatusPanel : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCurrencyStatusPanel, vgui::EditablePanel );
public:
	CCurrencyStatusPanel( const char *pszElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );
	virtual bool ShouldDraw( void );

	virtual bool UpdateHUD( void );

private:
	int m_nCurrency;
	int m_nTargetCurrency;
};

//=========================================================
class CInWorldCurrencyStatus : public vgui::EditablePanel 
{
	DECLARE_CLASS_SIMPLE( CInWorldCurrencyStatus, vgui::EditablePanel );
public:
	CInWorldCurrencyStatus( Panel *parent, const char *name );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );

private:

	vgui::Label *m_pGood;
	vgui::Label *m_pBad;
};

//=========================================================
class CWaveStatusPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CWaveStatusPanel, vgui::EditablePanel );
public:
	CWaveStatusPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );

private:
	void UpdateEnemyCounts( void );

	void AddClassIconBeingUsed( CUtlVector< const char* > &vector, const char *pchIcon ) const; // used temporarily to track the icons we're showing
	bool IsClassIconBeingUsed( CUtlVector< const char* > &vector, const char *pchIcon ) const; // used temporarily to track the icons we're showing

private:
	int		m_nEnemyRemainingNoSupport;
	int		m_nBarOrgX;
	int		m_nBarOrgY;
	int		m_nBarOrgW;
	int		m_nBarOrgT;
	int		m_nBgOrgX;
	int		m_nBgOrgY;
	int		m_nBgOrgW;
	int		m_nBgOrgT;
	int		m_nWaveCount;
	int		m_nMaxWaveCount;
	bool	m_bPanelDirty;

	vgui::Panel *m_pSeparatorBar;
	CExLabel *m_pSupportLabel;
	vgui::ScalableImagePanel *m_pProgressBar;
	vgui::ScalableImagePanel *m_pProgressBarBG;
	vgui::ScalableImagePanel *m_pBackground;

	CUtlVector< CEnemyCountPanel* > m_EnemyCountPanels;

	CPanelAnimationVarAliasType( int, m_nWaveCountYPos, "count_ypos", "32", "proportional_ypos" ); // ypos for the icon/count pairs
	CPanelAnimationVarAliasType( int, m_nSupportLabelYOffset, "support_label_yOffset", "20", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nWaveCountOffset, "count_offset", "5", "proportional_xpos" ); // space between the icon/count pairs
	CPanelAnimationVarAliasType( int, m_nWaveCountBGMinWidth, "count_bg_min_width", "200", "proportional_xpos" ); // min background width
	CPanelAnimationVar( Color, m_clrNormal, "color_normal", "TanLight" );
	CPanelAnimationVar( Color, m_clrMiniBoss, "color_miniboss", "RedSolid" );

	// keeps the two rounded ends a certain distance apart from each other so the rounded line continues to look good even at low values
	CPanelAnimationVarAliasType( int, m_nWidthSpacer, "width_spacer", "2", "proportional_xpos" );

	CPanelAnimationVar( bool, m_bVerbose, "verbose", "0" );
	CPanelAnimationVarAliasType( int, m_nNormalHeight, "normal_height", "35", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nVerboseHeightNoNumbers, "verbose_height_no_numbers", "53", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nVerboseHeight, "verbose_height", "65", "proportional_ypos" );
};

//=========================================================
class CWarningSwoop : public vgui::ImagePanel
{
	DECLARE_CLASS_SIMPLE( CWarningSwoop, vgui::ImagePanel );
public:
	CWarningSwoop( Panel *parent, const char *name );

	virtual void PaintBackground( void );
	virtual bool IsVisible( void );

	void StartSwoop( void );

private:
	float m_flStartCapAnimStart;

	CPanelAnimationVar( float, m_flSwoopTime, "time", "0.3" );
};

//=========================================================
class CWaveCompleteSummaryPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CWaveCompleteSummaryPanel, vgui::EditablePanel );
public:
	CWaveCompleteSummaryPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );

	void ShowWaveSummary( int nWaveNumber );

private:
	enum
	{
		CREDITS_COLLECT = 0,
		CREDITS_MISSED,
		CREDITS_BONUS,
		RATING_LABEL,
		RATING_SCORE,
		CHECKPOINT,
		WAIT,
		FINISHED,
	};

	bool StateUpdateValue( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int endValue );

	void RatingLabelUpdate( void );
	void RatingScoreUpdate( void );
	
	bool CheckState( float targetTime, float currentTime, int nextState );
	void CheckCredits ();

	vgui::EditablePanel *m_pWaveCompleteContainer;
	vgui::EditablePanel *m_pCreditContainerPanel;
	vgui::EditablePanel *m_pRatingContainerPanel;

	vgui::Label *m_pCreditBonusTextLabel;
	vgui::Label *m_pCreditBonusCountLabel;

	vgui::ScalableImagePanel *m_pRespecBackground;
	vgui::EditablePanel *m_pRespecContainerPanel;
	vgui::Label *m_pRespecTextLabel;
	vgui::Label *m_pRespecCountLabel;

	float m_fPreviousTick;
	float m_fStateRunningTime;

	int m_nWaveNumber;
	int m_eState;

	int m_nCreditsCollected;
	int m_nCreditsMissed;
	int m_nCreditBonus;
};

//=========================================================
class CVictorySplash : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CVictorySplash, vgui::EditablePanel );
public:
	CVictorySplash( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );

private:
	
};

//=========================================================
class CMvMBombCarrierProgress : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMBombCarrierProgress, vgui::EditablePanel );
public:
	CMvMBombCarrierProgress( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

};

//=========================================================
class CTFHudMannVsMachineStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudMannVsMachineStatus, vgui::EditablePanel );

public:
	CTFHudMannVsMachineStatus( const char *pElementName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );
	virtual bool ShouldDraw( void );
	virtual void OnTick( void );
	//virtual void OnCommand( const char *command );

	void WaveFailed( void );
	void ShowWaveSummary ( int nWaveNumber );
	void MVMVictory( bool bIsKicking, int bTime );
	void MVMServerKickTimeUpdate( int bTime );
	void MVMVictoryGCResponse( CMsgMvMVictoryInfo &pData );

	void ForceVictoryRefresh();

	void ReopenVictoryPanel( void );

	bool IsVictoryPanelVisible()
	{
		return m_pVictoryContainer && m_pVictoryContainer->IsVictoryPanelVisible();
	}

	virtual GameActionSet_t GetPreferredActionSet() { return IsActive() ? (IsVictoryPanelVisible() ? GAME_ACTION_SET_MENUCONTROLS : CHudElement::GetPreferredActionSet()) : GAME_ACTION_SET_NONE; }

	bool IsWaveCompletePanelVisible()
	{
		return m_pWaveCompletePanel && m_pWaveCompletePanel->IsVisible();
	}
private:

	void UpdateBombCarrierProgress ( void );

	void UpdateServerMessage( void );

	CWarningSwoop *m_pWarningSwoop;
	CWaveStatusPanel *m_pWaveStatusPanel;
	CWaveCompleteSummaryPanel *m_pWaveCompletePanel;

	CVictorySplash *m_pVictorySplash;
	CMvMVictoryPanelContainer *m_pVictoryContainer;
	CMvMWaveLossPanel *m_pWaveLossPanel;

	vgui::EditablePanel *m_pServerChangeMessage;

	bool	m_bInVictorySplash;
	float	m_flVictoryTimer;

	int		m_nNextWaveTime;
 
	bool	m_bSpecPanelVisible;
	int		m_nSpyMissionCount;

	int		m_nFlagCarrierUpgradeLevel;
	vgui::EditablePanel *m_pUpgradeLevelContainer;
	vgui::ImagePanel	*m_pUpgradeLevel1;
	vgui::ImagePanel	*m_pUpgradeLevel2;
	vgui::ImagePanel	*m_pUpgradeLevel3;
	vgui::ImagePanel	*m_pUpgradeLevelBoss;

	int m_nUpgradeMaskBaseWidth;
	int m_nUpgradeMaskMaxWidth;
	vgui::EditablePanel *m_pBombUpgradeMeterMask;

	bool m_bAdjustWaveStatusPanel;

	bool m_bIsServerKicking;
	float m_flServerEndTime;
};

#endif // TF_HUD_MANN_VS_MACHINE_STATUS_H

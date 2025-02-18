//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MANN_VS_MACHINE_VICTORY_H
#define TF_HUD_MANN_VS_MACHINE_VICTORY_H
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
#include <vgui_controls/ImageList.h>
#include <vgui/KeyCode.h>
#include "vgui_controls/SectionedListPanel.h"
#include "c_tf_objective_resource.h"
#include "vgui_avatarimage.h"
#include "item_model_panel.h"
#include "c_playerresource.h"
#include "tf_gcmessages.h"
#include "tf_mann_vs_machine_stats.h"
#include "tf_gamerules.h"
#include "tf_hud_mann_vs_machine_stats.h"
#include "tf_gc_client.h"

class CTFParticlePanel;

//=========================================================
class CVictoryPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CVictoryPanel, vgui::EditablePanel );
public:
	CVictoryPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnTick( void );
	
	void ResetVictoryPanel();

	void SetMapAndPopFile ();
	
private:
	enum
	{
		INITIAL_VICTORY = 0,
		CREDITS_COLLECT,
		CREDITS_MISSED,
		CREDITS_BONUS,
		YOUR_UPGRADES,
		YOUR_BUYBACK,
		YOUR_BOTTLES,
		RATING_LABEL,
		RATING_SCORE,
		FINISHED,
	};

	void CaptureStats();

	bool StateUpdateValue( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int endValue );
	bool StateUpdateCreditText( vgui::EditablePanel *parent, char* field, float targetTime, float currentTime, int nextState, int useValue, int creditValue );
	bool CheckState( float targetTime, float currentTime, int nextState );

	void RatingLabelUpdate( void );
	void RatingScoreUpdate( void );

	float m_fPreviousTick;
	float m_fStateRunningTime;

	vgui::EditablePanel *m_pHeaderContainer;
	vgui::EditablePanel *m_pCreditContainerPanel;
	vgui::EditablePanel *m_pTeamStatsContainerPanel;
	vgui::EditablePanel *m_pYourStatsContainerPanel;
	vgui::EditablePanel *m_pRatingContainerPanel;
	CExImageButton		*m_pDoneButton;

	CCreditSpendPanel *m_pTotalGameCreditSpendPanel;

	int m_eState;

	int m_nCreditsCollected;
	int m_nCreditsMissed;
	int m_nCreditBonus;

	int m_nYourBuybacksCredits;
	int m_nYourBottlesCredits;
	int m_nYourUpgradeCredits;
};

//=========================================================
class CMvMVictoryMannUpLoot : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMVictoryMannUpLoot, vgui::EditablePanel );

public:
	CMvMVictoryMannUpLoot( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	void SetEconItem( CEconItem *econItem );
	void HideEconItem( );
	void SetEconToolTip( CItemModelPanelToolTip *pToolTip );

private:

	CItemModelPanel *m_pItemModelPanel;
};


//=========================================================
class CMvMVictoryMannUpEntry : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMVictoryMannUpEntry, vgui::EditablePanel );

public:
	CMvMVictoryMannUpEntry( Panel *parent, const char *pName );
	~CMvMVictoryMannUpEntry();

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	void SetActive( bool bActive );
	void SetData( KeyValues *kv );
	void ClearPlayerData();
#ifdef USE_MVM_TOUR
	void SetPlayerData ( const CMsgMvMVictoryInfo_Player& player, int nMissionCount );
#else // new mm
	void SetPlayerData ( const CMsgMvMVictoryInfo_Player& player, int nMissionIndex );
#endif // USE_MVM_TOUR
	void SetItemsToolTip( CItemModelPanelToolTip *pToolTip );

	bool AnimateProgressBar( void );
	bool AnimateLoot( CTFParticlePanel* pParticlePanel );

	void ForceFinishAllAnimation();
	void SetLootAnimationPause( float flPause );
private:
	void PlayVCD( const char * pszVCDName );
	bool SetModelPanelInfo( C_TFPlayer* pPlayer );
	void SetBadgeProgressBarProgress( float flPercent );
#ifdef USE_MVM_TOUR
	int GetBadgeCompletionCount( uint32 iProgressBits );
#endif // USE_MVM_TOUR
	vgui::EditablePanel* AddLootRow();

	void CheckBadgeLevel ( const CMsgMvMVictoryInfo_Player& player );
	void ClearEconItems ();

	bool AnimTimePassed( float flTime ) const;
	bool AnimateLoot_Internal( CTFParticlePanel* pParticlePanel );

	void UpdatePlayerData();

	CTFParticlePanel *m_pBehindItemParticlePanel;
	vgui::EditablePanel *m_pTourProgress;
	vgui::EditablePanel *m_pProgressBarBG;
	vgui::EditablePanel *m_pProgressBarFGStatic;
	vgui::EditablePanel *m_pHeader;
	vgui::EditablePanel *m_pProgressBarFGAnim;

	vgui::EditablePanel *m_pProgressCheckOnBackground;
	vgui::ImagePanel *m_pProgressCheckOn;
	vgui::EditablePanel *m_pSquadSurplusBackground;
	vgui::ImagePanel *m_pSquadSurplus;

	vgui::Label *m_pMissingVoucher;

	CUtlVector< CEconItem* > m_MannUpEconItems;

	vgui::HFont m_DividerLabelFont;

	CMsgMvMVictoryInfo_Player m_playerData;
	bool m_bHasData;

	EHANDLE m_hPlayer;
	bool m_bBadgeUpdated;
	int m_iProgressWidthStart;
	int m_iProgressWidthEnd;
	int m_nBadgeLevel;

	float m_flPBarCurrTime;
	float m_flPBarPreviousTime;

	int m_iLootAnimIndex;
	float m_flLootAnimTime;
	float m_flLastLootAnimTime;

#ifdef USE_MVM_TOUR
	int m_nChallengeCount;
#else // new mm
	int m_nMissionIndex;
#endif // USE_MVM_TOUR

	int m_nItemColumns;
	int m_nItemXSpacing;
	int m_nItemYSpacing;

	class CMvMLootItem : public CItemModelPanel
	{
		DECLARE_CLASS_SIMPLE( CMvMLootItem, CItemModelPanel );
	public:
		CMvMLootItem( vgui::Panel *parent, const char *name )
			: CItemModelPanel( parent, name )
			, m_nIndex( 0 )
			, m_pUnopenedPanel( NULL )
		{
			m_pUnopenedPanel = new vgui::ImagePanel( parent, VarArgs( "unopened_%s", name ) );
		}
		~CMvMLootItem()
		{}
		
		vgui::ImagePanel *m_pUnopenedPanel;
		CMsgMvMVictoryInfo_GrantReason m_eReason;
		int m_nIndex;
	};

	CUtlVector< CMvMLootItem* > m_vecLootPanels;
	CUtlVector< vgui::EditablePanel* > m_vecRows;
	
	KeyValues *m_pItemModelPanelKVs;
	KeyValues *m_pRowKVs;
	KeyValues *m_pUnopenedLootKVs;

	CUtlMap< int, CExLabel* > m_LootLables;

	class CTFPlayerModelPanel *m_pPlayerModelPanel;
	class vgui::PanelListPanel *m_pListPanel;
};

//=========================================================
class CMvMVictoryMannUpPlayerTab : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMVictoryMannUpPlayerTab, vgui::EditablePanel );

public:
	CMvMVictoryMannUpPlayerTab( Panel *parent, const char *pName );

	void SetPlayer( const CSteamID& steamID );
	void SetSelected( bool bState );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnCommand( const char *command ) OVERRIDE;

private:

	CAvatarImagePanel *m_pAvatarImage;
	vgui::EditablePanel *m_pActiveTab;
	vgui::EditablePanel *m_pMouseoverHighlightPanel;
	bool m_bIsActive;
};

//=========================================================
class CMvMVictoryMannUpPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMVictoryMannUpPanel, vgui::EditablePanel );

public:
	CMvMVictoryMannUpPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void OnTick( void ) OVERRIDE;
	virtual void SetVisible( bool bState ) OVERRIDE;

	void ShowVictoryPanel();
	void ClearData();
	void MannUpServerResponse( CMsgMvMVictoryInfo &pData );
	void ForceFinishAllAnimation();

	bool HasData() { return m_hasData; }

	virtual void OnCommand( const char *command ) OVERRIDE;
private:

	void LoadVictoryData();
	void UpdateHighlight();
	void SetTabActive( int nIndex );

	CTFParticlePanel *m_pParticlePanel;
	CUtlVector< vgui::Button* > m_vecTabButtons;
	CUtlVector< CMvMVictoryMannUpPlayerTab* > m_vecTabs;
	CUtlVector< CMvMVictoryMannUpEntry* > m_PlayerEntryPanels;
	CExImageButton *m_pDoneButton;
	
	CItemModelPanel *m_pMouseOverItemPanel;
	CItemModelPanelToolTip *m_pMouseOverTooltip;

	vgui::EditablePanel *m_pNoItemServerContainer;

	bool m_hasData;

	int m_iMannUpLootIndex;

	bool m_bAnimationComplete;

	float m_flChangeTabPauseTime;

	CMsgMvMVictoryInfo m_victoryInfo;
};

//=========================================================
class CMvMVictoryPanelContainer : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CMvMVictoryPanelContainer, vgui::EditablePanel );

public:
	CMvMVictoryPanelContainer( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void FireGameEvent( IGameEvent *event );
	virtual void OnTick( void );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodePressed( vgui::KeyCode code );

	void MannUpServerResponse( CMsgMvMVictoryInfo &pData )
	{ 
		m_pVictoryPanelMannUp->MannUpServerResponse( pData );
		m_pObjective = TFObjectiveResource();
	}

	void ShowVictoryPanel( bool bIsReopening );
	void CreateReOpenNotification();

	bool IsVictoryPanelVisible()
	{
		return ( m_pVictoryPanelNormal && m_pVictoryPanelNormal->IsVisible() ) || ( m_pVictoryPanelMannUp && m_pVictoryPanelMannUp->IsVisible() );
	}

private:

	CVictoryPanel *m_pVictoryPanelNormal;
	CMvMVictoryMannUpPanel *m_pVictoryPanelMannUp;
	const C_TFObjectiveResource *m_pObjective;
};



#endif // TF_HUD_MANN_VS_MACHINE_VICTORY_H

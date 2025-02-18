//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MATCH_STATUS_H
#define TF_HUD_MATCH_STATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "hudelement.h"
#include "basemodelpanel.h"
#include "tf_teamstatus.h"
#include "tf_matchmaking_shared.h"

using namespace vgui;

bool ShouldUseMatchHUD();

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CRoundCounterPanel : public EditablePanel, public CGameEventListener 
{
public:
	DECLARE_CLASS_SIMPLE( CRoundCounterPanel, EditablePanel );
	typedef CUtlVector< ImagePanel* > ImageVector;

	CRoundCounterPanel( Panel *parent, const char *panelName );
	~CRoundCounterPanel();

	virtual void ApplySchemeSettings(IScheme *pScheme) OVERRIDE;
	virtual void ApplySettings(KeyValues *inResourceData) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual void OnThink() OVERRIDE;

	virtual void FireGameEvent(IGameEvent * event) OVERRIDE;

private:

	void CreateRoundPanels( ImageVector& vecImages, const char* pszName, KeyValues* pKVSettings );

	enum EAlignment
	{
		ALIGN_WEST,
		ALIGN_EAST
	};

	void LayoutPanels( ImageVector& vecImages, EAlignment eAlignment, int nStartPos, int nMaxWide );

	KeyValues* m_pRoundIndicatorKVs;
	KeyValues* m_pRoundWinIndicatorRedKV;
	KeyValues* m_pRoundWinIndicatorBlueKV;

	ImageVector m_vecRedRoundIndicators;
	ImageVector m_vecBlueRoundIndicators;

	ImageVector m_vecRedWinIndicators;
	ImageVector m_vecBlueWinIndicators;

	bool m_bCountDirty;

	CPanelAnimationVarAliasType( int, m_nStartingWidth, "starting_width", "10", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nWidthPerRound, "width_per_round", "10", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nIndicatorStartOffset, "indicator_start_offset", "8", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nIndicatorPanelStep, "indicator_max_wide", "10", "proportional_int" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFHudMatchStatus : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudMatchStatus, EditablePanel );
public:
	CTFHudMatchStatus( const char *pElementName );
	virtual ~CTFHudMatchStatus( void );
	
	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void FireGameEvent( IGameEvent * event ) OVERRIDE;
	virtual void OnThink() OVERRIDE;
	virtual void PerformLayout() OVERRIDE;

	virtual bool IsVisible( void ) OVERRIDE;
	virtual bool ShouldDraw( void ) OVERRIDE;

	virtual void Reset() OVERRIDE;

	void SetPanelsVisible();

private:

	void ShowMatchStartDoors();
	void ShowRoundSign( int nRoundNumber );
	void InitPlayerList( SectionedListPanel *pPlayerList, int nTeam );
	void UpdatePlayerList();
	void UpdatePlayerAvatar( int playerIndex, KeyValues *kv );
	void UpdateTeamInfo();
	void HandleCountdown( int nTime );

	CRoundCounterPanel	*m_pRoundCounter;
	class CTFHudTimeStatus	*m_pTimePanel;
	CModelPanel			*m_pRoundSignModel;
	CTFTeamStatus		*m_pTeamStatus;
	CModelPanel			*m_pMatchStartModelPanel;
	ETFMatchGroup			m_eMatchGroupSettings;

	vgui::EditablePanel			*m_pBlueTeamPanel;
	vgui::SectionedListPanel	*m_pPlayerListBlue;
	vgui::EditablePanel			*m_pRedTeamPanel;
	vgui::SectionedListPanel	*m_pPlayerListRed;
	vgui::ImageList				*m_pImageList;
	CUtlMap<CSteamID, int>		m_mapAvatarsToImageList;

	CAvatarImagePanel			*m_pRedLeaderAvatarImage;
	EditablePanel				*m_pRedLeaderAvatarBG;
	vgui::ImagePanel			*m_pRedTeamImage;
	CExLabel					*m_pRedTeamName;
	CAvatarImagePanel			*m_pBlueLeaderAvatarImage;
	EditablePanel				*m_pBlueLeaderAvatarBG;
	vgui::ImagePanel			*m_pBlueTeamImage;
	CExLabel					*m_pBlueTeamName;

	CPanelAnimationVar( int, m_iAvatarWidth, "avatar_width", "34" );		// Avatar width doesn't scale with resolution
	CPanelAnimationVarAliasType( int, m_iSpacerWidth, "spacer", "5", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iNameWidth, "name_width", "136", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iHorizFillInset, "horiz_inset", "4", "proportional_int" );

	vgui::HFont					m_hPlayerListFont;

	bool m_bUseMatchHUD;
};

#endif	// TF_HUD_MATCH_STATUS_H

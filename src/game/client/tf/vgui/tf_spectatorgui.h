//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_SPECTATORGUI_H
#define TF_SPECTATORGUI_H
#ifdef _WIN32
#pragma once
#endif

#include <spectatorgui.h>
#include "hudelement.h"
#include "tf_hud_playerstatus.h"
#include "item_model_panel.h"
#include "tf_gamerules.h"
#include <vgui_controls/EditablePanel.h>

extern ConVar cl_use_tournament_specgui;
class CAvatarImagePanel;
class CTFPlayerPanel;
class CSCHintIcon;

//-----------------------------------------------------------------------------
// Purpose: Custom health panel used to show spectator target's health
//-----------------------------------------------------------------------------
class CTFSpectatorGUIHealth : public CTFHudPlayerHealth
{
public:
	CTFSpectatorGUIHealth( Panel *parent, const char *name ) : CTFHudPlayerHealth( parent, name )
	{
	}

	virtual const char *GetResFilename( void ) 
	{ 
		return "resource/UI/SpectatorGUIHealth.res"; 
	}
	virtual void OnThink()
	{
		// Do nothing. We're just preventing the base health panel from updating.
	}
};


//-----------------------------------------------------------------------------
// Purpose: TF Spectator UI
//-----------------------------------------------------------------------------
class CTFSpectatorGUI : public CSpectatorGUI, public CGameEventListener
{
private:
	DECLARE_CLASS_SIMPLE( CTFSpectatorGUI, CSpectatorGUI );

public:
	CTFSpectatorGUI( IViewPort *pViewPort );
	~CTFSpectatorGUI( void );
		
	virtual void Reset( void );
	virtual void PerformLayout( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );

	virtual void Update( void );
	virtual bool NeedsUpdate( void );
	virtual bool ShouldShowPlayerLabel( int specmode ) { return false; }
	void		 UpdateReinforcements( void );
	virtual void ShowPanel(bool bShow);
	virtual Color GetBlackBarColor( void ) { return Color(52,48,45, 255); }

	void		UpdateKeyLabels( void );

	virtual void FireGameEvent( IGameEvent *event );
	void		UpdateItemPanel( bool bForce = false );
	void		ForceItemPanelCycle( void );
	virtual const char *GetResFile( void );

	// Tournament mode handling
	bool		InTournamentGUI( void );
	void		RecalculatePlayerPanels( void );
	void		UpdatePlayerPanels( void );
	void		SelectSpec( int iSlot );

	virtual int GetTopBarHeight();

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_SPECTATOR; }

protected:	
	int		m_nLastSpecMode;
	float	m_flNextTipChangeTime;		// time at which to next change the tip
	int		m_iTipClass;				// class that current tip is for

	// used to store the x and y position of the Engy and Spy build panels so we can reset them when the spec panel goes away
	int		m_nEngBuilds_xpos;
	int		m_nEngBuilds_ypos;
	int		m_nSpyBuilds_xpos;
	int		m_nSpyBuilds_ypos;

	int		m_nMannVsMachineStatus_xpos;
	int		m_nMannVsMachineStatus_ypos;

	vgui::Label				*m_pReinforcementsLabel;
	CExLabel				*m_pBuyBackLabel;
	vgui::Label				*m_pClassOrTeamLabel;
	CExLabel				*m_pClassOrTeamKeyLabel;
	vgui::Label				*m_pSwitchCamModeKeyLabel;
	vgui::Label				*m_pCycleTargetFwdKeyLabel;
	vgui::Label				*m_pCycleTargetRevKeyLabel;
	vgui::Label				*m_pMapLabel;
	CItemModelPanel			*m_pItemPanel;
	CSCHintIcon				*m_pCycleTargetFwdHintIcon;
	CSCHintIcon				*m_pCycleTargetRevHintIcon;
	CSCHintIcon				*m_pClassOrTeamHintIcon;

	float					m_flNextItemPanelUpdate;
	EHANDLE					m_hPrevItemPlayer;
	int						m_iPrevItemShown;
	int						m_iFirstItemShown;
	bool					m_bShownItems;

	// Tournament mode player panel handling
	CUtlVector<CTFPlayerPanel*>	m_PlayerPanels;
	KeyValues				*m_pPlayerPanelKVs;
	bool					m_bReapplyPlayerPanelKVs;
	bool					m_bPrevTournamentMode;
	float					m_flNextPlayerPanelUpdate;

	// Coaching
	bool					m_bCoaching;
	CAvatarImagePanel		*m_pAvatar;
	CTFSpectatorGUIHealth	*m_pStudentHealth;

	CPanelAnimationVarAliasType( int, m_iTeam1PlayerBaseOffsetX, "team1_player_base_offset_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam1PlayerBaseX, "team1_player_base_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam1PlayerBaseY, "team1_player_base_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam2PlayerBaseX, "team2_player_base_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam2PlayerBaseOffsetX, "team2_player_base_offset_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam2PlayerBaseY, "team2_player_base_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam1PlayerDeltaX, "team1_player_delta_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam1PlayerDeltaY, "team1_player_delta_y", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam2PlayerDeltaX, "team2_player_delta_x", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iTeam2PlayerDeltaY, "team2_player_delta_y", "0", "proportional_int" );
};

#endif // TF_SPECTATORGUI_H

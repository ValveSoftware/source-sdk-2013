//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================


#ifndef TF_HUD_TOURNAMENT_H
#define TF_HUD_TOURNAMENT_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_clientscoreboard.h"
#include "tf_playerpanel.h"
#include "basemodelpanel.h"

class CTFHudTimeStatus;

class CHudTournament : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTournament, EditablePanel );

public:
	CHudTournament( const char *pElementName );
	~CHudTournament();

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySettings( KeyValues *inResourceData );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual void	SetVisible( bool state );
	virtual void	PerformLayout( void );

	virtual void	FireGameEvent( IGameEvent * event );

	void			PreparePanel( void );
	void			PlaySounds( int nTime );

	virtual bool	ShouldDraw( void ) { return m_bShouldBeVisible; }

private:
	void		RecalculatePlayerPanels( void );
	void		UpdatePlayerPanels( void );
	void		SetPlayerPanelsVisible( bool bVisible );
	CTFPlayerPanel *GetOrAddPanel( int iPanelIndex );

	void InitPlayerList( SectionedListPanel *pPlayerList, int nTeam );
	static bool TFPlayerSortFunc( vgui::SectionedListPanel *list, int itemID1, int itemID2 );
	void UpdatePlayerList();
	void UpdatePlayerAvatar( int playerIndex, KeyValues *kv );

	void UpdateTeamInfo();

private:
	Label			*m_pTournamentLabel;
	Label			*m_pReasonLabel;
	bool			m_bShouldBeVisible;

	float			m_flNextUpdate;

	bool			m_bTeamReady[MAX_TEAMS];

	bool			m_bReadyStatusMode;
	bool			m_bCompetitiveMode;
	bool			m_bReadyTextBlinking;
	bool			m_bCountDownVisible;

	CUtlVector<CTFPlayerPanel*>	m_PlayerPanels;
	KeyValues				*m_pPlayerPanelKVs;
	bool					m_bReapplyPlayerPanelKVs;

	vgui::DHANDLE< CTFClientScoreBoardDialog > m_pScoreboard;

	vgui::ScalableImagePanel	*m_pCountdownBG;
	CExLabel					*m_pCountdownLabel;
	CExLabel					*m_pCountdownLabelShadow;
	vgui::ImagePanel			*m_pModeImage;
	
	vgui::ScalableImagePanel	*m_pHudTournamentBG;
	CExLabel					*m_pTournamentConditionLabel;
	
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
	CPanelAnimationVarAliasType( int, m_iTeamsPlayerDeltaXComp, "teams_player_delta_x_comp", "0", "proportional_int" );
};

class CHudTournamentSetup : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudTournamentSetup, EditablePanel );

public:
	CHudTournamentSetup( const char *pElementName );

	virtual void	Init( void );
	virtual void	OnTick( void );
	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *scheme );
	virtual bool	ShouldDraw( void );

	virtual void	FireGameEvent( IGameEvent * event );
	void			EnableInput( void );
	void			DisableInput( void );
	bool			ToggleState( ButtonCode_t code );
	virtual void	OnCommand( const char *command );

	virtual void OnKeyCodeTyped(vgui::KeyCode code)
	{
		if ( code == KEY_ESCAPE  || code == KEY_F4  || code == KEY_ENTER )
		{
			ToggleState( code );
		}
		else
		{
			BaseClass::OnKeyCodeTyped( code );
		}
	}

private:
	TextEntry *m_pNameEntry;
	CTFImagePanel *m_pEntryBG;
	CExButton *m_pReadyButton;
	CExButton *m_pNotReadyButton;
	vgui::Label	  *m_pTeamNameLabel;


	float	m_flNextThink;
};

class CHudStopWatch : public CHudElement, public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CHudStopWatch, EditablePanel );

public:
	CHudStopWatch( const char *pElementName );

	virtual void	LevelInit( void );
	virtual void	ApplySchemeSettings( IScheme *pScheme );
	virtual void	OnTick( void );
	virtual bool	ShouldDraw( void );
	virtual void	FireGameEvent( IGameEvent * event );

private:

	CTFHudTimeStatus *m_pTimePanel;
	CExLabel		 *m_pStopWatchLabel;
	CExLabel		 *m_pStopWatchScore;
	CExLabel		 *m_pStopWatchPointsLabel;
	ImagePanel		 *m_pStopWatchImage;
	CExLabel		 *m_pStopWatchDescriptionLabel;
	Panel			 *m_pStopWatchDescriptionBG;
	bool			 m_bShouldBeVisible;
};

#endif // TF_HUD_TOURNAMENT_H

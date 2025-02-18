//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_PLAYERPANEL_H
#define TF_PLAYERPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_spectatorgui.h"

//-----------------------------------------------------------------------------
// Purpose: Custom health panel used to show spectator target's health in Tournament HUD
//-----------------------------------------------------------------------------
class CTFPlayerPanelGUIHealth : public CTFSpectatorGUIHealth
{
public:
	CTFPlayerPanelGUIHealth( Panel *parent, const char *name ) : CTFSpectatorGUIHealth( parent, name )
	{
	}

	virtual const char *GetResFilename( void ) 
	{ 
		return "resource/UI/SpectatorTournamentGUIHealth.res"; 
	}
};

//-----------------------------------------------------------------------------
// Purpose: A panel representing a player, shown in tournament mode Spec GUI
//-----------------------------------------------------------------------------
class CTFPlayerPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFPlayerPanel, vgui::EditablePanel );
public:
	CTFPlayerPanel( vgui::Panel *parent, const char *name );

	virtual void	Reset( void );
	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual bool	Update( void );
	void	SetPlayerIndex( int iIndex );
	int		GetPlayerIndex( void ) { return m_iPlayerIndex; }
	void	Setup( int iPlayerIndex, CSteamID steamID, const char *pszPlayerName, int nLobbyTeam = TEAM_INVALID );
	void	SetSpecIndex( int iIndex );
	int		GetSpecIndex( void ) { return m_iSpecIndex; }
	virtual void	UpdateBorder( void );
	int		GetTeam( void );

	inline	CSteamID GetSteamID() const { return m_steamID; }

protected:
	int						m_iPlayerIndex;
	CUtlString				m_sPlayerName;
	CSteamID				m_steamID;
	int						m_iSpecIndex;
	CTFClassImage			*m_pClassImage;
	CTFPlayerPanelGUIHealth	*m_pHealthIcon;
	int						m_iPrevHealth;
	bool					m_bPrevAlive;
	int						m_iPrevClass;
	int						m_iPrevRespawnWait;
	int						m_iPrevCharge;
	vgui::ScalableImagePanel		*m_pReadyBG;
	vgui::ImagePanel				*m_pReadyImage;

	// Ready state
	bool					m_bPrevReady;
	int						m_iPrevState;
	bool					m_bPlayerReadyModeActive;
	int						m_nGCTeam;
};



#endif // TF_PLAYERPANEL_H

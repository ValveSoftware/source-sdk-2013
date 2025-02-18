//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_TIME_PANEL_H
#define TF_TIME_PANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "GameEventListener.h"
#include "hudelement.h"
#include <vgui_controls/ImagePanel.h>

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTFProgressBar : public vgui::ImagePanel
{
public:
	DECLARE_CLASS_SIMPLE( CTFProgressBar, vgui::ImagePanel );

	CTFProgressBar( vgui::Panel *parent, const char *name );

	virtual void Paint();
	void SetPercentage( float flPercentage ){ m_flPercent = flPercentage; }

private:

	float	m_flPercent;
	int		m_iTexture;

	CPanelAnimationVar( Color, m_clrActive, "color_active", "TimerProgress.Active" );
	CPanelAnimationVar( Color, m_clrInActive, "color_inactive", "TimerProgress.InActive" );
	CPanelAnimationVar( Color, m_clrWarning, "color_warning", "TimerProgress.Active" );
	CPanelAnimationVar( float, m_flPercentWarning, "percent_warning", "0.75" );
};


// Floating delta text items, float off the top of the frame to 
// show changes to the timer value
typedef struct 
{
	// amount of delta
	int m_nAmount;

	// die time
	float m_flDieTime;

} timer_delta_t;

#define NUM_TIMER_DELTA_ITEMS 10

class CExLabel;

//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudTimeStatus : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHudTimeStatus, vgui::EditablePanel );

public:

	CTFHudTimeStatus( Panel *parent, const char *name );

	virtual void Paint( void );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();

	int GetTimerIndex( void ){ return m_iTimerIndex; }
	void SetTimerIndex( int index ){ m_iTimerIndex = ( index >= 0 ) ? index : 0; SetExtraTimePanels(); }
	
	virtual void FireGameEvent( IGameEvent *event );

	void SetTeam( int nTeam ){ m_nTeam = nTeam; }

protected:

	virtual void OnThink();

private:

	void SetExtraTimePanels();
	void SetTimeAdded( int iIndex, int nSeconds );
	void CheckClockLabelLength( CExLabel *pLabel, vgui::Panel *pBG );
	void SetTeamBackground( void );

private:

	float				m_flNextThink;
	int					m_iTimerIndex;
	bool				m_bSuddenDeath;
	bool				m_bOvertime;

	CExLabel			*m_pTimeValue;
	CTFProgressBar		*m_pProgressBar;

	vgui::ScalableImagePanel	*m_pTimerBG;

	CExLabel			*m_pWaitingForPlayersLabel;
	vgui::Panel			*m_pWaitingForPlayersBG;

	CExLabel			*m_pOvertimeLabel;
	vgui::Panel			*m_pOvertimeBG;

	CExLabel			*m_pSetupLabel;
	vgui::Panel			*m_pSetupBG;

	CExLabel			*m_pServerTimeLabel;
	vgui::Panel			*m_pServerTimeLabelBG;

	// we'll have a second label/bg set for the SuddenDeath panel in case we want to change the look from the Overtime label
	CExLabel			*m_pSuddenDeathLabel;
	vgui::Panel			*m_pSuddenDeathBG;

	// delta stuff
	int m_iTimerDeltaHead;
	timer_delta_t m_TimerDeltaItems[NUM_TIMER_DELTA_ITEMS];
	CPanelAnimationVarAliasType( float, m_flDeltaItemStartPos, "delta_item_start_y", "100", "proportional_float" );
	CPanelAnimationVarAliasType( float, m_flDeltaItemEndPos, "delta_item_end_y", "0", "proportional_float" );

	CPanelAnimationVarAliasType( float, m_flDeltaItemX, "delta_item_x", "0", "proportional_float" );

	CPanelAnimationVar( Color, m_DeltaPositiveColor, "PositiveColor", "0 255 0 255" );
	CPanelAnimationVar( Color, m_DeltaNegativeColor, "NegativeColor", "255 0 0 255" );

	CPanelAnimationVar( float, m_flDeltaLifetime, "delta_lifetime", "2.0" );

	CPanelAnimationVar( vgui::HFont, m_hDeltaItemFont, "delta_item_font", "Default" );

	int m_nTeam;
	bool m_bKothMode;
	bool m_bCachedOvertime;
};


//-----------------------------------------------------------------------------
// Purpose:  
//-----------------------------------------------------------------------------
class CTFHudKothTimeStatus : public CHudElement, public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudKothTimeStatus, vgui::EditablePanel );

public:
	CTFHudKothTimeStatus( const char *pElementName );
	~CTFHudKothTimeStatus();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void Reset();
	virtual void Think();
	virtual bool ShouldDraw();
	virtual void SetVisible( bool bVisible ) OVERRIDE;

	virtual int GetRenderGroupPriority( void ) { return 80; }	// higher than build menus

private:
	void UpdateActiveTeam( void );

private:
	CTFHudTimeStatus	*m_pBluePanel;
	CTFHudTimeStatus	*m_pRedPanel;
	Panel				*m_pActiveTimerBG;

	int					m_nActiveTeam;

	CPanelAnimationVarAliasType( int, m_nBlueActiveXPos, "blue_active_xpos", "0", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nRedActiveXPos, "red_active_xpos", "0", "proportional_int" );
};


#endif	// TF_TIME_PANEL_H

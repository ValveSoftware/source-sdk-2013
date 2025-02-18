//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_ROBOT_DESTRUCTION_STATUS_H
#define TF_HUD_ROBOT_DESTRUCTION_STATUS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_controls.h"
#include "tf_imagepanel.h"
#include "hud_controlpointicons.h"
#include "GameEventListener.h"
#include "tf_logic_robot_destruction.h"
#include "tf_time_panel.h"
#include "entity_capture_flag.h"

class CTFHudRobotDestruction_StateImage : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_StateImage, vgui::EditablePanel );
public:
	CTFHudRobotDestruction_StateImage( Panel *parent, const char *name, const char *pszResFile );

	virtual void ApplySchemeSettings( IScheme *pScheme ) OVERRIDE;
	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	
	void SetImageVisible( bool bVisible ) { m_pImage->SetVisible( bVisible ); }
protected:
	vgui::ImagePanel	*m_pImage;
	vgui::ImagePanel	*m_pRobotImage;
	const char			*m_pszResFile;
};

//-----------------------------------------------------------------------------
class CTFHudRobotDestruction_DeadImage : public CTFHudRobotDestruction_StateImage
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_DeadImage, CTFHudRobotDestruction_StateImage );
public:
	CTFHudRobotDestruction_DeadImage( Panel *parent, const char *name, const char *pszResFile  );

	void SetProgress( float flProgress );
private:
	CTFProgressBar *m_pRespawnProgressBar;
};

//-----------------------------------------------------------------------------
class CTFHudRobotDestruction_ActiveImage : public CTFHudRobotDestruction_StateImage
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_ActiveImage, CTFHudRobotDestruction_StateImage );
public:
	CTFHudRobotDestruction_ActiveImage( Panel *parent, const char *name, const char *pszResFile  );

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
};

//-----------------------------------------------------------------------------
class CTFHudRobotDestruction_RobotIndicator : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudRobotDestruction_RobotIndicator, vgui::EditablePanel );

public:

	CTFHudRobotDestruction_RobotIndicator( vgui::Panel *pParent, const char *pszName, CTFRobotDestruction_RobotGroup *pGroup );

	virtual void	ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void	PerformLayout() OVERRIDE;
	virtual void	ApplySettings(	KeyValues *inResourceData );
	virtual void	OnTick() OVERRIDE;

	void			DoUnderAttackBlink();
	int				GetGroupNumber() const;
	int				GetTeamNumber() const;
	void			UpdateState();
	eRobotUIState	GetState() const { return m_eState; }
	const CTFRobotDestruction_RobotGroup *GetGroup() const { return m_hGroup.Get(); }
	void			SetNextRobotIndicator( CTFHudRobotDestruction_RobotIndicator * pNext ) { m_pNextRobotIndicator = pNext; }
	void			SetPrevRobotIndicator( CTFHudRobotDestruction_RobotIndicator * pPrev ) { m_pPrevRobotIndicator = pPrev; }
private:

	CTFHudRobotDestruction_RobotIndicator *m_pPrevRobotIndicator;
	CTFHudRobotDestruction_RobotIndicator *m_pNextRobotIndicator;
	CHandle< CTFRobotDestruction_RobotGroup > m_hGroup;
	vgui::EditablePanel					*m_pRobotStateContainer;
	CTFHudRobotDestruction_DeadImage	*m_pDeadPanel;
	CTFHudRobotDestruction_ActiveImage	*m_pActivePanel;
	CTFHudRobotDestruction_StateImage	*m_pShieldedPanel;

	float								m_flHealthPercentage;
	eRobotUIState						m_eState;

	CControlPointIconSwoop * m_pSwoop;
};

class CTFHUDRobotDestruction : public vgui::EditablePanel, public CGameEventListener
{
	DECLARE_CLASS_SIMPLE( CTFHUDRobotDestruction, vgui::EditablePanel );

public:
	typedef CUtlVector< CTFHudRobotDestruction_RobotIndicator* > RobotVector_t;

	CTFHUDRobotDestruction( vgui::Panel *parent, const char *name );
	~CTFHUDRobotDestruction();

	virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PerformLayout() OVERRIDE;
	virtual bool IsVisible( void ) OVERRIDE;
	virtual void Reset();
	virtual void OnTick() OVERRIDE;
	virtual void PaintBackground() OVERRIDE;
	virtual void Paint() OVERRIDE;

	virtual void FireGameEvent( IGameEvent *pEvent ) OVERRIDE;

	void ReinitializeEverything();
	
	void UpdateRobotElements();
private:
	void PaintPDPlayerScore( const CTFPlayer* pPlayer );

	void UpdateStolenPoints( int nTeam, EditablePanel* pContainer );
	void UpdateCarriedFlagStatus( C_BasePlayer *pNewOwner, C_BaseEntity *pFlag );
	void UpdateStolenFlagStatus( int nTeam, C_BaseEntity *pFlag );
	void PerformRobotLayout( RobotVector_t& vecRobots, int nTeam );
	void SetPlayingToLabelVisible( bool bVisible );
	void UpdateTeamRobotCounts();
	
	int					m_nStealLeftEdge;
	int					m_nStealRightEdge;
	KeyValues			*m_pRobotIndicatorKVs;
	CExLabel			*m_pPlayingTo;
	vgui::Panel			*m_pPlayingToBG;
	RobotVector_t		m_vecRedRobots;
	RobotVector_t		m_vecBlueRobots;
	EditablePanel		*m_pCarriedContainer;
	vgui::ImagePanel	*m_pCarriedImage;
	EditablePanel		*m_pScoreContainer;
	EditablePanel		*m_pProgressBarsContainer;
	EditablePanel		*m_pBlueStolenContainer;
	EditablePanel		*m_pBlueDroppedPanel;
	EditablePanel		*m_pRedStolenContainer;
	EditablePanel		*m_pRedDroppedPanel;
	EditablePanel		*m_pBlueScoreValueContainer;
	EditablePanel		*m_pRedScoreValueContainer;
	EditablePanel		*m_pCountdownContainer;	 // used in the player destruction .res file
	CTFImagePanel		*m_pTeamLeaderImage;
	bool				m_bPlayingRD;

	class CProgressPanel : public ImagePanel, public CGameEventListener
	{
		DECLARE_CLASS_SIMPLE( CProgressPanel, ImagePanel );
	public:

		CProgressPanel( vgui::Panel *parent, const char *name );
		
		virtual void ApplySettings( KeyValues *inResourceData ) OVERRIDE;
		virtual void PaintBackground() OVERRIDE;
		virtual void OnTick() OVERRIDE;
		virtual void FireGameEvent( IGameEvent * pEvent ) OVERRIDE;

		void SetProgress( float flProgress, bool bInstant = false );
		void Blink();
		void SetApproachSpeed( float flApproachSpeed ) { m_flApproachSpeed = flApproachSpeed; }
		void SetColor( const Color& c ) { m_StandardColor = c; }
	private:
		void CaptureBounds();
		void CalculateSize();

		float m_flWidth;
		float m_flXpos;
		int m_nXOrg;
		int m_nYOrg;
		int m_nWideOrg;
		int m_nTallOrg;
		float m_flLastScoreTime;
		float m_flCurrentProgress;
		float m_flEndProgress;
		float m_flLastTick;

		CPanelAnimationVarAliasType( int, m_nLeftOffset, "left_offset", "25", "proportional_int" );
		CPanelAnimationVarAliasType( int, m_nRightOffset, "right_offset", "25", "proportional_int" );
		CPanelAnimationVar( Color, m_StandardColor, "standard_color", "255 255 255 255" );
		CPanelAnimationVar( Color, m_BrightColor, "bright_color", "255 255 255 255" );
		CPanelAnimationVar( bool, m_bLeftToRight, "left_to_right", "1" );
		CPanelAnimationVar( float, m_flApproachSpeed, "approach_speed", "1.f" );
		CPanelAnimationVar( float, m_flBlinkThreshold, "blink_threshold", "2.f" );
		CPanelAnimationVar( float, m_flBlinkRate, "blink_rate", "3.f" );
	};

	CProgressPanel *m_pCarriedFlagProgressBar;
	EditablePanel  *m_pRedVictoryPanel;
	CProgressPanel *m_pRedProgressBar;
	CProgressPanel *m_pRedProgressBarEscrow;
	EditablePanel  *m_pBlueVictoryPanel;
	CProgressPanel *m_pBlueProgressBar;
	CProgressPanel *m_pBlueProgressBarEscrow;
	CHandle< CCaptureFlag > m_hRedFlag;
	CHandle< CCaptureFlag > m_hBlueFlag;

	CPanelAnimationVarAliasType( int, m_nStealLeftEdgeOffset, "left_steal_edge_offset", "25", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_nStealRightEdgeOffset, "right_steal_edge_offset", "100", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRobotXOffset, "robot_x_offset", "6", "proportional_int");
	CPanelAnimationVarAliasType( int, m_iRobotYOffset, "robot_y_offset", "25", "proportional_int" );
	CPanelAnimationVarAliasType( int, m_iRobotXStep, "robot_x_step", "5", "proportional_int");
	CPanelAnimationVarAliasType( int, m_iRobotYStep, "robot_y_step", "0", "proportional_int");

	CPanelAnimationVar( Color, m_ColorBlue, "color_blue", "0 0 255 255" );
	CPanelAnimationVar( Color, m_ColorRed, "color_red", "255 0 0 255" );

	CPanelAnimationVar( vgui::HFont, m_hPDPlayerScoreFont, "player_name_font", "HudFontSmallBold" );
	CPanelAnimationVar( Color, m_TextColor, "text_color", "255 255 255 255" );
};

#endif	// TF_HUD_ROBOT_DESTRUCTION_STATUS_H


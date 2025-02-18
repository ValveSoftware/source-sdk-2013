//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef TF_HUD_SAXXYCONTEST_H
#define TF_HUD_SAXXYCONTEST_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include "game_controls/basemodel_panel.h"

using namespace vgui;

class KeyValues;
class CExButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSaxxyAwardsPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSaxxyAwardsPanel, vgui::EditablePanel );
public:
	CSaxxyAwardsPanel( Panel *pParent, const char *pName );
	~CSaxxyAwardsPanel();

	virtual void ApplySettings( KeyValues *pInResourceData );

	void Refresh();

private:
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnCommand( const char *pCommand );
	virtual void PaintBackground();

	struct FlashInfo_t
	{
		int			m_nCenterX;
		int			m_nCenterY;
		int			m_nCurW;
		int			m_nCurH;
		int			m_nMinSize;
		int			m_nMaxSize;
		float		m_flStartTime;
		float		m_flLifeLength;
		ImagePanel	*m_pPanel;
		bool		m_bInUse;
	};

	void Init();

	void OnTick();
	void UpdateMousePos( float flElapsed );
	void RotateModel( float flElapsed );
	void CurtainsThink();
	void FlashThink( bool bOtherPanelsOpen );
	void SpotlightThink();
	void ClapsThink( float flCurTime, float flElapsed, bool bOtherPanelsOpen );

	void SetupContestPanels();
	bool CreateFlash();
	void PlaceFlash( FlashInfo_t *pFlashInfo );
	void ClearFlash( FlashInfo_t *pFlashInfo );
	void ClearFlashes();
	void PlaySomeClaps();
	void SetNextPossibleClapTime( float *pClapTime );

	int GetActiveFlashCount() const;
	int GetUnusedFlashCount() const;
	int GetUnusedFlashSlot() const;
	float GetCurrentTime() const;
	bool InInitialFreakoutPeriod() const;
	bool InFreakoutMode() const;
	bool CurtainsClosed() const;
	bool FlashingStartTimePassed() const;
	bool AreOtherPanelsOpen( float flCurTime );
	bool AreNonMainMenuPanelsOpen( VPANEL vRoot, const char **pCarePanels, int nNumCarePanels );
	VPANEL GetDialogsParent();

	enum Consts_t
	{
		MAX_FLASHES = 3,
		MAX_GLOWS = 2,
		MAX_CLAPS = 2,
	};

	CBaseModelPanel		*m_pSaxxyModelPanel;
	Vector				m_vSaxxyDefaultPos;
	CExButton			*m_pSubmitButton;
	Panel				*m_pInfoLabel;
	Panel				*m_pContestOverLabel;
	EditablePanel		*m_pBackgroundPanel;
	ImagePanel			*m_pStageBgPanel;
	EditablePanel		*m_pCurtainPanel;		// Main container panel
	ImagePanel			*m_pSpotlightPanel;
	float				m_aClapPlayTimes[MAX_CLAPS];
	float				m_flShowTime;
	float				m_flNextPanelTestTime;
	VPANEL				m_vDialogsParent;

	struct CurtainInfo_t
	{
		CurtainInfo_t() : m_pPanel( NULL ) {}
		ImagePanel		*m_pPanel;
		int				m_aInitialPos[2];
	};
	CurtainInfo_t		m_Curtains[2];		// [0] = left, [1] = right
	float				m_flCurtainStartAnimTime;

	KeyValues			*m_pCameraFlashKv;

	QAngle				m_angModelRot;

	FlashInfo_t m_aFlashes[ MAX_FLASHES ];
	int			m_nNumTargetFlashes;
	float		m_aFilteredMousePos[2][2];		// [0][0] and [0][1] is current mouse pos (x,y).  [1][0] and [1][1] are target (x,y).
	float		m_flLastTickTime;
	float		m_flEarliestNextFlashTime;
	float		m_flGlowFade;

	static int	sm_nShowCounter;				// Will be 0 when the game first loads, and nonzero otherwise

	CPanelAnimationVarAliasType( int, m_nFlashBoundsX, "flashbounds_x", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nFlashBoundsY, "flashbounds_y", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nFlashBoundsW, "flashbounds_w", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nFlashBoundsH, "flashbounds_h", "0", "proportional_ypos" );

	CPanelAnimationVarAliasType( int, m_nFlashStartSizeMin, "flashstartsize_min", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nFlashStartSizeMax, "flashstartsize_max", "0", "proportional_xpos" );

	CPanelAnimationVar( float, m_flFlashMaxScale, "flash_maxscale", "0.0f" );

	CPanelAnimationVar( float, m_flFlashLifeLengthMin, "flash_lifelength_min", "0.0f" );
	CPanelAnimationVar( float, m_flFlashLifeLengthMax, "flash_lifelength_max", "0.0f" );

	CPanelAnimationVar( float, m_flCurtainAnimDuration, "curtain_anim_duration", "0.0f" );
	CPanelAnimationVar( float, m_flOpenCurtainsTime, "curtain_open_time", "0.0f" );
	CPanelAnimationVar( float, m_flInitialFreakoutDuration, "initial_freakout_duration", "0.0f" );

	CPanelAnimationVar( float, m_flFlashStartTime, "flash_start_time", "0.0f" );

	CPanelAnimationVar( float, m_flClapSoundDuration, "clap_sound_duration", "0.0f" );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CSaxxyAwardsSubmitForm : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CSaxxyAwardsSubmitForm, vgui::EditablePanel );
public:
	CSaxxyAwardsSubmitForm( Panel *pParent );

private:
	virtual void	ApplySchemeSettings( IScheme *pScheme );
	virtual void	PerformLayout();
	virtual void	OnCommand( const char *pCommand );
	virtual void	OnKeyCodeTyped( vgui::KeyCode nCode );

	void Close();

	vgui::TextEntry		*m_pURLInput;
	vgui::ComboBox		*m_pCategoryCombo;
};

#endif //TF_HUD_SAXXYCONTEST_H

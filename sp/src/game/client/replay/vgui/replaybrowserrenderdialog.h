//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef REPLAYBROWSER_RENDERDIALOG_H
#define REPLAYBROWSER_RENDERDIALOG_H
#ifdef _WIN32
#pragma once
#endif

#include "replaybrowserbasepanel.h"
#include "vgui/IScheme.h"
#include "vgui_controls/CheckButton.h"
#include "vgui_controls/ComboBox.h"
#include "vgui_controls/Slider.h"
#include "replay/replayhandle.h"

using namespace vgui;

class CExLabel;
class CExButton;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CReplayRenderDialog : public CReplayBasePanel
{
	DECLARE_CLASS_SIMPLE( CReplayRenderDialog, CReplayBasePanel );
public:
	CReplayRenderDialog( Panel *pParent, ReplayHandle_t hReplay, bool bSetQuit, int iPerformance );

	virtual void	ApplySchemeSettings( IScheme *pScheme );
	virtual void	PerformLayout();
	virtual void	OnCommand( const char *pCommand );
	virtual void	OnKeyCodeTyped( vgui::KeyCode code );
	virtual void	OnThink();

	MESSAGE_FUNC_PARAMS( OnTextChanged, "TextChanged", data );
	MESSAGE_FUNC_PTR( OnCheckButtonChecked, "CheckButtonChecked", panel );

private:
	MESSAGE_FUNC( OnSetFocus, "SetFocus" );

	void Close();
	void Render();
	void ValidateRenderData();
	void UpdateControlsValues();
	void AddControlToAutoLayout( Panel *pPanel, bool bAdvanced );
	void SetValuesFromQualityPreset();

	bool				m_bShowAdvancedOptions;	
	int					m_iQualityPreset;
	ReplayHandle_t		m_hReplay;
	bool				m_bSetQuit;
	int					m_iPerformance;
	CheckButton			*m_pPlayVoiceCheck;
	CheckButton			*m_pShowAdvancedOptionsCheck;
	CheckButton			*m_pQuitWhenDoneCheck;
	CheckButton			*m_pExportRawCheck;
	CExButton			*m_pCancelButton;
	CExButton			*m_pRenderButton;
	TextEntry			*m_pTitleText;
	ComboBox			*m_pVideoModesCombo;
	ComboBox			*m_pCodecCombo;
	CExLabel			*m_pQualityPresetLabel;
	ComboBox			*m_pQualityPresetCombo;
	CExLabel			*m_pResolutionNoteLabel;
	CExLabel			*m_pEnterANameLabel;
	CExLabel			*m_pVideoModeLabel;
	CExLabel			*m_pTitleLabel;
	CExLabel			*m_pLockWarningLabel;
	CExLabel			*m_pCodecLabel;
	CExLabel			*m_pEstimateTimeLabel;
	CExLabel			*m_pEstimateFileLabel;
	CheckButton			*m_pMotionBlurCheck;
	CExLabel			*m_pMotionBlurLabel;
	Slider				*m_pMotionBlurSlider;
	CExLabel			*m_pQualityLabel;
	Slider				*m_pQualitySlider;
	EditablePanel		*m_pBgPanel;
	Panel				*m_pSeparator;
	CheckButton			*m_pGlowEnabledCheck;

	struct LayoutInfo_t
	{
		Panel	*pPanel;
		int		nOffsetX;
		int		nOffsetY;
		bool	bAdvanced;
	};

	CUtlLinkedList< LayoutInfo_t * >		m_lstControls;
	CPanelAnimationVarAliasType( int, m_nStartY, "start_y", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nVerticalBuffer, "vertical_buffer", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nDefaultX, "default_x", "0", "proportional_xpos" );

	friend class CReplayGameStatsHelper;
};

//-----------------------------------------------------------------------------

void ReplayUI_ShowRenderDialog( Panel* pParent, ReplayHandle_t hReplay, bool bSetQuit, int iPerformance );

//-----------------------------------------------------------------------------

#endif // REPLAYBROWSER_RENDERDIALOG_H

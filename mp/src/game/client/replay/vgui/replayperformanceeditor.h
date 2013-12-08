//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#if defined( REPLAY_ENABLED )

#ifndef REPLAYPERFORMANCEEDITOR_H
#define REPLAYPERFORMANCEEDITOR_H
#ifdef _WIN32
#pragma once
#endif

#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ImagePanel.h"
#include "vgui_controls/ImageList.h"
#include "tf/vgui/tf_controls.h"
#include "replay/replayhandle.h"
#include "replay/ireplayperformanceeditor.h"
#include "replay/ireplayperformancecontroller.h"

//-----------------------------------------------------------------------------

class CPlayerCell;
class CCameraOptionsPanel;
class CRecLightPanel;
class CReplay;
class CReplayPerformance;
class CReplayTipLabel;
class CSavingDialog;

//-----------------------------------------------------------------------------

// NOTE: Should not change order here - if you do, you need to modify g_pCamNames.
enum CameraMode_t
{
	CAM_INVALID = -1,
	CAM_FREE,
	CAM_THIRD,
	CAM_FIRST,
	COMPONENT_TIMESCALE,
	NCAMS
};

//-----------------------------------------------------------------------------

class CReplayPerformanceEditorPanel : public vgui::EditablePanel,
									  public IReplayPerformanceEditor
{
	DECLARE_CLASS_SIMPLE( CReplayPerformanceEditorPanel, vgui::EditablePanel );
public:
	CReplayPerformanceEditorPanel( Panel *parent, ReplayHandle_t hReplay );
	virtual ~CReplayPerformanceEditorPanel();

	virtual void ShowPanel( bool bShow );

	bool OnEndOfReplayReached();
	void OnInGameMouseWheelEvent( int nDelta );
	void UpdateCameraSelectionPosition( CameraMode_t nCameraMode );
	void UpdateFreeCamSettings( const SetViewParams_t &params );
	void UpdateTimeScale( float flScale );
	void HandleUiToggle();
	void Exit();
	void Exit_ShowDialogs();

private:
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *pInResourceData );
	virtual void PerformLayout();
	virtual void OnCommand( const char *command );
	virtual void OnMouseWheeled( int nDelta );
	virtual void OnTick();

	void Achievements_Think( float flElapsed );
	void Achievements_OnSpaceBarPressed();
	void Achievements_Grant();

	friend class CReplayButton;
	friend class CSavingDialog;

	void SetButtonTip( wchar_t *pTipText, Panel *pContextPanel );
	void ShowButtonTip( bool bShow );

	void ShowSavingDialog();

	//
	// IReplayPerformanceEditor:
	//
	virtual CReplay *GetReplay();
	virtual void OnRewindComplete();

	// Called when the user attempts to change to a different camera, etc.
	// Returns true if request is immediately granted - false means the event
	// was queued and the user has been asked if they are OK with nuking any
	// changes after the current time.
	bool OnStateChangeRequested( const char *pEventStr );

	void EnsureRecording( bool bShouldSnip = true );	// Start recording now if not already doing so

	bool IsPaused();

	void UpdateCameraButtonImages( bool bForceUseUnselected = false );
	void LayoutPlayerCells();
	void SetupHighlightPanel( EditablePanel *pPanel, CPlayerCell *pPlayerCell );
	void UpdateTimeLabels();
	void ClearPlayerCellData();

	void HandleMouseWheel( int nDelta );

private:
	enum ControlButtons_t
	{
		CTRLBUTTON_IN,
		CTRLBUTTON_GOTOBEGINNING,
		CTRLBUTTON_REWIND,
		CTRLBUTTON_PLAY,
		CTRLBUTTON_FF,
		CTRLBUTTON_GOTOEND,
		CTRLBUTTON_OUT,

		NUM_CTRLBUTTONS
	};

	CReplayPerformance *GetPerformance() const;
	CReplayPerformance *GetSavedPerformance() const;

	int		GetCameraModeFromButtonIndex( CameraMode_t iCamera );
	void	AddSetViewEvent();
	void	AddTimeScaleEvent( float flTimeScale );
	void	AddPanelKeyboardInputDisableList( vgui::Panel *pPanel );
	CameraMode_t IsMouseOverActiveCameraOptionsPanel( int nMouseX, int nMouseY );
	void	SetOrRemoveInTick( int nTick, bool bRemoveIfSet );
	void	SetOrRemoveOutTick( int nTick, bool bRemoveIfSet );
	void	SetOrRemoveTick( int nTick, bool bUseInTick, bool bRemoveIfSet );
	void	ToggleMenu();
	void	OnMenuCommand_Save( bool bExitEditorWhenDone = false );
	void	OnMenuCommand_SaveAs( bool bExitEditorWhenDone = false );
	void	OnMenuCommand_Exit();
	void	DisplaySavedTip( bool bSucceess );
	void	OnSaveComplete();

	void	SaveAs( const wchar_t *pTitle );

	void	ShowRewindConfirmMessage();

	static void OnConfirmSaveAs( bool bShouldSave, wchar_t *pTitle, void *pContext );
	static void	OnConfirmDestroyChanges( bool bConfirmed, void *pContext );
	static void	OnConfirmDiscard( bool bConfirmed, void *pContext );
	static void OnConfirmExit( bool bConfirmed, void *pContext );
	static void	OnConfirmRewind( bool bConfirmed, void *pContext );

	MESSAGE_FUNC_PARAMS( OnSliderMoved, "SliderMoved", pParams );

	ReplayHandle_t		m_hReplay;

	float				m_flLastTime;	// Can't use gpGlobals->frametime when playback is paused
	float				m_flOldFps;

	CExLabel			*m_pCurTimeLabel;
	CExLabel			*m_pTotalTimeLabel;
	CExLabel			*m_pPlayerNameLabel;

	KeyValues			*m_pPlayerCellData;
	CPlayerCell			*m_pPlayerCells[2][MAX_PLAYERS+1];
	vgui::ImageList		*m_pImageList;

	EditablePanel		*m_pMouseTargetPanel;
	EditablePanel		*m_pBottom;
	CPlayerCell			*m_pCurTargetCell;

	CExImageButton		*m_pCameraButtons[NCAMS];
	CExImageButton		*m_pCtrlButtons[NUM_CTRLBUTTONS];

	float				m_flTimeScaleProxy;

	EditablePanel		*m_pPlayerCellsPanel;

	vgui::ImagePanel	*m_pCameraSelection;
	CameraMode_t		m_iCameraSelection;	// NOTE: Indexes into some arrays

	CReplayTipLabel		*m_pButtonTip;
	CSavingDialog		*m_pSavingDlg;

	enum MenuItems_t
	{
		MENU_SAVE,
		MENU_SAVEAS,
		MENU_EXIT,

		NUM_MENUITEMS
	};

	CExImageButton		*m_pMenuButton;
	vgui::Menu			*m_pMenu;
	int					m_aMenuItemIds[ NUM_MENUITEMS ];

	CExButton			*m_pSlowMoButton;

	CCameraOptionsPanel *m_pCameraOptionsPanels[NCAMS];

	CUtlLinkedList< vgui::Panel *, int >	m_lstDisableKeyboardInputPanels;

	int					m_nRedBlueLabelRightX;
	int					m_nBottomPanelStartY;
	int					m_nBottomPanelHeight;
	int					m_nRedBlueSigns[2];
	int					m_iCurPlayerTarget;
	float				m_flSpaceDownStart;		// The time at which user started holding down space bar
	bool				m_bSpaceDown;
	bool				m_bSpacePressed;
	int					m_nLastRoundedTime;
	bool				m_bMousePressed;
	bool				m_bMouseDown;
	float				m_flDefaultFramerate;	// host_framerate before perf editor started mucking about with it
	CameraMode_t		m_nMouseClickedOverCameraSettingsPanel;	// Allows user to drag slider outside of camera settings panel w/o the panel disappearing
	CRecLightPanel		*m_pRecLightPanel;
	bool				m_bShownAtLeastOnce;	// Has the replay editor shown at least once?  In other words, has the user hit the space bar at all yet?
	char				m_szSuspendedEvent[128];

	bool				m_bAchievementAwarded;	// Was an achievement awarded during this editing session?
	float				m_flLastTimeSpaceBarPressed;
	float				m_flActiveTimeInEditor;	// Will be zero'd out if user is idle (ie if they don't press space bar often enough)

	CPanelAnimationVarAliasType( int, m_nRightMarginWidth, "right_margin_width", "0", "proportional_xpos" );

	bool				m_bCurrentTargetNeedsVisibilityUpdate;
};

//-----------------------------------------------------------------------------

CReplayPerformanceEditorPanel *ReplayUI_InitPerformanceEditor( ReplayHandle_t hReplay );
CReplayPerformanceEditorPanel *ReplayUI_GetPerformanceEditor();
void ReplayUI_ClosePerformanceEditor();

//-----------------------------------------------------------------------------

#endif // REPLAYPERFORMANCEEDITOR_H

#endif
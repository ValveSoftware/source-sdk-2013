//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef REPLAYBROWSER_DETAILSPANEL_H
#define REPLAYBROWSER_DETAILSPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include "vgui_controls/EditablePanel.h"
#include "vgui_controls/ScrollableEditablePanel.h"
#include "replay/iqueryablereplayitem.h"
#include "replay/ireplaymovie.h"
#include "replay/replayhandle.h"
#include "replay/gamedefs.h"
#include "econ/econ_controls.h"

using namespace vgui;

//-----------------------------------------------------------------------------

#define NUM_CLASSES_IN_LOADOUT_PANEL		(TF_LAST_NORMAL_CLASS-1)		// We don't allow unlockables for the civilian

//-----------------------------------------------------------------------------
// Purpose: Forward declarations
//-----------------------------------------------------------------------------
class CExLabel;
class CExButton;
class CTFReplay;
class CReplayPerformance; 
class IReplayItemManager;

//-----------------------------------------------------------------------------
// Purpose: A panel containing 2 labels: one key, one value
//-----------------------------------------------------------------------------
class CKeyValueLabelPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CKeyValueLabelPanel, EditablePanel );
public:
	CKeyValueLabelPanel( Panel *pParent, const char *pKey, const char *pValue );
	CKeyValueLabelPanel( Panel *pParent, const char *pKey, const wchar_t *pValue );
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	int GetHeight() const;
	int GetValueHeight() const;

	void SetValue( const wchar_t *pValue );

private:
	CExLabel		*m_pLabels[2];
};

//-----------------------------------------------------------------------------
// Purpose: Base details panel with left/top padding and black border
//-----------------------------------------------------------------------------
class CBaseDetailsPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBaseDetailsPanel, EditablePanel );
public:
	CBaseDetailsPanel( Panel *pParent, const char *pName, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	int GetMarginSize() const { return XRES(6); }

	bool ShouldShow() const { return m_bShouldShow; }

protected:
	EditablePanel *GetInset() { return m_pInsetPanel; }

	ReplayHandle_t	m_hReplay;
	bool m_bShouldShow;

private:
	EditablePanel	*m_pInsetPanel;		// padding on left/top
};

//-----------------------------------------------------------------------------
// Purpose: Score panel - contains score & any records from the round
//-----------------------------------------------------------------------------
class CRecordsPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CRecordsPanel, CBaseDetailsPanel );
public:
	CRecordsPanel( Panel *pParent, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

private:
	ImagePanel	*m_pClassImage;
};

//-----------------------------------------------------------------------------
// Purpose: Stats panel
//-----------------------------------------------------------------------------
class CStatsPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CStatsPanel, CBaseDetailsPanel );
public:
	CStatsPanel( Panel *pParent, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

private:
	CKeyValueLabelPanel	*m_paStatLabels[ REPLAY_MAX_DISPLAY_GAMESTATS ];
};


//-----------------------------------------------------------------------------
// Purpose: Dominations panel
//-----------------------------------------------------------------------------
class CDominationsPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CDominationsPanel, CBaseDetailsPanel );
public:
	CDominationsPanel( Panel *pParent, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	ImagePanel			*m_pNumDominationsImage;
	CUtlVector< ImagePanel * > m_vecDominationImages;
};


//-----------------------------------------------------------------------------
// Purpose: Kills panel
//-----------------------------------------------------------------------------
class CKillsPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CKillsPanel, CBaseDetailsPanel );
public:
	CKillsPanel( Panel *pParent, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	CKeyValueLabelPanel *m_pKillLabels;
	CUtlVector< ImagePanel * > m_vecKillImages;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBasicLifeInfoPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CBasicLifeInfoPanel, CBaseDetailsPanel );
public:
	CBasicLifeInfoPanel( Panel *pParent, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

private:
	CKeyValueLabelPanel		*m_pKilledByLabels;
	CKeyValueLabelPanel		*m_pMapLabels;
	CKeyValueLabelPanel		*m_pLifeLabels;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMovieInfoPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CMovieInfoPanel, CBaseDetailsPanel );
public:
	CMovieInfoPanel( Panel *pParent, ReplayHandle_t hReplay, QueryableReplayItemHandle_t hMovie,
		IReplayItemManager *pItemManager );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

private:
	CKeyValueLabelPanel		*m_pRenderTimeLabels;
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CYouTubeInfoPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CYouTubeInfoPanel, CBaseDetailsPanel );
public:
	CYouTubeInfoPanel( Panel *pParent );

	virtual void PerformLayout();

	void SetInfo( const wchar_t *pInfo );

private:
	CKeyValueLabelPanel		*m_pLabels;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CTitleEditPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTitleEditPanel, EditablePanel );
public:
	CTitleEditPanel( Panel *pParent, QueryableReplayItemHandle_t hReplayItem, IReplayItemManager *pItemManager );
	~CTitleEditPanel();
	
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();
	virtual void PaintBackground();

	virtual void OnKeyCodeTyped(vgui::KeyCode code);

	virtual void OnTick();

	bool			m_bMouseOver;
	TextEntry		*m_pTitleEntry;
	ImagePanel		*m_pHeaderLine;
	CExLabel		*m_pClickToEditLabel;
	CExLabel		*m_pCaratLabel;
	QueryableReplayItemHandle_t	m_hReplayItem;
	IReplayItemManager	*m_pItemManager;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CReplayScreenshotSlideshowPanel;

class CPlaybackPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CPlaybackPanel, EditablePanel );
public:
	CPlaybackPanel( Panel *pParent );
	~CPlaybackPanel();

	virtual void FreeMovieMaterial() {}

protected:
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	inline int GetMarginSize()			{ return 9; }
	inline int GetViewWidth()			{ return GetWide() - 2 * GetMarginSize(); }
	inline int GetViewHeight()			{ return GetTall() - 2 * GetMarginSize(); }
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CPlaybackPanelSlideshow : public CPlaybackPanel
{
	DECLARE_CLASS_SIMPLE( CPlaybackPanelSlideshow, CPlaybackPanel );
public:
	CPlaybackPanelSlideshow( Panel *pParent, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

private:
	ReplayHandle_t						m_hReplay;
	CExLabel							*m_pNoScreenshotLabel;
	CReplayScreenshotSlideshowPanel		*m_pScreenshotImage;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMoviePlayerPanel;

class CPlaybackPanelMovie : public CPlaybackPanel
{
	DECLARE_CLASS_SIMPLE( CPlaybackPanelMovie, CPlaybackPanel );
public:
	CPlaybackPanelMovie( Panel *pParent, ReplayHandle_t hReplay );

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	virtual void FreeMovieMaterial();

private:
	CExLabel			*m_pLoadingLabel;
	CMoviePlayerPanel	*m_pMoviePlayerPanel;
	ReplayHandle_t		m_hMovie;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CCutImagePanel : public CExImageButton
{
	DECLARE_CLASS_SIMPLE( CCutImagePanel, CExImageButton );
public:
	CCutImagePanel( Panel *pParent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void SetSelected( bool bState );

private:
	virtual IBorder *GetBorder( bool bDepressed, bool bArmed, bool bSelected, bool bKeyFocus );

	IBorder		*m_pSelectedBorder;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CReplayDetailsPanel;

class CCutsPanel : public CBaseDetailsPanel
{
	DECLARE_CLASS_SIMPLE( CCutsPanel, CBaseDetailsPanel );
public:
	CCutsPanel( Panel *pParent, ReplayHandle_t hReplay, int iPerformance );
	~CCutsPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnCommand( const char *pCommand );
	virtual void ApplySettings( KeyValues *pInResourceData );

	void OnPerformanceDeleted( int iPerformance );

	CPanelAnimationVarAliasType( int, m_nCutButtonWidth, "cut_button_width", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nCutButtonHeight, "cut_button_height", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nCutButtonBuffer, "cut_button_buffer", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nCutButtonSpace, "cut_button_space", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nCutButtonSpaceWide, "cut_button_space_wide", "0", "proportional_xpos" );
	CPanelAnimationVarAliasType( int, m_nTopMarginHeight, "top_margin_height", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nNameLabelTopMargin, "name_label_top_margin", "0", "proportional_ypos" );
	CPanelAnimationVarAliasType( int, m_nButtonStartY, "button_start_y", "0", "proportional_ypos" );

	void UpdateNameLabel( int iPerformance );

private:
	void SelectButtonFromPerformance( int iPerformance );
	void SetPage( int iPage, int iButtonToSelect = 0 );
	int ButtonToPerformance( int iButton ) const;
	int PerformanceToButton( int iPerformance ) const;
	const CReplayPerformance *GetPerformance( int iPerformance ) const;

	virtual void OnTick();

	struct ButtonInfo_t
	{
		CExImageButton	*m_pButton;
		CExButton		*m_pAddToRenderQueueButton;
		int				m_iPerformance;
	};

	enum Consts_t
	{
		BUTTONS_PER_PAGE = 4
	};

	ButtonInfo_t					m_aButtons[ BUTTONS_PER_PAGE ];
	EditablePanel					*m_pVerticalLine;
	CExLabel						*m_pNoCutsLabel;
	CExLabel						*m_pOriginalLabel;
	CExLabel						*m_pCutsLabel;
	CExLabel						*m_pNameLabel;
	CExButton						*m_pPrevButton;
	CExButton						*m_pNextButton;
	int								m_iPage;
	int								m_nVisibleButtons;
	vgui::DHANDLE< CReplayDetailsPanel >	m_hDetailsPanel;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class IReplayItemManager;
class CConfirmDialog;
class CYouTubeGetStatsHandler;

class CReplayDetailsPanel : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CReplayDetailsPanel, EditablePanel );
public:
	CReplayDetailsPanel( Panel *pParent, QueryableReplayItemHandle_t hReplayItem, int iPerformance, IReplayItemManager *pItemManager );
	~CReplayDetailsPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	virtual void OnMousePressed( MouseCode code );
	virtual void OnKeyCodeTyped( KeyCode code );

	virtual void OnCommand( const char *pCommand );

	virtual void OnMessage( const KeyValues* pParams, VPANEL hFromPanel );

	EditablePanel *GetInset() { return m_pInsetPanel; }

	void ShowRenderDialog();
	void FreeMovieFileLock();
	void ShowExportDialog();

	static void	OnPlayerWarningDlgConfirm( bool bConfirmed, void *pContext );	

	enum eYouTubeStatus
	{
		kYouTubeStatus_Private,
		kYouTubeStatus_RetrievingInfo,
		kYouTubeStatus_RetrievedInfo,
		kYouTubeStatus_CouldNotRetrieveInfo,
		kYouTubeStatus_NotUploaded		
	};

	void SetYouTubeStatus( eYouTubeStatus status );

	EditablePanel		*m_pInsetPanel;				// Parent to most child panels listed here - narrower than screen width
	EditablePanel		*m_pInfoPanel;				// Container for info panels
	ScrollableEditablePanel *m_pScrollPanel;

	CPlaybackPanel		*m_pPlaybackPanel;			// Contains screenshot, playback button
	CRecordsPanel		*m_pRecordsPanel;				// Contains score, records
	CStatsPanel			*m_pStatsPanel;				// Contains stats
	CDominationsPanel	*m_pDominationsPanel;		// Dominations
	CBasicLifeInfoPanel *m_pBasicInfoPanel;			// Killed by, map, life
	CKillsPanel			*m_pKillsPanel;				// # kills, kill class icons
	CYouTubeInfoPanel	*m_pYouTubeInfoPanel;			// YouTube Info
	CCutsPanel			*m_pCutsPanel;				// Buttons for performances
	CUtlVector< CBaseDetailsPanel* >	m_vecInfoPanels;	// List of panels on the right
	CTitleEditPanel		*m_pTitleEditPanel;
	CExButton			*m_pBackButton;
	CExButton			*m_pDeleteButton;
	CExButton			*m_pRenderButton;
	CExButton			*m_pPlayButton;
	CExButton			*m_pExportMovie;
	CExButton			*m_pYouTubeUpload;
	CExButton			*m_pYouTubeView;
	CExButton			*m_pYouTubeShareURL;
	CExImageButton		*m_pShowRenderInfoButton;
	QueryableReplayItemHandle_t		m_hReplayItem;
	ReplayHandle_t		m_hReplay;
	IReplayItemManager	*m_pItemManager;
	int					m_iSelectedPerformance;		// Which performance to play/render/delete
	CYouTubeGetStatsHandler *m_pYouTubeResponseHandler;
	vgui::FileOpenDialog *m_hExportMovieDialog;

private:
	void ShowRenderInfo();

	MESSAGE_FUNC_PARAMS( OnConfirmDisconnect, "ConfirmDlgResult", data );
	MESSAGE_FUNC_CHARPTR( OnFileSelected, "FileSelected", fullpath );

	CPanelAnimationVarAliasType( int, m_nMarginWidth, "margin_width", "0", "proportional_xpos" );

	void GoBack();
	void ShowPlayConfirmationDialog();
};

#endif // REPLAYBROWSER_DETAILSPANEL_H

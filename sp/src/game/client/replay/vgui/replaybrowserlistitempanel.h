//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYLISTITEMPANEL_H
#define REPLAYLISTITEMPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include "replaybrowserbasepanel.h"
#include "replaybrowseritemmanager.h"
#include "replay/genericclassbased_replay.h"
#include "game_controls/slideshowpanel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: Slideshow panel that adds all screenshots associated 
// with a given replay.
//-----------------------------------------------------------------------------
class CReplayScreenshotSlideshowPanel : public CSlideshowPanel
{
	DECLARE_CLASS_SIMPLE( CReplayScreenshotSlideshowPanel, CSlideshowPanel );
public:
	CReplayScreenshotSlideshowPanel( Panel *pParent, const char *pName, ReplayHandle_t hReplay );

	virtual void PerformLayout();

private:
	ReplayHandle_t		m_hReplay;
};

//-----------------------------------------------------------------------------
// Purpose: An individual Replay thumbnail, with download button, title, etc.
//-----------------------------------------------------------------------------
class CExButton;
class CExLabel;
class IReplayItemManager;
class CMoviePlayerPanel;

class CReplayBrowserThumbnail : public CReplayBasePanel
{
	DECLARE_CLASS_SIMPLE( CReplayBrowserThumbnail, CReplayBasePanel );
public:
	CReplayBrowserThumbnail( Panel *pParent, const char *pName, QueryableReplayItemHandle_t hReplayItem, IReplayItemManager *pReplayItemManager );
	~CReplayBrowserThumbnail();

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();
	virtual void OnMousePressed( MouseCode code );

	virtual void OnTick();

	virtual void OnCommand( const char *pCommand );

	void UpdateTitleText();

	void SetReplayItem( QueryableReplayItemHandle_t hReplayItem );
	
	CGenericClassBasedReplay	*GetReplay();
	IQueryableReplayItem		*GetReplayItem();

	MESSAGE_FUNC_PARAMS( OnDownloadClicked, "Download", pParams );
	MESSAGE_FUNC_PARAMS( OnDeleteReplay, "delete_replayitem", pParams );

	CCrossfadableImagePanel			*m_pScreenshotThumb;
	QueryableReplayItemHandle_t		m_hReplayItem;

private:
	void SetupReplayItemUserData( void *pUserData );
	void UpdateProgress( bool bDownloadPhase, const CReplay *pReplay );

	Label				*m_pTitle;
	Label				*m_pDownloadLabel;
	Label				*m_pRecordingInProgressLabel;
	ProgressBar			*m_pDownloadProgress;
	CExButton			*m_pDownloadButton;
	CExButton			*m_pDeleteButton;
	Label				*m_pErrorLabel;
	CMoviePlayerPanel	*m_pMoviePlayer;
	Panel				*m_pDownloadOverlay;
	EditablePanel		*m_pBorderPanel;
	Color				m_clrHighlight;
	Color				m_clrDefaultBg;
	bool				m_bMouseOver;
	IReplayItemManager	*m_pReplayItemManager;
	float				m_flLastMovieScrubTime;
	float				m_flHoverStartTime;
	float				m_flLastProgressChangeTime;
};

//-----------------------------------------------------------------------------
// Purpose: A row of Replay thumbnails (CReplayBrowserThumbnail's)
//-----------------------------------------------------------------------------
class CReplayBrowserThumbnailRow : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CReplayBrowserThumbnailRow, EditablePanel );
public:
	CReplayBrowserThumbnailRow( Panel *pParent, const char *pName, IReplayItemManager *pReplayItemManager );

	void AddReplayThumbnail( const IQueryableReplayItem *pReplay );
	void AddReplayThumbnail( QueryableReplayItemHandle_t hReplayItem );
	void DeleteReplayItemThumbnail( const IQueryableReplayItem *pReplayItem );
	int GetNumReplayItems() const { return m_vecThumbnails.Count(); }
	int GetNumVisibleReplayItems() const;

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	CReplayBrowserThumbnail *FindThumbnail( const IQueryableReplayItem *pReplay );

	CUtlVector< CReplayBrowserThumbnail * > m_vecThumbnails;
	IReplayItemManager		*m_pReplayItemManager;
};

//-----------------------------------------------------------------------------
// Purpose: A collection of CReplayBrowserThumbnailRows containing replays
// recorded on a given day.
//-----------------------------------------------------------------------------
class CExLabel;
class CExButton;
class CReplayListPanel;

class CBaseThumbnailCollection : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CBaseThumbnailCollection, EditablePanel );
public:
	CBaseThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager );

	void			AddReplay( const IQueryableReplayItem *pItem );

	virtual bool	IsMovieCollection() const = 0;

	void			CleanupUIForReplayItem( ReplayItemHandle_t hReplayItem );

	virtual void	PerformLayout();
	virtual void	ApplySchemeSettings( IScheme *pScheme );

	void			RemoveEmptyRows();
	void			RemoveAll();

	void			OnUpdated();

	void			OnCommand( const char *pCommand );

	CReplayBrowserThumbnailRow *FindReplayItemThumbnailRow( const IQueryableReplayItem *pReplayItem );

	inline int		GetNumRows() const { return m_vecRows.Count(); }

	typedef CUtlVector< CReplayBrowserThumbnailRow * > RowContainer_t;
	RowContainer_t	m_vecRows;

protected:
	// Called from PerformLayout() - layout any panels that should appear at the top (vertically)-most position
	virtual void	LayoutUpperPanels( int nStartY, int nBgWidth ) = 0;
	virtual void	LayoutBackgroundPanel( int nWide, int nTall ) {}
	virtual Panel	*GetLowestPanel( int &nVerticalBuffer ) = 0;

	void			UpdateViewingPage( void );

	int				m_nStartX;

protected:
	CExLabel			*m_pNoReplayItemsLabel;
	IReplayItemManager	*m_pReplayItemManager;

	CExButton			*m_pShowNextButton;
	CExButton			*m_pShowPrevButton;
	CUtlVector<ReplayItemHandle_t>	m_vecReplays;
	int					m_iViewingPage;

	int					m_nReplayThumbnailsPerRow;
	int					m_nMaxRows;

	CExLabel			*m_pCaratLabel;
	CExLabel			*m_pTitleLabel;
	CExButton			*m_pRenderAllButton;

private:
	int GetRowStartY();

	CReplayListPanel	*m_pParentListPanel;		// Parent gets altered so we keep this cached ptr around
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CReplayThumbnailCollection : public CBaseThumbnailCollection
{
	DECLARE_CLASS_SIMPLE( CReplayThumbnailCollection, CBaseThumbnailCollection );
public:
	CReplayThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager );

	virtual bool	IsMovieCollection() const;

	virtual void	PerformLayout();
	virtual void	ApplySchemeSettings( IScheme *pScheme );

	virtual void	LayoutUpperPanels( int nStartY, int nBgWidth );
	virtual void	LayoutBackgroundPanel( int nWide, int nTall );
	virtual Panel	*GetLowestPanel( int &nVerticalBuffer );

	Panel			*m_pLinePanel;
	CExLabel		*m_pWarningLabel;
	Panel			*m_pUnconvertedBg;
};

#define OLDER_MOVIES_COLLECTION		-2

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CMovieThumbnailCollection : public CBaseThumbnailCollection
{
	DECLARE_CLASS_SIMPLE( CMovieThumbnailCollection, CBaseThumbnailCollection );
public:
	CMovieThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager,
								     int nDay, int nMonth, int nYear, bool bShowSavedMoviesLabel );
	CMovieThumbnailCollection( CReplayListPanel *pParent, const char *pName, IReplayItemManager *pReplayItemManager,
								     bool bShowSavedMoviesLabel );

	bool			DoesDateMatch( int nDay, int nMonth, int nYear );
	virtual bool	IsMovieCollection() const;

private:
	void			Init( int nDay, int nMonth, int nYear, bool bShowSavedMoviesLabel );
	virtual void	PerformLayout();
	virtual void	ApplySchemeSettings( IScheme *pScheme );

	Panel			*GetLowestPanel( int &nVerticalBuffer );
	void			LayoutUpperPanels( int nStartY, int nBgWidth );

	int				m_nDay, m_nMonth, m_nYear;
	CExLabel		*m_pDateLabel;
	bool			m_bShowSavedMoviesLabel;
};

#endif // REPLAYLISTITEMPANEL_H

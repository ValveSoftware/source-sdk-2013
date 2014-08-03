//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef REPLAYBROWSER_PREVIEWPANEL_H
#define REPLAYBROWSER_PREVIEWPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <game/client/iviewport.h>
#include "vgui_controls/PropertyPage.h"
#include "vgui_controls/Button.h"
#include "vgui_controls/PanelListPanel.h"
#include "vgui_controls/EditablePanel.h"
#include "replaybrowseritemmanager.h"
#include "replay/genericclassbased_replay.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CExLabel;
class CBaseThumbnailCollection;
class CReplayDetailsPanel;
class CReplayScreenshotSlideshowPanel;

//-----------------------------------------------------------------------------
// Purpose: Preview balloon
//-----------------------------------------------------------------------------
class CGenericClassBasedReplay;
class CCrossfadableImagePanel;
class CSlideshowPanel;	

class CReplayPreviewPanelBase : public EditablePanel
{
	DECLARE_CLASS_SIMPLE( CReplayPreviewPanelBase, EditablePanel );
public:
	CReplayPreviewPanelBase( Panel *pParent, QueryableReplayItemHandle_t hItem, IReplayItemManager *pItemManager );
	~CReplayPreviewPanelBase();
	
	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	ReplayHandle_t GetReplayHandle();

protected:
	CGenericClassBasedReplay *GetReplay();

	virtual bool ShoudlUseLargeClassImage() { return false; }
	virtual void LayoutView( int &nWide, int &nTall, int &nCurY );

protected:
	IReplayItemManager	*m_pItemManager;
	QueryableReplayItemHandle_t	m_hItem;

private:
	ImagePanel			*m_pClassImage;
	vgui::EditablePanel	*m_pInfoPanel;

	CExLabel			*m_pMapLabel;
	CExLabel			*m_pDateTimeLabel;

	enum ELabels
	{
		LABEL_PLAYED_AS,
		LABEL_KILLED_BY,
		LABEL_LIFE_LENGTH,
		NUM_INFO_LABELS
	};
	CExLabel			*m_pReplayInfoLabels[NUM_INFO_LABELS][2];
};

//-----------------------------------------------------------------------------
// Purpose: Preview balloon for slideshows (actual replays)
//-----------------------------------------------------------------------------
class CReplayPreviewPanelSlideshow : public CReplayPreviewPanelBase
{
	DECLARE_CLASS_SIMPLE( CReplayPreviewPanelSlideshow, CReplayPreviewPanelBase );
public:
	CReplayPreviewPanelSlideshow( Panel *pParent, QueryableReplayItemHandle_t hItem, IReplayItemManager *pItemManager );

private:
	virtual void PerformLayout();
	virtual void LayoutView( int &nWide, int &nTall, int &nCurY );

	CReplayScreenshotSlideshowPanel		*m_pScreenshotPanel;
	CExLabel							*m_pNoScreenshotLabel;
};

//-----------------------------------------------------------------------------
// Purpose: Preview balloon for movies (rendered replays)
//-----------------------------------------------------------------------------
class CMoviePlayerPanel;
class IReplayMovie;

class CReplayPreviewPanelMovie : public CReplayPreviewPanelBase
{
	DECLARE_CLASS_SIMPLE( CReplayPreviewPanelMovie, CReplayPreviewPanelBase );
public:
	CReplayPreviewPanelMovie( Panel *pParent, QueryableReplayItemHandle_t hItem, IReplayItemManager *pItemManager );
	~CReplayPreviewPanelMovie();

private:
	virtual void OnTick();
	virtual void LayoutView( int &nWide, int &nTall, int &nCurY );

	virtual IReplayMovie *GetReplayMovie();

	CMoviePlayerPanel	*m_pMoviePlayerPanel;
	float				m_flCreateTime;
};



#endif // REPLAYBROWSER_PREVIEWPANEL_H

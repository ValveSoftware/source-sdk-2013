//========= Copyright Valve Corporation, All rights reserved. ============//
//
//=======================================================================================//

#ifndef REPLAYBROWSER_LISTPANEL_H
#define REPLAYBROWSER_LISTPANEL_H
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
class CBaseThumbnailCollection;
class CReplayPreviewPanelBase;

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CReplayBrowserThumbnail;
class CExLabel;

class CReplayListPanel : public PanelListPanel
{
	DECLARE_CLASS_SIMPLE( CReplayListPanel, PanelListPanel );
public:
	CReplayListPanel( Panel *pParent, const char *pName );
	~CReplayListPanel();

	virtual void ApplySchemeSettings( IScheme *pScheme );
	virtual void PerformLayout();

	void AddReplayItem( ReplayItemHandle_t hItem );
	void CleanupUIForReplayItem( ReplayItemHandle_t hReplayItem );
	void AddReplaysToList();
	void RemoveCollection( CBaseThumbnailCollection *pCollection );

	virtual void OnTick();

	void OnItemPanelEntered( Panel *pPanel );
	void OnItemPanelExited( Panel *pPanel );

	void SetupBorderArrow( bool bLeft );

	void ClearPreviewPanel();

	void ApplyFilter( const wchar_t *pFilterText );

protected:
	virtual void OnMouseWheeled(int delta);

private:
	const IQueryableReplayItem *FindItem( ReplayItemHandle_t hItem, int *pItemManagerIndex );
	CBaseThumbnailCollection *FindOrAddReplayThumbnailCollection( const IQueryableReplayItem *pItem, IReplayItemManager *pItemManager );
	CReplayBrowserThumbnail *FindThumbnailAtCursor( int x, int y );
	bool PassesFilter( IQueryableReplayItem *pItem );

	CBaseThumbnailCollection	*m_pReplaysCollection;
	CBaseThumbnailCollection	*m_pMoviesCollection;

	CUtlVector< CBaseThumbnailCollection * > m_vecCollections;
	CReplayPreviewPanelBase	*m_pPreviewPanel;
	Panel					*m_pPrevHoverPanel;

	ImagePanel				*m_pBorderArrowImg;
	int						m_aBorderArrowDims[2];
	wchar_t					m_wszFilter[256];
};

#endif // REPLAYBROWSER_LISTPANEL_H

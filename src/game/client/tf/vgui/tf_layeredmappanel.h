//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Layered Map that contains a background and stateful locations.
//	Each location will support mouse over information
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_LAYEREDMAPPANEL_H
#define TF_LAYEREDMAPPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>
#include <vgui_controls/ScalableImagePanel.h>
#include <vgui_controls/Tooltip.h>
#include "item_model_panel.h"

class CTFLayeredMapPanel;
class CTFLayeredMapItemPanel;

//=========================================================
class CLayeredMapToolTip : public vgui::BaseTooltip 
{
	DECLARE_CLASS_SIMPLE( CLayeredMapToolTip, vgui::BaseTooltip );
public:
	CLayeredMapToolTip(vgui::Panel *parent, const char *text = NULL);

	void SetText(const char *text) { return; }
	const char *GetText() { return NULL; }

	virtual void PerformLayout();
	virtual void ShowTooltip( vgui::Panel *currentPanel );
	virtual void HideTooltip();

	void SetupPanels( CTFLayeredMapPanel *pParentPanel, vgui::EditablePanel *pControlledPanel );

private:
	void GetPosition( itempanel_tooltippos_t iTooltipPosition, CTFLayeredMapItemPanel *pMapItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos );
	bool ValidatePosition( CTFLayeredMapItemPanel *pItemPanel, int iItemX, int iItemY, int *iXPos, int *iYPos );

	vgui::DHANDLE<CTFLayeredMapItemPanel> m_hCurrentPanel;

	CTFLayeredMapPanel *m_pParentPanel;
	vgui::EditablePanel	*m_pControlledPanel;
};

//=========================================================
class CTFLayeredMapItemPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFLayeredMapItemPanel, vgui::EditablePanel );
public:
	CTFLayeredMapItemPanel( Panel *parent, const char *pName );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	// Button 
	virtual void OnCursorEntered();
	virtual void OnCursorExited();
	virtual void OnMousePressed(vgui::MouseCode code);
	
	//
	void SetCompletionState( bool bIsCompleted );
	KeyValues * GetItemKvData () { return m_kvData; }

private:

	vgui::ScalableImagePanel	*m_pIsCompleted;
	vgui::ScalableImagePanel	*m_pIsCompletedHighlight;

	vgui::ScalableImagePanel	*m_pNotCompleted;
	vgui::ScalableImagePanel	*m_pNotCompletedHighlight;

	bool m_bIsCompleted;
	bool m_bIsMouseOvered;

	KeyValues *m_kvData;
};

//=========================================================
class CTFLayeredMapPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFLayeredMapPanel, vgui::EditablePanel );
public:
	CTFLayeredMapPanel( Panel *parent, const char *pName );

	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	

	//CUtlVector<vgui::ImagePanel*> m_pImages;

private:

	EditablePanel				*m_pToolTipPanel;
	CLayeredMapToolTip			*m_pToolTip;

	KeyValues *m_pLayeredMapKv;

	CUtlVector<CTFLayeredMapItemPanel*> m_MapItems;
};

#endif // TF_LAYEREDIMAGEPANEL_H

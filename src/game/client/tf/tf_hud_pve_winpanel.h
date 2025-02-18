//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_PVE_WINPANEL_H
#define TF_HUD_PVE_WINPANEL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include <vgui/IScheme.h>
#include "hud.h"
#include "hudelement.h"
#include "tf_shareddefs.h"

using namespace vgui;

class CCreditDisplayPanel;


class CTFPVEWinPanel : public EditablePanel, public CGameEventListener, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFPVEWinPanel, EditablePanel );

public:
	CTFPVEWinPanel( IViewPort *pViewPort );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ApplySettings( KeyValues *inResourceData );
	virtual void FireGameEvent( IGameEvent * event );
	virtual void OnTick();

	virtual int GetRenderGroupPriority() { return 70; }

	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }
	virtual const char *GetName( void ){ return PANEL_PVE_WIN; }
	virtual void SetData( KeyValues *data ){ return; }
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow ){ };
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
	virtual bool IsVisible() { return BaseClass::IsVisible(); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_NONE; }

private:

	vgui::ScalableImagePanel *m_pRespecBackground;
	vgui::EditablePanel *m_pRespecContainerPanel;
	vgui::Label *m_pRespecTextLabel;
	vgui::Label *m_pRespecCountLabel;

	bool	m_bShouldBeVisible;
};

#endif //TF_HUD_PVE_WINPANEL_H

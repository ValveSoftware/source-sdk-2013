//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef INTROMENU_H
#define INTROMENU_H
#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/Frame.h>
#include <vgui_controls/Button.h>
#include <vgui_controls/Label.h>

#include <game/client/iviewport.h>

namespace vgui
{
	class TextEntry;
}

class CIntroMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CIntroMenu, vgui::Frame );

public:
	CIntroMenu( IViewPort *pViewPort );
	virtual ~CIntroMenu();

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual const char *GetName( void ){ return PANEL_INTRO; }
	virtual void SetData( KeyValues *data ){ return; }
	virtual void Reset();
	virtual void Update();
	virtual bool NeedsUpdate( void ) { return false; }
	virtual bool HasInputElements( void ) { return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ) { return BaseClass::GetVPanel(); }
  	virtual bool IsVisible() { return BaseClass::IsVisible(); }
  	virtual void SetParent( vgui::VPANEL parent ) { BaseClass::SetParent( parent ); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_IN_GAME_HUD; }

protected:	
	// vgui overrides
	virtual void OnCommand( const char *command );

	IViewPort		*m_pViewPort;
	vgui::Label		*m_pTitleLabel;
};

#endif // INTROMENU_H

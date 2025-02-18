//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_MAPINFOMENU_H
#define TF_MAPINFOMENU_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Frame.h>
#include "vgui_controls/KeyRepeat.h"

//-----------------------------------------------------------------------------
// Purpose: displays the MapInfo menu
//-----------------------------------------------------------------------------

class CTFMapInfoMenu : public vgui::Frame, public IViewPortPanel
{
private:
	DECLARE_CLASS_SIMPLE( CTFMapInfoMenu, vgui::Frame );

public:
	CTFMapInfoMenu( IViewPort *pViewPort );
	virtual ~CTFMapInfoMenu();

	virtual const char *GetName( void ){ return PANEL_MAPINFO; }
	virtual void SetData( KeyValues *data ){}
	virtual void Reset(){ Update(); }
	virtual void Update();
	virtual bool NeedsUpdate( void ){ return false; }
	virtual bool HasInputElements( void ){ return true; }
	virtual void ShowPanel( bool bShow );

	// both vgui::Frame and IViewPortPanel define these, so explicitly define them here as passthroughs to vgui
	vgui::VPANEL GetVPanel( void ){ return BaseClass::GetVPanel(); }
	virtual bool IsVisible(){ return BaseClass::IsVisible(); }
	virtual void SetParent( vgui::VPANEL parent ){ BaseClass::SetParent( parent ); }

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_IN_GAME_HUD; }

protected:
	virtual void OnKeyCodePressed(vgui::KeyCode code);
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void OnCommand( const char *command );
	virtual void OnKeyCodeReleased( vgui::KeyCode code );
	virtual void OnThink();
	
private:
	// helper functions
	void LoadMapPage();
	void SetMapTitle();
	bool HasViewedMovieForMap();
	bool CheckForIntroMovie();
	void CheckIntroState();
	void CheckBackContinueButtons();

protected:
	IViewPort			*m_pViewPort;
	CExLabel			*m_pTitle;
	CExRichText			*m_pMapInfo;

#ifdef _X360
	CTFFooter			*m_pFooter;
#else
	CExButton			*m_pContinue;
	CExButton			*m_pBack;
	CExButton			*m_pIntro;
	CSCHintIcon			*m_pContinueHintIcon;
	CSCHintIcon			*m_pBackHintIcon;
	CSCHintIcon			*m_pIntroHintIcon;
#endif

	vgui::ImagePanel	*m_pMapImage;

	char				m_szMapName[MAX_PATH];

	vgui::CKeyRepeatHandler	m_KeyRepeat;
};


#endif // TF_MAPINFOMENU_H

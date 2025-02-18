//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TFTEXTWINDOW_H
#define TFTEXTWINDOW_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/Panel.h>
#include "vguitextwindow.h"
#include "tf_controls.h"
#include "IconPanel.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// Purpose: displays the MOTD
//-----------------------------------------------------------------------------

class CTFTextWindow : public CTextWindow
{
private:
	DECLARE_CLASS_SIMPLE( CTFTextWindow, CTextWindow );

public:
	CTFTextWindow( IViewPort *pViewPort );
	virtual ~CTFTextWindow();

	virtual void SetData(KeyValues *data);
	virtual void Update();
	virtual void Reset();
	virtual void SetVisible(bool state);
	virtual void ShowPanel( bool bShow );
	virtual void OnKeyCodePressed( vgui::KeyCode code );
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );
	virtual void ShowFile( const char *filename );
	virtual void ShowURL( const char *URL, bool bAllowUserToDisable = true );
	virtual void ShowText( const char *text );
	void ShowTitleLabel( bool show );
	virtual void OnThink();

	virtual GameActionSet_t GetPreferredActionSet() { return GAME_ACTION_SET_IN_GAME_HUD; }

public:
	virtual void PaintBackground();

protected:
	// vgui overrides
	virtual void OnCommand( const char *command );

private:
	CExRichText		*m_pTFTextMessage;
	bool			m_bCustomSvrPage;
};


#endif // TFTEXTWINDOW_H

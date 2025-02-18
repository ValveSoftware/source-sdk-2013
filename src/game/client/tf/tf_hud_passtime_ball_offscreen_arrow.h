//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_PASSTIME_BALL_OFFSCREEN_ARROW_H
#define TF_HUD_PASSTIME_BALL_OFFSCREEN_ARROW_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui_controls/EditablePanel.h>
#include <game/client/iviewport.h>
#include "ehandle.h"

class C_TFPlayer;

//-----------------------------------------------------------------------------
class CTFHudPasstimeOffscreenArrow : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CTFHudPasstimeOffscreenArrow, vgui::EditablePanel );
public:
	CTFHudPasstimeOffscreenArrow( vgui::Panel *pParent, const char *name );
	~CTFHudPasstimeOffscreenArrow();
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme ) OVERRIDE;
	virtual void PaintBackground() OVERRIDE;
	
protected:
	virtual C_BaseEntity *PreparePaint( vgui::ImagePanel *pImage, C_TFPlayer *pLocalPlayer ) = 0;

private:
	IMaterial *m_pArrowMaterial;
	vgui::ImagePanel *m_pImage;
};

//-----------------------------------------------------------------------------
class CTFHudPasstimeBallOffscreenArrow : public CTFHudPasstimeOffscreenArrow
{
	DECLARE_CLASS_SIMPLE( CTFHudPasstimeBallOffscreenArrow, CTFHudPasstimeOffscreenArrow );
public:
	CTFHudPasstimeBallOffscreenArrow( vgui::Panel *pParent );
private:
	virtual C_BaseEntity *PreparePaint( vgui::ImagePanel *pImage, C_TFPlayer *pLocalPlayer ) OVERRIDE;
};

//-----------------------------------------------------------------------------
class CTFHudPasstimePlayerOffscreenArrow : public CTFHudPasstimeOffscreenArrow
{
	DECLARE_CLASS_SIMPLE( CTFHudPasstimePlayerOffscreenArrow, CTFHudPasstimeOffscreenArrow );
public:
	CTFHudPasstimePlayerOffscreenArrow( vgui::Panel *pParent, int iPlayerIndex );
private:
	virtual C_BaseEntity *PreparePaint( vgui::ImagePanel *pImage, C_TFPlayer *pLocalPlayer ) OVERRIDE;

	int m_iPlayerIndex;
};

#endif // TF_HUD_PASSTIME_BALL_OFFSCREEN_ARROW_H  

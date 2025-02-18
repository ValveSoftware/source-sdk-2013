//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef TF_HUD_MANN_VS_MACHINE_STATS_H
#define TF_HUD_MANN_VS_MACHINE_STATS_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include "tf_controls.h"
#include "hud.h"
#include <vgui/IScheme.h>
#include <vgui_controls/EditablePanel.h>

//=========================================================
class CCreditDisplayPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCreditDisplayPanel, vgui::EditablePanel );
public:
	CCreditDisplayPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

};

//=========================================================
class CCreditSpendPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_SIMPLE( CCreditSpendPanel, vgui::EditablePanel );
public:
	CCreditSpendPanel( Panel *parent, const char *pName );

	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

};


#endif // TF_HUD_MANN_VS_MACHINE_STATS_H

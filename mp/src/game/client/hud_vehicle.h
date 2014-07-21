//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef HUD_CROSSHAIR_H
#define HUD_CROSSHAIR_H
#ifdef _WIN32
#pragma once
#endif

#include "hudelement.h"
#include <vgui_controls/Panel.h>

namespace vgui
{
	class IScheme;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CHudVehicle : public CHudElement, public vgui::Panel
{
	DECLARE_CLASS_SIMPLE( CHudVehicle, vgui::Panel );
public:
	CHudVehicle( const char *pElementName );

	virtual bool	ShouldDraw();

	virtual void	ApplySchemeSettings( vgui::IScheme *scheme );
	virtual void	Paint( void );

private:

	IClientVehicle	*GetLocalPlayerVehicle();
};

#endif // HUD_CROSSHAIR_H

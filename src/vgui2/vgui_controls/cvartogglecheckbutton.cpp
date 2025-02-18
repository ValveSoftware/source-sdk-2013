//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "tier1/KeyValues.h"

#include <vgui/ISurface.h>
#include <vgui/IScheme.h>
#include <vgui_controls/cvartogglecheckbutton.h>

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

using namespace vgui;

vgui::Panel *Create_CvarToggleCheckButton()
{
	return new CvarToggleCheckButton< UIConVarRef >( NULL, NULL );
}

DECLARE_BUILD_FACTORY_CUSTOM_ALIAS( CvarToggleCheckButton<UIConVarRef>, CvarToggleCheckButton, Create_CvarToggleCheckButton );


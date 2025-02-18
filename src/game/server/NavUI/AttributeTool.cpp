//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "AttributeTool.h"
#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SERVER_USES_VGUI

using namespace vgui;


//--------------------------------------------------------------------------------------------------------
AttributeToolPanel::AttributeToolPanel( vgui::Panel *parent, const char *toolName ) : CNavUIToolPanel( parent, toolName )
{
	LoadControlSettings( "Resource/UI/NavTools/AttributeTool.res" );
}

#endif // SERVER_USES_VGUI

//--------------------------------------------------------------------------------------------------------

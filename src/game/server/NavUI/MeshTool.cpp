//--------------------------------------------------------------------------------------------------------
//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"

#include "MeshTool.h"
#include "nav_mesh.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#ifdef SERVER_USES_VGUI

using namespace vgui;


//--------------------------------------------------------------------------------------------------------
MeshToolPanel::MeshToolPanel( vgui::Panel *parent, const char *toolName ) : CNavUIToolPanel( parent, toolName )
{
	LoadControlSettings( "Resource/UI/NavTools/MeshTool.res" );
}

#endif // SERVER_USES_VGUI

//--------------------------------------------------------------------------------------------------------

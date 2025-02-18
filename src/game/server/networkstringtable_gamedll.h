//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef NETWORKSTRINGTABLE_GAMEDLL_H
#define NETWORKSTRINGTABLE_GAMEDLL_H
#ifdef _WIN32
#pragma once
#endif

#include "networkstringtabledefs.h"

class CStringTableSaveRestoreOps;

// String tables used by the game DLL
#define MAX_VGUI_SCREEN_STRING_BITS		8
#define MAX_VGUI_SCREEN_STRINGS			( 1 << MAX_VGUI_SCREEN_STRING_BITS )
#define VGUI_SCREEN_INVALID_STRING		( MAX_VGUI_SCREEN_STRINGS - 1 )

#define MAX_MATERIAL_STRING_BITS		10
#define MAX_MATERIAL_STRINGS			( 1 << MAX_MATERIAL_STRING_BITS )
#define OVERLAY_MATERIAL_INVALID_STRING	( MAX_MATERIAL_STRINGS - 1 )

#define MAX_CHOREO_SCENES_STRING_BITS	13
#define MAX_CHOREO_SCENES_STRINGS		( 1 << MAX_CHOREO_SCENES_STRING_BITS )
#define CHOREO_SCENES_INVALID_STRING	( MAX_CHOREO_SCENES_STRINGS - 1 )

#define MAX_PARTICLESYSTEMS_STRING_BITS		12
#define MAX_PARTICLESYSTEMS_STRINGS			( 1 << MAX_PARTICLESYSTEMS_STRING_BITS )
#define PARTICLESYSTEMS_INVALID_STRING	( MAX_PARTICLESYSTEMS_STRINGS - 1 )

extern INetworkStringTableContainer *networkstringtable;
extern INetworkStringTable *g_pStringTableVguiScreen;
extern INetworkStringTable *g_pStringTableEffectDispatch;
extern INetworkStringTable *g_pStringTableClientSideChoreoScenes;

#define MAX_INFOPANEL_STRINGS			128

// save/load
extern CStringTableSaveRestoreOps g_VguiScreenStringOps;


#endif // NETWORKSTRINGTABLE_GAMEDLL_H

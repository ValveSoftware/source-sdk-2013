//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//===========================================================================//

#ifndef NETWORKSTRINGTABLE_CLIENTDLL_H
#define NETWORKSTRINGTABLE_CLIENTDLL_H
#ifdef _WIN32
#pragma once
#endif

#include "networkstringtabledefs.h"

extern INetworkStringTableContainer *networkstringtable;

// String tables used by the client DLL	
// (see InstallStringTableCallback for where they're initialized)
extern INetworkStringTable *g_StringTableVguiScreen;
extern INetworkStringTable *g_StringTableEffectDispatch;
extern INetworkStringTable *g_StringTableMaterials;
extern INetworkStringTable *g_pStringTableInfoPanel;
extern INetworkStringTable *g_pStringTableClientSideChoreoScenes;
extern INetworkStringTable *g_pStringTableServerMapCycle;

#ifdef TF_CLIENT_DLL
extern INetworkStringTable *g_pStringTableServerPopFiles;
extern INetworkStringTable *g_pStringTableServerMapCycleMvM;
#endif

#endif // NETWORKSTRINGTABLE_CLIENTDLL_H

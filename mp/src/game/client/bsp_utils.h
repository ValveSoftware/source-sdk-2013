//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Exposes bsp tools to game for e.g. workshop use
//
// $NoKeywords: $
//===========================================================================//

#include "../utils/common/bsplib.h"
#include "ibsppack.h"

// Loads bsppack module (IBSPPack) and calls RepackBSP()
bool BSP_SyncRepack( const char *pszInputMapFile,
                     const char *pszOutputMapFile,
                     IBSPPack::eRepackBSPFlags eRepackFlags = (IBSPPack::eRepackBSPFlags) ( IBSPPack::eRepackBSP_CompressLumps |
                                                                                            IBSPPack::eRepackBSP_CompressPackfile ) );

// Helper to spawn a background thread that runs SyncRepack
void BSP_BackgroundRepack( const char *pszInputMapFile,
                           const char *pszOutputMapFile,
                           IBSPPack::eRepackBSPFlags eRepackFlags = (IBSPPack::eRepackBSPFlags) ( IBSPPack::eRepackBSP_CompressLumps |
                                                                                                  IBSPPack::eRepackBSP_CompressPackfile ) );

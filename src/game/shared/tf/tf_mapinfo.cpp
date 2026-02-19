//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================
#include "cbase.h"

#include <filesystem.h>
#include "GameEventListener.h"
#include "econ_item_system.h"
#include "tf_item_inventory.h"
#include "econ_contribution.h"
#include "tf_duel_summary.h"
#include "gc_clientsystem.h"
#include "tf_gamerules.h"

#ifdef CLIENT_DLL
#include "usermessages.h"
#endif // CLIENT_DLL

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>
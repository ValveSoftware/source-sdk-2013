//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "ai_speechconcept.h"

#ifdef GAME_DLL
#include "game.h"
#include "ai_basenpc.h"
#include "sceneentity.h"
#endif

#include "engine/ienginesound.h"
#include "keyvalues.h"
#include "ai_criteria.h"
#include "isaverestore.h"


// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>


// empty
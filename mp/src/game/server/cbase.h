//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef CBASE_H
#define CBASE_H
#ifdef _WIN32
#pragma once
#endif

#ifdef _WIN32
// Silence certain warnings
#pragma warning(disable : 4244)		// int or float down-conversion
#pragma warning(disable : 4305)		// int or float data truncation
#pragma warning(disable : 4201)		// nameless struct/union
#pragma warning(disable : 4511)     // copy constructor could not be generated
#pragma warning(disable : 4675)     // resolved overload was found by argument dependent lookup
#endif

#ifdef _DEBUG
#define DEBUG 1
#endif

// Misc C-runtime library headers
#include <math.h>

#include <stdio.h>

// tier 0
#include "tier0/dbg.h"
#include "tier0/platform.h"
#include "basetypes.h"

// tier 1
#include "tier1/strtools.h"
#include "utlvector.h"
#include "mathlib/vmatrix.h"

// tier 2
#include "string_t.h"

// tier 3
#include "vphysics_interface.h"

// Shared engine/DLL constants
#include "const.h"
#include "edict.h"

// Shared header describing protocol between engine and DLLs
#include "eiface.h"
#include "iserverentity.h"

#include "dt_send.h"

// Shared header between the client DLL and the game DLLs
#include "shareddefs.h"
#include "ehandle.h"

// app
#if defined(_X360)
#define DISABLE_DEBUG_HISTORY 1
#endif


#include "datamap.h"
#include "util.h"
#include "predictable_entity.h"
#include "predictableid.h"
#include "variant_t.h"
#include "takedamageinfo.h"
#include "utllinkedlist.h"
#include "touchlink.h"
#include "groundlink.h"
#include "base_transmit_proxy.h"
#include "soundflags.h"
#include "networkvar.h"
#include "baseentity_shared.h"
#include "basetoggle.h"
#include "igameevents.h"

// saverestore.h declarations
class ISave;
class IRestore;

// maximum number of targets a single multi_manager entity may be assigned.
#define MAX_MULTI_TARGETS	16 

// NPCEvent.h declarations
struct animevent_t;

struct studiohdr_t;
class CStudioHdr;

extern void FireTargets( const char *targetName, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

// people gib if their health is <= this at the time of death
#define	GIB_HEALTH_VALUE	-30

#define MAX_OLD_ENEMIES		4 // how many old enemies to remember

// used by suit voice to indicate damage sustained and repaired type to player

enum
{
	itbd_Paralyze = 0,
	itbd_NerveGas,
	itbd_PoisonRecover,
	itbd_Radiation,
	itbd_DrownRecover,
	itbd_Acid,
	itbd_SlowBurn,
	itbd_SlowFreeze,

	// Must be last!
	CDMG_TIMEBASED
};

// when calling KILLED(), a value that governs gib behavior is expected to be 
// one of these three values
#define GIB_NORMAL			0// gib if entity was overkilled
#define GIB_NEVER			1// never gib, no matter how much death damage is done ( freezing, etc )
#define GIB_ALWAYS			2// always gib

class CAI_BaseNPC;
class CAI_ScriptedSequence;
class CSound;

#ifdef _XBOX
//#define FUNCTANK_AUTOUSE  We haven't made the decision to use this yet (sjb)
#else
#undef FUNCTANK_AUTOUSE
#endif//_XBOX

// This is a precompiled header.  Include a bunch of common stuff.
// This is kind of ugly in that it adds a bunch of dependency where it isn't needed.
// But on balance, the compile time is much lower (even incrementally) once the precompiled
// headers contain these headers.
#include "precache_register.h"
#include "baseanimating.h"
#include "basecombatweapon.h"
#include "basecombatcharacter.h"
#include "gamerules.h"
#include "entitylist.h"
#include "basetempentity.h"
#include "player.h"
#include "te.h"
#include "physics.h"
#include "ndebugoverlay.h"
#include "recipientfilter.h"

#endif // CBASE_H

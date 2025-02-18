//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef PROCEDURALPRESETS_H
#define PROCEDURALPRESETS_H
#ifdef _WIN32
#pragma once
#endif

enum
{
	PROCEDURAL_PRESET_NOT = 0,
	PROCEDURAL_PRESET_IN_CROSSFADE,
	PROCEDURAL_PRESET_OUT_CROSSFADE,
	PROCEDURAL_PRESET_REVEAL,
	PROCEDURAL_PRESET_PASTE,
	PROCEDURAL_PRESET_JITTER,
	PROCEDURAL_PRESET_SMOOTH,
	PROCEDURAL_PRESET_SHARPEN,
	PROCEDURAL_PRESET_SOFTEN,
	PROCEDURAL_PRESET_STAGGER,

	// Must be last
	NUM_PROCEDURAL_PRESET_TYPES,
};

#endif // PROCEDURALPRESETS_H

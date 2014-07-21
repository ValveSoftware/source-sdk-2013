//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SCRATCHPAD_GAMEDLL_HELPERS_H
#define SCRATCHPAD_GAMEDLL_HELPERS_H
#ifdef _WIN32
#pragma once
#endif


class IScratchPad3D;


#define SPDRAWWORLD_DRAW_WORLD				0x001
#define SPDRAWWORLD_DRAW_PLAYERS			0x002	// Draw player entities?
#define SPDRAWWORLD_DRAW_ENTITIES			0x004	// Draw entities other than players?
#define SPDRAWWORLD_DRAW_ENTITY_CLASSNAMES	0x008
#define SPDRAWWORLD_DRAW_EDICT_INDICES		0x010

#define SPDRAWWORLD_DRAW_ALL			0xFFFFFFFF


// Draws the world and various things in it into the scratchpad.
// flags is a combination of the SPDRAWWORLD_ flags.
void ScratchPad_DrawWorldToScratchPad(
	IScratchPad3D *pPad,
	unsigned long flags );

// Draw a specific entity into the scratch pad.
void ScratchPad_DrawEntityToScratchPad(
	IScratchPad3D *pPad,
	unsigned long flags,
	CBaseEntity *pEnt,
	const Vector &vColor );


#endif // SCRATCHPAD_GAMEDLL_HELPERS_H

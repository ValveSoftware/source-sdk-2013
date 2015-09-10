//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COMBINEOPERATIONS_H
#define COMBINEOPERATIONS_H
#pragma once

// New combines can be written in the middle (and generally should be written before Error).
// Keep these in sync with cCombineMaterialName in ctexturecompositor.cpp
enum ECombineOperation
{
	ECO_Multiply = 0,
	ECO_Add,
	ECO_Lerp,

	ECO_Select,

	ECO_Legacy_Lerp_FirstPass,
	ECO_Legacy_Lerp_SecondPass,

	ECO_Error,
	ECO_COUNT
};

#endif /* COMBINEOPERATIONS_H */

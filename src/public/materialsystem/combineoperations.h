//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef COMBINEOPERATIONS_H
#define COMBINEOPERATIONS_H
#pragma once

// New combines must be added between FirstPrecacheMaterial and LastPrecacheMaterial.
// Keep these in sync with cCombineMaterialName in ctexturecompositor.cpp
enum ECombineOperation
{
	ECO_FirstPrecacheMaterial = 0,
	ECO_Multiply = 0,
	ECO_Add = 1,
	ECO_Lerp = 2,

	ECO_Select = 3,

	ECO_Legacy_Lerp_FirstPass = 4,	// Must be 4, or shaders need to be updated!
	ECO_Legacy_Lerp_SecondPass = 5, // Must be 5, or shaders need to be updated!

	ECO_Blend = 6,

	ECO_LastPrecacheMaterial,

	ECO_Error,

	ECO_COUNT

};

#endif /* COMBINEOPERATIONS_H */

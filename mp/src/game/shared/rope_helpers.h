//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ROPE_HELPERS_H
#define ROPE_HELPERS_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib/vector.h"


//
// This function can help you choose starting conditions for your rope. It is fairly
// expensive (slightly less than 0.5ms for a 10-node rope), but it's within reason for the 
// frequency we create ropes at.
//
// Input: 
//    - rope endpoints
//    - the number of nodes the client will be simulating (CRopeKeyframe::m_nSegments)
//    - how low you want the rope to hang (below the lowest of the two endpoints)
//
// Output: 
//	  - pOutputLength = length of the rope
//    - pOutputSlack  = slack you should set to produce the desired hang
//
void CalcRopeStartingConditions( 
	const Vector &vStartPos,
	const Vector &vEndPos,
	int const nNodes,
	float const desiredHang,
	float *pOutputLength,
	float *pOutputSlack
	);


#endif // ROPE_HELPERS_H

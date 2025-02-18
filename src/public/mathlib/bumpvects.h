//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef BUMPVECTS_H
#define BUMPVECTS_H

#ifdef _WIN32
#pragma once
#endif

#include "mathlib/mathlib.h"

#define OO_SQRT_2 0.70710676908493042f
#define OO_SQRT_3 0.57735025882720947f
#define OO_SQRT_6 0.40824821591377258f
// sqrt( 2 / 3 )
#define OO_SQRT_2_OVER_3 0.81649661064147949f

#define NUM_BUMP_VECTS 3

const TableVector g_localBumpBasis[NUM_BUMP_VECTS] = 
{
	{	OO_SQRT_2_OVER_3, 0.0f, OO_SQRT_3 },
	{  -OO_SQRT_6, OO_SQRT_2, OO_SQRT_3 },
	{  -OO_SQRT_6, -OO_SQRT_2, OO_SQRT_3 }
};

void GetBumpNormals( const Vector& sVect, const Vector& tVect, const Vector& flatNormal, 
					 const Vector& phongNormal, Vector bumpNormals[NUM_BUMP_VECTS] );

#endif // BUMPVECTS_H

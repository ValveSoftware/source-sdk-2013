//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#if !defined(_STATIC_LINKED) || defined(_SHARED_LIB)


#ifdef QUIVER
#include "r_local.h"
#endif
#include "mathlib/bumpvects.h"
#include "mathlib/vector.h"
#include <assert.h>

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// z is coming out of the face.

void GetBumpNormals( const Vector& sVect, const Vector& tVect, const Vector& flatNormal, 
					 const Vector& phongNormal, Vector bumpNormals[NUM_BUMP_VECTS] )
{
	Vector tmpNormal;
	bool leftHanded;
	int i;

	assert( NUM_BUMP_VECTS == 3 );
	
	// Are we left or right handed?
	CrossProduct( sVect, tVect, tmpNormal );
	if( DotProduct( flatNormal, tmpNormal ) < 0.0f )
	{
		leftHanded = true;
	}
	else
	{
		leftHanded = false;
	}

	// Build a basis for the face around the phong normal
	matrix3x4_t smoothBasis;
	CrossProduct( phongNormal.Base(), sVect.Base(), smoothBasis[1] );
	VectorNormalize( smoothBasis[1] );
	CrossProduct( smoothBasis[1], phongNormal.Base(), smoothBasis[0] );
	VectorNormalize( smoothBasis[0] );
	VectorCopy( phongNormal.Base(), smoothBasis[2] );
	
	if( leftHanded )
	{
		VectorNegate( smoothBasis[1] );
	}
	
	// move the g_localBumpBasis into world space to create bumpNormals
	for( i = 0; i < 3; i++ )
	{
		VectorIRotate( g_localBumpBasis[i], smoothBasis, bumpNormals[i] );
	}
}

#endif // !_STATIC_LINKED || _SHARED_LIB

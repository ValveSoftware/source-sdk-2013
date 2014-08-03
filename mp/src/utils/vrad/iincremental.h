//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IINCREMENTAL_H
#define IINCREMENTAL_H
#ifdef _WIN32
#pragma once
#endif


#include "mathlib/vector.h"
#include "utlvector.h"


typedef unsigned short IncrementalLightID;


// Incremental lighting manager.
class IIncremental
{
// IIncremental overrides.
public:

	virtual				~IIncremental() {}

	// Sets up for incremental mode. The BSP file (in bsplib) should be loaded
	// already so it can detect if the incremental file is up to date.
	virtual bool		Init( char const *pBSPFilename, char const *pIncrementalFilename ) = 0;

	// Prepare to light. You must call Init once, but then you can
	// do as many Prepare/AddLight/Finalize phases as you want.
	virtual bool		PrepareForLighting() = 0;

	// Called every time light is added to a face.
	// NOTE: This is the ONLY threadsafe function in IIncremental.
	virtual void		AddLightToFace( 
		IncrementalLightID lightID, 
		int iFace, 
		int iSample,
		int lmSize,
		float dot,
		int iThread ) = 0;

	// Called when it's done applying light from the specified light to the specified face.
	virtual void		FinishFace (
		IncrementalLightID lightID,
		int iFace,
		int iThread ) = 0;

	// For each face that was changed during the lighting process, save out
	// new data for it in the incremental file.
	// Returns false if the incremental lighting isn't active.
	virtual bool		Finalize() = 0;

	// Grows touched to a size of 'numfaces' and sets each byte to 0 or 1 telling
	// if the face's lightmap was updated in Finalize.
	virtual void		GetFacesTouched( CUtlVector<unsigned char> &touched ) = 0;

	// This saves the .r0 file and updates the lighting in the BSP file.
	virtual bool		Serialize() = 0;
};


extern IIncremental* GetIncremental();


#endif // IINCREMENTAL_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ISOUNDCOMBINER_H
#define ISOUNDCOMBINER_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"

class IFileSystem;

struct CombinerEntry
{
	CombinerEntry()
	{
		wavefile[ 0 ] = 0;
		startoffset = 0.0f;
	}

	char	wavefile[ MAX_PATH ];
	float	startoffset;
};

abstract_class ISoundCombiner
{
public:
	virtual ~ISoundCombiner() {}

	virtual bool CombineSoundFiles( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info ) = 0;
	virtual bool IsCombinedFileChecksumValid( IFileSystem *filesystem, char const *outfile, CUtlVector< CombinerEntry >& info ) = 0;
};

extern ISoundCombiner *soundcombiner;

#endif // ISOUNDCOMBINER_H

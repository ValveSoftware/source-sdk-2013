//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef KVPACKER_H
#define KVPACKER_H

#ifdef _WIN32
#pragma once
#endif

#include "KeyValues.h"

//-----------------------------------------------------------------------------
// Purpose: Handles packing KeyValues binary packing and unpacking in a a
//			consistent way across all branches, including Steam. If you change
//			this packing format you need to do so in a backward compatible way
//			so that the Steam servers and all the various GCs can still talk to
//			each other.
//-----------------------------------------------------------------------------
class KVPacker
{
public:
	bool WriteAsBinary( KeyValues *pNode, CUtlBuffer &buffer );
	bool ReadAsBinary( KeyValues *pNode, CUtlBuffer &buffer );

private:
	// These types are used for serialization of KeyValues nodes.
	// Do not renumber them or you will break serialization across
	// branches.
	enum EPackType
	{
		PACKTYPE_NONE = 0,
		PACKTYPE_STRING,
		PACKTYPE_INT,
		PACKTYPE_FLOAT,
		PACKTYPE_PTR,
		PACKTYPE_WSTRING,
		PACKTYPE_COLOR,
		PACKTYPE_UINT64,
		PACKTYPE_NULLMARKER,				// used to mark the end of a block in the binary format
	};
};


#endif // KVPACKER_H

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PHYFILE_H
#define PHYFILE_H
#pragma once

#include "datamap.h"

typedef struct phyheader_s
{
	DECLARE_BYTESWAP_DATADESC();
	int		size;
	int		id;
	int		solidCount;
	int32	checkSum;	// checksum of source .mdl file
} phyheader_t;

#endif // PHYFILE_H

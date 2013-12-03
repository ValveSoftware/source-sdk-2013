//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef VCOLLIDE_H
#define VCOLLIDE_H
#ifdef _WIN32
#pragma once
#endif

class CPhysCollide;

struct vcollide_t
{
	unsigned short solidCount : 15;
	unsigned short isPacked : 1;
	unsigned short descSize;
	// VPhysicsSolids
	CPhysCollide	**solids;
	char			*pKeyValues;
};

#endif // VCOLLIDE_H

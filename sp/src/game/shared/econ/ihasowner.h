//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef IHASOWNER_H
#define IHASOWNER_H
#ifdef _WIN32
#pragma once
#endif

class CBaseEntity;

//-----------------------------------------------------------------------------
// Purpose: Allows an entity to access its owner regardless of entity type 
//-----------------------------------------------------------------------------
class IHasOwner
{
public:
	virtual CBaseEntity			*GetOwnerViaInterface( void ) = 0;
};

#endif // IHASOWNER_H

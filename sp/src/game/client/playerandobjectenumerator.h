//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERANDOBJECTENUMERATOR_H
#define PLAYERANDOBJECTENUMERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "ehandle.h"
#include "ispatialpartition.h"

class C_BaseEntity;
class C_BasePlayer;

// Enumator class for finding other players and objects close to the
//  local player
class CPlayerAndObjectEnumerator : public IPartitionEnumerator
{
	DECLARE_CLASS_NOBASE( CPlayerAndObjectEnumerator );
public:
	//Forced constructor
	CPlayerAndObjectEnumerator( float radius );

	//Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );

	int	GetObjectCount();
	C_BaseEntity *GetObject( int index );

public:
	//Data members
	float	m_flRadiusSquared;

	CUtlVector< CHandle< C_BaseEntity > > m_Objects;
	C_BasePlayer *m_pLocal;
};

#endif // PLAYERANDOBJECTENUMERATOR_H

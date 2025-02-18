//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef PLAYERENUMERATOR_H
#define PLAYERENUMERATOR_H
#ifdef _WIN32
#pragma once
#endif

#include "utlvector.h"
#include "ehandle.h"
#include "ispatialpartition.h"

class C_BaseEntity;

class CPlayerEnumerator : public IPartitionEnumerator
{
	DECLARE_CLASS_NOBASE( CPlayerEnumerator );
public:
	//Forced constructor
	CPlayerEnumerator( float radius, Vector vecOrigin )
	{
		m_flRadiusSquared = radius * radius;
		m_vecOrigin = vecOrigin;
		m_Objects.RemoveAll();
	}

	int	GetObjectCount() { return m_Objects.Size(); }

	C_BaseEntity *GetObject( int index )
	{
		if ( index < 0 || index >= GetObjectCount() )
			return NULL;

		return m_Objects[ index ];
	}

	//Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		C_BaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		if ( !pEnt->IsPlayer() )
			return ITERATION_CONTINUE;

		Vector	deltaPos = pEnt->GetAbsOrigin() - m_vecOrigin;
		if ( deltaPos.LengthSqr() > m_flRadiusSquared )
			return ITERATION_CONTINUE;

		CHandle< C_BaseEntity > h;
		h = pEnt;
		m_Objects.AddToTail( h );

		return ITERATION_CONTINUE;
	}

public:
	//Data members
	float	m_flRadiusSquared;
	Vector m_vecOrigin;

	CUtlVector< CHandle< C_BaseEntity > > m_Objects;
};

#endif // PLAYERENUMERATOR_H
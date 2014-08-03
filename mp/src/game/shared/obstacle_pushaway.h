//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef OBSTACLE_PUSHAWAY_H
#define OBSTACLE_PUSHAWAY_H
#ifdef _WIN32
#pragma once
#endif

#include "props_shared.h"
#ifndef CLIENT_DLL
#include "func_breakablesurf.h"
#include "BasePropDoor.h"
#include "doors.h"
#endif // CLIENT_DLL

//--------------------------------------------------------------------------------------------------------------
bool IsPushAwayEntity( CBaseEntity *pEnt );
bool IsPushableEntity( CBaseEntity *pEnt );

//--------------------------------------------------------------------------------------------------------------
#ifndef CLIENT_DLL
bool IsBreakableEntity( CBaseEntity *pEnt );
#endif // !CLIENT_DLL

//--------------------------------------------------------------------------------------------------------------
class CPushAwayEnumerator : public IPartitionEnumerator
{
public:
	// Forced constructor
	CPushAwayEnumerator(CBaseEntity **ents, int nMaxEnts)
	{
		m_nAlreadyHit = 0;
		m_AlreadyHit = ents;
		m_nMaxHits = nMaxEnts;
	}
	
	// Actual work code
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
#ifdef CLIENT_DLL
		CBaseEntity *pEnt = ClientEntityList().GetBaseEntityFromHandle( pHandleEntity->GetRefEHandle() );
#else
		CBaseEntity *pEnt = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
#endif // CLIENT_DLL

		if ( IsPushAwayEntity( pEnt ) && m_nAlreadyHit < m_nMaxHits )
		{
			m_AlreadyHit[m_nAlreadyHit] = pEnt;
			m_nAlreadyHit++;
		}

		return ITERATION_CONTINUE;
	}

public:

	CBaseEntity **m_AlreadyHit;
	int m_nAlreadyHit;
	int m_nMaxHits;
};


#ifndef CLIENT_DLL
//--------------------------------------------------------------------------------------------------------------
/**
 * This class will collect breakable objects in a volume.  Physics props that can be damaged, func_breakable*, etc
 * are all collected by this class.
 */
class CBotBreakableEnumerator : public CPushAwayEnumerator
{
public:
	CBotBreakableEnumerator(CBaseEntity **ents, int nMaxEnts) : CPushAwayEnumerator(ents, nMaxEnts)
	{
	}

	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		CBaseEntity *pEnt = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );

		if ( !IsBreakableEntity( pEnt ) )
			return ITERATION_CONTINUE;

		// ignore breakables parented to doors
		if ( pEnt->GetParent() &&
			( FClassnameIs( pEnt->GetParent(), "func_door*" ) ||
			FClassnameIs( pEnt, "prop_door*" ) ) )
			return ITERATION_CONTINUE;

		if ( m_nAlreadyHit < m_nMaxHits )
		{
			m_AlreadyHit[m_nAlreadyHit] = pEnt;
			m_nAlreadyHit++;
		}

		return ITERATION_CONTINUE;
	}
};


//--------------------------------------------------------------------------------------------------------------
/**
 * This class will collect door objects in a volume.
 */
class CBotDoorEnumerator : public CPushAwayEnumerator
{
public:
	CBotDoorEnumerator(CBaseEntity **ents, int nMaxEnts) : CPushAwayEnumerator(ents, nMaxEnts)
	{
	}

	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
	{
		CBaseEntity *pEnt = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );

		if ( pEnt == NULL )
			return ITERATION_CONTINUE;

		if ( ( pEnt->ObjectCaps() & FCAP_IMPULSE_USE ) == 0 )
		{
			return ITERATION_CONTINUE;
		}

		if ( FClassnameIs( pEnt, "func_door*" ) )
		{
			CBaseDoor *door = dynamic_cast<CBaseDoor *>(pEnt);
			if ( !door )
			{
				return ITERATION_CONTINUE;
			}

			if ( door->m_toggle_state == TS_GOING_UP || door->m_toggle_state == TS_GOING_DOWN )
			{
				return ITERATION_CONTINUE;
			}
		}
		else if ( FClassnameIs( pEnt, "prop_door*" ) )
		{
			CBasePropDoor *door = dynamic_cast<CBasePropDoor *>(pEnt);
			if ( !door )
			{
				return ITERATION_CONTINUE;
			}

			if ( door->IsDoorOpening() || door->IsDoorClosing() )
			{
				return ITERATION_CONTINUE;
			}
		}
		else
		{
			return ITERATION_CONTINUE;
		}

		if ( m_nAlreadyHit < m_nMaxHits )
		{
			m_AlreadyHit[m_nAlreadyHit] = pEnt;
			m_nAlreadyHit++;
		}

		return ITERATION_CONTINUE;
	}
};


//--------------------------------------------------------------------------------------------------------------
/**
 *  Returns an entity that matches the filter that is along the line segment
 */
CBaseEntity * CheckForEntitiesAlongSegment( const Vector &start, const Vector &end, const Vector &mins, const Vector &maxs, CPushAwayEnumerator *enumerator );
#endif // CLIENT_DLL


//--------------------------------------------------------------------------------------------------------------
// Retrieves physics objects near pPushingEntity
void AvoidPushawayProps(  CBaseCombatCharacter *pPlayer, CUserCmd *pCmd );
int GetPushawayEnts( CBaseCombatCharacter *pPushingEntity, CBaseEntity **ents, int nMaxEnts, float flPlayerExpand, int PartitionMask, CPushAwayEnumerator *enumerator = NULL );

//--------------------------------------------------------------------------------------------------------------
// Pushes physics objects away from the entity
void PerformObstaclePushaway( CBaseCombatCharacter *pPushingEntity );


#endif // OBSTACLE_PUSHAWAY_H

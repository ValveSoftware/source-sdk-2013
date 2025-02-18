//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#ifndef PUSHENTITY_H
#define PUSHENTITY_H
#ifdef _WIN32
#pragma once
#endif

#include "movetype_push.h"

//-----------------------------------------------------------------------------
// Purpose: Keeps track of original positions of any entities that are being possibly pushed
//  and handles restoring positions for those objects if the push is aborted
//-----------------------------------------------------------------------------
class CPhysicsPushedEntities
{
public:

	DECLARE_CLASS_NOBASE( CPhysicsPushedEntities );

	CPhysicsPushedEntities( void );

	// Purpose: Tries to rotate an entity hierarchy, returns the blocker if any
	CBaseEntity *PerformRotatePush( CBaseEntity *pRoot, float movetime );

	// Purpose: Tries to linearly push an entity hierarchy, returns the blocker if any
	CBaseEntity *PerformLinearPush( CBaseEntity *pRoot, float movetime );

	int			CountMovedEntities() { return m_rgMoved.Count(); }
	void		StoreMovedEntities( physicspushlist_t &list );
	void		BeginPush( CBaseEntity *pRootEntity );

protected:

	// describes the per-frame incremental motion of a rotating MOVETYPE_PUSH
	struct RotatingPushMove_t
	{
		Vector		origin;
		matrix3x4_t	startLocalToWorld;
		matrix3x4_t	endLocalToWorld;
		QAngle		amove;		// delta orientation
	};

	// Pushers + their original positions also (for touching triggers)
	struct PhysicsPusherInfo_t
	{
		CBaseEntity			*m_pEntity;
		Vector				m_vecStartAbsOrigin;
	};

	// Pushed entities + various state related to them being pushed
	struct PhysicsPushedInfo_t
	{
		CBaseEntity			*m_pEntity;
		Vector				m_vecStartAbsOrigin;
		trace_t				m_Trace;
		bool				m_bBlocked;
		bool				m_bPusherIsGround;
	};

	// Adds the specified entity to the list
	void	AddEntity( CBaseEntity *ent );

	// If a move fails, restores all entities to their original positions
	void	RestoreEntities( );

	// Compute the direction to move the rotation blocker
	void	ComputeRotationalPushDirection( CBaseEntity *pBlocker, const RotatingPushMove_t &rotPushMove, Vector *pMove, CBaseEntity *pRoot );

	// Speculatively checks to see if all entities in this list can be pushed
	bool SpeculativelyCheckPush( PhysicsPushedInfo_t &info, const Vector &vecAbsPush, bool bRotationalPush );

	// Speculatively checks to see if all entities in this list can be pushed
	virtual bool SpeculativelyCheckRotPush( const RotatingPushMove_t &rotPushMove, CBaseEntity *pRoot );

	// Speculatively checks to see if all entities in this list can be pushed
	virtual bool	SpeculativelyCheckLinearPush( const Vector &vecAbsPush );

	// Registers a blockage
	CBaseEntity *RegisterBlockage();

	// Some fixup for objects pushed by rotating objects
	virtual void	FinishRotPushedEntity( CBaseEntity *pPushedEntity, const RotatingPushMove_t &rotPushMove );

	// Commits the speculative movement
	void	FinishPush( bool bIsRotPush = false, const RotatingPushMove_t *pRotPushMove = NULL );

	// Generates a list of all entities potentially blocking all pushers
	void	GenerateBlockingEntityList();
	void	GenerateBlockingEntityListAddBox( const Vector &vecMoved );

	// Purpose: Gets a list of all entities hierarchically attached to the root 
	void	SetupAllInHierarchy( CBaseEntity *pParent );

	// Unlink + relink the pusher list so we can actually do the push
	void	UnlinkPusherList( int *pPusherHandles );
	void	RelinkPusherList( int *pPusherHandles );

	// Causes all entities in the list to touch triggers from their prev position
	void	FinishPushers();

	// Purpose: Rotates the root entity, fills in the pushmove structure
	void	RotateRootEntity( CBaseEntity *pRoot, float movetime, RotatingPushMove_t &rotation );

	// Purpose: Linearly moves the root entity
	void	LinearlyMoveRootEntity( CBaseEntity *pRoot, float movetime, Vector *pAbsPushVector );

	bool	IsPushedPositionValid( CBaseEntity *pBlocker );

protected:

	CUtlVector<PhysicsPusherInfo_t>	m_rgPusher;
	CUtlVector<PhysicsPushedInfo_t>	m_rgMoved;
	int								m_nBlocker;
	bool							m_bIsUnblockableByPlayer;
	Vector							m_rootPusherStartLocalOrigin;
	QAngle							m_rootPusherStartLocalAngles;
	float							m_rootPusherStartLocaltime;
	float							m_flMoveTime;

	friend class CPushBlockerEnum;
};

class CTraceFilterPushMove : public CTraceFilterSimple
{
	DECLARE_CLASS( CTraceFilterPushMove, CTraceFilterSimple );

public:
	CTraceFilterPushMove( CBaseEntity *pEntity, int nCollisionGroup ) 
		: CTraceFilterSimple( pEntity, nCollisionGroup )
	{
		m_pRootParent = pEntity->GetRootMoveParent();
	}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		Assert( dynamic_cast<CBaseEntity*>(pHandleEntity) );
		CBaseEntity *pTestEntity = static_cast<CBaseEntity*>(pHandleEntity);

		if ( UTIL_EntityHasMatchingRootParent( m_pRootParent, pTestEntity ) )
			return false;

		if ( pTestEntity->GetMoveType() == MOVETYPE_VPHYSICS && 
			pTestEntity->VPhysicsGetObject() && pTestEntity->VPhysicsGetObject()->IsMoveable() )
			return false;

		return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
	}

private:

	CBaseEntity *m_pRootParent;
};

extern CPhysicsPushedEntities *g_pPushedEntities;

#endif  // PUSHENTITY_H
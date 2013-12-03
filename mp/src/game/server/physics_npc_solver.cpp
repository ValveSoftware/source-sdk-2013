//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "physics_saverestore.h"
#include "vphysics/friction.h"
#include "ai_basenpc.h"
#include "movevars_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CPhysicsNPCSolver : public CLogicalEntity, public IMotionEvent
{
	DECLARE_CLASS( CPhysicsNPCSolver, CLogicalEntity );
public:
	CPhysicsNPCSolver();
	~CPhysicsNPCSolver();
	DECLARE_DATADESC();
	void Init( CAI_BaseNPC *pNPC, CBaseEntity *pPhysicsObject, bool disableCollisions, float separationTime );
	static CPhysicsNPCSolver *Create( CAI_BaseNPC *pNPC, CBaseEntity *pPhysicsObject, bool disableCollisions, float separationTime );

	// CBaseEntity
	virtual void Spawn();
	virtual void UpdateOnRemove();
	virtual void Think();
	virtual void OnRestore()
	{
		BaseClass::OnRestore();
		if ( m_allowIntersection )
		{
			PhysDisableEntityCollisions( m_hNPC, m_hEntity );
		}
	}

	// IMotionEvent
	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

public:
	CPhysicsNPCSolver *m_pNext;
private:
	// locals
	void ResetCancelTime();
	void BecomePenetrationSolver();
	bool IsIntersecting();
	bool IsContactOnNPCHead( IPhysicsFrictionSnapshot *pSnapshot, IPhysicsObject *pPhysics, CAI_BaseNPC *pNPC );
	bool CheckTouching();
	friend bool NPCPhysics_SolverExists( CAI_BaseNPC *pNPC, CBaseEntity *pPhysicsObject );

	CHandle<CAI_BaseNPC>		m_hNPC;
	EHANDLE						m_hEntity;
	IPhysicsMotionController	*m_pController;
	float						m_separationDuration;
	float						m_cancelTime;
	bool						m_allowIntersection;
};

LINK_ENTITY_TO_CLASS( physics_npc_solver, CPhysicsNPCSolver );

BEGIN_DATADESC( CPhysicsNPCSolver )

	DEFINE_FIELD( m_hNPC, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_separationDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_cancelTime, FIELD_TIME ),
	DEFINE_FIELD( m_allowIntersection, FIELD_BOOLEAN ),
	DEFINE_PHYSPTR( m_pController ),
	//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),

END_DATADESC()

CEntityClassList<CPhysicsNPCSolver> g_SolverList;
template <> CPhysicsNPCSolver *CEntityClassList<CPhysicsNPCSolver>::m_pClassList = NULL;

bool NPCPhysics_SolverExists( CAI_BaseNPC *pNPC, CBaseEntity *pPhysicsObject )
{
	CPhysicsNPCSolver *pSolver = g_SolverList.m_pClassList;
	while ( pSolver )
	{
		if ( pSolver->m_hEntity == pPhysicsObject && pSolver->m_hNPC == pNPC )
			return true;
		pSolver = pSolver->m_pNext;
	}

	return false;
}

CPhysicsNPCSolver *CPhysicsNPCSolver::Create( CAI_BaseNPC *pNPC, CBaseEntity *pPhysicsObject, bool disableCollisions, float separationTime )
{
	CPhysicsNPCSolver *pSolver = (CPhysicsNPCSolver *)CBaseEntity::CreateNoSpawn( "physics_npc_solver", vec3_origin, vec3_angle, NULL );
	pSolver->Init( pNPC, pPhysicsObject, disableCollisions, separationTime );
	pSolver->Spawn();
	//NDebugOverlay::EntityBounds(pNPC, 255, 255, 0, 64, 0.5f );
	return pSolver;
}

CPhysicsNPCSolver::CPhysicsNPCSolver()
{
	g_SolverList.Insert( this );
}

CPhysicsNPCSolver::~CPhysicsNPCSolver()
{
	g_SolverList.Remove( this );
}

void CPhysicsNPCSolver::Init( CAI_BaseNPC *pNPC, CBaseEntity *pPhysicsObject, bool disableCollisions, float separationTime )
{
	m_hNPC = pNPC;
	m_hEntity = pPhysicsObject;
	m_pController = NULL;
	m_separationDuration = separationTime;
	m_allowIntersection = disableCollisions;

}

void CPhysicsNPCSolver::ResetCancelTime()
{
	m_cancelTime = gpGlobals->curtime + m_separationDuration;
	SetNextThink( m_cancelTime );
}

void CPhysicsNPCSolver::BecomePenetrationSolver()
{
	CBaseEntity *pEntity = m_hEntity.Get();
	if ( pEntity )
	{
		m_allowIntersection = true;
		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int listCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
		PhysDisableEntityCollisions( m_hNPC, pEntity );
		m_pController = physenv->CreateMotionController( this );
		for ( int i = 0; i < listCount; i++ )
		{
			m_pController->AttachObject( pList[i], false );
			pList[i]->Wake();
		}
		m_pController->SetPriority( IPhysicsMotionController::HIGH_PRIORITY );
	}
}

void CPhysicsNPCSolver::Spawn()
{
	if ( m_allowIntersection )
	{
		BecomePenetrationSolver();
	}
	else
	{
		m_hEntity->SetNavIgnore();
	}
	ResetCancelTime();
}

void CPhysicsNPCSolver::UpdateOnRemove()
{
	if ( m_allowIntersection )
	{
		physenv->DestroyMotionController( m_pController );
		m_pController = NULL;
		PhysEnableEntityCollisions( m_hNPC, m_hEntity );
	}
	else
	{
		if ( m_hEntity.Get() )
		{
			m_hEntity->ClearNavIgnore();
		}
	}
	//NDebugOverlay::EntityBounds(m_hNPC, 0, 255, 0, 64, 0.5f );
	BaseClass::UpdateOnRemove();
}

bool CPhysicsNPCSolver::IsIntersecting()
{
	CAI_BaseNPC *pNPC = m_hNPC.Get();
	CBaseEntity *pPhysics = m_hEntity.Get();
	if ( pNPC && pPhysics )
	{
		Ray_t ray;
		// bloated bounds to force slight separation
		Vector mins = pNPC->WorldAlignMins() - Vector(1,1,1);
		Vector maxs = pNPC->WorldAlignMaxs() + Vector(1,1,1);

		ray.Init( pNPC->GetAbsOrigin(), pNPC->GetAbsOrigin(), mins, maxs );
		trace_t tr;
		enginetrace->ClipRayToEntity( ray, pNPC->PhysicsSolidMaskForEntity(), pPhysics, &tr );
		if ( tr.startsolid )
			return true;
	}
	return false;
}

bool CPhysicsNPCSolver::IsContactOnNPCHead( IPhysicsFrictionSnapshot *pSnapshot, IPhysicsObject *pPhysics, CAI_BaseNPC *pNPC )
{
	float heightCheck = pNPC->GetAbsOrigin().z + pNPC->GetHullMaxs().z;
	Vector vel, point;
	pPhysics->GetVelocity( &vel, NULL );
	pSnapshot->GetContactPoint( point );
	// don't care if the object is already moving away
	if ( vel.LengthSqr() < 10.0f*10.0f )
	{
		float topdist = fabs(point.z-heightCheck);
		if ( topdist < 2.0f )
		{
			return true;
		}
	}
	return false;
}

bool CPhysicsNPCSolver::CheckTouching()
{
	CAI_BaseNPC *pNPC = m_hNPC.Get();
	if ( !pNPC )
		return false;

	CBaseEntity *pPhysicsEnt = m_hEntity.Get();
	if ( !pPhysicsEnt )
		return false;

	IPhysicsObject *pPhysics = pPhysicsEnt->VPhysicsGetObject();
	IPhysicsObject *pNPCPhysics = pNPC->VPhysicsGetObject();
	if ( !pNPCPhysics || !pPhysics )
		return false;

	IPhysicsFrictionSnapshot *pSnapshot = pPhysics->CreateFrictionSnapshot();
	bool found = false;
	bool penetrate = false;

	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		if ( pOther == pNPCPhysics )
		{
			found = true;
			if ( IsContactOnNPCHead(pSnapshot, pPhysics, pNPC ) )
			{
				penetrate = true;
				pSnapshot->MarkContactForDelete();
			}
			break;
		}
		pSnapshot->NextFrictionData();
	}
	pSnapshot->DeleteAllMarkedContacts( true );
	pPhysics->DestroyFrictionSnapshot( pSnapshot );

	// if the object is penetrating something, check to see if it's intersecting this NPC
	// if so, go ahead and switch over to penetration solver mode
	if ( !penetrate && (pPhysics->GetGameFlags() & FVPHYSICS_PENETRATING) )
	{
		penetrate = IsIntersecting();
	}

	if ( penetrate )
	{
		pPhysicsEnt->ClearNavIgnore();
		BecomePenetrationSolver();
	}

	return found;
}

void CPhysicsNPCSolver::Think()
{
	bool finished = m_allowIntersection ? !IsIntersecting() : !CheckTouching();

	if ( finished )
	{
		UTIL_Remove(this);
		return;
	}
	if ( m_allowIntersection )
	{
		IPhysicsObject *pObject = m_hEntity->VPhysicsGetObject();
		if ( !pObject )
		{
			UTIL_Remove(this);
			return;
		}
		pObject->Wake();
	}
	ResetCancelTime();
}

IMotionEvent::simresult_e CPhysicsNPCSolver::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, 
													  float deltaTime, Vector &linear, AngularImpulse &angular )
{
	if ( IsIntersecting() )
	{
		const float PUSH_SPEED = 150.0f;

		if ( pObject->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
		{
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			if ( pPlayer )
			{
				pPlayer->ForceDropOfCarriedPhysObjects( m_hEntity );
			}
		}

		ResetCancelTime();
		angular.Init();
		linear.Init();
		
		// Don't push on vehicles because they won't move
		if ( pObject->GetGameFlags() & FVPHYSICS_MULTIOBJECT_ENTITY )
		{
			if ( m_hEntity->GetServerVehicle() )
				return SIM_NOTHING;
		}

		Vector origin, vel;
		pObject->GetPosition( &origin, NULL );
		pObject->GetVelocity( &vel, NULL );
		Vector dir = origin - m_hNPC->GetAbsOrigin();
		dir.z = dir.z > 0 ? 0.1f : -0.1f;
		VectorNormalize(dir);
		AngularImpulse angVel;
		angVel.Init();

		// NOTE: Iterate this object's contact points 
		// if it can't move in this direction, try sliding along the plane/crease
		Vector pushImpulse;
		PhysComputeSlideDirection( pObject, dir * PUSH_SPEED, angVel, &pushImpulse, NULL, 0 );

		dir = pushImpulse;
		VectorNormalize(dir);

		if ( DotProduct( vel, dir ) < PUSH_SPEED * 0.5f )
		{
			linear = pushImpulse;
			if ( pObject->GetContactPoint(NULL,NULL) )
			{
				linear.z += GetCurrentGravity();
			}
		}
		return SIM_GLOBAL_ACCELERATION;
	}
	return SIM_NOTHING;
}


CBaseEntity *NPCPhysics_CreateSolver( CAI_BaseNPC *pNPC, CBaseEntity *pPhysicsObject, bool disableCollisions, float separationDuration )
{
	if ( disableCollisions )
	{
		if ( PhysEntityCollisionsAreDisabled( pNPC, pPhysicsObject ) )
			return NULL;
	}
	else
	{
		if ( pPhysicsObject->IsNavIgnored() )
			return NULL;
	}
	return CPhysicsNPCSolver::Create( pNPC, pPhysicsObject, disableCollisions, separationDuration );
}


class CPhysicsEntitySolver : public CLogicalEntity//, public IMotionEvent
{
	DECLARE_CLASS( CPhysicsEntitySolver, CLogicalEntity );
public:
	DECLARE_DATADESC();
	void Init( CBaseEntity *pMovingEntity, CBaseEntity *pPhysicsBlocker, float separationTime );
	static CPhysicsEntitySolver *Create( CBaseEntity *pMovingEntity, CBaseEntity *pPhysicsBlocker, float separationTime );

	// CBaseEntity
	virtual void Spawn();
	virtual void UpdateOnRemove();
	virtual void Think();

	// IMotionEvent
	//virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

private:
	// locals
	void ResetCancelTime();
	void BecomePenetrationSolver();
	//bool IsIntersecting();
	//bool IsTouching();

	EHANDLE						m_hMovingEntity;
	EHANDLE						m_hPhysicsBlocker;
	//IPhysicsMotionController	*m_pController;
	float						m_separationDuration;
	float						m_cancelTime;
	int							m_savedCollisionGroup;
};

LINK_ENTITY_TO_CLASS( physics_entity_solver, CPhysicsEntitySolver );

BEGIN_DATADESC( CPhysicsEntitySolver )

	DEFINE_FIELD( m_hMovingEntity, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hPhysicsBlocker, FIELD_EHANDLE ),
	DEFINE_FIELD( m_separationDuration, FIELD_FLOAT ),
	DEFINE_FIELD( m_cancelTime, FIELD_TIME ),
	DEFINE_FIELD( m_savedCollisionGroup, FIELD_INTEGER ),
	//DEFINE_PHYSPTR( m_pController ),

END_DATADESC()

CPhysicsEntitySolver *CPhysicsEntitySolver::Create( CBaseEntity *pMovingEntity, CBaseEntity *pPhysicsBlocker, float separationTime )
{
	CPhysicsEntitySolver *pSolver = (CPhysicsEntitySolver *)CBaseEntity::CreateNoSpawn( "physics_entity_solver", vec3_origin, vec3_angle, NULL );
	pSolver->Init( pMovingEntity, pPhysicsBlocker, separationTime );
	pSolver->Spawn();
	//NDebugOverlay::EntityBounds(pNPC, 255, 255, 0, 64, 0.5f );
	return pSolver;
}

void CPhysicsEntitySolver::Init( CBaseEntity *pMovingEntity, CBaseEntity *pPhysicsBlocker, float separationTime )
{
	m_hMovingEntity = pMovingEntity;
	m_hPhysicsBlocker = pPhysicsBlocker;
	//m_pController = NULL;
	m_separationDuration = separationTime;
}

void CPhysicsEntitySolver::Spawn()
{
	SetNextThink( gpGlobals->curtime + m_separationDuration );
	PhysDisableEntityCollisions( m_hMovingEntity, m_hPhysicsBlocker );
	m_savedCollisionGroup = m_hPhysicsBlocker->GetCollisionGroup();
	m_hPhysicsBlocker->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	if ( m_hPhysicsBlocker->VPhysicsGetObject() )
	{
		m_hPhysicsBlocker->VPhysicsGetObject()->RecheckContactPoints();
	}
}

void CPhysicsEntitySolver::Think()
{
	UTIL_Remove(this);
}

void CPhysicsEntitySolver::UpdateOnRemove()
{
	//physenv->DestroyMotionController( m_pController );
	//m_pController = NULL;
	CBaseEntity *pEntity = m_hMovingEntity.Get();
	CBaseEntity *pPhysics = m_hPhysicsBlocker.Get();
	if ( pEntity && pPhysics )
	{
		PhysEnableEntityCollisions( pEntity, pPhysics );
	}
	if ( pPhysics )
	{
		pPhysics->SetCollisionGroup( m_savedCollisionGroup );
	}
	BaseClass::UpdateOnRemove();
}


CBaseEntity *EntityPhysics_CreateSolver( CBaseEntity *pMovingEntity, CBaseEntity *pPhysicsObject, bool disableCollisions, float separationDuration )
{
	if ( PhysEntityCollisionsAreDisabled( pMovingEntity, pPhysicsObject ) )
		return NULL;

	return CPhysicsEntitySolver::Create( pMovingEntity, pPhysicsObject, separationDuration );
}


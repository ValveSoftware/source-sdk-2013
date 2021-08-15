//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Interface layer for ipion IVP physics.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//===========================================================================//


#include "cbase.h"
#include "coordsize.h"
#include "entitylist.h"
#include "vcollide_parse.h"
#include "soundenvelope.h"
#include "game.h"
#include "utlvector.h"
#include "init_factory.h"
#include "igamesystem.h"
#include "hierarchy.h"
#include "IEffects.h"
#include "engine/IEngineSound.h"
#include "world.h"
#include "decals.h"
#include "physics_fx.h"
#include "vphysics_sound.h"
#include "vphysics/vehicles.h"
#include "vehicle_sounds.h"
#include "movevars_shared.h"
#include "physics_saverestore.h"
#include "solidsetdefaults.h"
#include "tier0/vprof.h"
#include "engine/IStaticPropMgr.h"
#include "physics_prop_ragdoll.h"
#if HL2_EPISODIC
#include "particle_parse.h"
#endif
#include "vphysics/object_hash.h"
#include "vphysics/collision_set.h"
#include "vphysics/friction.h"
#include "fmtstr.h"
#include "physics_npc_solver.h"
#include "physics_collisionevent.h"
#include "vphysics/performance.h"
#include "positionwatcher.h"
#include "tier1/callqueue.h"
#include "vphysics/constraints.h"

#ifdef PORTAL
#include "portal_physics_collisionevent.h"
#include "physicsshadowclone.h"
#include "PortalSimulation.h"
void PortalPhysFrame( float deltaTime ); //small wrapper for PhysFrame that simulates all 3 environments at once
#endif

void PrecachePhysicsSounds( void );

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar phys_speeds( "phys_speeds", "0" );

// defined in phys_constraint
extern IPhysicsConstraintEvent *g_pConstraintEvents;


CEntityList *g_pShadowEntities = NULL;
#ifdef PORTAL
CEntityList *g_pShadowEntities_Main = NULL;
#endif

// local variables
static float g_PhysAverageSimTime;
CCallQueue g_PostSimulationQueue;


// local routines
static IPhysicsObject *PhysCreateWorld( CBaseEntity *pWorld );
static void PhysFrame( float deltaTime );
static bool IsDebris( int collisionGroup );

void TimescaleChanged( IConVar *var, const char *pOldString, float flOldValue )
{
	if ( physenv )
	{
		physenv->ResetSimulationClock();
	}
}

ConVar phys_timescale( "phys_timescale", "1", 0, "Scale time for physics", TimescaleChanged );

#if _DEBUG
ConVar phys_dontprintint( "phys_dontprintint", "1", FCVAR_NONE, "Don't print inter-penetration warnings." );
#endif

#ifdef PORTAL
	CPortal_CollisionEvent g_Collisions;
#else
	CCollisionEvent g_Collisions;
#endif


IPhysicsCollisionSolver * const g_pCollisionSolver = &g_Collisions;
IPhysicsCollisionEvent * const g_pCollisionEventHandler = &g_Collisions;
IPhysicsObjectEvent * const g_pObjectEventHandler = &g_Collisions;


struct vehiclescript_t
{
	string_t scriptName;
	vehicleparams_t params;
	vehiclesounds_t sounds;
};

class CPhysicsHook : public CBaseGameSystemPerFrame
{
public:
	virtual const char *Name() { return "CPhysicsHook"; }

	virtual bool Init();
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();
	virtual void LevelShutdownPreEntity();
	virtual void LevelShutdownPostEntity();
	virtual void FrameUpdatePostEntityThink();
	virtual void PreClientUpdate();

	bool FindOrAddVehicleScript( const char *pScriptName, vehicleparams_t *pVehicle, vehiclesounds_t *pSounds );
	void FlushVehicleScripts()
	{
		m_vehicleScripts.RemoveAll();
	}

	bool ShouldSimulate()
	{
		return (physenv && !m_bPaused) ? true : false;
	}

	physicssound::soundlist_t m_impactSounds;
	CUtlVector<physicssound::breaksound_t> m_breakSounds;

	CUtlVector<masscenteroverride_t>	m_massCenterOverrides;
	CUtlVector<vehiclescript_t>			m_vehicleScripts;

	float		m_impactSoundTime;
	bool		m_bPaused;
	bool		m_isFinalTick;
};


CPhysicsHook	g_PhysicsHook;

//-----------------------------------------------------------------------------
// Singleton access
//-----------------------------------------------------------------------------
IGameSystem* PhysicsGameSystem()
{
	return &g_PhysicsHook;
}


//-----------------------------------------------------------------------------
// Purpose: The physics hook callback implementations
//-----------------------------------------------------------------------------
bool CPhysicsHook::Init( void )
{
	factorylist_t factories;
	
	// Get the list of interface factories to extract the physics DLL's factory
	FactoryList_Retrieve( factories );

	if ( !factories.physicsFactory )
		return false;

	if ((physics = (IPhysics *)factories.physicsFactory( VPHYSICS_INTERFACE_VERSION, NULL )) == NULL ||
		(physcollision = (IPhysicsCollision *)factories.physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL )) == NULL ||
		(physprops = (IPhysicsSurfaceProps *)factories.physicsFactory( VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, NULL )) == NULL
		)
		return false;

	PhysParseSurfaceData( physprops, filesystem );

	m_isFinalTick = true;
	m_impactSoundTime = 0;
	m_vehicleScripts.EnsureCapacity(4);
	return true;
}


// a little debug wrapper to help fix bugs when entity pointers get trashed
#if 0
struct physcheck_t
{
	IPhysicsObject *pPhys;
	char			string[512];
};

CUtlVector< physcheck_t > physCheck;

void PhysCheckAdd( IPhysicsObject *pPhys, const char *pString )
{
	physcheck_t tmp;
	tmp.pPhys = pPhys;
	Q_strncpy( tmp.string, pString ,sizeof(tmp.string));
	physCheck.AddToTail( tmp );
}

const char *PhysCheck( IPhysicsObject *pPhys )
{
	for ( int i = 0; i < physCheck.Size(); i++ )
	{
		if ( physCheck[i].pPhys == pPhys )
			return physCheck[i].string;
	}

	return "unknown";
}
#endif

void CPhysicsHook::LevelInitPreEntity() 
{
	physenv = physics->CreateEnvironment();
	physics_performanceparams_t params;
	params.Defaults();
	params.maxCollisionsPerObjectPerTimestep = 10;
	physenv->SetPerformanceSettings( &params );

#ifdef PORTAL
	physenv_main = physenv;
#endif
	{
	g_EntityCollisionHash = physics->CreateObjectPairHash();
	}
	factorylist_t factories;
	FactoryList_Retrieve( factories );
	physenv->SetDebugOverlay( factories.engineFactory );
	physenv->EnableDeleteQueue( true );

	physenv->SetCollisionSolver( &g_Collisions );
	physenv->SetCollisionEventHandler( &g_Collisions );
	physenv->SetConstraintEventHandler( g_pConstraintEvents );
	physenv->EnableConstraintNotify( true ); // callback when an object gets deleted that is attached to a constraint

	physenv->SetObjectEventHandler( &g_Collisions );
	
	physenv->SetSimulationTimestep( gpGlobals->interval_per_tick ); // 15 ms per tick
	// HL Game gravity, not real-world gravity
	physenv->SetGravity( Vector( 0, 0, -GetCurrentGravity() ) );
	g_PhysAverageSimTime = 0;

	g_PhysWorldObject = PhysCreateWorld( GetWorldEntity() );

	g_pShadowEntities = new CEntityList;
#ifdef PORTAL
	g_pShadowEntities_Main  = g_pShadowEntities;
#endif

	PrecachePhysicsSounds();

	m_bPaused = true;
}



void CPhysicsHook::LevelInitPostEntity() 
{
	m_bPaused = false;
}

void CPhysicsHook::LevelShutdownPreEntity() 
{
	if ( !physenv )
		return;
	physenv->SetQuickDelete( true );
}

void CPhysicsHook::LevelShutdownPostEntity() 
{
	if ( !physenv )
		return;

	g_pPhysSaveRestoreManager->ForgetAllModels();

	g_Collisions.LevelShutdown();

	physics->DestroyEnvironment( physenv );
	physenv = NULL;

	physics->DestroyObjectPairHash( g_EntityCollisionHash );
	g_EntityCollisionHash = NULL;

	physics->DestroyAllCollisionSets();

	g_PhysWorldObject = NULL;

	delete g_pShadowEntities;
	g_pShadowEntities = NULL;
	m_impactSounds.RemoveAll();
	m_breakSounds.RemoveAll();
	m_massCenterOverrides.Purge();
	FlushVehicleScripts();
}


bool CPhysicsHook::FindOrAddVehicleScript( const char *pScriptName, vehicleparams_t *pVehicle, vehiclesounds_t *pSounds )
{
	bool bLoadedSounds = false;
	int index = -1;
	for ( int i = 0; i < m_vehicleScripts.Count(); i++ )
	{
		if ( !Q_stricmp(m_vehicleScripts[i].scriptName.ToCStr(), pScriptName) )
		{
			index = i;
			bLoadedSounds = true;
			break;
		}
	}

	if ( index < 0 )
	{
		byte *pFile = UTIL_LoadFileForMe( pScriptName, NULL );
		if ( pFile )
		{
			// new script, parse it and write to the table
			index = m_vehicleScripts.AddToTail();
			m_vehicleScripts[index].scriptName = AllocPooledString(pScriptName);
			m_vehicleScripts[index].sounds.Init();

			IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( (char *)pFile );
			while ( !pParse->Finished() )
			{
				const char *pBlock = pParse->GetCurrentBlockName();
				if ( !strcmpi( pBlock, "vehicle" ) )
				{
					pParse->ParseVehicle( &m_vehicleScripts[index].params, NULL );
				}
				else if ( !Q_stricmp( pBlock, "vehicle_sounds" ) )
				{
					bLoadedSounds = true;
					CVehicleSoundsParser soundParser;
					pParse->ParseCustom( &m_vehicleScripts[index].sounds, &soundParser );
				}
				else
				{
					pParse->SkipBlock();
				}
			}
			physcollision->VPhysicsKeyParserDestroy( pParse );
			UTIL_FreeFile( pFile );
		}
	}

	if ( index >= 0 )
	{
		if ( pVehicle )
		{
			*pVehicle = m_vehicleScripts[index].params;
		}
		if ( pSounds )
		{
			// We must pass back valid data here!
			if ( bLoadedSounds == false )
				return false;

			*pSounds = m_vehicleScripts[index].sounds;
		}
		return true;
	}

	return false;
}

// called after entities think
void CPhysicsHook::FrameUpdatePostEntityThink( ) 
{
	VPROF_BUDGET( "CPhysicsHook::FrameUpdatePostEntityThink", VPROF_BUDGETGROUP_PHYSICS );

	// Tracker 24846:  If game is paused, don't simulate vphysics
	float interval = ( gpGlobals->frametime > 0.0f ) ? TICK_INTERVAL : 0.0f;

	// update the physics simulation, not we don't use gpGlobals->frametime, since that can be 30 msec or 15 msec
	// depending on whether IsSimulatingOnAlternateTicks is true or not
	if ( CBaseEntity::IsSimulatingOnAlternateTicks() )
	{
		m_isFinalTick = false;

#ifdef PORTAL //slight detour if we're the portal mod
		PortalPhysFrame( interval );
#else
		PhysFrame( interval );
#endif

	}
	m_isFinalTick = true;

#ifdef PORTAL //slight detour if we're the portal mod
	PortalPhysFrame( interval );
#else
	PhysFrame( interval );
#endif

}

void CPhysicsHook::PreClientUpdate()
{
	m_impactSoundTime += gpGlobals->frametime;
	if ( m_impactSoundTime > 0.05f )
	{
		physicssound::PlayImpactSounds( m_impactSounds );
		m_impactSoundTime = 0.0f;
		physicssound::PlayBreakSounds( m_breakSounds );
	}
}

bool PhysIsFinalTick()
{
	return g_PhysicsHook.m_isFinalTick;
}

IPhysicsObject *PhysCreateWorld( CBaseEntity *pWorld )
{
	staticpropmgr->CreateVPhysicsRepresentations( physenv, &g_SolidSetup, pWorld );
	return PhysCreateWorld_Shared( pWorld, modelinfo->GetVCollide(1), g_PhysDefaultObjectParams );
}


// vehicle wheels can only collide with things that can't get stuck in them during game physics
// because they aren't in the game physics world at present
static bool WheelCollidesWith( IPhysicsObject *pObj, CBaseEntity *pEntity )
{
#if defined( INVASION_DLL )
	if ( pEntity->GetCollisionGroup() == TFCOLLISION_GROUP_OBJECT )
		return false;
#endif

	// Cull against interactive debris
	if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		return false;

	// Hit physics ents
	if ( pEntity->GetMoveType() == MOVETYPE_PUSH || pEntity->GetMoveType() == MOVETYPE_VPHYSICS || pObj->IsStatic() )
		return true;

	return false;
}

CCollisionEvent::CCollisionEvent()
{
	m_inCallback = 0;
	m_bBufferTouchEvents = false;
	m_lastTickFrictionError = 0;
}

int CCollisionEvent::ShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 )
#if _DEBUG
{
	int x0 = ShouldCollide_2(pObj0, pObj1, pGameData0, pGameData1);
	int x1 = ShouldCollide_2(pObj1, pObj0, pGameData1, pGameData0);
	Assert(x0==x1);
	return x0;
}
int CCollisionEvent::ShouldCollide_2( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 )
#endif
{
	CallbackContext check(this);

	CBaseEntity *pEntity0 = static_cast<CBaseEntity *>(pGameData0);
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pGameData1);

	if ( !pEntity0 || !pEntity1 )
		return 1;

	unsigned short gameFlags0 = pObj0->GetGameFlags();
	unsigned short gameFlags1 = pObj1->GetGameFlags();

	if ( pEntity0 == pEntity1 )
	{
		// allow all-or-nothing per-entity disable
		if ( (gameFlags0 | gameFlags1) & FVPHYSICS_NO_SELF_COLLISIONS )
			return 0;

		IPhysicsCollisionSet *pSet = physics->FindCollisionSet( pEntity0->GetModelIndex() );
		if ( pSet )
			return pSet->ShouldCollide( pObj0->GetGameIndex(), pObj1->GetGameIndex() );

		return 1;
	}

	// objects that are both constrained to the world don't collide with each other
	if ( (gameFlags0 & gameFlags1) & FVPHYSICS_CONSTRAINT_STATIC )
	{
		return 0;
	}

	// Special collision rules for vehicle wheels
	// Their entity collides with stuff using the normal rules, but they
	// have different rules than the vehicle body for various reasons.
	// sort of a hack because we don't have spheres to represent them in the game
	// world for speculative collisions.
	if ( pObj0->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL )
	{
		if ( !WheelCollidesWith( pObj1, pEntity1 ) )
			return false;
	}
	if ( pObj1->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL )
	{
		if ( !WheelCollidesWith( pObj0, pEntity0 ) )
			return false;
	}

	if ( pEntity0->ForceVPhysicsCollide( pEntity1 ) || pEntity1->ForceVPhysicsCollide( pEntity0 ) )
		return 1;

	if ( pEntity0->edict() && pEntity1->edict() )
	{
		// don't collide with your owner
		if ( pEntity0->GetOwnerEntity() == pEntity1 || pEntity1->GetOwnerEntity() == pEntity0 )
			return 0;
	}

	if ( pEntity0->GetMoveParent() || pEntity1->GetMoveParent() )
	{
		CBaseEntity *pParent0 = pEntity0->GetRootMoveParent();
		CBaseEntity *pParent1 = pEntity1->GetRootMoveParent();
		
		// NOTE: Don't let siblings/parents collide.  If you want this behavior, do it
		// with constraints, not hierarchy!
		if ( pParent0 == pParent1 )
			return 0;

		if ( g_EntityCollisionHash->IsObjectPairInHash( pParent0, pParent1 ) )
			return 0;

		IPhysicsObject *p0 = pParent0->VPhysicsGetObject();
		IPhysicsObject *p1 = pParent1->VPhysicsGetObject();
		if ( p0 && p1 )
		{
			if ( g_EntityCollisionHash->IsObjectPairInHash( p0, p1 ) )
				return 0;
		}
	}

	int solid0 = pEntity0->GetSolid();
	int solid1 = pEntity1->GetSolid();
	int nSolidFlags0 = pEntity0->GetSolidFlags();
	int nSolidFlags1 = pEntity1->GetSolidFlags();

	int movetype0 = pEntity0->GetMoveType();
	int movetype1 = pEntity1->GetMoveType();

	// entities with non-physical move parents or entities with MOVETYPE_PUSH
	// are considered as "AI movers".  They are unchanged by collision; they exert
	// physics forces on the rest of the system.
	bool aiMove0 = (movetype0==MOVETYPE_PUSH) ? true : false;
	bool aiMove1 = (movetype1==MOVETYPE_PUSH) ? true : false;

	if ( pEntity0->GetMoveParent() )
	{
		// if the object & its parent are both MOVETYPE_VPHYSICS, then this must be a special case
		// like a prop_ragdoll_attached
		if ( !(movetype0 == MOVETYPE_VPHYSICS && pEntity0->GetRootMoveParent()->GetMoveType() == MOVETYPE_VPHYSICS) )
		{
			aiMove0 = true;
		}
	}
	if ( pEntity1->GetMoveParent() )
	{
		// if the object & its parent are both MOVETYPE_VPHYSICS, then this must be a special case.
		if ( !(movetype1 == MOVETYPE_VPHYSICS && pEntity1->GetRootMoveParent()->GetMoveType() == MOVETYPE_VPHYSICS) )
		{
			aiMove1 = true;
		}
	}

	// AI movers don't collide with the world/static/pinned objects or other AI movers
	if ( (aiMove0 && !pObj1->IsMoveable()) ||
		(aiMove1 && !pObj0->IsMoveable()) ||
		(aiMove0 && aiMove1) )
		return 0;

	// two objects under shadow control should not collide.  The AI will figure it out
	if ( pObj0->GetShadowController() && pObj1->GetShadowController() )
		return 0;

	// BRJ 1/24/03
	// You can remove the assert if it's problematic; I *believe* this condition
	// should be met, but I'm not sure.
	//Assert ( (solid0 != SOLID_NONE) && (solid1 != SOLID_NONE) );
	if ( (solid0 == SOLID_NONE) || (solid1 == SOLID_NONE) )
		return 0;

	// not solid doesn't collide with anything
	if ( (nSolidFlags0|nSolidFlags1) & FSOLID_NOT_SOLID )
	{
		// might be a vphysics trigger, collide with everything but "not solid"
		if ( pObj0->IsTrigger() && !(nSolidFlags1 & FSOLID_NOT_SOLID) )
			return 1;
		if ( pObj1->IsTrigger() && !(nSolidFlags0 & FSOLID_NOT_SOLID) )
			return 1;

		return 0;
	}
	
	if ( (nSolidFlags0 & FSOLID_TRIGGER) && 
		!(solid1 == SOLID_VPHYSICS || solid1 == SOLID_BSP || movetype1 == MOVETYPE_VPHYSICS) )
		return 0;

	if ( (nSolidFlags1 & FSOLID_TRIGGER) && 
		!(solid0 == SOLID_VPHYSICS || solid0 == SOLID_BSP || movetype0 == MOVETYPE_VPHYSICS) )
		return 0;

	if ( !g_pGameRules->ShouldCollide( pEntity0->GetCollisionGroup(), pEntity1->GetCollisionGroup() ) )
		return 0;

	// check contents
	if ( !(pObj0->GetContents() & pEntity1->PhysicsSolidMaskForEntity()) || !(pObj1->GetContents() & pEntity0->PhysicsSolidMaskForEntity()) )
		return 0;

	if ( g_EntityCollisionHash->IsObjectPairInHash( pGameData0, pGameData1 ) )
		return 0;

	if ( g_EntityCollisionHash->IsObjectPairInHash( pObj0, pObj1 ) )
		return 0;

	return 1;
}

bool FindMaxContact( IPhysicsObject *pObject, float minForce, IPhysicsObject **pOtherObject, Vector *contactPos, Vector *pForce )
{
	float mass = pObject->GetMass();
	float maxForce = minForce;
	*pOtherObject = NULL;
	IPhysicsFrictionSnapshot *pSnapshot = pObject->CreateFrictionSnapshot();
	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		if ( pOther->IsMoveable() && pOther->GetMass() > mass )
		{
			float force = pSnapshot->GetNormalForce();
			if ( force > maxForce )
			{
				*pOtherObject = pOther;
				pSnapshot->GetContactPoint( *contactPos );
				pSnapshot->GetSurfaceNormal( *pForce );
				*pForce *= force;
			}
		}
		pSnapshot->NextFrictionData();
	}
	pObject->DestroyFrictionSnapshot( pSnapshot );
	if ( *pOtherObject )
		return true;

	return false;
}

bool CCollisionEvent::ShouldFreezeObject( IPhysicsObject *pObject )
{
	extern bool PropIsGib(CBaseEntity *pEntity);
	// for now, don't apply a per-object limit to ai MOVETYPE_PUSH objects
	// NOTE: If this becomes a problem (too many collision checks this tick) we should add a path
	// to inform the logic in VPhysicsUpdatePusher() about the limit being applied so 
	// that it doesn't falsely block the object when it's simply been temporarily frozen
	// for performance reasons
	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	if ( pEntity )
	{
		if (pEntity->GetMoveType() == MOVETYPE_PUSH )
			return false;
		
		// don't limit vehicle collisions either, limit can make breaking through a pile of breakable
		// props very hitchy
		if (pEntity->GetServerVehicle() && !(pObject->GetCallbackFlags() & CALLBACK_IS_VEHICLE_WHEEL))
			return false;
	}

	// if we're freezing a debris object, then it's probably due to some kind of solver issue
	// usually this is a large object resting on the debris object in question which is not
	// very stable.
	// After doing the experiment of constraining the dynamic range of mass while solving friction
	// contacts, I like the results of this tradeoff better.  So damage or remove the debris object
	// wherever possible once we hit this case:
	if ( IsDebris( pEntity->GetCollisionGroup()) && !pEntity->IsNPC() )
	{
		IPhysicsObject *pOtherObject = NULL;
		Vector contactPos;
		Vector force;
		// find the contact with the moveable object applying the most contact force
		if ( FindMaxContact( pObject, pObject->GetMass() * 10, &pOtherObject, &contactPos, &force ) )
		{
			CBaseEntity *pOther = static_cast<CBaseEntity *>(pOtherObject->GetGameData());
			// this object can take damage, crush it
			if ( pEntity->m_takedamage > DAMAGE_EVENTS_ONLY )
			{
				CTakeDamageInfo dmgInfo( pOther, pOther, force, contactPos, force.Length() * 0.1f, DMG_CRUSH );
				PhysCallbackDamage( pEntity, dmgInfo );
			}
			else
			{
				// can't be damaged, so do something else:
				if ( PropIsGib(pEntity) )
				{
					// it's always safe to delete gibs, so kill this one to avoid simulation problems
					PhysCallbackRemove( pEntity->NetworkProp() );
				}
				else
				{
					// not a gib, create a solver:
					// UNDONE: Add a property to override this in gameplay critical scenarios?
					g_PostSimulationQueue.QueueCall( EntityPhysics_CreateSolver, pOther, pEntity, true, 1.0f );
				}
			}
		}
	}
	return true;
}

bool CCollisionEvent::ShouldFreezeContacts( IPhysicsObject **pObjectList, int objectCount )
{
	if ( m_lastTickFrictionError > gpGlobals->tickcount || m_lastTickFrictionError < (gpGlobals->tickcount-1) )
	{
		DevWarning("Performance Warning: large friction system (%d objects)!!!\n", objectCount );
#if _DEBUG
		for ( int i = 0; i < objectCount; i++ )
		{
			CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObjectList[i]->GetGameData());
			pEntity->m_debugOverlays |= OVERLAY_ABSBOX_BIT | OVERLAY_PIVOT_BIT;
		}
#endif
	}
	m_lastTickFrictionError = gpGlobals->tickcount;
	return false;
}

// NOTE: these are fully edge triggered events 
// called when an object wakes up (starts simulating)
void CCollisionEvent::ObjectWake( IPhysicsObject *pObject )
{
	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	if ( pEntity && pEntity->HasDataObjectType( VPHYSICSWATCHER ) )
	{
		ReportVPhysicsStateChanged( pObject, pEntity, true );
	}
}
// called when an object goes to sleep (no longer simulating)
void CCollisionEvent::ObjectSleep( IPhysicsObject *pObject )
{
	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	if ( pEntity && pEntity->HasDataObjectType( VPHYSICSWATCHER ) )
	{
		ReportVPhysicsStateChanged( pObject, pEntity, false );
	}
}

bool PhysShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1 )
{
	void *pGameData0 = pObj0->GetGameData();
	void *pGameData1 = pObj1->GetGameData();
	if ( !pGameData0 || !pGameData1 )
		return false;
	return g_Collisions.ShouldCollide( pObj0, pObj1, pGameData0, pGameData1 ) ? true : false;
}

bool PhysIsInCallback()
{
	if ( (physenv && physenv->IsInSimulation()) || g_Collisions.IsInCallback() )
		return true;

	return false;
}


static void ReportPenetration( CBaseEntity *pEntity, float duration )
{
	if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		if ( g_pDeveloper->GetInt() > 1 )
		{
			pEntity->m_debugOverlays |= OVERLAY_ABSBOX_BIT;
		}

		pEntity->AddTimedOverlay( UTIL_VarArgs("VPhysics Penetration Error (%s)!", pEntity->GetDebugName()), duration );
	}
}

static bool IsDebris( int collisionGroup )
{
	switch ( collisionGroup )
	{
	case COLLISION_GROUP_DEBRIS:
	case COLLISION_GROUP_INTERACTIVE_DEBRIS:
	case COLLISION_GROUP_DEBRIS_TRIGGER:
		return true;
	default:
		break;
	}
	return false;
}

static void UpdateEntityPenetrationFlag( CBaseEntity *pEntity, bool isPenetrating )
{
	if ( !pEntity )
		return;
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	for ( int i = 0; i < count; i++ )
	{
		if ( !pList[i]->IsStatic() )
		{
			if ( isPenetrating )
			{
				PhysSetGameFlags( pList[i], FVPHYSICS_PENETRATING );
			}
			else
			{
				PhysClearGameFlags( pList[i], FVPHYSICS_PENETRATING );
			}
		}
	}
}

void CCollisionEvent::GetListOfPenetratingEntities( CBaseEntity *pSearch, CUtlVector<CBaseEntity *> &list )
{
	for ( int i = m_penetrateEvents.Count()-1; i >= 0; --i )
	{
		if ( m_penetrateEvents[i].hEntity0 == pSearch && m_penetrateEvents[i].hEntity1.Get() != NULL )
		{
			list.AddToTail( m_penetrateEvents[i].hEntity1 );
		}
		else if ( m_penetrateEvents[i].hEntity1 == pSearch && m_penetrateEvents[i].hEntity0.Get() != NULL )
		{
			list.AddToTail( m_penetrateEvents[i].hEntity0 );
		}
	}
}

void CCollisionEvent::UpdatePenetrateEvents( void )
{
	for ( int i = m_penetrateEvents.Count()-1; i >= 0; --i )
	{
		CBaseEntity *pEntity0 = m_penetrateEvents[i].hEntity0;
		CBaseEntity *pEntity1 = m_penetrateEvents[i].hEntity1;

		if ( m_penetrateEvents[i].collisionState == COLLSTATE_TRYDISABLE )
		{
			if ( pEntity0 && pEntity1 )
			{
				IPhysicsObject *pObj0 = pEntity0->VPhysicsGetObject();
				if ( pObj0 )
				{
					PhysForceEntityToSleep( pEntity0, pObj0 );
				}
				IPhysicsObject *pObj1 = pEntity1->VPhysicsGetObject();
				if ( pObj1 )
				{
					PhysForceEntityToSleep( pEntity1, pObj1 );
				}
				m_penetrateEvents[i].collisionState = COLLSTATE_DISABLED;
				continue;
			}
			// missing entity or object, clear event
		}
		else if ( m_penetrateEvents[i].collisionState == COLLSTATE_TRYNPCSOLVER )
		{
			if ( pEntity0 && pEntity1 )
			{
				CAI_BaseNPC *pNPC = pEntity0->MyNPCPointer();
				CBaseEntity *pBlocker = pEntity1;
				if ( !pNPC )
				{
					pNPC = pEntity1->MyNPCPointer();
					Assert(pNPC);
					pBlocker = pEntity0;
				}
				NPCPhysics_CreateSolver( pNPC, pBlocker, true, 1.0f );
			}
			// transferred to solver, clear event
		}
		else if ( m_penetrateEvents[i].collisionState == COLLSTATE_TRYENTITYSOLVER )
		{
			if ( pEntity0 && pEntity1 )
			{
				if ( !IsDebris(pEntity1->GetCollisionGroup()) || pEntity1->GetMoveType() != MOVETYPE_VPHYSICS )
				{
					CBaseEntity *pTmp = pEntity0;
					pEntity0 = pEntity1;
					pEntity1 = pTmp;
				}
				EntityPhysics_CreateSolver( pEntity0, pEntity1, true, 1.0f );
			}
			// transferred to solver, clear event
		}
		else if ( gpGlobals->curtime - m_penetrateEvents[i].timeStamp > 1.0 )
		{
			if ( m_penetrateEvents[i].collisionState == COLLSTATE_DISABLED )
			{
				if ( pEntity0 && pEntity1 )
				{
					IPhysicsObject *pObj0 = pEntity0->VPhysicsGetObject();
					IPhysicsObject *pObj1 = pEntity1->VPhysicsGetObject();
					if ( pObj0 && pObj1 )
					{
						m_penetrateEvents[i].collisionState = COLLSTATE_ENABLED;
						continue;
					}
				}
			}
			// haven't penetrated for 1 second, so remove
		}
		else
		{
			// recent timestamp, don't remove the event yet
			continue;
		}
		// done, clear event
		m_penetrateEvents.FastRemove(i);
		UpdateEntityPenetrationFlag( pEntity0, false );
		UpdateEntityPenetrationFlag( pEntity1, false );
	}
}

penetrateevent_t &CCollisionEvent::FindOrAddPenetrateEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1 )
{
	int index = -1;
	for ( int i = m_penetrateEvents.Count()-1; i >= 0; --i )
	{
		if ( m_penetrateEvents[i].hEntity0.Get() == pEntity0 && m_penetrateEvents[i].hEntity1.Get() == pEntity1 )
		{
			index = i;
			break;
		}
	}
	if ( index < 0 )
	{
		index = m_penetrateEvents.AddToTail();
		penetrateevent_t &event = m_penetrateEvents[index];
		event.hEntity0 = pEntity0;
		event.hEntity1 = pEntity1;
		event.startTime = gpGlobals->curtime;
		event.collisionState = COLLSTATE_ENABLED;
		UpdateEntityPenetrationFlag( pEntity0, true );
		UpdateEntityPenetrationFlag( pEntity1, true );
	}
	penetrateevent_t &event = m_penetrateEvents[index];
	event.timeStamp = gpGlobals->curtime;
	return event;
}



static ConVar phys_penetration_error_time( "phys_penetration_error_time", "10", 0, "Controls the duration of vphysics penetration error boxes." );

static bool CanResolvePenetrationWithNPC( CBaseEntity *pEntity, IPhysicsObject *pObject )
{
	if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		// hinged objects won't be able to be pushed out anyway, so don't try the npc solver
		if ( !pObject->IsHinged() && !pObject->IsAttachedToConstraint(true) )
		{
			if ( pObject->IsMoveable() || pEntity->GetServerVehicle() )
				return true;
		}
	}
	return false;
}

int CCollisionEvent::ShouldSolvePenetration( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1, float dt )
{
	CallbackContext check(this);
	
	// Pointers to the entity for each physics object
	CBaseEntity *pEntity0 = static_cast<CBaseEntity *>(pGameData0);
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pGameData1);

	// this can get called as entities are being constructed on the other side of a game load or level transition
	// Some entities may not be fully constructed, so don't call into their code until the level is running
	if ( g_PhysicsHook.m_bPaused )
		return true;

	// solve it yourself here and return 0, or have the default implementation do it
	if ( pEntity0 > pEntity1 )
	{
		// swap sort
		CBaseEntity *pTmp = pEntity0;
		pEntity0 = pEntity1;
		pEntity1 = pTmp;
		IPhysicsObject *pTmpObj = pObj0;
		pObj0 = pObj1;
		pObj1 = pTmpObj;
	}
	if ( pEntity0 == pEntity1 )
	{
		if ( pObj0->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL )
		{
			DevMsg(2, "Solving ragdoll self penetration! %s (%s) (%d v %d)\n", pObj0->GetName(), pEntity0->GetDebugName(), pObj0->GetGameIndex(), pObj1->GetGameIndex() );
			ragdoll_t *pRagdoll = Ragdoll_GetRagdoll( pEntity0 );
			pRagdoll->pGroup->SolvePenetration( pObj0, pObj1 );
			return false;
		}
	}
	penetrateevent_t &event = FindOrAddPenetrateEvent( pEntity0, pEntity1 );
	float eventTime = gpGlobals->curtime - event.startTime;
	 
	// NPC vs. physics object.  Create a game DLL solver and remove this event
	if ( (pEntity0->MyNPCPointer() && CanResolvePenetrationWithNPC(pEntity1, pObj1)) ||
		(pEntity1->MyNPCPointer() && CanResolvePenetrationWithNPC(pEntity0, pObj0)) )
	{ 
		event.collisionState = COLLSTATE_TRYNPCSOLVER;
	}
  
	if ( (IsDebris( pEntity0->GetCollisionGroup() ) && !pObj1->IsStatic()) || (IsDebris( pEntity1->GetCollisionGroup() ) && !pObj0->IsStatic()) )
	{
		if ( eventTime > 0.5f )
		{
			//Msg("Debris stuck in non-static!\n");
			event.collisionState = COLLSTATE_TRYENTITYSOLVER;
		}
	}
#if _DEBUG
	if ( phys_dontprintint.GetBool() == false )
	{
		const char *pName1 = STRING(pEntity0->GetModelName());
		const char *pName2 = STRING(pEntity1->GetModelName());
		if ( pEntity0 == pEntity1 )
		{
			int index0 = physcollision->CollideIndex( pObj0->GetCollide() );
			int index1 = physcollision->CollideIndex( pObj1->GetCollide() );
			DevMsg(1, "***Inter-penetration on %s (%d & %d) (%.0f, %.0f)\n", pName1?pName1:"(null)", index0, index1, gpGlobals->curtime, eventTime );
		}
		else
		{
			DevMsg(1, "***Inter-penetration between %s(%s) AND %s(%s) (%.0f, %.0f)\n", pName1?pName1:"(null)", pEntity0->GetDebugName(), pName2?pName2:"(null)", pEntity1->GetDebugName(), gpGlobals->curtime, eventTime );
		}
	}
#endif

	if ( eventTime > 3 )
	{
		// don't report penetrations on ragdolls with themselves, or outside of developer mode
		if ( g_pDeveloper->GetInt() && pEntity0 != pEntity1 )
		{
			ReportPenetration( pEntity0, phys_penetration_error_time.GetFloat() );
			ReportPenetration( pEntity1, phys_penetration_error_time.GetFloat() );
		}
		event.startTime = gpGlobals->curtime;
		// don't put players or game physics controlled objects to sleep
		if ( !pEntity0->IsPlayer() && !pEntity1->IsPlayer() && !pObj0->GetShadowController() && !pObj1->GetShadowController() )
		{
			// two objects have been stuck for more than 3 seconds, try disabling simulation
			event.collisionState = COLLSTATE_TRYDISABLE;
			return false;
		}
	}


	return true;
}


void CCollisionEvent::FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) 
{
	CallbackContext check(this);
	if ( ( pObject == NULL ) || ( pFluid == NULL ) )
		return;

	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	if ( !pEntity )
		return;

	pEntity->AddEFlags( EFL_TOUCHING_FLUID );
	pEntity->OnEntityEvent( ENTITY_EVENT_WATER_TOUCH, (void*)pFluid->GetContents() );

	float timeSinceLastCollision = DeltaTimeSinceLastFluid( pEntity );
	if ( timeSinceLastCollision < 0.5f )
		return;

	// UNDONE: Use this for splash logic instead?
	// UNDONE: Use angular term too - push splashes in rotAxs cross normal direction?
	Vector normal;
	float dist;
	pFluid->GetSurfacePlane( &normal, &dist );
	Vector vel;
	AngularImpulse angVel;
	pObject->GetVelocity( &vel, &angVel );
	Vector unitVel = vel;
	VectorNormalize( unitVel );
	
	// normal points out of the surface, we want the direction that points in
	float dragScale = pFluid->GetDensity() * physenv->GetSimulationTimestep();
	normal = -normal;
	float linearScale = 0.5f * DotProduct( unitVel, normal ) * pObject->CalculateLinearDrag( normal ) * dragScale;
	linearScale = clamp( linearScale, 0.0f, 1.0f );
	vel *= -linearScale;

	// UNDONE: Figure out how much of the surface area has crossed the water surface and scale angScale by that
	// For now assume 25%
	Vector rotAxis = angVel;
	VectorNormalize(rotAxis);
	float angScale = 0.25f * pObject->CalculateAngularDrag( angVel ) * dragScale;
	angScale = clamp( angScale, 0.0f, 1.0f );
	angVel *= -angScale;
	
	// compute the splash before we modify the velocity
	PhysicsSplash( pFluid, pObject, pEntity );

	// now damp out some motion toward the surface
	pObject->AddVelocity( &vel, &angVel );
}

void CCollisionEvent::FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid ) 
{
	CallbackContext check(this);
	if ( ( pObject == NULL ) || ( pFluid == NULL ) )
		return;

	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	if ( !pEntity )
		return;

	float timeSinceLastCollision = DeltaTimeSinceLastFluid( pEntity );
	if ( timeSinceLastCollision >= 0.5f )
	{
		PhysicsSplash( pFluid, pObject, pEntity );
	}

	pEntity->RemoveEFlags( EFL_TOUCHING_FLUID );
	pEntity->OnEntityEvent( ENTITY_EVENT_WATER_UNTOUCH, (void*)pFluid->GetContents() );
}

class CSkipKeys : public IVPhysicsKeyHandler
{
public:
	virtual void ParseKeyValue( void *pData, const char *pKey, const char *pValue ) {}
	virtual void SetDefaults( void *pData ) {}
};

void PhysSolidOverride( solid_t &solid, string_t overrideScript )
{
	if ( overrideScript != NULL_STRING)
	{
		// parser destroys this data
		bool collisions = solid.params.enableCollisions;

		char pTmpString[4096];

		// write a header for a solid_t
		Q_strncpy( pTmpString, "solid { ", sizeof(pTmpString) );

		// suck out the comma delimited tokens and turn them into quoted key/values
		char szToken[256];
		const char *pStr = nexttoken(szToken, STRING(overrideScript), ',');
		while ( szToken[0] != 0 )
		{
			Q_strncat( pTmpString, "\"", sizeof(pTmpString), COPY_ALL_CHARACTERS );
			Q_strncat( pTmpString, szToken, sizeof(pTmpString), COPY_ALL_CHARACTERS );
			Q_strncat( pTmpString, "\" ", sizeof(pTmpString), COPY_ALL_CHARACTERS );
			pStr = nexttoken(szToken, pStr, ',');
		}
		// terminate the script
		Q_strncat( pTmpString, "}", sizeof(pTmpString), COPY_ALL_CHARACTERS );

		// parse that sucker
		IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pTmpString );
		CSkipKeys tmp;
		pParse->ParseSolid( &solid, &tmp );
		physcollision->VPhysicsKeyParserDestroy( pParse );

		// parser destroys this data
		solid.params.enableCollisions = collisions;
	}
}

void PhysSetMassCenterOverride( masscenteroverride_t &override )
{
	if ( override.entityName != NULL_STRING )
	{
		g_PhysicsHook.m_massCenterOverrides.AddToTail( override );
	}
}

// NOTE: This will remove the entry from the list as well
int PhysGetMassCenterOverrideIndex( string_t name )
{
	if ( name != NULL_STRING && g_PhysicsHook.m_massCenterOverrides.Count() )
	{
		for ( int i = 0; i < g_PhysicsHook.m_massCenterOverrides.Count(); i++ )
		{
			if ( g_PhysicsHook.m_massCenterOverrides[i].entityName == name )
			{
				return i;
			}
		}
	}
	return -1;
}

void PhysGetMassCenterOverride( CBaseEntity *pEntity, vcollide_t *pCollide, solid_t &solidOut )
{
	int index = PhysGetMassCenterOverrideIndex( pEntity->GetEntityName() );

	if ( index >= 0 )
	{
		masscenteroverride_t &override = g_PhysicsHook.m_massCenterOverrides[index];
		Vector massCenterWS = override.center;
		switch ( override.alignType )
		{
		case masscenteroverride_t::ALIGN_POINT:
			VectorITransform( massCenterWS, pEntity->EntityToWorldTransform(), solidOut.massCenterOverride );
			break;
		case masscenteroverride_t::ALIGN_AXIS:
			{
				Vector massCenterLocal, defaultMassCenterWS;
				physcollision->CollideGetMassCenter( pCollide->solids[solidOut.index], &massCenterLocal );
				VectorTransform( massCenterLocal, pEntity->EntityToWorldTransform(), defaultMassCenterWS );
				massCenterWS += override.axis * 
					( DotProduct(defaultMassCenterWS,override.axis) - DotProduct( override.axis, override.center ) );
				VectorITransform( massCenterWS, pEntity->EntityToWorldTransform(), solidOut.massCenterOverride );
			}
			break;
		}
		g_PhysicsHook.m_massCenterOverrides.FastRemove( index );

		if ( solidOut.massCenterOverride.Length() > DIST_EPSILON )
		{
			solidOut.params.massCenterOverride = &solidOut.massCenterOverride;
		}
	}
}

float PhysGetEntityMass( CBaseEntity *pEntity )
{
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int physCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	float otherMass = 0;
	for ( int i = 0; i < physCount; i++ )
	{
		otherMass += pList[i]->GetMass();
	}

	return otherMass;
}


typedef void (*EntityCallbackFunction) ( CBaseEntity *pEntity );

void IterateActivePhysicsEntities( EntityCallbackFunction func )
{
	int activeCount = physenv->GetActiveObjectCount();
	IPhysicsObject **pActiveList = NULL;
	if ( activeCount )
	{
		pActiveList = (IPhysicsObject **)stackalloc( sizeof(IPhysicsObject *)*activeCount );
		physenv->GetActiveObjects( pActiveList );
		for ( int i = 0; i < activeCount; i++ )
		{
			CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pActiveList[i]->GetGameData());
			if ( pEntity )
			{
				func( pEntity );
			}
		}
	}
}


static void CallbackHighlight( CBaseEntity *pEntity )
{
	pEntity->m_debugOverlays |= OVERLAY_ABSBOX_BIT | OVERLAY_PIVOT_BIT;
}

static void CallbackReport( CBaseEntity *pEntity )
{
	const char *pName = STRING(pEntity->GetEntityName());
	if ( !Q_strlen(pName) )
	{
		pName = STRING(pEntity->GetModelName());
	}
	Msg( "%s - %s\n", pEntity->GetClassname(), pName );
}

CON_COMMAND(physics_highlight_active, "Turns on the absbox for all active physics objects")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	IterateActivePhysicsEntities( CallbackHighlight );
}

CON_COMMAND(physics_report_active, "Lists all active physics objects")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	IterateActivePhysicsEntities( CallbackReport );
}

CON_COMMAND_F(surfaceprop, "Reports the surface properties at the cursor", FCVAR_CHEAT )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	CBasePlayer *pPlayer = UTIL_GetCommandClient();

	trace_t tr;
	Vector forward;
	pPlayer->EyeVectors( &forward );
	UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE,
		MASK_SHOT_HULL|CONTENTS_GRATE|CONTENTS_DEBRIS, pPlayer, COLLISION_GROUP_NONE, &tr );

	if ( tr.DidHit() )
	{
		const model_t *pModel = modelinfo->GetModel( tr.m_pEnt->GetModelIndex() );
		const char *pModelName = STRING(tr.m_pEnt->GetModelName());
		if ( tr.DidHitWorld() && tr.hitbox > 0 )
		{
			ICollideable *pCollide = staticpropmgr->GetStaticPropByIndex( tr.hitbox-1 );
			pModel = pCollide->GetCollisionModel();
			pModelName = modelinfo->GetModelName( pModel );
		}
		CFmtStr modelStuff;
		if ( pModel )
		{
			modelStuff.sprintf("%s.%s ", modelinfo->IsTranslucent( pModel ) ? "Translucent" : "Opaque", 
				modelinfo->IsTranslucentTwoPass( pModel ) ? "  Two-pass." : "" );
		}
		
		// Calculate distance to surface that was hit
		Vector vecVelocity = tr.startpos - tr.endpos;
		int length = vecVelocity.Length();

		Msg("Hit surface \"%s\" (entity %s, model \"%s\" %s), texture \"%s\"\n", physprops->GetPropName( tr.surface.surfaceProps ), tr.m_pEnt->GetClassname(), pModelName, modelStuff.Access(), tr.surface.name);
		Msg("Distance to surface: %d\n", length );
	}
}

static void OutputVPhysicsDebugInfo( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		Msg("Entity %s (%s) %s Collision Group %d\n", pEntity->GetClassname(), pEntity->GetDebugName(), pEntity->IsNavIgnored() ? "NAV IGNORE" : "", pEntity->GetCollisionGroup() );
		CUtlVector<CBaseEntity *> list;
		g_Collisions.GetListOfPenetratingEntities( pEntity, list );
		for ( int i = 0; i < list.Count(); i++ )
		{
			Msg("  penetration with entity %s (%s)\n", list[i]->GetDebugName(), STRING(list[i]->GetModelName()) );
		}

		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int physCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
		if ( physCount )
		{
			if ( physCount > 1 )
			{
				for ( int i = 0; i < physCount; i++ )
				{
					Msg("Object %d (of %d) =========================\n", i+1, physCount );
					pList[i]->OutputDebugInfo();
				}
			}
			else
			{
				pList[0]->OutputDebugInfo();
			}
		}
	}
}

class CConstraintFloodEntry
{
public:
	CConstraintFloodEntry() : isMarked(false), isConstraint(false) {}

	CUtlVector<CBaseEntity *> linkList;
	bool isMarked;
	bool isConstraint;
};

class CConstraintFloodList
{
public:
	CConstraintFloodList()
	{
		SetDefLessFunc( m_list );
		m_list.EnsureCapacity(64);
		m_entryList.EnsureCapacity(64);
	}

	bool IsWorldEntity( CBaseEntity *pEnt )
	{
		if ( pEnt->edict() )
			return pEnt->IsWorld();
		return false;
	}

	void AddLink( CBaseEntity *pEntity, CBaseEntity *pLink, bool bIsConstraint )
	{
		if ( !pEntity || !pLink || IsWorldEntity(pEntity) || IsWorldEntity(pLink) )
			return;
		int listIndex = m_list.Find(pEntity);
		if ( listIndex == m_list.InvalidIndex() )
		{
			int entryIndex = m_entryList.AddToTail();
			m_entryList[entryIndex].isConstraint = bIsConstraint;
			listIndex = m_list.Insert( pEntity, entryIndex );
		}
		int entryIndex = m_list.Element(listIndex);
		CConstraintFloodEntry &entry = m_entryList.Element(entryIndex);
		Assert( entry.isConstraint == bIsConstraint );
		if ( entry.linkList.Find(pLink) < 0 )
		{
			entry.linkList.AddToTail( pLink );
		}
	}

	void BuildGraphFromEntity( CBaseEntity *pEntity, CUtlVector<CBaseEntity *> &constraintList )
	{
		int listIndex = m_list.Find(pEntity);
		if ( listIndex != m_list.InvalidIndex() )
		{
			int entryIndex = m_list.Element(listIndex);
			CConstraintFloodEntry &entry = m_entryList.Element(entryIndex);
			if ( !entry.isMarked )
			{
				if ( entry.isConstraint )
				{
					Assert( constraintList.Find(pEntity) < 0);
					constraintList.AddToTail( pEntity );
				}
				entry.isMarked = true;
				for ( int i = 0; i < entry.linkList.Count(); i++ )
				{
					// now recursively traverse the graph from here
					BuildGraphFromEntity( entry.linkList[i], constraintList );
				}
			}
		}
	}
	CUtlMap<CBaseEntity *, int>	m_list;
	CUtlVector<CConstraintFloodEntry> m_entryList;
};

// traverses the graph of attachments (currently supports springs & constraints) starting at an entity
// Then turns on debug info for each link in the graph (springs/constraints are links)
static void DebugConstraints( CBaseEntity *pEntity )
{
	extern bool GetSpringAttachments( CBaseEntity *pEntity, CBaseEntity *pAttach[2], IPhysicsObject *pAttachVPhysics[2] );
	extern bool GetConstraintAttachments( CBaseEntity *pEntity, CBaseEntity *pAttach[2], IPhysicsObject *pAttachVPhysics[2] );
	extern void DebugConstraint(CBaseEntity *pEntity);

	if ( !pEntity )
		return;

	CBaseEntity *pAttach[2];
	IPhysicsObject *pAttachVPhysics[2];
	CConstraintFloodList list;

	for ( CBaseEntity *pList = gEntList.FirstEnt(); pList != NULL; pList = gEntList.NextEnt(pList) )
	{
		if ( GetConstraintAttachments(pList, pAttach, pAttachVPhysics) || GetSpringAttachments(pList, pAttach, pAttachVPhysics) )
		{
			list.AddLink( pList, pAttach[0], true );
			list.AddLink( pList, pAttach[1], true );
			list.AddLink( pAttach[0], pList, false );
			list.AddLink( pAttach[1], pList, false );
		}
	}

	CUtlVector<CBaseEntity *> constraints;
	list.BuildGraphFromEntity( pEntity, constraints );
	for ( int i = 0; i < constraints.Count(); i++ )
	{
		if ( !GetConstraintAttachments(constraints[i], pAttach, pAttachVPhysics) )
		{
			GetSpringAttachments(constraints[i], pAttach, pAttachVPhysics);
		}
		const char *pName0 = "world";
		const char *pName1 = "world";
		const char *pModel0 = "";
		const char *pModel1 = "";
		int index0 = 0;
		int index1 = 0;
		if ( pAttach[0] )
		{
			pName0 = pAttach[0]->GetClassname();
			pModel0 = STRING(pAttach[0]->GetModelName());
			index0 = pAttachVPhysics[0]->GetGameIndex();
		}
		if ( pAttach[1] )
		{
			pName1 = pAttach[1]->GetClassname();
			pModel1 = STRING(pAttach[1]->GetModelName());
			index1 = pAttachVPhysics[1]->GetGameIndex();
		}
		Msg("**********************\n%s connects %s(%s:%d) to %s(%s:%d)\n", constraints[i]->GetClassname(), pName0, pModel0, index0, pName1, pModel1, index1 );
		DebugConstraint(constraints[i]);
		constraints[i]->m_debugOverlays |= OVERLAY_BBOX_BIT | OVERLAY_TEXT_BIT;
	}
}

static void MarkVPhysicsDebug( CBaseEntity *pEntity )
{
	if ( pEntity )
	{
		IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
		if ( pPhysics )
		{
			unsigned short callbacks = pPhysics->GetCallbackFlags();
			callbacks ^= CALLBACK_MARKED_FOR_TEST;
			pPhysics->SetCallbackFlags( callbacks );
		}
	}
}

void PhysicsCommand( const CCommand &args, void (*func)( CBaseEntity *pEntity ) )
{
	if ( args.ArgC() < 2 )
	{
		CBasePlayer *pPlayer = UTIL_GetCommandClient();

		trace_t tr;
		Vector forward;
		pPlayer->EyeVectors( &forward );
		UTIL_TraceLine(pPlayer->EyePosition(), pPlayer->EyePosition() + forward * MAX_COORD_RANGE,
			MASK_SHOT_HULL|CONTENTS_GRATE|CONTENTS_DEBRIS, pPlayer, COLLISION_GROUP_NONE, &tr );

		if ( tr.DidHit() )
		{
			func( tr.m_pEnt );
		}
	}
	else
	{
		CBaseEntity *pEnt = NULL;
		while ( ( pEnt = gEntList.FindEntityGeneric( pEnt, args[1] ) ) != NULL )
		{
			func( pEnt );
		}
	}
}

CON_COMMAND(physics_constraints, "Highlights constraint system graph for an entity")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	PhysicsCommand( args, DebugConstraints );
}

CON_COMMAND(physics_debug_entity, "Dumps debug info for an entity")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	PhysicsCommand( args, OutputVPhysicsDebugInfo );
}

CON_COMMAND(physics_select, "Dumps debug info for an entity")
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	PhysicsCommand( args, MarkVPhysicsDebug );
}

CON_COMMAND( physics_budget, "Times the cost of each active object" )
{
	if ( !UTIL_IsCommandIssuedByServerAdmin() )
		return;

	int activeCount = physenv->GetActiveObjectCount();

	IPhysicsObject **pActiveList = NULL;
	CUtlVector<CBaseEntity *> ents;
	if ( activeCount )
	{
		int i;

		pActiveList = (IPhysicsObject **)stackalloc( sizeof(IPhysicsObject *)*activeCount );
		physenv->GetActiveObjects( pActiveList );
		for ( i = 0; i < activeCount; i++ )
		{
			CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pActiveList[i]->GetGameData());
			if ( pEntity )
			{
				int index = -1;
				for ( int j = 0; j < ents.Count(); j++ )
				{
					if ( pEntity == ents[j] )
					{
						index = j;
						break;
					}
				}
				if ( index >= 0 )
					continue;

				ents.AddToTail( pEntity );
			}
		}
		stackfree( pActiveList );

		if ( !ents.Count() )
			return;

		CUtlVector<float> times;
		float totalTime = 0.f;
		g_Collisions.BufferTouchEvents( true );
		float full = engine->Time();
		physenv->Simulate( gpGlobals->interval_per_tick );
		full = engine->Time() - full;
		float lastTime = full;

		times.SetSize( ents.Count() );


		// NOTE: This is just a heuristic.  Attempt to estimate cost by putting each object to sleep in turn.
		//	note that simulation may wake the objects again and some costs scale with sets of objects/constraints/etc
		//	so these are only generally useful for broad questions, not real metrics!
		for ( i = 0; i < ents.Count(); i++ )
		{
			for ( int j = 0; j < i; j++ )
			{
				PhysForceEntityToSleep( ents[j], ents[j]->VPhysicsGetObject() );
			}
			float start = engine->Time();
			physenv->Simulate( gpGlobals->interval_per_tick );
			float end = engine->Time();

			float elapsed = end - start;
			float avgTime = lastTime - elapsed;
			times[i] = clamp( avgTime, 0.00001f, 1.0f );
			totalTime += times[i];
			lastTime = elapsed;
 		}

		totalTime = MAX( totalTime, 0.001 );
		for ( i = 0; i < ents.Count(); i++ )
		{
			float fraction = times[i] / totalTime;
			Msg( "%s (%s): %.3fms (%.3f%%) @ %s\n", ents[i]->GetClassname(), ents[i]->GetDebugName(), fraction * totalTime * 1000.0f, fraction * 100.0f, VecToString(ents[i]->GetAbsOrigin()) );
		}
		g_Collisions.BufferTouchEvents( false );
	}

}


#ifdef PORTAL
ConVar sv_fullsyncclones("sv_fullsyncclones", "1", FCVAR_CHEAT );
void PortalPhysFrame( float deltaTime ) //small wrapper for PhysFrame that simulates all environments at once
{
	CPortalSimulator::PrePhysFrame();

	if( sv_fullsyncclones.GetBool() )
		CPhysicsShadowClone::FullSyncAllClones();

	g_Collisions.BufferTouchEvents( true );

	PhysFrame( deltaTime );

	g_Collisions.PortalPostSimulationFrame();

	g_Collisions.BufferTouchEvents( false );
	g_Collisions.FrameUpdate();

	CPortalSimulator::PostPhysFrame();
}
#endif

// Advance physics by time (in seconds)
void PhysFrame( float deltaTime )
{
	static int lastObjectCount = 0;
	entitem_t *pItem;

	if ( !g_PhysicsHook.ShouldSimulate() )
		return;

	// Trap interrupts and clock changes
	if ( deltaTime > 1.0f || deltaTime < 0.0f )
	{
		deltaTime = 0;
		Msg( "Reset physics clock\n" );
	}
	else if ( deltaTime > 0.1f )	// limit incoming time to 100ms
	{
		deltaTime = 0.1f;
	}
	float simRealTime = 0;

	deltaTime *= phys_timescale.GetFloat();
	// !!!HACKHACK -- hard limit scaled time to avoid spending too much time in here
	// Limit to 100 ms
	if ( deltaTime > 0.100f )
		deltaTime = 0.100f;

	bool bProfile = phys_speeds.GetBool();

	if ( bProfile )
	{
		simRealTime = engine->Time();
	}

#ifdef _DEBUG
	physenv->DebugCheckContacts();
#endif

#ifndef PORTAL //instead of wrapping 1 simulation with this, portal needs to wrap 3
	g_Collisions.BufferTouchEvents( true );
#endif

	physenv->Simulate( deltaTime );

	int activeCount = physenv->GetActiveObjectCount();
	IPhysicsObject **pActiveList = NULL;
	if ( activeCount )
	{
		pActiveList = (IPhysicsObject **)stackalloc( sizeof(IPhysicsObject *)*activeCount );
		physenv->GetActiveObjects( pActiveList );

		for ( int i = 0; i < activeCount; i++ )
		{
			CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pActiveList[i]->GetGameData());
			if ( pEntity )
			{
				if ( pEntity->CollisionProp()->DoesVPhysicsInvalidateSurroundingBox() )
				{
					pEntity->CollisionProp()->MarkSurroundingBoundsDirty();
				}
				pEntity->VPhysicsUpdate( pActiveList[i] );
			}
		}
		stackfree( pActiveList );
	}

	for ( pItem = g_pShadowEntities->m_pItemList; pItem; pItem = pItem->pNext )
	{
		CBaseEntity *pEntity = pItem->hEnt.Get();
		if ( !pEntity )
		{
			Msg( "Dangling pointer to physics entity!!!\n" );
			continue;
		}

		IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
		// apply updates
		if ( pPhysics && !pPhysics->IsAsleep() )
		{
			pEntity->VPhysicsShadowUpdate( pPhysics );
		}
	}

	if ( bProfile )
	{
		simRealTime = engine->Time() - simRealTime;

		if ( simRealTime < 0 )
			simRealTime = 0;
		g_PhysAverageSimTime *= 0.8;
		g_PhysAverageSimTime += (simRealTime * 0.2);
		if ( lastObjectCount != 0 || activeCount != 0 )
		{
			Msg( "Physics: %3d objects, %4.1fms / AVG: %4.1fms\n", activeCount, simRealTime * 1000, g_PhysAverageSimTime * 1000 );
		}

		lastObjectCount = activeCount;
	}

#ifndef PORTAL //instead of wrapping 1 simulation with this, portal needs to wrap 3
	g_Collisions.BufferTouchEvents( false );
	g_Collisions.FrameUpdate();
#endif
}


void PhysAddShadow( CBaseEntity *pEntity )
{
	g_pShadowEntities->AddEntity( pEntity );
}

void PhysRemoveShadow( CBaseEntity *pEntity )
{
	g_pShadowEntities->DeleteEntity( pEntity );
}

bool PhysHasShadow( CBaseEntity *pEntity )
{
	EHANDLE hTestEnt = pEntity;
	entitem_t *pCurrent = g_pShadowEntities->m_pItemList;
	while( pCurrent )
	{
		if( pCurrent->hEnt == hTestEnt )
		{
			return true;
		}
		pCurrent = pCurrent->pNext;
	}
	return false;
}

void PhysEnableFloating( IPhysicsObject *pObject, bool bEnable )
{
	if ( pObject != NULL )
	{
		unsigned short flags = pObject->GetCallbackFlags();
		if ( bEnable )
		{
			flags |= CALLBACK_DO_FLUID_SIMULATION;
		}
		else
		{
			flags &= ~CALLBACK_DO_FLUID_SIMULATION;
		}
		pObject->SetCallbackFlags( flags );
	}
}


//-----------------------------------------------------------------------------
// CollisionEvent system 
//-----------------------------------------------------------------------------
// NOTE: PreCollision/PostCollision ALWAYS come in matched pairs!!!
void CCollisionEvent::PreCollision( vcollisionevent_t *pEvent )
{
	CallbackContext check(this);
	m_gameEvent.Init( pEvent );

	// gather the pre-collision data that the game needs to track
	for ( int i = 0; i < 2; i++ )
	{
		IPhysicsObject *pObject = pEvent->pObjects[i];
		if ( pObject )
		{
			if ( pObject->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			{
				CBaseEntity *pOtherEntity = reinterpret_cast<CBaseEntity *>(pEvent->pObjects[!i]->GetGameData());
				if ( pOtherEntity && !pOtherEntity->IsPlayer() )
				{
					Vector velocity;
					AngularImpulse angVel;
					// HACKHACK: If we totally clear this out, then Havok will think the objects
					// are penetrating and generate forces to separate them
					// so make it fairly small and have a tiny collision instead.
					pObject->GetVelocity( &velocity, &angVel );
					float len = VectorNormalize(velocity);
					len = MAX( len, 10 );
					velocity *= len;
					len = VectorNormalize(angVel);
					len = MAX( len, 1 );
					angVel *= len;
					pObject->SetVelocity( &velocity, &angVel );
				}
			}
			pObject->GetVelocity( &m_gameEvent.preVelocity[i], &m_gameEvent.preAngularVelocity[i] );
		}
	}
}

void CCollisionEvent::PostCollision( vcollisionevent_t *pEvent )
{
	CallbackContext check(this);
	bool isShadow[2] = {false,false};
	int i;

	for ( i = 0; i < 2; i++ )
	{
		IPhysicsObject *pObject = pEvent->pObjects[i];
		if ( pObject )
		{
			CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pObject->GetGameData());
			if ( !pEntity )
				return;

			// UNDONE: This is here to trap crashes due to NULLing out the game data on delete
			m_gameEvent.pEntities[i] = pEntity;
			unsigned int flags = pObject->GetCallbackFlags();
			pObject->GetVelocity( &m_gameEvent.postVelocity[i], NULL );
			if ( flags & CALLBACK_SHADOW_COLLISION )
			{
				isShadow[i] = true;
			}

			// Shouldn't get impacts with triggers
			Assert( !pObject->IsTrigger() );
		}
	}

	// copy off the post-collision variable data
	m_gameEvent.collisionSpeed = pEvent->collisionSpeed;
	m_gameEvent.pInternalData = pEvent->pInternalData;

	// special case for hitting self, only make one non-shadow call
	if ( m_gameEvent.pEntities[0] == m_gameEvent.pEntities[1] )
	{
		if ( pEvent->isCollision && m_gameEvent.pEntities[0] )
		{
			m_gameEvent.pEntities[0]->VPhysicsCollision( 0, &m_gameEvent );
		}
		return;
	}

	if ( isShadow[0] && isShadow[1] )
	{
		pEvent->isCollision = false;
	}

	for ( i = 0; i < 2; i++ )
	{
		if ( pEvent->isCollision )
		{
			m_gameEvent.pEntities[i]->VPhysicsCollision( i, &m_gameEvent );
		}
		if ( pEvent->isShadowCollision && isShadow[i] )
		{
			m_gameEvent.pEntities[i]->VPhysicsShadowCollision( i, &m_gameEvent );
		}
	}
}

void PhysForceEntityToSleep( CBaseEntity *pEntity, IPhysicsObject *pObject )
{
	// UNDONE: Check to see if the object is touching the player first?
	// Might get the player stuck?
	if ( !pObject || !pObject->IsMoveable() )
		return;

	DevMsg(2, "Putting entity to sleep: %s\n", pEntity->GetClassname() );
	MEM_ALLOC_CREDIT();
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int physCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	for ( int i = 0; i < physCount; i++ )
	{
		PhysForceClearVelocity( pList[i] );
		pList[i]->Sleep();
	}
}

void CCollisionEvent::Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData )
{
	CallbackContext check(this);
	//Get our friction information
	Vector vecPos, vecVel;
	pData->GetContactPoint( vecPos );
	pObject->GetVelocityAtPoint( vecPos, &vecVel );

	CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pObject->GetGameData());
		
	if ( pEntity  )
	{
		friction_t *pFriction = g_Collisions.FindFriction( pEntity );

		if ( pFriction && pFriction->pObject) 
		{
			// in MP mode play sound and effects once every 500 msecs,
			// no ongoing updates, takes too much bandwidth
			if ( (pFriction->flLastEffectTime + 0.5f) > gpGlobals->curtime)
			{
				pFriction->flLastUpdateTime = gpGlobals->curtime;
				return; 			
			}
		}

		pEntity->VPhysicsFriction( pObject, energy, surfaceProps, surfacePropsHit );
	}

	PhysFrictionEffect( vecPos, vecVel, energy, surfaceProps, surfacePropsHit );
}


friction_t *CCollisionEvent::FindFriction( CBaseEntity *pObject )
{
	friction_t *pFree = NULL;

	for ( int i = 0; i < ARRAYSIZE(m_current); i++ )
	{
		if ( !m_current[i].pObject && !pFree )
			pFree = &m_current[i];

		if ( m_current[i].pObject == pObject )
			return &m_current[i];
	}

	return pFree;
}

void CCollisionEvent::ShutdownFriction( friction_t &friction )
{
//	Msg( "Scrape Stop %s \n", STRING(friction.pObject->m_iClassname) );
	CSoundEnvelopeController::GetController().SoundDestroy( friction.patch );
	friction.patch = NULL;
	friction.pObject = NULL;
}

void CCollisionEvent::UpdateRemoveObjects()
{
	Assert(!PhysIsInCallback());
	for ( int i = 0 ; i < m_removeObjects.Count(); i++ )
	{
		UTIL_Remove(m_removeObjects[i]);
	}
	m_removeObjects.RemoveAll();
}

void CCollisionEvent::PostSimulationFrame()
{
	UpdateDamageEvents();
	g_PostSimulationQueue.CallQueued();
	UpdateRemoveObjects();
}

void CCollisionEvent::FlushQueuedOperations()
{
	int loopCount = 0;
	while ( loopCount < 20 )
	{
		int count = m_triggerEvents.Count() + m_touchEvents.Count() + m_damageEvents.Count() + m_removeObjects.Count() + g_PostSimulationQueue.Count();
		if ( !count )
			break;
		// testing, if this assert fires it proves we've fixed the crash
		// after that the assert + warning can safely be removed
		Assert(0);
		Warning("Physics queue not empty, error!\n");
		loopCount++;
		UpdateTouchEvents();
		UpdateDamageEvents();
		g_PostSimulationQueue.CallQueued();
		UpdateRemoveObjects();
	}
}

void CCollisionEvent::FrameUpdate( void )
{
	UpdateFrictionSounds();
	UpdateTouchEvents();
	UpdatePenetrateEvents();
	UpdateFluidEvents();
	UpdateDamageEvents(); // if there was no PSI in physics, we'll still need to do some of these because collisions are solved in between PSIs
	g_PostSimulationQueue.CallQueued();
	UpdateRemoveObjects();

	// There are some queued operations that must complete each frame, iterate until these are done
	FlushQueuedOperations();
}

// the delete list is getting flushed, clean up ours
void PhysOnCleanupDeleteList()
{
	g_Collisions.FlushQueuedOperations();
	if ( physenv )
	{
		physenv->CleanupDeleteList();
	}
}

void CCollisionEvent::UpdateFluidEvents( void )
{
	for ( int i = m_fluidEvents.Count()-1; i >= 0; --i )
	{
		if ( (gpGlobals->curtime - m_fluidEvents[i].impactTime) > FLUID_TIME_MAX )
		{
			m_fluidEvents.FastRemove(i);
		}
	}
}


float CCollisionEvent::DeltaTimeSinceLastFluid( CBaseEntity *pEntity )
{
	for ( int i = m_fluidEvents.Count()-1; i >= 0; --i )
	{
		if ( m_fluidEvents[i].hEntity.Get() == pEntity )
		{
			return gpGlobals->curtime - m_fluidEvents[i].impactTime;
		}
	}

	int index = m_fluidEvents.AddToTail();
	m_fluidEvents[index].hEntity = pEntity;
	m_fluidEvents[index].impactTime = gpGlobals->curtime;
	return FLUID_TIME_MAX;
}

void CCollisionEvent::UpdateFrictionSounds( void )
{
	for ( int i = 0; i < ARRAYSIZE(m_current); i++ )
	{
		if ( m_current[i].patch )
		{
			if ( m_current[i].flLastUpdateTime < (gpGlobals->curtime-0.1f) )
			{
				// friction wasn't updated the last 100msec, assume fiction finished
				ShutdownFriction( m_current[i] );
			}
		}
	}
}


void CCollisionEvent::DispatchStartTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1, const Vector &point, const Vector &normal )
{
	trace_t trace;
	memset( &trace, 0, sizeof(trace) );
	trace.endpos = point;
	trace.plane.dist = DotProduct( point, normal );
	trace.plane.normal = normal;

	// NOTE: This sets up the touch list for both entities, no call to pEntity1 is needed
	pEntity0->PhysicsMarkEntitiesAsTouchingEventDriven( pEntity1, trace );
}

void CCollisionEvent::DispatchEndTouch( CBaseEntity *pEntity0, CBaseEntity *pEntity1 )
{
	// frees the event-driven touchlinks
	pEntity0->PhysicsNotifyOtherOfUntouch( pEntity0, pEntity1 );
	pEntity1->PhysicsNotifyOtherOfUntouch( pEntity1, pEntity0 );
}

void CCollisionEvent::UpdateTouchEvents( void )
{
	int i;
	// Turn on buffering in case new touch events occur during processing
	bool bOldTouchEvents = m_bBufferTouchEvents;
	m_bBufferTouchEvents = true;
	for ( i = 0; i < m_touchEvents.Count(); i++ )
	{
		const touchevent_t &event = m_touchEvents[i];
		if ( event.touchType == TOUCH_START )
		{
			DispatchStartTouch( event.pEntity0, event.pEntity1, event.endPoint, event.normal );
		}
		else
		{
			// TOUCH_END
			DispatchEndTouch( event.pEntity0, event.pEntity1 );
		}
	}
	m_touchEvents.RemoveAll();

	for ( i = 0; i < m_triggerEvents.Count(); i++ )
	{
		m_currentTriggerEvent = m_triggerEvents[i];
		if ( m_currentTriggerEvent.bStart )
		{
			m_currentTriggerEvent.pTriggerEntity->StartTouch( m_currentTriggerEvent.pEntity );
		}
		else
		{
			m_currentTriggerEvent.pTriggerEntity->EndTouch( m_currentTriggerEvent.pEntity );
		}
	}
	m_triggerEvents.RemoveAll();
	m_currentTriggerEvent.Clear();
	m_bBufferTouchEvents = bOldTouchEvents;
}

void CCollisionEvent::UpdateDamageEvents( void )
{
	for ( int i = 0; i < m_damageEvents.Count(); i++ )
	{
		damageevent_t &event = m_damageEvents[i];

		// Track changes in the entity's life state
		int iEntBits = event.pEntity->IsAlive() ? 0x0001 : 0;
		iEntBits |= event.pEntity->IsMarkedForDeletion() ? 0x0002 : 0;
		iEntBits |= (event.pEntity->GetSolidFlags() & FSOLID_NOT_SOLID) ? 0x0004 : 0;
#if 0
		// Go ahead and compute the current static stress when hit by a large object (with a force high enough to do damage).  
		// That way you die from the impact rather than the stress of the object resting on you whenever possible. 
		// This makes the damage effects cleaner.
		if ( event.pInflictorPhysics && event.pInflictorPhysics->GetMass() > VPHYSICS_LARGE_OBJECT_MASS )
		{
			CBaseCombatCharacter *pCombat = event.pEntity->MyCombatCharacterPointer();
			if ( pCombat )
			{
				vphysics_objectstress_t stressOut;
				event.info.AddDamage( pCombat->CalculatePhysicsStressDamage( &stressOut, pCombat->VPhysicsGetObject() ) );
			}
		}
#endif

		event.pEntity->TakeDamage( event.info );
		int iEntBits2 = event.pEntity->IsAlive() ? 0x0001 : 0;
		iEntBits2 |= event.pEntity->IsMarkedForDeletion() ? 0x0002 : 0;
		iEntBits2 |= (event.pEntity->GetSolidFlags() & FSOLID_NOT_SOLID) ? 0x0004 : 0;

		if ( event.bRestoreVelocity && iEntBits != iEntBits2 )
		{
			// UNDONE: Use ratio of masses to blend in a little of the collision response?
			// UNDONE: Damage for future events is already computed - it would be nice to
			//			go back and recompute it now that the values have
			//			been adjusted
			RestoreDamageInflictorState( event.pInflictorPhysics );
		}
	}
	m_damageEvents.RemoveAll();
	m_damageInflictors.RemoveAll();
}

void CCollisionEvent::RestoreDamageInflictorState( int inflictorStateIndex, float velocityBlend )
{
	inflictorstate_t &state = m_damageInflictors[inflictorStateIndex];
	if ( state.restored )
		return;

	// so we only restore this guy once
	state.restored = true;

	if ( velocityBlend > 0 )
	{
		Vector velocity;
		AngularImpulse angVel;
		state.pInflictorPhysics->GetVelocity( &velocity, &angVel );
		state.savedVelocity = state.savedVelocity*velocityBlend + velocity*(1-velocityBlend);
		state.savedAngularVelocity = state.savedAngularVelocity*velocityBlend + angVel*(1-velocityBlend);
		state.pInflictorPhysics->SetVelocity( &state.savedVelocity, &state.savedAngularVelocity );
	}

	if ( state.nextIndex >= 0 )
	{
		RestoreDamageInflictorState( state.nextIndex, velocityBlend );
	}
}

void CCollisionEvent::RestoreDamageInflictorState( IPhysicsObject *pInflictor )
{
	if ( !pInflictor )
		return;

	int index = FindDamageInflictor( pInflictor );
	if ( index >= 0 )
	{
		inflictorstate_t &state = m_damageInflictors[index];
		if ( !state.restored )
		{
			float velocityBlend = 1.0;
			float inflictorMass = state.pInflictorPhysics->GetMass();
			if ( inflictorMass < VPHYSICS_LARGE_OBJECT_MASS && !(state.pInflictorPhysics->GetGameFlags() & FVPHYSICS_DMG_SLICE) )
			{
				float otherMass = state.otherMassMax > 0 ? state.otherMassMax : 1;
				float massRatio = inflictorMass / otherMass;
				massRatio = clamp( massRatio, 0.1f, 10.0f );
				if ( massRatio < 1 )
				{
					velocityBlend = RemapVal( massRatio, 0.1, 1, 0, 0.5 );
				}
				else
				{
					velocityBlend = RemapVal( massRatio, 1.0, 10, 0.5, 1 );
				}
			}
			RestoreDamageInflictorState( index, velocityBlend );
		}
	}
}

bool CCollisionEvent::GetInflictorVelocity( IPhysicsObject *pInflictor, Vector &velocity, AngularImpulse &angVelocity )
{
	int index = FindDamageInflictor( pInflictor );
	if ( index >= 0 )
	{
		inflictorstate_t &state = m_damageInflictors[index];
		velocity = state.savedVelocity;
		angVelocity = state.savedAngularVelocity;
		return true;
	}

	return false;
}

bool PhysGetDamageInflictorVelocityStartOfFrame( IPhysicsObject *pInflictor, Vector &velocity, AngularImpulse &angVelocity )
{
	return g_Collisions.GetInflictorVelocity( pInflictor, velocity, angVelocity );
}

void CCollisionEvent::AddTouchEvent( CBaseEntity *pEntity0, CBaseEntity *pEntity1, int touchType, const Vector &point, const Vector &normal )
{
	if ( !pEntity0 || !pEntity1 )
		return;

	int index = m_touchEvents.AddToTail();
	touchevent_t &event = m_touchEvents[index];
	event.pEntity0 = pEntity0;
	event.pEntity1 = pEntity1;
	event.touchType = touchType;
	event.endPoint = point;
	event.normal = normal;
}

void CCollisionEvent::AddDamageEvent( CBaseEntity *pEntity, const CTakeDamageInfo &info, IPhysicsObject *pInflictorPhysics, bool bRestoreVelocity, const Vector &savedVel, const AngularImpulse &savedAngVel )
{
	if ( pEntity->IsMarkedForDeletion() )
		return;

	int iTimeBasedDamage = g_pGameRules->Damage_GetTimeBased();
	if ( !( info.GetDamageType() & (DMG_BURN | DMG_DROWN | iTimeBasedDamage | DMG_PREVENT_PHYSICS_FORCE) ) )
	{
		Assert( info.GetDamageForce() != vec3_origin && info.GetDamagePosition() != vec3_origin );
	}

	int index = m_damageEvents.AddToTail();
	damageevent_t &event = m_damageEvents[index];
	event.pEntity = pEntity;
	event.info = info;
	event.pInflictorPhysics = pInflictorPhysics;
	event.bRestoreVelocity = bRestoreVelocity;
	if ( !pInflictorPhysics || !pInflictorPhysics->IsMoveable() )
	{
		event.bRestoreVelocity = false;
	}

	if ( event.bRestoreVelocity )
	{
		float otherMass = pEntity->VPhysicsGetObject()->GetMass();
		int inflictorIndex = FindDamageInflictor(pInflictorPhysics);
		if ( inflictorIndex >= 0 )
		{
			// if this is a bigger mass, save that info
			inflictorstate_t &state = m_damageInflictors[inflictorIndex];
			if ( otherMass > state.otherMassMax )
			{
				state.otherMassMax = otherMass;
			}

		}
		else
		{
			AddDamageInflictor( pInflictorPhysics, otherMass, savedVel, savedAngVel, true );
		}
	}

}

//-----------------------------------------------------------------------------
// Impulse events
//-----------------------------------------------------------------------------
static void PostSimulation_ImpulseEvent( IPhysicsObject *pObject, const Vector &centerForce, const AngularImpulse &centerTorque )
{
	pObject->ApplyForceCenter( centerForce );
	pObject->ApplyTorqueCenter( centerTorque );
}

void PostSimulation_SetVelocityEvent( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	pPhysicsObject->SetVelocity( &vecVelocity, NULL );
}

void CCollisionEvent::AddRemoveObject(IServerNetworkable *pRemove)
{
	if ( pRemove && m_removeObjects.Find(pRemove) == -1 )
	{
		m_removeObjects.AddToTail(pRemove);
	}
}
int CCollisionEvent::FindDamageInflictor( IPhysicsObject *pInflictorPhysics )
{
	// UNDONE: Linear search?  Probably ok with a low count here
	for ( int i = m_damageInflictors.Count()-1; i >= 0; --i )
	{
		const inflictorstate_t &state = m_damageInflictors[i];
		if ( state.pInflictorPhysics == pInflictorPhysics )
			return i;
	}

	return -1;
}


int CCollisionEvent::AddDamageInflictor( IPhysicsObject *pInflictorPhysics, float otherMass, const Vector &savedVel, const AngularImpulse &savedAngVel, bool addList )
{
	// NOTE: Save off the state of the object before collision
	// restore if the impact is a kill
	// UNDONE: Should we absorb some energy here?
	// NOTE: we can't save a delta because there could be subsequent post-fatal collisions

	int addIndex = m_damageInflictors.AddToTail();
	{
		inflictorstate_t &state = m_damageInflictors[addIndex];
		state.pInflictorPhysics = pInflictorPhysics;
		state.savedVelocity = savedVel;
		state.savedAngularVelocity = savedAngVel;
		state.otherMassMax = otherMass;
		state.restored = false;
		state.nextIndex = -1;
	}

	if ( addList )
	{
		CBaseEntity *pEntity = static_cast<CBaseEntity *>(pInflictorPhysics->GetGameData());
		if ( pEntity )
		{
			IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
			int physCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
			if ( physCount > 1 )
			{
				int currentIndex = addIndex;
				for ( int i = 0; i < physCount; i++ )
				{
					if ( pList[i] != pInflictorPhysics )
					{
						Vector vel;
						AngularImpulse angVel;
						pList[i]->GetVelocity( &vel, &angVel );
						int next = AddDamageInflictor( pList[i], otherMass, vel, angVel, false );
						m_damageInflictors[currentIndex].nextIndex = next;
						currentIndex = next;
					}
				}
			}
		}
	}
	return addIndex;
}


void CCollisionEvent::LevelShutdown( void )
{
	for ( int i = 0; i < ARRAYSIZE(m_current); i++ )
	{
		if ( m_current[i].patch )
		{
			ShutdownFriction( m_current[i] );
		}
	}
}


void CCollisionEvent::StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData )
{
	CallbackContext check(this);
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pObject1->GetGameData());
	CBaseEntity *pEntity2 = static_cast<CBaseEntity *>(pObject2->GetGameData());

	if ( !pEntity1 || !pEntity2 )
		return;

	Vector endPoint, normal;
	pTouchData->GetContactPoint( endPoint );
	pTouchData->GetSurfaceNormal( normal );
	if ( !m_bBufferTouchEvents )
	{
		DispatchStartTouch( pEntity1, pEntity2, endPoint, normal );
	}
	else
	{
		AddTouchEvent( pEntity1, pEntity2, TOUCH_START, endPoint, normal );
	}
}

static int CountPhysicsObjectEntityContacts( IPhysicsObject *pObject, CBaseEntity *pEntity )
{
	IPhysicsFrictionSnapshot *pSnapshot = pObject->CreateFrictionSnapshot();
	int count = 0;
	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		CBaseEntity *pOtherEntity = static_cast<CBaseEntity *>(pOther->GetGameData());
		if ( pOtherEntity == pEntity )
			count++;
		pSnapshot->NextFrictionData();
	}
	pObject->DestroyFrictionSnapshot( pSnapshot );
	return count;
}

void CCollisionEvent::EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData )
{
	CallbackContext check(this);
	CBaseEntity *pEntity1 = static_cast<CBaseEntity *>(pObject1->GetGameData());
	CBaseEntity *pEntity2 = static_cast<CBaseEntity *>(pObject2->GetGameData());

	if ( !pEntity1 || !pEntity2 )
		return;

	// contact point deleted, but entities are still touching?
	IPhysicsObject *list[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pEntity1->VPhysicsGetObjectList( list, ARRAYSIZE(list) );

	int contactCount = 0;
	for ( int i = 0; i < count; i++ )
	{
		contactCount += CountPhysicsObjectEntityContacts( list[i], pEntity2 );
		
		// still touching
		if ( contactCount > 1 )
			return;
	}

	// should have exactly one contact point (the one getting deleted here)
	//Assert( contactCount == 1 );

	Vector endPoint, normal;
	pTouchData->GetContactPoint( endPoint );
	pTouchData->GetSurfaceNormal( normal );

	if ( !m_bBufferTouchEvents )
	{
		DispatchEndTouch( pEntity1, pEntity2 );
	}
	else
	{
		AddTouchEvent( pEntity1, pEntity2, TOUCH_END, vec3_origin, vec3_origin );
	}
}

// UNDONE: This is functional, but minimally.
void CCollisionEvent::ObjectEnterTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject )
{
	CBaseEntity *pTriggerEntity = static_cast<CBaseEntity *>(pTrigger->GetGameData());
	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	if ( pTriggerEntity && pEntity )
	{
		// UNDONE: Don't buffer these until we can solve generating touches at object creation time
		if ( 0 && m_bBufferTouchEvents )
		{
			int index = m_triggerEvents.AddToTail();
			m_triggerEvents[index].Init( pTriggerEntity, pTrigger, pEntity, pObject, true );
		}
		else
		{
			CallbackContext check(this);
			m_currentTriggerEvent.Init( pTriggerEntity, pTrigger, pEntity, pObject, true ); 
			pTriggerEntity->StartTouch( pEntity );
			m_currentTriggerEvent.Clear();
		}
	}
}

void CCollisionEvent::ObjectLeaveTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject )
{
	CBaseEntity *pTriggerEntity = static_cast<CBaseEntity *>(pTrigger->GetGameData());
	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	if ( pTriggerEntity && pEntity )
	{
		// UNDONE: Don't buffer these until we can solve generating touches at object creation time
		if ( 0 && m_bBufferTouchEvents )
		{
			int index = m_triggerEvents.AddToTail();
			m_triggerEvents[index].Init( pTriggerEntity, pTrigger, pEntity, pObject, false );
		}
		else
		{
			CallbackContext check(this);
			m_currentTriggerEvent.Init( pTriggerEntity, pTrigger, pEntity, pObject, false ); 
			pTriggerEntity->EndTouch( pEntity );
			m_currentTriggerEvent.Clear();
		}
	}
}

bool CCollisionEvent::GetTriggerEvent( triggerevent_t *pEvent, CBaseEntity *pTriggerEntity )
{
	if ( pEvent && pTriggerEntity == m_currentTriggerEvent.pTriggerEntity )
	{
		*pEvent = m_currentTriggerEvent;
		return true;
	}

	return false;
}

void PhysGetListOfPenetratingEntities( CBaseEntity *pSearch, CUtlVector<CBaseEntity *> &list )
{
	g_Collisions.GetListOfPenetratingEntities( pSearch, list );
}

bool PhysGetTriggerEvent( triggerevent_t *pEvent, CBaseEntity *pTriggerEntity )
{
	return g_Collisions.GetTriggerEvent( pEvent, pTriggerEntity );
}
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// External interface to collision sounds
//-----------------------------------------------------------------------------

void PhysicsImpactSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed )
{
	physicssound::AddImpactSound( g_PhysicsHook.m_impactSounds, pEntity, pEntity->entindex(), channel, pPhysObject, surfaceProps, surfacePropsHit, volume, impactSpeed );
}

void PhysCollisionSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, int surfacePropsHit, float deltaTime, float speed )
{
	if ( deltaTime < 0.05f || speed < 70.0f )
		return;

	float volume = speed * speed * (1.0f/(320.0f*320.0f));	// max volume at 320 in/s
	if ( volume > 1.0f )
		volume = 1.0f;

	PhysicsImpactSound( pEntity, pPhysObject, channel, surfaceProps, surfacePropsHit, volume, speed );
}

void PhysBreakSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, Vector vecOrigin )
{
	if ( !pPhysObject)
		return;

	physicssound::AddBreakSound( g_PhysicsHook.m_breakSounds, vecOrigin, pPhysObject->GetMaterialIndex() );
}

ConVar collision_shake_amp("collision_shake_amp", "0.2");
ConVar collision_shake_freq("collision_shake_freq", "0.5");
ConVar collision_shake_time("collision_shake_time", "0.5");
				 
void PhysCollisionScreenShake( gamevcollisionevent_t *pEvent, int index )
{
	int otherIndex = !index;
	float mass = pEvent->pObjects[index]->GetMass();
	if ( mass >= VPHYSICS_LARGE_OBJECT_MASS && pEvent->pObjects[otherIndex]->IsStatic() && 
		!(pEvent->pObjects[index]->GetGameFlags() & FVPHYSICS_PENETRATING) )
	{
		mass = clamp(mass, VPHYSICS_LARGE_OBJECT_MASS, 2000.f);
		if ( pEvent->collisionSpeed > 30 && pEvent->deltaCollisionTime > 0.25f )
		{
			Vector vecPos;
			pEvent->pInternalData->GetContactPoint( vecPos );
			float impulse = pEvent->collisionSpeed * mass;
			float amplitude = impulse * (collision_shake_amp.GetFloat() / (30.0f * VPHYSICS_LARGE_OBJECT_MASS));
			UTIL_ScreenShake( vecPos, amplitude, collision_shake_freq.GetFloat(), collision_shake_time.GetFloat(), amplitude * 60, SHAKE_START );
		}
	}
}

#if HL2_EPISODIC
// Uses DispatchParticleEffect because, so far as I know, that is the new means of kicking
// off flinders for this kind of collision. Should this be in g_pEffects instead? 
void PhysCollisionWarpEffect( gamevcollisionevent_t *pEvent, surfacedata_t *phit )
{
	Vector vecPos; 
	QAngle vecAngles;

	pEvent->pInternalData->GetContactPoint( vecPos );
	{
		Vector vecNormal;
		pEvent->pInternalData->GetSurfaceNormal(vecNormal);
		VectorAngles( vecNormal, vecAngles );
	}

	DispatchParticleEffect( "warp_shield_impact", vecPos, vecAngles );
}
#endif

void PhysCollisionDust( gamevcollisionevent_t *pEvent, surfacedata_t *phit )
{

	switch ( phit->game.material )
	{
	case CHAR_TEX_SAND:
	case CHAR_TEX_DIRT:

		if ( pEvent->collisionSpeed < 200.0f )
			return;
		
		break;

	case CHAR_TEX_CONCRETE:

		if ( pEvent->collisionSpeed < 340.0f )
			return;

		break;

#if HL2_EPISODIC 
		// this is probably redundant because BaseEntity::VHandleCollision should have already dispatched us elsewhere
	case CHAR_TEX_WARPSHIELD:
		PhysCollisionWarpEffect(pEvent,phit);
		return;

		break;
#endif

	default:
		return;
	}

	//Kick up dust
	Vector	vecPos, vecVel;

	pEvent->pInternalData->GetContactPoint( vecPos );

	vecVel.Random( -1.0f, 1.0f );
	vecVel.z = random->RandomFloat( 0.3f, 1.0f );
	VectorNormalize( vecVel );
	g_pEffects->Dust( vecPos, vecVel, 8.0f, pEvent->collisionSpeed );
}

void PhysFrictionSound( CBaseEntity *pEntity, IPhysicsObject *pObject, const char *pSoundName, HSOUNDSCRIPTHANDLE& handle, float flVolume )
{
	if ( !pEntity )
		return;
	
	// cut out the quiet sounds
	// UNDONE: Separate threshold for starting a sound vs. continuing?
	flVolume = clamp( flVolume, 0.0f, 1.0f );
	if ( flVolume > (1.0f/128.0f) )
	{
		friction_t *pFriction = g_Collisions.FindFriction( pEntity );
		if ( !pFriction )
			return;

		CSoundParameters params;
		if ( !CBaseEntity::GetParametersForSound( pSoundName, handle, params, NULL ) )
			return;

		if ( !pFriction->pObject )
		{
			// don't create really quiet scrapes
			if ( params.volume * flVolume <= 0.1f )
				return;

			pFriction->pObject = pEntity;
			CPASAttenuationFilter filter( pEntity, params.soundlevel );
			pFriction->patch = CSoundEnvelopeController::GetController().SoundCreate( 
				filter, pEntity->entindex(), CHAN_BODY, pSoundName, params.soundlevel );
			CSoundEnvelopeController::GetController().Play( pFriction->patch, params.volume * flVolume, params.pitch );
		}
		else
		{
			float pitch = (flVolume * (params.pitchhigh - params.pitchlow)) + params.pitchlow;
			CSoundEnvelopeController::GetController().SoundChangeVolume( pFriction->patch, params.volume * flVolume, 0.1f );
			CSoundEnvelopeController::GetController().SoundChangePitch( pFriction->patch, pitch, 0.1f );
		}

		pFriction->flLastUpdateTime = gpGlobals->curtime;
		pFriction->flLastEffectTime = gpGlobals->curtime;
	}
}

void PhysCleanupFrictionSounds( CBaseEntity *pEntity )
{
	friction_t *pFriction = g_Collisions.FindFriction( pEntity );
	if ( pFriction && pFriction->patch )
	{
		g_Collisions.ShutdownFriction( *pFriction );
	}
}


//-----------------------------------------------------------------------------
// Applies force impulses at a later time
//-----------------------------------------------------------------------------
void PhysCallbackImpulse( IPhysicsObject *pPhysicsObject, const Vector &vecCenterForce, const AngularImpulse &vecCenterTorque )
{
	Assert( physenv->IsInSimulation() );
	g_PostSimulationQueue.QueueCall( PostSimulation_ImpulseEvent, pPhysicsObject, RefToVal(vecCenterForce), RefToVal(vecCenterTorque) );
}

void PhysCallbackSetVelocity( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	Assert( physenv->IsInSimulation() );
	g_PostSimulationQueue.QueueCall( PostSimulation_SetVelocityEvent, pPhysicsObject, RefToVal(vecVelocity) );
}

void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex )
{
	Assert( physenv->IsInSimulation() );
	int otherIndex = !hurtIndex;
	g_Collisions.AddDamageEvent( pEntity, info, event.pObjects[otherIndex], true, event.preVelocity[otherIndex], event.preAngularVelocity[otherIndex] );
}

void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info )
{
	if ( PhysIsInCallback() )
	{
		CBaseEntity *pInflictor = info.GetInflictor();
		IPhysicsObject *pInflictorPhysics = (pInflictor) ? pInflictor->VPhysicsGetObject() : NULL;
		g_Collisions.AddDamageEvent( pEntity, info, pInflictorPhysics, false, vec3_origin, vec3_origin );
		if ( pEntity && info.GetInflictor() )
		{
			DevMsg( 2, "Warning: Physics damage event with no recovery info!\nObjects: %s, %s\n", pEntity->GetClassname(), info.GetInflictor()->GetClassname() );
		}
	}
	else
	{
		pEntity->TakeDamage( info );
	}
}

void PhysCallbackRemove(IServerNetworkable *pRemove)
{
	if ( PhysIsInCallback() )
	{
		g_Collisions.AddRemoveObject(pRemove);
	}
	else
	{
		UTIL_Remove(pRemove);
	}
}

void PhysSetEntityGameFlags( CBaseEntity *pEntity, unsigned short flags )
{
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int count = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	for ( int i = 0; i < count; i++ )
	{
		PhysSetGameFlags( pList[i], flags );
	}
}

bool PhysFindOrAddVehicleScript( const char *pScriptName, vehicleparams_t *pParams, vehiclesounds_t *pSounds )
{
	return g_PhysicsHook.FindOrAddVehicleScript(pScriptName, pParams, pSounds);
}

void PhysFlushVehicleScripts()
{
	g_PhysicsHook.FlushVehicleScripts();
}

IPhysicsObject *FindPhysicsObjectByName( const char *pName, CBaseEntity *pErrorEntity )
{
	if ( !pName || !strlen(pName) )
		return NULL;

	CBaseEntity *pEntity = NULL;
	IPhysicsObject *pBestObject = NULL;
	while (1)
	{
		pEntity = gEntList.FindEntityByName( pEntity, pName );
		if ( !pEntity )
			break;
		if ( pEntity->VPhysicsGetObject() )
		{
			if ( pBestObject )
			{
				const char *pErrorName = pErrorEntity ? pErrorEntity->GetClassname() : "Unknown";
				Vector origin = pErrorEntity ? pErrorEntity->GetAbsOrigin() : vec3_origin;
				DevWarning("entity %s at %s has physics attachment to more than one entity with the name %s!!!\n", pErrorName, VecToString(origin), pName );
				while ( ( pEntity = gEntList.FindEntityByName( pEntity, pName ) ) != NULL )
				{
					DevWarning("Found %s\n", pEntity->GetClassname() );
				}
				break;

			}
			pBestObject = pEntity->VPhysicsGetObject();
		}
	}
	return pBestObject;
}

void CC_AirDensity( const CCommand &args )
{
	if ( !physenv )
		return;

	if ( args.ArgC() < 2 )
	{
		Msg( "air_density <value>\nCurrent air density is %.2f\n", physenv->GetAirDensity() );
	}
	else
	{
		float density = atof( args[1] );
		physenv->SetAirDensity( density );
	}
}
static ConCommand air_density("air_density", CC_AirDensity, "Changes the density of air for drag computations.", FCVAR_CHEAT);

void DebugDrawContactPoints(IPhysicsObject *pPhysics)
{
	IPhysicsFrictionSnapshot *pSnapshot = pPhysics->CreateFrictionSnapshot();

	while ( pSnapshot->IsValid() )
	{
		Vector pt, normal;
		pSnapshot->GetContactPoint( pt );
		pSnapshot->GetSurfaceNormal( normal );
		NDebugOverlay::Box( pt, -Vector(1,1,1), Vector(1,1,1), 0, 255, 0, 32, 0 );
		NDebugOverlay::Line( pt, pt - normal * 20, 0, 255, 0, false, 0 );
		IPhysicsObject *pOther = pSnapshot->GetObject(1);
		CBaseEntity *pEntity0 = static_cast<CBaseEntity *>(pOther->GetGameData());
		CFmtStr str("%s (%s): %s [%0.2f]", pEntity0->GetClassname(), STRING(pEntity0->GetModelName()), pEntity0->GetDebugName(), pSnapshot->GetFrictionCoefficient() );
		NDebugOverlay::Text( pt, str.Access(), false, 0 );
		pSnapshot->NextFrictionData();
	}
	pSnapshot->DeleteAllMarkedContacts( true );
	pPhysics->DestroyFrictionSnapshot( pSnapshot );
}



#if 0

#include "filesystem.h"
//-----------------------------------------------------------------------------
// Purpose: This will append a collide to a glview file.  Then you can view the 
//			collisionmodels with glview.
// Input  : *pCollide - collision model
//			&origin - position of the instance of this model
//			&angles - orientation of instance
//			*pFilename - output text file
//-----------------------------------------------------------------------------
// examples:
// world:
//	DumpCollideToGlView( pWorldCollide->solids[0], vec3_origin, vec3_origin, "jaycollide.txt" );
// static_prop:
//	DumpCollideToGlView( info.m_pCollide->solids[0], info.m_Origin, info.m_Angles, "jaycollide.txt" );
//
//-----------------------------------------------------------------------------
void DumpCollideToGlView( CPhysCollide *pCollide, const Vector &origin, const QAngle &angles, const char *pFilename )
{
	if ( !pCollide )
		return;

	printf("Writing %s...\n", pFilename );
	Vector *outVerts;
	int vertCount = physcollision->CreateDebugMesh( pCollide, &outVerts );
	FileHandle_t fp = filesystem->Open( pFilename, "ab" );
	int triCount = vertCount / 3;
	int vert = 0;
	VMatrix tmp = SetupMatrixOrgAngles( origin, angles );
	int i;
	for ( i = 0; i < vertCount; i++ )
	{
		outVerts[i] = tmp.VMul4x3( outVerts[i] );
	}
	for ( i = 0; i < triCount; i++ )
	{
		filesystem->FPrintf( fp, "3\n" );
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f 1 0 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f 0 1 0\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
		filesystem->FPrintf( fp, "%6.3f %6.3f %6.3f 0 0 1\n", outVerts[vert].x, outVerts[vert].y, outVerts[vert].z );
		vert++;
	}
	filesystem->Close( fp );
	physcollision->DestroyDebugMesh( vertCount, outVerts );
}
#endif


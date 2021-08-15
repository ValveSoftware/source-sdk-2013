//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "vcollide_parse.h"
#include "filesystem.h"
#include "engine/IStaticPropMgr.h"
#include "solidsetdefaults.h"
#include "engine/IEngineSound.h"
#include "vphysics_sound.h"
#include "movevars_shared.h"
#include "engine/ivmodelinfo.h"
#include "fx.h"
#include "tier0/vprof.h"
#include "c_world.h"
#include "vphysics/object_hash.h"
#include "vphysics/collision_set.h"
#include "soundenvelope.h"
#include "fx_water.h"
#include "positionwatcher.h"
#include "vphysics/constraints.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

// file system interface
extern IFileSystem *filesystem;

ConVar	cl_phys_timescale( "cl_phys_timescale", "1.0", FCVAR_CHEAT, "Sets the scale of time for client-side physics (ragdolls)" );

void PrecachePhysicsSounds( void );

//FIXME: Replicated from server end, consolidate?


extern IVEngineClient *engine;

class CCollisionEvent : public IPhysicsCollisionEvent, public IPhysicsCollisionSolver, public IPhysicsObjectEvent
{
public:
	CCollisionEvent( void );

	void	ObjectSound( int index, vcollisionevent_t *pEvent );
	void	PreCollision( vcollisionevent_t *pEvent ) {}
	void	PostCollision( vcollisionevent_t *pEvent );
	void	Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData );

	void	BufferTouchEvents( bool enable ) { m_bBufferTouchEvents = enable; }

	void	StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData );
	void	EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData );

	void	FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid );
	void	FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid );
	void	PostSimulationFrame() {}

	virtual void ObjectEnterTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject ) {}
	virtual void ObjectLeaveTrigger( IPhysicsObject *pTrigger, IPhysicsObject *pObject ) {}

	float	DeltaTimeSinceLastFluid( CBaseEntity *pEntity );
	void	FrameUpdate( void );

	void	UpdateFluidEvents( void );
	void	UpdateTouchEvents( void );

	// IPhysicsCollisionSolver
	int		ShouldCollide( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 );
#if _DEBUG
	int		ShouldCollide_2( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1 );
#endif
	// debugging collision problem in TF2
	int		ShouldSolvePenetration( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1, float dt );
	bool	ShouldFreezeObject( IPhysicsObject *pObject ) { return true; }
	int		AdditionalCollisionChecksThisTick( int currentChecksDone ) { return 0; }
	bool ShouldFreezeContacts( IPhysicsObject **pObjectList, int objectCount )  { return true; }

	// IPhysicsObjectEvent
	virtual void ObjectWake( IPhysicsObject *pObject )
	{
		C_BaseEntity *pEntity = static_cast<C_BaseEntity *>(pObject->GetGameData());
		if (pEntity && pEntity->HasDataObjectType(VPHYSICSWATCHER))
		{
			ReportVPhysicsStateChanged( pObject, pEntity, true );
		}
	}

	virtual void ObjectSleep( IPhysicsObject *pObject )
	{
		C_BaseEntity *pEntity = static_cast<C_BaseEntity *>(pObject->GetGameData());
		if ( pEntity && pEntity->HasDataObjectType( VPHYSICSWATCHER ) )
		{
			ReportVPhysicsStateChanged( pObject, pEntity, false );
		}
	}


	friction_t *FindFriction( CBaseEntity *pObject );
	void ShutdownFriction( friction_t &friction );
	void UpdateFrictionSounds();
	bool IsInCallback() { return m_inCallback > 0 ? true : false; }

private:
	class CallbackContext
	{
	public:
		CallbackContext(CCollisionEvent *pOuter)
		{
			m_pOuter = pOuter;
			m_pOuter->m_inCallback++;
		}
		~CallbackContext()
		{
			m_pOuter->m_inCallback--;
		}
	private:
		CCollisionEvent *m_pOuter;
	};
	friend class CallbackContext;
	
	void	AddTouchEvent( C_BaseEntity *pEntity0, C_BaseEntity *pEntity1, int touchType, const Vector &point, const Vector &normal );
	void	DispatchStartTouch( C_BaseEntity *pEntity0, C_BaseEntity *pEntity1, const Vector &point, const Vector &normal );
	void	DispatchEndTouch( C_BaseEntity *pEntity0, C_BaseEntity *pEntity1 );

	friction_t					m_current[8];
	CUtlVector<fluidevent_t>	m_fluidEvents;
	CUtlVector<touchevent_t>	m_touchEvents;
	int							m_inCallback;
	bool						m_bBufferTouchEvents;
};

CCollisionEvent g_Collisions;

bool PhysIsInCallback()
{
	if ( (physenv && physenv->IsInSimulation()) || g_Collisions.IsInCallback() )
		return true;

	return false;
}

bool PhysicsDLLInit( CreateInterfaceFn physicsFactory )
{
	if ((physics = (IPhysics *)physicsFactory( VPHYSICS_INTERFACE_VERSION, NULL )) == NULL ||
		(physprops = (IPhysicsSurfaceProps *)physicsFactory( VPHYSICS_SURFACEPROPS_INTERFACE_VERSION, NULL )) == NULL ||
		(physcollision = (IPhysicsCollision *)physicsFactory( VPHYSICS_COLLISION_INTERFACE_VERSION, NULL )) == NULL )
	{
		return false;
	}

	if ( IsX360() )
	{
		// Reduce timescale to save perf on 360
		cl_phys_timescale.SetValue(0.9f);
	}
	PhysParseSurfaceData( physprops, filesystem );
	return true;
}

#define DEFAULT_XBOX_CLIENT_VPHYSICS_TICK	0.025		// 25ms ticks on xbox ragdolls
void PhysicsLevelInit( void )
{
	physenv = physics->CreateEnvironment();
	assert( physenv );
#ifdef PORTAL
	physenv_main = physenv;
#endif
	{
	MEM_ALLOC_CREDIT();
	g_EntityCollisionHash = physics->CreateObjectPairHash();
	}

	// TODO: need to get the right factory function here
	//physenv->SetDebugOverlay( appSystemFactory );
	physenv->SetGravity( Vector(0, 0, -GetCurrentGravity() ) );
	// 15 ms per tick
	// NOTE: Always run client physics at this rate - helps keep ragdolls stable
	physenv->SetSimulationTimestep( IsXbox() ? DEFAULT_XBOX_CLIENT_VPHYSICS_TICK : gpGlobals->interval_per_tick );
	physenv->SetCollisionEventHandler( &g_Collisions );
	physenv->SetCollisionSolver( &g_Collisions );

	g_PhysWorldObject = PhysCreateWorld_Shared( GetClientWorldEntity(), modelinfo->GetVCollide(1), g_PhysDefaultObjectParams );

	staticpropmgr->CreateVPhysicsRepresentations( physenv, &g_SolidSetup, NULL );
}

void PhysicsReset()
{
	if ( !physenv )
		return;

	physenv->ResetSimulationClock();
}


ConVar cl_ragdoll_collide( "cl_ragdoll_collide", "0" );

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
	CallbackContext callback(this);

	C_BaseEntity *pEntity0 = static_cast<C_BaseEntity *>(pGameData0);
	C_BaseEntity *pEntity1 = static_cast<C_BaseEntity *>(pGameData1);

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
	// Obey collision group rules
	Assert(GameRules());
	if ( GameRules() )
	{
		if (!GameRules()->ShouldCollide( pEntity0->GetCollisionGroup(), pEntity1->GetCollisionGroup() ))
			return 0;
	}

	if ( (pObj0->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL) && (pObj1->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL) )
	{
		if ( !cl_ragdoll_collide.GetBool() )
			return 0;
	}

	// check contents
	if ( !(pObj0->GetContents() & pEntity1->PhysicsSolidMaskForEntity()) || !(pObj1->GetContents() & pEntity0->PhysicsSolidMaskForEntity()) )
		return 0;

	if ( g_EntityCollisionHash->IsObjectPairInHash( pGameData0, pGameData1 ) )
		return 0;

	if ( g_EntityCollisionHash->IsObjectPairInHash( pObj0, pObj1 ) )
		return 0;

#if 0
	int solid0 = pEntity0->GetSolid();
	int solid1 = pEntity1->GetSolid();
	int nSolidFlags0 = pEntity0->GetSolidFlags();
	int nSolidFlags1 = pEntity1->GetSolidFlags();
#endif

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
	return 1;
}

int CCollisionEvent::ShouldSolvePenetration( IPhysicsObject *pObj0, IPhysicsObject *pObj1, void *pGameData0, void *pGameData1, float dt )
{
	CallbackContext callback(this);
	// solve it yourself here and return 0, or have the default implementation do it
	if ( pGameData0 == pGameData1 )
	{
		if ( pObj0->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL )
		{
			// this is a ragdoll, self penetrating
			C_BaseEntity *pEnt = reinterpret_cast<C_BaseEntity *>(pGameData0);
			C_BaseAnimating *pAnim = pEnt->GetBaseAnimating();

			if ( pAnim && pAnim->m_pRagdoll )
			{
				IPhysicsConstraintGroup *pGroup = pAnim->m_pRagdoll->GetConstraintGroup();
				if ( pGroup )
				{
					pGroup->SolvePenetration( pObj0, pObj1 );
					return false;
				}
			}
		}
	}

	return true;
}


// A class that implements an IClientSystem for physics
class CPhysicsSystem : public CAutoGameSystemPerFrame
{
public:
	CPhysicsSystem( char const *name ) : CAutoGameSystemPerFrame( name )
	{
	}

	// HACKHACK: PhysicsDLLInit() is called explicitly because it requires a parameter
	virtual bool Init();
	virtual void Shutdown();

	// Level init, shutdown
	virtual void LevelInitPreEntity();
	virtual void LevelInitPostEntity();

	// The level is shutdown in two parts
	virtual void LevelShutdownPreEntity();
	
	virtual void LevelShutdownPostEntity();

	void AddImpactSound( void *pGameData, IPhysicsObject *pObject, int surfaceProps, int surfacePropsHit, float volume, float speed );

	virtual void Update( float frametime );

	void PhysicsSimulate();

private:
	physicssound::soundlist_t m_impactSounds;
};

static CPhysicsSystem g_PhysicsSystem( "CPhysicsSystem" );
// singleton to hook into the client system
IGameSystem *PhysicsGameSystem( void )
{
	return &g_PhysicsSystem;
}


// HACKHACK: PhysicsDLLInit() is called explicitly because it requires a parameter
bool CPhysicsSystem::Init()
{
	return true;
}

void CPhysicsSystem::Shutdown()
{
}

// Level init, shutdown
void CPhysicsSystem::LevelInitPreEntity( void )
{
	m_impactSounds.RemoveAll();
	PrecachePhysicsSounds();
}

void CPhysicsSystem::LevelInitPostEntity( void )
{
	PhysicsLevelInit();
}

// The level is shutdown in two parts
void CPhysicsSystem::LevelShutdownPreEntity()
{
	if ( physenv )
	{
		// we may have deleted multiple objects including the world by now, so 
		// don't try to wake them up
		physenv->SetQuickDelete( true );
	}
}

void CPhysicsSystem::LevelShutdownPostEntity()
{
	if ( physenv )
	{
		// environment destroys all objects
		// entities are gone, so this is safe now
		physics->DestroyEnvironment( physenv );
	}
	physics->DestroyObjectPairHash( g_EntityCollisionHash );
	g_EntityCollisionHash = NULL;

	physics->DestroyAllCollisionSets();

	physenv = NULL;
	g_PhysWorldObject = NULL;
}

void CPhysicsSystem::AddImpactSound( void *pGameData, IPhysicsObject *pObject, int surfaceProps, int surfacePropsHit, float volume, float speed )
{
	physicssound::AddImpactSound( m_impactSounds, pGameData, SOUND_FROM_WORLD, CHAN_STATIC, pObject, surfaceProps, surfacePropsHit, volume, speed );
}


void CPhysicsSystem::Update( float frametime )
{
	// THIS WAS MOVED TO POST-ENTITY SIM
	//PhysicsSimulate();
}


void CPhysicsSystem::PhysicsSimulate()
{
	VPROF_BUDGET( "CPhysicsSystem::PhysicsSimulate", VPROF_BUDGETGROUP_PHYSICS );
	float frametime = gpGlobals->frametime;

	if ( physenv )
	{
		tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s %d", __FUNCTION__, physenv->GetActiveObjectCount() );

		g_Collisions.BufferTouchEvents( true );
#ifdef _DEBUG
		physenv->DebugCheckContacts();
#endif
		physenv->Simulate( frametime * cl_phys_timescale.GetFloat() );

		int activeCount = physenv->GetActiveObjectCount();
		IPhysicsObject **pActiveList = NULL;
		if ( activeCount )
		{
			pActiveList = (IPhysicsObject **)stackalloc( sizeof(IPhysicsObject *)*activeCount );
			physenv->GetActiveObjects( pActiveList );

			for ( int i = 0; i < activeCount; i++ )
			{
				C_BaseEntity *pEntity = reinterpret_cast<C_BaseEntity *>(pActiveList[i]->GetGameData());
				if ( pEntity )
				{
					if ( pEntity->CollisionProp()->DoesVPhysicsInvalidateSurroundingBox() )
					{
						pEntity->CollisionProp()->MarkSurroundingBoundsDirty();
					}
					pEntity->VPhysicsUpdate( pActiveList[i] );
				}
			}
		}
		
		g_Collisions.BufferTouchEvents( false );
		g_Collisions.FrameUpdate();
	}
	physicssound::PlayImpactSounds( m_impactSounds );
}


void PhysicsSimulate()
{
	g_PhysicsSystem.PhysicsSimulate();
}



CCollisionEvent::CCollisionEvent( void ) 
{ 
}

void CCollisionEvent::ObjectSound( int index, vcollisionevent_t *pEvent )
{
	IPhysicsObject *pObject = pEvent->pObjects[index];
	if ( !pObject || pObject->IsStatic() )
		return;

	float speed = pEvent->collisionSpeed * pEvent->collisionSpeed;
	int surfaceProps = pEvent->surfaceProps[index];

	void *pGameData = pObject->GetGameData();
		
	if ( pGameData )
	{
		float volume = speed * (1.0f/(320.0f*320.0f));	// max volume at 320 in/s
		
		if ( volume > 1.0f )
			volume = 1.0f;

		if ( surfaceProps >= 0 )
		{
			g_PhysicsSystem.AddImpactSound( pGameData, pObject, surfaceProps, pEvent->surfaceProps[!index], volume, speed );
		}
	}
}

void CCollisionEvent::PostCollision( vcollisionevent_t *pEvent )
{
	CallbackContext callback(this);
	if ( pEvent->deltaCollisionTime > 0.1f && pEvent->collisionSpeed > 70 )
	{
		ObjectSound( 0, pEvent );
		ObjectSound( 1, pEvent );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCollisionEvent::FrameUpdate( void )
{
	UpdateFrictionSounds();
	UpdateTouchEvents();
	UpdateFluidEvents();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CCollisionEvent::UpdateTouchEvents( void )
{
	// Turn on buffering in case new touch events occur during processing
	bool bOldTouchEvents = m_bBufferTouchEvents;
	m_bBufferTouchEvents = true;
	for ( int i = 0; i < m_touchEvents.Count(); i++ )
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
	m_bBufferTouchEvents = bOldTouchEvents;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity0 - 
//			*pEntity1 - 
//			touchType - 
//-----------------------------------------------------------------------------
void CCollisionEvent::AddTouchEvent( C_BaseEntity *pEntity0, C_BaseEntity *pEntity1, int touchType, const Vector &point, const Vector &normal )
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject1 - 
//			*pObject2 - 
//			*pTouchData - 
//-----------------------------------------------------------------------------
void CCollisionEvent::StartTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData )
{
	CallbackContext callback(this);
	C_BaseEntity *pEntity1 = static_cast<C_BaseEntity *>(pObject1->GetGameData());
	C_BaseEntity *pEntity2 = static_cast<C_BaseEntity *>(pObject2->GetGameData());

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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity0 - 
//			*pEntity1 - 
//-----------------------------------------------------------------------------
void CCollisionEvent::DispatchStartTouch( C_BaseEntity *pEntity0, C_BaseEntity *pEntity1, const Vector &point, const Vector &normal )
{
	trace_t trace;
	memset( &trace, 0, sizeof(trace) );
	trace.endpos = point;
	trace.plane.dist = DotProduct( point, normal );
	trace.plane.normal = normal;

	// NOTE: This sets up the touch list for both entities, no call to pEntity1 is needed
	pEntity0->PhysicsMarkEntitiesAsTouchingEventDriven( pEntity1, trace );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject1 - 
//			*pObject2 - 
//			*pTouchData - 
//-----------------------------------------------------------------------------
void CCollisionEvent::EndTouch( IPhysicsObject *pObject1, IPhysicsObject *pObject2, IPhysicsCollisionData *pTouchData )
{
	CallbackContext callback(this);
	C_BaseEntity *pEntity1 = static_cast<C_BaseEntity *>(pObject1->GetGameData());
	C_BaseEntity *pEntity2 = static_cast<C_BaseEntity *>(pObject2->GetGameData());

	if ( !pEntity1 || !pEntity2 )
		return;

	if ( !m_bBufferTouchEvents )
	{
		DispatchEndTouch( pEntity1, pEntity2 );
	}
	else
	{
		AddTouchEvent( pEntity1, pEntity2, TOUCH_END, vec3_origin, vec3_origin );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity0 - 
//			*pEntity1 - 
//-----------------------------------------------------------------------------
void CCollisionEvent::DispatchEndTouch( C_BaseEntity *pEntity0, C_BaseEntity *pEntity1 )
{
	// frees the event-driven touchlinks
	pEntity0->PhysicsNotifyOtherOfUntouch( pEntity0, pEntity1 );
	pEntity1->PhysicsNotifyOtherOfUntouch( pEntity1, pEntity0 );
}

void CCollisionEvent::Friction( IPhysicsObject *pObject, float energy, int surfaceProps, int surfacePropsHit, IPhysicsCollisionData *pData )
{
	CallbackContext callback(this);
	if ( energy < 0.05f || surfaceProps < 0 )
		return;

	//Get our friction information
	Vector vecPos, vecVel;
	pData->GetContactPoint( vecPos );
	pObject->GetVelocityAtPoint( vecPos, &vecVel );

	CBaseEntity *pEntity = reinterpret_cast<CBaseEntity *>(pObject->GetGameData());
		
	if ( pEntity  )
	{
		friction_t *pFriction = g_Collisions.FindFriction( pEntity );

		if ( (gpGlobals->maxClients > 1) && pFriction && pFriction->pObject) 
		{
			// in MP mode play sound and effects once every 500 msecs,
			// no ongoing updates, takes too much bandwidth
			if ( (pFriction->flLastEffectTime + 0.5f) > gpGlobals->curtime)
			{
				pFriction->flLastUpdateTime = gpGlobals->curtime;
				return; 			
			}
		}

		PhysFrictionSound( pEntity, pObject, energy, surfaceProps, surfacePropsHit );
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &matrix - 
//			&normal - 
// Output : static int
//-----------------------------------------------------------------------------
static int BestAxisMatchingNormal( matrix3x4_t &matrix, const Vector &normal )
{
	float bestDot = -1;
	int best = 0;
	for ( int i = 0; i < 3; i++ )
	{
		Vector tmp;
		MatrixGetColumn( matrix, i, tmp );
		float dot = fabs(DotProduct( tmp, normal ));
		if ( dot > bestDot )
		{
			bestDot = dot;
			best = i;
		}
	}

	return best;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFluid - 
//			*pObject - 
//			*pEntity - 
//-----------------------------------------------------------------------------
void PhysicsSplash( IPhysicsFluidController *pFluid, IPhysicsObject *pObject, CBaseEntity *pEntity )
{
	//FIXME: For now just allow ragdolls for E3 - jdw
	if ( ( pObject->GetGameFlags() & FVPHYSICS_PART_OF_RAGDOLL ) == false )
		return;

	Vector velocity;
	pObject->GetVelocity( &velocity, NULL );
	
	float impactSpeed = velocity.Length();

	if ( impactSpeed < 25.0f )
		return;

	Vector normal;
	float dist;
	pFluid->GetSurfacePlane( &normal, &dist );

	matrix3x4_t &matrix = pEntity->EntityToWorldTransform();
	
	// Find the local axis that best matches the water surface normal
	int bestAxis = BestAxisMatchingNormal( matrix, normal );

	Vector tangent, binormal;
	MatrixGetColumn( matrix, (bestAxis+1)%3, tangent );
	binormal = CrossProduct( normal, tangent );
	VectorNormalize( binormal );
	tangent = CrossProduct( binormal, normal );
	VectorNormalize( tangent );

	// Now we have a basis tangent to the surface that matches the object's local orientation as well as possible
	// compute an OBB using this basis
	
	// Get object extents in basis
	Vector tanPts[2], binPts[2];
	tanPts[0] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), -tangent );
	tanPts[1] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), tangent );
	binPts[0] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), -binormal );
	binPts[1] = physcollision->CollideGetExtent( pObject->GetCollide(), pEntity->GetAbsOrigin(), pEntity->GetAbsAngles(), binormal );

	// now compute the centered bbox
	float mins[2], maxs[2], center[2], extents[2];
	mins[0] = DotProduct( tanPts[0], tangent );
	maxs[0] = DotProduct( tanPts[1], tangent );

	mins[1] = DotProduct( binPts[0], binormal );
	maxs[1] = DotProduct( binPts[1], binormal );

	center[0] = 0.5 * (mins[0] + maxs[0]);
	center[1] = 0.5 * (mins[1] + maxs[1]);

	extents[0] = maxs[0] - center[0];
	extents[1] = maxs[1] - center[1];

	Vector centerPoint = center[0] * tangent + center[1] * binormal + dist * normal;

	Vector axes[2];
	axes[0] = (maxs[0] - center[0]) * tangent;
	axes[1] = (maxs[1] - center[1]) * binormal;

	// visualize OBB hit
	/*
	Vector corner1 = centerPoint - axes[0] - axes[1];
	Vector corner2 = centerPoint + axes[0] - axes[1];
	Vector corner3 = centerPoint + axes[0] + axes[1];
	Vector corner4 = centerPoint - axes[0] + axes[1];
	NDebugOverlay::Line( corner1, corner2, 0, 0, 255, false, 10 );
	NDebugOverlay::Line( corner2, corner3, 0, 0, 255, false, 10 );
	NDebugOverlay::Line( corner3, corner4, 0, 0, 255, false, 10 );
	NDebugOverlay::Line( corner4, corner1, 0, 0, 255, false, 10 );
	*/

	Vector	corner[4];

	corner[0] = centerPoint - axes[0] - axes[1];
	corner[1] = centerPoint + axes[0] - axes[1];
	corner[2] = centerPoint + axes[0] + axes[1];
	corner[3] = centerPoint - axes[0] + axes[1];

	int contents = enginetrace->GetPointContents( centerPoint-Vector(0,0,2) );

	bool bInSlime = ( contents & CONTENTS_SLIME ) ? true : false;

	Vector	color = vec3_origin;
	float	luminosity = 1.0f;
	
	if ( !bInSlime )
	{
		// Get our lighting information
		FX_GetSplashLighting( centerPoint + ( normal * 8.0f ), &color, &luminosity );
	}

	if ( impactSpeed > 150 )
	{
		if ( bInSlime )
		{
			FX_GunshotSlimeSplash( centerPoint, normal, random->RandomFloat( 8, 10 ) );
		}
		else
		{
			FX_GunshotSplash( centerPoint, normal, random->RandomFloat( 8, 10 ) );
		}
	}
	else if ( !bInSlime )
	{
		FX_WaterRipple( centerPoint, 1.5f, &color, 1.5f, luminosity );
	}
	
	int		splashes = 4;
	Vector	point;

	for ( int i = 0; i < splashes; i++ )
	{
		point = RandomVector( -32.0f, 32.0f );
		point[2] = 0.0f;

		point += corner[i];

		if ( impactSpeed > 150 )
		{
			if ( bInSlime )
			{
				FX_GunshotSlimeSplash( centerPoint, normal, random->RandomFloat( 4, 6 ) );
			}
			else
			{
				FX_GunshotSplash( centerPoint, normal, random->RandomFloat( 4, 6 ) );
			}
		}
		else if ( !bInSlime )
		{
			FX_WaterRipple( point, random->RandomFloat( 0.25f, 0.5f ), &color, luminosity, random->RandomFloat( 0.5f, 1.0f ) );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
// Output : float
//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//			*pFluid - 
//-----------------------------------------------------------------------------
void CCollisionEvent::FluidStartTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid )
{
	CallbackContext callback(this);
	if ( ( pObject == NULL ) || ( pFluid == NULL ) )
		return;

	CBaseEntity *pEntity = static_cast<CBaseEntity *>(pObject->GetGameData());
	
	if ( pEntity )
	{
		float timeSinceLastCollision = DeltaTimeSinceLastFluid( pEntity );
		
		if ( timeSinceLastCollision < 0.5f )
			return;

		PhysicsSplash( pFluid, pObject, pEntity );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pObject - 
//			*pFluid - 
//-----------------------------------------------------------------------------
void CCollisionEvent::FluidEndTouch( IPhysicsObject *pObject, IPhysicsFluidController *pFluid )
{
	CallbackContext callback(this);
	//FIXME: Do nothing for now
}

IPhysicsObject *GetWorldPhysObject ( void )
{
	return g_PhysWorldObject;
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
			int entindex = pEntity->entindex();

			// clientside created entites doesn't have a valid entindex, let 'world' play the sound for them
			if ( entindex < 0 )
				entindex = 0;

			pFriction->patch = CSoundEnvelopeController::GetController().SoundCreate( 
				filter, entindex, CHAN_BODY, pSoundName, params.soundlevel );
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

float PhysGetNextSimTime()
{
	return physenv->GetSimulationTime() + gpGlobals->frametime * cl_phys_timescale.GetFloat();
}

float PhysGetSyncCreateTime()
{
	float nextTime = physenv->GetNextFrameTime();
	float simTime = PhysGetNextSimTime();
	if ( nextTime < simTime )
	{
		// The next simulation frame begins before the end of this frame
		// so create physics objects at that time so that they will reach the current
		// position at curtime.  Otherwise the physics object will simulate forward from curtime
		// and pop into the future a bit at this point of transition
		return gpGlobals->curtime + nextTime - simTime;
	}
	return gpGlobals->curtime;
}

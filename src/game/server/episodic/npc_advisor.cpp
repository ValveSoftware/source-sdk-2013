//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Advisors. Large sluglike aliens with creepy psychic powers!
//
//=============================================================================

#include "cbase.h"
#include "game.h"
#include "ai_basenpc.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "ai_hint.h"
#include "ai_motor.h"
#include "ai_navigator.h"
#include "beam_shared.h"
#include "hl2_shareddefs.h"
#include "ai_route.h"
#include "npcevent.h"
#include "gib.h"
#include "ai_interactions.h"
#include "ndebugoverlay.h"
#include "physics_saverestore.h"
#include "saverestore_utlvector.h"
#include "soundent.h"
#include "vstdlib/random.h"
#include "engine/IEngineSound.h"
#include "movevars_shared.h"
#include "particle_parse.h"
#include "weapon_physcannon.h"
// #include "mathlib/noise.h"

// this file contains the definitions for the message ID constants (eg ADVISOR_MSG_START_BEAM etc)
#include "npc_advisor_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//
// Custom activities.
//

//
// Skill settings.
//
ConVar sk_advisor_health( "sk_advisor_health", "0" );
ConVar advisor_use_impact_table("advisor_use_impact_table","1",FCVAR_NONE,"If true, advisor will use her custom impact damage table.");

#if NPC_ADVISOR_HAS_BEHAVIOR
ConVar advisor_throw_velocity( "advisor_throw_velocity", "1100" );
ConVar advisor_throw_rate( "advisor_throw_rate", "4" );					// Throw an object every 4 seconds.
ConVar advisor_throw_warn_time( "advisor_throw_warn_time", "1.0" );		// Warn players one second before throwing an object.
ConVar advisor_throw_lead_prefetch_time ( "advisor_throw_lead_prefetch_time", "0.66", FCVAR_NONE, "Save off the player's velocity this many seconds before throwing.");
ConVar advisor_throw_stage_distance("advisor_throw_stage_distance","180.0",FCVAR_NONE,"Advisor will try to hold an object this far in front of him just before throwing it at you. Small values will clobber the shield and be very bad.");
// ConVar advisor_staging_num("advisor_staging_num","1",FCVAR_NONE,"Advisor will queue up this many objects to throw at Gordon.");
ConVar advisor_throw_clearout_vel("advisor_throw_clearout_vel","200",FCVAR_NONE,"TEMP: velocity with which advisor clears things out of a throwable's way");
// ConVar advisor_staging_duration("

// how long it will take an object to get hauled to the staging point
#define STAGING_OBJECT_FALLOFF_TIME 0.15f
#endif



//
// Spawnflags.
//

//
// Animation events.
//


#if NPC_ADVISOR_HAS_BEHAVIOR
//
// Custom schedules.
//
enum
{
	SCHED_ADVISOR_COMBAT = LAST_SHARED_SCHEDULE,
	SCHED_ADVISOR_IDLE_STAND,
	SCHED_ADVISOR_TOSS_PLAYER
};


//
// Custom tasks.
//
enum 
{
	TASK_ADVISOR_FIND_OBJECTS = LAST_SHARED_TASK,
	TASK_ADVISOR_LEVITATE_OBJECTS,
	TASK_ADVISOR_STAGE_OBJECTS,
	TASK_ADVISOR_BARRAGE_OBJECTS,

	TASK_ADVISOR_PIN_PLAYER,
};

//
// Custom conditions.
//
enum
{
	COND_ADVISOR_PHASE_INTERRUPT = LAST_SHARED_CONDITION,
};
#endif

class CNPC_Advisor;


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
class CAdvisorLevitate : public IMotionEvent
{
	DECLARE_SIMPLE_DATADESC();

public:

	// in the absence of goal entities, we float up before throwing and down after
	inline bool OldStyle( void )
	{
		return !(m_vecGoalPos1.IsValid() && m_vecGoalPos2.IsValid());
	}

	virtual simresult_e	Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular );

	EHANDLE m_Advisor; ///< handle to the advisor.

	Vector m_vecGoalPos1;
	Vector m_vecGoalPos2;

	float m_flFloat;
};

BEGIN_SIMPLE_DATADESC( CAdvisorLevitate )
	DEFINE_FIELD( m_flFloat, FIELD_FLOAT ),
	DEFINE_FIELD( m_vecGoalPos1, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecGoalPos2, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_Advisor, FIELD_EHANDLE ),
END_DATADESC()



//-----------------------------------------------------------------------------
// The advisor class.
//-----------------------------------------------------------------------------
class CNPC_Advisor : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Advisor, CAI_BaseNPC );

#if NPC_ADVISOR_HAS_BEHAVIOR
	DECLARE_SERVERCLASS();
#endif

public:

	//
	// CBaseEntity:
	//
	virtual void Activate();
	virtual void Spawn();
	virtual void Precache();
	virtual void OnRestore();
	virtual void UpdateOnRemove();

	virtual int DrawDebugTextOverlays();

	//
	// CAI_BaseNPC:
	//
	virtual float MaxYawSpeed() { return 120.0f; }
	
	virtual Class_T Classify();

#if NPC_ADVISOR_HAS_BEHAVIOR
	virtual int GetSoundInterests();
	virtual int SelectSchedule();
	virtual void StartTask( const Task_t *pTask );
	virtual void RunTask( const Task_t *pTask );
	virtual void OnScheduleChange( void );
#endif

	virtual void PainSound( const CTakeDamageInfo &info );
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual void IdleSound();
	virtual void AlertSound();

#if NPC_ADVISOR_HAS_BEHAVIOR
	virtual bool QueryHearSound( CSound *pSound );
	virtual void GatherConditions( void );

	/// true iff I recently threw the given object (not so fast)
	bool DidThrow(const CBaseEntity *pEnt);
#else
	inline bool DidThrow(const CBaseEntity *pEnt) { return false; }
#endif

	virtual bool IsHeavyDamage( const CTakeDamageInfo &info );
	virtual int	 OnTakeDamage( const CTakeDamageInfo &info );

	virtual const impactdamagetable_t &GetPhysicsImpactDamageTable( void );
	COutputInt   m_OnHealthIsNow;

#if NPC_ADVISOR_HAS_BEHAVIOR

	DEFINE_CUSTOM_AI;

	void InputSetThrowRate( inputdata_t &inputdata );
	void InputWrenchImmediate( inputdata_t &inputdata ); ///< immediately wrench an object into the air
	void InputSetStagingNum( inputdata_t &inputdata );
	void InputPinPlayer( inputdata_t &inputdata );
	void InputTurnBeamOn( inputdata_t &inputdata );
	void InputTurnBeamOff( inputdata_t &inputdata );
	void InputElightOn( inputdata_t &inputdata );
	void InputElightOff( inputdata_t &inputdata );

	COutputEvent m_OnPickingThrowable, m_OnThrowWarn, m_OnThrow;

	enum { kMaxThrownObjectsTracked = 4 };
#endif

	DECLARE_DATADESC();

protected:

#if NPC_ADVISOR_HAS_BEHAVIOR
	Vector GetThrowFromPos( CBaseEntity *pEnt ); ///< Get the position in which we shall hold an object prior to throwing it
#endif

	bool CanLevitateEntity( CBaseEntity *pEntity, int minMass, int maxMass );
	void StartLevitatingObjects( void );


#if NPC_ADVISOR_HAS_BEHAVIOR
	// void PurgeThrownObjects(); ///< clean out the recently thrown objects array
	void AddToThrownObjects(CBaseEntity *pEnt); ///< add to the recently thrown objects array

	void HurlObjectAtPlayer( CBaseEntity *pEnt, const Vector &leadVel );
	void PullObjectToStaging( CBaseEntity *pEnt, const Vector &stagingPos );
	CBaseEntity *ThrowObjectPrepare( void );

	CBaseEntity *PickThrowable( bool bRequireInView ); ///< choose an object to throw at the player (so it can get stuffed in the handle array)

	/// push everything out of the way between an object I'm about to throw and the player.
	void PreHurlClearTheWay( CBaseEntity *pThrowable, const Vector &toPos );
#endif

	CUtlVector<EHANDLE>	m_physicsObjects;
	IPhysicsMotionController *m_pLevitateController;
	CAdvisorLevitate m_levitateCallback;
	
	EHANDLE m_hLevitateGoal1;
	EHANDLE m_hLevitateGoal2;
	EHANDLE m_hLevitationArea;

#if NPC_ADVISOR_HAS_BEHAVIOR
	// EHANDLE m_hThrowEnt;
	CUtlVector<EHANDLE>	m_hvStagedEnts;
	CUtlVector<EHANDLE>	m_hvStagingPositions; 
	// todo: write accessor functions for m_hvStagedEnts so that it doesn't have members added and removed willy nilly throughout
	// code (will make the networking below more reliable)

	void Write_BeamOn(  CBaseEntity *pEnt ); 	///< write a message turning a beam on
	void Write_BeamOff( CBaseEntity *pEnt );   	///< write a message turning a beam off
	void Write_AllBeamsOff( void );				///< tell client to kill all beams

	// for the pin-the-player-to-something behavior
	EHANDLE m_hPlayerPinPos;
	float  m_playerPinFailsafeTime;

	// keep track of up to four objects after we have thrown them, to prevent oscillation or levitation of recently thrown ammo.
	EHANDLE m_haRecentlyThrownObjects[kMaxThrownObjectsTracked]; 
	float   m_flaRecentlyThrownObjectTimes[kMaxThrownObjectsTracked];
#endif

	string_t m_iszLevitateGoal1;
	string_t m_iszLevitateGoal2;
	string_t m_iszLevitationArea;


#if NPC_ADVISOR_HAS_BEHAVIOR
	string_t m_iszStagingEntities;
	string_t m_iszPriorityEntityGroupName;

	float m_flStagingEnd; 
	float m_flThrowPhysicsTime;
	float m_flLastThrowTime;
	float m_flLastPlayerAttackTime; ///< last time the player attacked something. 

	int   m_iStagingNum; ///< number of objects advisor stages at once
	bool  m_bWasScripting;

	// unsigned char m_pickFailures; // the number of times we have tried to pick a throwable and failed 

	Vector m_vSavedLeadVel; ///< save off player velocity for leading a bit before actually pelting them. 
#endif
};


LINK_ENTITY_TO_CLASS( npc_advisor, CNPC_Advisor );

BEGIN_DATADESC( CNPC_Advisor )

	DEFINE_KEYFIELD( m_iszLevitateGoal1, FIELD_STRING, "levitategoal_bottom" ),
	DEFINE_KEYFIELD( m_iszLevitateGoal2, FIELD_STRING, "levitategoal_top" ),
	DEFINE_KEYFIELD( m_iszLevitationArea, FIELD_STRING, "levitationarea"), ///< we will float all the objects in this volume

	DEFINE_PHYSPTR( m_pLevitateController ),
	DEFINE_EMBEDDED( m_levitateCallback ),
	DEFINE_UTLVECTOR( m_physicsObjects, FIELD_EHANDLE ),

	DEFINE_FIELD( m_hLevitateGoal1, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLevitateGoal2, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hLevitationArea, FIELD_EHANDLE ),

#if NPC_ADVISOR_HAS_BEHAVIOR
	DEFINE_KEYFIELD( m_iszStagingEntities, FIELD_STRING, "staging_ent_names"), ///< entities named this constitute the positions to which we stage objects to be thrown
	DEFINE_KEYFIELD( m_iszPriorityEntityGroupName, FIELD_STRING, "priority_grab_name"),
	
	DEFINE_UTLVECTOR( m_hvStagedEnts, FIELD_EHANDLE ),
	DEFINE_UTLVECTOR( m_hvStagingPositions, FIELD_EHANDLE ),
	DEFINE_ARRAY( m_haRecentlyThrownObjects, FIELD_EHANDLE, CNPC_Advisor::kMaxThrownObjectsTracked ),
	DEFINE_ARRAY( m_flaRecentlyThrownObjectTimes, FIELD_TIME, CNPC_Advisor::kMaxThrownObjectsTracked ),

	DEFINE_FIELD( m_hPlayerPinPos, FIELD_EHANDLE ),
	DEFINE_FIELD( m_playerPinFailsafeTime, FIELD_TIME ),

	// DEFINE_FIELD( m_hThrowEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_flThrowPhysicsTime, FIELD_TIME ),
	DEFINE_FIELD( m_flLastPlayerAttackTime, FIELD_TIME ),
	DEFINE_FIELD( m_flStagingEnd, FIELD_TIME ),
	DEFINE_FIELD( m_iStagingNum, FIELD_INTEGER ),
	DEFINE_FIELD( m_bWasScripting, FIELD_BOOLEAN ),

	DEFINE_FIELD( m_flLastThrowTime, FIELD_TIME ),
	DEFINE_FIELD( m_vSavedLeadVel, FIELD_VECTOR ),

	DEFINE_OUTPUT( m_OnPickingThrowable, "OnPickingThrowable" ),
	DEFINE_OUTPUT( m_OnThrowWarn, "OnThrowWarn" ),
	DEFINE_OUTPUT( m_OnThrow, "OnThrow" ),
	DEFINE_OUTPUT( m_OnHealthIsNow, "OnHealthIsNow" ),

	DEFINE_INPUTFUNC( FIELD_FLOAT,   "SetThrowRate",    InputSetThrowRate ),
	DEFINE_INPUTFUNC( FIELD_STRING,  "WrenchImmediate", InputWrenchImmediate ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetStagingNum",   InputSetStagingNum),
	DEFINE_INPUTFUNC( FIELD_STRING,  "PinPlayer",       InputPinPlayer ),
	DEFINE_INPUTFUNC( FIELD_STRING,  "BeamOn",          InputTurnBeamOn ),
	DEFINE_INPUTFUNC( FIELD_STRING,  "BeamOff",         InputTurnBeamOff ),
	DEFINE_INPUTFUNC( FIELD_STRING,  "ElightOn",         InputElightOn ),
	DEFINE_INPUTFUNC( FIELD_STRING,  "ElightOff",         InputElightOff ),
#endif

END_DATADESC()



#if NPC_ADVISOR_HAS_BEHAVIOR
IMPLEMENT_SERVERCLASS_ST(CNPC_Advisor, DT_NPC_Advisor)

END_SEND_TABLE()
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::Spawn()
{
	BaseClass::Spawn();

#ifdef _XBOX
	// Always fade the corpse
	AddSpawnFlags( SF_NPC_FADE_CORPSE );
#endif // _XBOX

	Precache();

	SetModel( STRING( GetModelName() ) );

	m_iHealth = sk_advisor_health.GetFloat();
	m_takedamage = DAMAGE_NO;

	SetHullType( HULL_LARGE_CENTERED );
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	// AddSolidFlags( FSOLID_NOT_SOLID );

	SetMoveType( MOVETYPE_FLY );

	m_flFieldOfView = VIEW_FIELD_FULL;
	SetViewOffset( Vector( 0, 0, 80 ) );		// Position of the eyes relative to NPC's origin.

	SetBloodColor( BLOOD_COLOR_GREEN );
	m_NPCState = NPC_STATE_NONE;

	CapabilitiesClear();

	NPCInit();

	SetGoalEnt( NULL );

	AddEFlags( EFL_NO_DISSOLVE );
}


#if NPC_ADVISOR_HAS_BEHAVIOR
//-----------------------------------------------------------------------------
// comparison function for qsort used below. Compares "StagingPriority" keyfield
//-----------------------------------------------------------------------------
int __cdecl AdvisorStagingComparator(const EHANDLE *pe1, const EHANDLE *pe2)
{
	// bool ReadKeyField( const char *varName, variant_t *var );

	variant_t var;
	int val1 = 10, val2 = 10; // default priority is ten
	
	// read field one
	if ( pe1->Get()->ReadKeyField( "StagingPriority", &var ) )
	{
		val1 = var.Int();
	}
	
	// read field two
	if ( pe2->Get()->ReadKeyField( "StagingPriority", &var ) )
	{
		val2 = var.Int();
	}

	// return comparison (< 0 if pe1<pe2) 
	return( val1 - val2 );
}
#endif

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

#pragma warning(push)
#pragma warning(disable : 4706)

void CNPC_Advisor::Activate()
{
	BaseClass::Activate();
	
	m_hLevitateGoal1  = gEntList.FindEntityGeneric( NULL, STRING( m_iszLevitateGoal1 ),  this );
	m_hLevitateGoal2  = gEntList.FindEntityGeneric( NULL, STRING( m_iszLevitateGoal2 ),  this );
	m_hLevitationArea = gEntList.FindEntityGeneric( NULL, STRING( m_iszLevitationArea ), this );

	m_levitateCallback.m_Advisor = this;

#if NPC_ADVISOR_HAS_BEHAVIOR
	// load the staging positions
	CBaseEntity *pEnt = NULL;
	m_hvStagingPositions.EnsureCapacity(6); // reserve six

	// conditional assignment: find an entity by name and save it into pEnt. Bail out when none are left.
	while ( pEnt = gEntList.FindEntityByName(pEnt,m_iszStagingEntities) )
	{
		m_hvStagingPositions.AddToTail(pEnt);
	}

	// sort the staging positions by their staging number.
	m_hvStagingPositions.Sort( AdvisorStagingComparator );

	// positions loaded, null out the m_hvStagedEnts array with exactly as many null spaces
	m_hvStagedEnts.SetCount( m_hvStagingPositions.Count() );

	m_iStagingNum = 1;

	AssertMsg(m_hvStagingPositions.Count() > 0, "You did not specify any staging positions in the advisor's staging_ent_names !");
#endif
}
#pragma warning(pop)


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::UpdateOnRemove()
{
	if ( m_pLevitateController )
	{
		physenv->DestroyMotionController( m_pLevitateController );
	}
	
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::OnRestore()
{
	BaseClass::OnRestore();
	StartLevitatingObjects();
}


//-----------------------------------------------------------------------------
//  Returns this monster's classification in the relationship table.
//-----------------------------------------------------------------------------
Class_T	CNPC_Advisor::Classify()
{
	return CLASS_COMBINE;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CNPC_Advisor::IsHeavyDamage( const CTakeDamageInfo &info )
{
	return (info.GetDamage() > 0);
}



//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::StartLevitatingObjects()
{
	if ( !m_pLevitateController )
	{
		m_pLevitateController = physenv->CreateMotionController( &m_levitateCallback );
	}

	m_pLevitateController->ClearObjects();
	
	int nCount = m_physicsObjects.Count();
	for ( int i = 0; i < nCount; i++ )
	{
		CBaseEntity *pEnt = m_physicsObjects.Element( i );
		if ( !pEnt )
			continue;

		//NDebugOverlay::Box( pEnt->GetAbsOrigin(), pEnt->CollisionProp()->OBBMins(), pEnt->CollisionProp()->OBBMaxs(), 0, 255, 0, 1, 0.1 );

		IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();
		if ( pPhys && pPhys->IsMoveable() )
		{
			m_pLevitateController->AttachObject( pPhys, false );
			pPhys->Wake();
		}
	}
}

// This function is used by both version of the entity finder below 
bool CNPC_Advisor::CanLevitateEntity( CBaseEntity *pEntity, int minMass, int maxMass )
{
	if (!pEntity || pEntity->IsNPC()) 
		return false;

	IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();
	if (!pPhys)
		return false;

	float mass = pPhys->GetMass();

	return ( mass >= minMass && 
			 mass <= maxMass && 
			 //pEntity->VPhysicsGetObject()->IsAsleep() && 
			 pPhys->IsMoveable() /* &&
			!DidThrow(pEntity) */ );
}

#if NPC_ADVISOR_HAS_BEHAVIOR
// find an object to throw at the player and start the warning on it. Return object's 
// pointer if we got something. Otherwise, return NULL if nothing left to throw. Will
// always leave the prepared object at the head of m_hvStagedEnts
CBaseEntity *CNPC_Advisor::ThrowObjectPrepare()
{

	CBaseEntity *pThrowable = NULL;
	while (m_hvStagedEnts.Count() > 0)
	{
		pThrowable = m_hvStagedEnts[0];

		if (pThrowable)
		{
			IPhysicsObject *pPhys = pThrowable->VPhysicsGetObject();
			if ( !pPhys )
			{
				// reject!
				
				Write_BeamOff(m_hvStagedEnts[0]);
				pThrowable = NULL;
			}
		}

		// if we still have pThrowable...
		if (pThrowable)
		{
			// we're good
			break;
		}
		else
		{
			m_hvStagedEnts.Remove(0);
		}
	}

	if (pThrowable)
	{
		Assert( pThrowable->VPhysicsGetObject() );

		// play the sound, attach the light, fire the trigger
		EmitSound( "NPC_Advisor.ObjectChargeUp" );

		m_OnThrowWarn.FireOutput(pThrowable,this); 
		m_flThrowPhysicsTime = gpGlobals->curtime + advisor_throw_warn_time.GetFloat();

		if ( GetEnemy() )
		{
			PreHurlClearTheWay( pThrowable, GetEnemy()->EyePosition() );
		}

		return pThrowable;
	}
	else // we had nothing to throw
	{
		return NULL;
	}
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		// DVS: TODO: if this gets expensive we can start caching the results and doing it less often.
		case TASK_ADVISOR_FIND_OBJECTS:
		{
			// if we have a trigger volume, use the contents of that. If not, use a hardcoded box (for debugging purposes)
			// in both cases we validate the objects using the same helper funclet just above. When we can count on the
			// trigger vol being there, we can elide the else{} clause here.

			CBaseEntity *pVolume = m_hLevitationArea;
			AssertMsg(pVolume, "Combine advisor needs 'levitationarea' key pointing to a trigger volume." );

			if (!pVolume)
			{
				TaskFail( "No levitation area found!" );
				break;
			}

			touchlink_t *touchroot = ( touchlink_t * )pVolume->GetDataObject( TOUCHLINK );
			if ( touchroot )
			{

				m_physicsObjects.RemoveAll();

				for ( touchlink_t *link = touchroot->nextLink; link != touchroot; link = link->nextLink )
				{
					CBaseEntity *pTouch = link->entityTouched;
					if ( CanLevitateEntity( pTouch, 10, 220 ) )
					{
						if ( pTouch->GetMoveType() == MOVETYPE_VPHYSICS )
						{
							//Msg( "   %d added %s\n", m_physicsObjects.Count(), STRING( list[i]->GetModelName() ) );
							m_physicsObjects.AddToTail( pTouch );
						}
					}
				}
			}

			/*
			// this is the old mechanism, using a hardcoded box and an entity enumerator. 
			// since deprecated.

			else
			{
				CBaseEntity *list[128];
				
				m_physicsObjects.RemoveAll();

				//NDebugOverlay::Box( GetAbsOrigin(), Vector( -408, -368, -188 ), Vector( 92, 208, 168 ), 255, 255, 0, 1, 5 );
				
				// one-off class used to determine which entities we want from the UTIL_EntitiesInBox
				class CAdvisorLevitateEntitiesEnum : public CFlaggedEntitiesEnum
				{
				public:
					CAdvisorLevitateEntitiesEnum( CBaseEntity **pList, int listMax, int nMinMass, int nMaxMass )
					:	CFlaggedEntitiesEnum( pList, listMax, 0 ),
						m_nMinMass( nMinMass ),
						m_nMaxMass( nMaxMass )
					{
					}

					virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity )
					{
						CBaseEntity *pEntity = gEntList.GetBaseEntity( pHandleEntity->GetRefEHandle() );
						if ( AdvisorCanLevitateEntity( pEntity, m_nMinMass, m_nMaxMass ) )
						{
							return CFlaggedEntitiesEnum::EnumElement( pHandleEntity );
						}
						return ITERATION_CONTINUE;
					}

					int m_nMinMass;
					int m_nMaxMass;
				};

				CAdvisorLevitateEntitiesEnum levitateEnum( list, ARRAYSIZE( list ), 10, 220 );

				int nCount = UTIL_EntitiesInBox( GetAbsOrigin() - Vector( 554, 368, 188 ), GetAbsOrigin() + Vector( 92, 208, 168 ), &levitateEnum );
				for ( int i = 0; i < nCount; i++ )
				{
					//Msg( "%d found %s\n", m_physicsObjects.Count(), STRING( list[i]->GetModelName() ) );
					if ( list[i]->GetMoveType() == MOVETYPE_VPHYSICS )
					{
						//Msg( "   %d added %s\n", m_physicsObjects.Count(), STRING( list[i]->GetModelName() ) );
						m_physicsObjects.AddToTail( list[i] );
					}
				}
			}
			*/

			if ( m_physicsObjects.Count() > 0 )
			{
				TaskComplete();
			}
			else
			{
				TaskFail( "No physics objects found!" );
			}

			break;
		}

		case TASK_ADVISOR_LEVITATE_OBJECTS:
		{
			StartLevitatingObjects();

			m_flThrowPhysicsTime = gpGlobals->curtime + advisor_throw_rate.GetFloat();
			
			break;
		}
		
		case TASK_ADVISOR_STAGE_OBJECTS:
		{
            // m_pickFailures = 0;
			// clear out previously staged throwables
			/*
			for (int ii = m_hvStagedEnts.Count() - 1; ii >= 0 ; --ii)
			{
				m_hvStagedEnts[ii] = NULL;
			}
			*/
			Write_AllBeamsOff();
			m_hvStagedEnts.RemoveAll();

			m_OnPickingThrowable.FireOutput(NULL,this);
			m_flStagingEnd = gpGlobals->curtime + pTask->flTaskData;

			break;
		}
	
		// we're about to pelt the player with everything. Start the warning effect on the first object.
		case TASK_ADVISOR_BARRAGE_OBJECTS:
		{

			CBaseEntity *pThrowable = ThrowObjectPrepare();

			if (!pThrowable || m_hvStagedEnts.Count() < 1)
			{
				TaskFail( "Nothing to throw!" );
				return;
			}
			
			m_vSavedLeadVel.Invalidate();

			break;
		}
		
		case TASK_ADVISOR_PIN_PLAYER:
		{

			// should never be here
			/*
			Assert( m_hPlayerPinPos.IsValid() );
			m_playerPinFailsafeTime = gpGlobals->curtime + 10.0f;

			break;
			*/
		}

		default:
		{
			BaseClass::StartTask( pTask );
		}
	}
}


//-----------------------------------------------------------------------------
// todo: find a way to guarantee that objects are made pickupable again when bailing out of a task
//-----------------------------------------------------------------------------
void CNPC_Advisor::RunTask( const Task_t *pTask )
{

	switch ( pTask->iTask )
	{
		// Raise up the objects that we found and then hold them.
		case TASK_ADVISOR_LEVITATE_OBJECTS:
		{
			float flTimeToThrow = m_flThrowPhysicsTime - gpGlobals->curtime;
			if ( flTimeToThrow < 0 )
			{
				TaskComplete();
				return;
			}
						
			// set the top and bottom on the levitation volume from the entities. If we don't have
			// both, zero it out so that we can use the old-style simpler mechanism.
			if ( m_hLevitateGoal1 && m_hLevitateGoal2 )
			{
				m_levitateCallback.m_vecGoalPos1 = m_hLevitateGoal1->GetAbsOrigin();
				m_levitateCallback.m_vecGoalPos2 = m_hLevitateGoal2->GetAbsOrigin();
				// swap them if necessary (1 must be the bottom)
				if (m_levitateCallback.m_vecGoalPos1.z > m_levitateCallback.m_vecGoalPos2.z)
				{
					swap(m_levitateCallback.m_vecGoalPos1,m_levitateCallback.m_vecGoalPos2);
				}

				m_levitateCallback.m_flFloat = 0.06f; // this is an absolute accumulation upon gravity
			}
			else
			{
				m_levitateCallback.m_vecGoalPos1.Invalidate(); 
				m_levitateCallback.m_vecGoalPos2.Invalidate(); 

				// the below two stanzas are used for old-style floating, which is linked 
				// to float up before thrown and down after
				if ( flTimeToThrow > 2.0f )
				{
					m_levitateCallback.m_flFloat = 1.06f; 
				}
				else
				{
					m_levitateCallback.m_flFloat = 0.94f; 
				}
			}

			/*
			// Draw boxes around the objects we're levitating.
			for ( int i = 0; i < m_physicsObjects.Count(); i++ )
			{
				CBaseEntity *pEnt = m_physicsObjects.Element( i );
				if ( !pEnt )
					continue;	// The prop has been broken!

				IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();
				if ( pPhys && pPhys->IsMoveable() )
				{
					NDebugOverlay::Box( pEnt->GetAbsOrigin(), pEnt->CollisionProp()->OBBMins(), pEnt->CollisionProp()->OBBMaxs(), 0, 255, 0, 1, 0.1 );
				}
			}*/

			break;
		}	
		
		// Pick a random object that we are levitating. If we have a clear LOS from that object
		// to our enemy's eyes, choose that one to throw. Otherwise, keep looking.
		case TASK_ADVISOR_STAGE_OBJECTS:
		{	
			if (m_iStagingNum > m_hvStagingPositions.Count())
			{
				Warning( "Advisor tries to stage %d objects but only has %d positions named %s! Overriding.\n", m_iStagingNum, m_hvStagingPositions.Count(), m_iszStagingEntities );
				m_iStagingNum = m_hvStagingPositions.Count() ;
			}


// advisor_staging_num

			// in the future i'll distribute the staging chronologically. For now, yank all the objects at once.
			if (m_hvStagedEnts.Count() < m_iStagingNum)
			{	
				// pull another object
				bool bDesperate = m_flStagingEnd - gpGlobals->curtime < 0.50f; // less than one half second left
				CBaseEntity *pThrowable = PickThrowable(!bDesperate);
				if (pThrowable)
				{
					// don't let the player take it from me
					IPhysicsObject *pPhys = pThrowable->VPhysicsGetObject();
					if ( pPhys )
					{
						// no pickup!
						pPhys->SetGameFlags(pPhys->GetGameFlags() | FVPHYSICS_NO_PLAYER_PICKUP );;
					}

					m_hvStagedEnts.AddToTail( pThrowable );
					Write_BeamOn(pThrowable);

					
					DispatchParticleEffect( "advisor_object_charge", PATTACH_ABSORIGIN_FOLLOW, 
							pThrowable, 0, 
							false  );
				}
			}


			Assert(m_hvStagedEnts.Count() <= m_hvStagingPositions.Count());

			// yank all objects into place
			for (int ii = m_hvStagedEnts.Count() - 1 ; ii >= 0 ; --ii)
			{

				// just ignore lost objects (if the player destroys one, that's fine, leave a hole)
				CBaseEntity *pThrowable = m_hvStagedEnts[ii];
				if (pThrowable)
				{
					PullObjectToStaging(pThrowable, m_hvStagingPositions[ii]->GetAbsOrigin());
				}
			}

			// are we done yet?
			if (gpGlobals->curtime > m_flStagingEnd)
			{
				TaskComplete();
				break;
			}
			
			break;
		}

		// Fling the object that we picked at our enemy's eyes!
		case TASK_ADVISOR_BARRAGE_OBJECTS:
		{
			Assert(m_hvStagedEnts.Count() > 0);

			// do I still have an enemy?
			if ( !GetEnemy() )
			{
				// no? bail all the objects. 
				for (int ii = m_hvStagedEnts.Count() - 1 ; ii >=0 ; --ii)
				{

					IPhysicsObject *pPhys = m_hvStagedEnts[ii]->VPhysicsGetObject();
					if ( pPhys )
					{  
						pPhys->SetGameFlags(pPhys->GetGameFlags() & (~FVPHYSICS_NO_PLAYER_PICKUP) );
					}
				}

				Write_AllBeamsOff();
				m_hvStagedEnts.RemoveAll();

				TaskFail( "Lost enemy" );
				return;
			}

			// do I still have something to throw at the player?
			CBaseEntity *pThrowable = m_hvStagedEnts[0];
			while (!pThrowable) 
			{	// player has destroyed whatever I planned to hit him with, get something else
                if (m_hvStagedEnts.Count() > 0)
				{
					pThrowable = ThrowObjectPrepare();
				}
				else
				{
					TaskComplete();
					break;
				}
			}

			// If we've gone NULL, then opt out
			if ( pThrowable == NULL )
			{
				TaskComplete();
				break;
			}

			if ( (gpGlobals->curtime > m_flThrowPhysicsTime - advisor_throw_lead_prefetch_time.GetFloat()) && 
				!m_vSavedLeadVel.IsValid() )
			{
				// save off the velocity we will use to lead the player a little early, so that if he jukes 
				// at the last moment he'll have a better shot of dodging the object.
				m_vSavedLeadVel = GetEnemy()->GetAbsVelocity();
			}

			// if it's time to throw something, throw it and go on to the next one. 
			if (gpGlobals->curtime > m_flThrowPhysicsTime)
			{
				IPhysicsObject *pPhys = pThrowable->VPhysicsGetObject();
				Assert(pPhys);

				pPhys->SetGameFlags(pPhys->GetGameFlags() & (~FVPHYSICS_NO_PLAYER_PICKUP) );
				HurlObjectAtPlayer(pThrowable,Vector(0,0,0)/*m_vSavedLeadVel*/);
				m_flLastThrowTime = gpGlobals->curtime;
				m_flThrowPhysicsTime = gpGlobals->curtime + 0.75f;
				// invalidate saved lead for next time
				m_vSavedLeadVel.Invalidate();

				EmitSound( "NPC_Advisor.Blast" );

				Write_BeamOff(m_hvStagedEnts[0]);
				m_hvStagedEnts.Remove(0);
				if (!ThrowObjectPrepare())
				{
					TaskComplete();
					break;
				}
			}
			else
			{	
				// wait, bide time
				// PullObjectToStaging(pThrowable, m_hvStagingPositions[ii]->GetAbsOrigin());
			}
		
			break;
		}

		case TASK_ADVISOR_PIN_PLAYER:
		{
			/*
			// bail out if the pin entity went away.
			CBaseEntity *pPinEnt = m_hPlayerPinPos;
			if (!pPinEnt)
			{
				GetEnemy()->SetGravity(1.0f);
				GetEnemy()->SetMoveType( MOVETYPE_WALK );
				TaskComplete();
				break;
			}

			// failsafe: don't do this for more than ten seconds.
			if ( gpGlobals->curtime > m_playerPinFailsafeTime )
			{
				GetEnemy()->SetGravity(1.0f);
				GetEnemy()->SetMoveType( MOVETYPE_WALK );
				Warning( "Advisor did not leave PIN PLAYER mode. Aborting due to ten second failsafe!\n" );
				TaskFail("Advisor did not leave PIN PLAYER mode. Aborting due to ten second failsafe!\n");
				break;
			}

			// if the player isn't the enemy, bail out.
			if ( !GetEnemy()->IsPlayer() )
			{
				GetEnemy()->SetGravity(1.0f);
				GetEnemy()->SetMoveType( MOVETYPE_WALK );
				TaskFail( "Player is not the enemy?!" );
				break;
			}

			GetEnemy()->SetMoveType( MOVETYPE_FLY );
			GetEnemy()->SetGravity(0);

			// use exponential falloff to peg the player to the pin point
			const Vector &desiredPos = pPinEnt->GetAbsOrigin();
			const Vector &playerPos = GetEnemy()->GetAbsOrigin();

			Vector displacement = desiredPos - playerPos;

			float desiredDisplacementLen = ExponentialDecay(0.250f,gpGlobals->frametime);// * sqrt(displacementLen);			

			Vector nuPos = playerPos + (displacement * (1.0f - desiredDisplacementLen));

			GetEnemy()->SetAbsOrigin( nuPos );

			break;
			*/
		}

		default:
		{
			BaseClass::RunTask( pTask );
		}
	}
}


#endif

// helper function for testing whether or not an avisor is allowed to grab an object
static bool AdvisorCanPickObject(CBasePlayer *pPlayer, CBaseEntity *pEnt)
{
	Assert( pPlayer != NULL );

	// Is the player carrying something?
	CBaseEntity *pHeldObject = GetPlayerHeldEntity(pPlayer);

	if( !pHeldObject )
	{
		pHeldObject = PhysCannonGetHeldEntity( pPlayer->GetActiveWeapon() );
	}

	if( pHeldObject == pEnt )
	{
		return false;
	}

	if ( pEnt->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
	{
		return false;
	}

	return true;
}


#if NPC_ADVISOR_HAS_BEHAVIOR
//-----------------------------------------------------------------------------
// Choose an object to throw.
// param bRequireInView : if true, only accept objects that are in the player's fov.
//
// Can always return NULL.
// todo priority_grab_name
//-----------------------------------------------------------------------------
CBaseEntity *CNPC_Advisor::PickThrowable( bool bRequireInView )
{
	CBasePlayer *pPlayer = ToBasePlayer( GetEnemy() );
	Assert(pPlayer);
	if (!pPlayer)
		return NULL;

	const int numObjs = m_physicsObjects.Count(); ///< total number of physics objects in my system
	if (numObjs < 1) 
		return NULL; // bail out if nothing available

	
	// used for require-in-view
	Vector eyeForward, eyeOrigin;
	if (pPlayer)
	{
		eyeOrigin = pPlayer->EyePosition();
		pPlayer->EyeVectors(&eyeForward);
	}
	else
	{
		bRequireInView = false;
	}

	// filter-and-choose algorithm:
	// build a list of candidates
	Assert(numObjs < 128); /// I'll come back and utlvector this shortly -- wanted easier debugging
	unsigned int candidates[128];
	unsigned int numCandidates = 0;

	if (!!m_iszPriorityEntityGroupName) // if the string isn't null
	{
		// first look to see if we have any priority objects.
		for (int ii = 0 ; ii < numObjs ; ++ii )
		{
			CBaseEntity *pThrowEnt = m_physicsObjects[ii];
			// Assert(pThrowEnt); 
			if (!pThrowEnt)
				continue;

			if (!pThrowEnt->NameMatches(m_iszPriorityEntityGroupName)) // if this is not a priority object
				continue;

			bool bCanPick = AdvisorCanPickObject( pPlayer, pThrowEnt ) && !m_hvStagedEnts.HasElement( m_physicsObjects[ii] );
			if (!bCanPick)
				continue;

			// bCanPick guaranteed true here

			if ( bRequireInView )
			{
				bCanPick = (pThrowEnt->GetAbsOrigin() - eyeOrigin).Dot(eyeForward) > 0;
			}

			if ( bCanPick )
			{
				candidates[numCandidates++] = ii; 
			}
		}
	}

	// if we found no priority objects (or don't have a priority), just grab whatever
	if (numCandidates == 0)
	{
		for (int ii = 0 ; ii < numObjs ; ++ii )
		{
			CBaseEntity *pThrowEnt = m_physicsObjects[ii];
			// Assert(pThrowEnt); 
			if (!pThrowEnt)
				continue;

			bool bCanPick = AdvisorCanPickObject( pPlayer, pThrowEnt ) && !m_hvStagedEnts.HasElement( m_physicsObjects[ii] );
			if (!bCanPick)
				continue;

			// bCanPick guaranteed true here

			if ( bRequireInView )
			{
				bCanPick = (pThrowEnt->GetAbsOrigin() - eyeOrigin).Dot(eyeForward) > 0;
			}

			if ( bCanPick )
			{
				candidates[numCandidates++] = ii; 
			}
		}
	}

	if ( numCandidates == 0 )
		return NULL; // must have at least one candidate

	// pick a random candidate.
	int nRandomIndex = random->RandomInt( 0, numCandidates - 1 );
	return m_physicsObjects[candidates[nRandomIndex]];

}

/*! \TODO
	Correct bug where Advisor seemed to be throwing stuff at people's feet. 
	This is because the object was falling slightly in between the staging 
	and when he threw it, and that downward velocity was getting accumulated 
	into the throw speed. This is temporarily fixed here by using SetVelocity 
	instead of AddVelocity, but the proper fix is to pin the object to its 
	staging point during the warn period. That will require maintaining a map
	of throwables to their staging points during the throw task.
*/
//-----------------------------------------------------------------------------
// Impart necessary force on any entity to make it clobber Gordon.
// Also detaches from levitate controller.
// The optional lead velocity parameter is for cases when we pre-save off the 
// player's speed, to make last-moment juking more effective
//-----------------------------------------------------------------------------
void CNPC_Advisor::HurlObjectAtPlayer( CBaseEntity *pEnt, const Vector &leadVel )
{
	IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();

	//
	// Lead the target accurately. This encourages hiding behind cover
	// and/or catching the thrown physics object!
	//
	Vector vecObjOrigin = pEnt->CollisionProp()->WorldSpaceCenter();
	Vector vecEnemyPos = GetEnemy()->EyePosition();
	// disabled -- no longer compensate for gravity: // vecEnemyPos.y += 12.0f;

//	const Vector &leadVel = pLeadVelocity ? *pLeadVelocity : GetEnemy()->GetAbsVelocity();

	Vector vecDelta = vecEnemyPos - vecObjOrigin;
	float flDist = vecDelta.Length();

	float flVelocity = advisor_throw_velocity.GetFloat();

	if ( flVelocity == 0 )
	{
		flVelocity = 1000;
	}
		
	float flFlightTime = flDist / flVelocity;

	Vector vecThrowAt = vecEnemyPos + flFlightTime * leadVel;
	Vector vecThrowDir = vecThrowAt - vecObjOrigin;
	VectorNormalize( vecThrowDir );
	
	Vector vecVelocity = flVelocity * vecThrowDir;
	pPhys->SetVelocity( &vecVelocity, NULL );

	AddToThrownObjects(pEnt);

	m_OnThrow.FireOutput(pEnt,this);

}


//-----------------------------------------------------------------------------
// do a sweep from an object I'm about to throw, to the target, pushing aside 
// anything floating in the way.
// TODO: this is probably a good profiling candidate.
//-----------------------------------------------------------------------------
void CNPC_Advisor::PreHurlClearTheWay( CBaseEntity *pThrowable, const Vector &toPos )
{
	// look for objects in the way of chucking.
	CBaseEntity *list[128];
	Ray_t ray;

	
	float boundingRadius = pThrowable->BoundingRadius();
	
	ray.Init( pThrowable->GetAbsOrigin(), toPos,
			  Vector(-boundingRadius,-boundingRadius,-boundingRadius),
		      Vector( boundingRadius, boundingRadius, boundingRadius) );

	int nFoundCt = UTIL_EntitiesAlongRay( list, 128, ray, 0 );
	AssertMsg(nFoundCt < 128, "Found more than 128 obstructions between advisor and Gordon while throwing. (safe to continue)\n");

	// for each thing in the way that I levitate, but is not something I'm staging
	// or throwing, push it aside.
	for (int i = 0 ; i < nFoundCt ; ++i )
	{
		CBaseEntity *obstruction = list[i];
		if (  obstruction != pThrowable                  &&
			  m_physicsObjects.HasElement( obstruction ) && // if it's floating
			 !m_hvStagedEnts.HasElement( obstruction )   && // and I'm not staging it
			 !DidThrow( obstruction ) )						// and I didn't just throw it
		{
            IPhysicsObject *pPhys = obstruction->VPhysicsGetObject();
			Assert(pPhys); 

			// this is an object we want to push out of the way. Compute a vector perpendicular
			// to the path of the throwables's travel, and thrust the object along that vector.
			Vector thrust;
			CalcClosestPointOnLine( obstruction->GetAbsOrigin(),
									pThrowable->GetAbsOrigin(), 
									toPos,
									thrust );
			// "thrust" is now the closest point on the line to the obstruction. 
			// compute the difference to get the direction of impulse
			thrust = obstruction->GetAbsOrigin() - thrust;

			// and renormalize it to equal a giant kick out of the way
			// (which I'll say is about ten feet per second -- if we want to be
			//  more precise we could do some kind of interpolation based on how
			//  far away the object is)
			float thrustLen = thrust.Length();
			if (thrustLen > 0.0001f)
			{
				thrust *= advisor_throw_clearout_vel.GetFloat() / thrustLen; 
			}

			// heave!
			pPhys->AddVelocity( &thrust, NULL );
		}
	}

/*

	// Otherwise only help out a little
	Vector extents = Vector(256, 256, 256);
	Ray_t ray;
	ray.Init( vecStartPoint, vecStartPoint + 2048 * vecVelDir, -extents, extents );
	int nCount = UTIL_EntitiesAlongRay( list, 1024, ray, FL_NPC | FL_CLIENT );
	for ( int i = 0; i < nCount; i++ )
	{
		if ( !IsAttractiveTarget( list[i] ) )
			continue;

		VectorSubtract( list[i]->WorldSpaceCenter(), vecStartPoint, vecDelta );
		distance = VectorNormalize( vecDelta );
		flDot = DotProduct( vecDelta, vecVelDir );
		
		if ( flDot > flMaxDot )
		{
			if ( distance < flBestDist )
			{
				pBestTarget = list[i];
				flBestDist = distance;
			}
		}
	}

*/

}

/* 
// commented out because unnecessary: we will do this during the DidThrow check

//-----------------------------------------------------------------------------
// clean out the recently thrown objects array
//-----------------------------------------------------------------------------
void CNPC_Advisor::PurgeThrownObjects() 
{
	float threeSecondsAgo = gpGlobals->curtime - 3.0f; // two seconds ago

	for (int ii = 0 ; ii < kMaxThrownObjectsTracked ; ++ii)
	{
		if ( m_haRecentlyThrownObjects[ii].IsValid() && 
			 m_flaRecentlyThrownObjectTimes[ii] < threeSecondsAgo )
		{
			 m_haRecentlyThrownObjects[ii].Set(NULL);
		}
	}
	
}
*/


//-----------------------------------------------------------------------------
// true iff an advisor threw the object in the last three seconds
//-----------------------------------------------------------------------------
bool CNPC_Advisor::DidThrow(const CBaseEntity *pEnt)
{
	// look through all my objects and see if they match this entity. Incidentally if 
	// they're more than three seconds old, purge them.
	float threeSecondsAgo = gpGlobals->curtime - 3.0f; 

	for (int ii = 0 ; ii < kMaxThrownObjectsTracked ; ++ii)
	{
		// if object is old, skip it.
		CBaseEntity *pTestEnt = m_haRecentlyThrownObjects[ii];

		if ( pTestEnt ) 
		{
			if ( m_flaRecentlyThrownObjectTimes[ii] < threeSecondsAgo )
			{
				m_haRecentlyThrownObjects[ii].Set(NULL);
				continue;
			}
			else if (pTestEnt == pEnt)
			{
				return true;
			}
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::AddToThrownObjects(CBaseEntity *pEnt)
{
	Assert(pEnt);

	// try to find an empty slot, or if none exists, the oldest object
	int oldestThrownObject = 0;
	for (int ii = 0 ; ii < kMaxThrownObjectsTracked ; ++ii)
	{
		if (m_haRecentlyThrownObjects[ii].IsValid())
		{
			if (m_flaRecentlyThrownObjectTimes[ii] < m_flaRecentlyThrownObjectTimes[oldestThrownObject])
			{
				oldestThrownObject = ii;
			}
		}
		else
		{	// just use this one
			oldestThrownObject = ii;
			break;
		}
	}

	m_haRecentlyThrownObjects[oldestThrownObject] = pEnt;
	m_flaRecentlyThrownObjectTimes[oldestThrownObject] = gpGlobals->curtime;

}


//-----------------------------------------------------------------------------
// Drag a particular object towards its staging location.
//-----------------------------------------------------------------------------
void CNPC_Advisor::PullObjectToStaging( CBaseEntity *pEnt, const Vector &stagingPos )
{
	IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();
	Assert(pPhys);

	Vector curPos = pEnt->CollisionProp()->WorldSpaceCenter();
	Vector displacement = stagingPos - curPos;

	// quick and dirty -- use exponential decay to haul the object into place
	// ( a better looking solution would be to use a spring system )

	float desiredDisplacementLen = ExponentialDecay(STAGING_OBJECT_FALLOFF_TIME, gpGlobals->frametime);// * sqrt(displacementLen);
	
	Vector vel; AngularImpulse angimp;
	pPhys->GetVelocity(&vel,&angimp);

	vel = (1.0f / gpGlobals->frametime)*(displacement * (1.0f - desiredDisplacementLen));
	pPhys->SetVelocity(&vel,&angimp);
}



#endif

int	CNPC_Advisor::OnTakeDamage( const CTakeDamageInfo &info )
{
	// Clip our max 
	CTakeDamageInfo newInfo = info;
	if ( newInfo.GetDamage() > 20.0f )
	{
		newInfo.SetDamage( 20.0f );
	}

	// Hack to make him constantly flinch
	m_flNextFlinchTime = gpGlobals->curtime;

	const float oldLastDamageTime = m_flLastDamageTime;
	int retval = BaseClass::OnTakeDamage(newInfo);

	// we have a special reporting output 
	if ( oldLastDamageTime != gpGlobals->curtime )
	{
		// only fire once per frame

		m_OnHealthIsNow.Set( GetHealth(), newInfo.GetAttacker(), this);
	}

	return retval;
}




#if NPC_ADVISOR_HAS_BEHAVIOR
//-----------------------------------------------------------------------------
//  Returns the best new schedule for this NPC based on current conditions.
//-----------------------------------------------------------------------------
int CNPC_Advisor::SelectSchedule()
{
    if ( IsInAScript() )
        return SCHED_ADVISOR_IDLE_STAND;

	switch ( m_NPCState )
	{
		case NPC_STATE_IDLE:
		case NPC_STATE_ALERT:
		{
			return SCHED_ADVISOR_IDLE_STAND;
		} 

		case NPC_STATE_COMBAT:
		{
			if ( GetEnemy() && GetEnemy()->IsAlive() )
			{
				if ( false /* m_hPlayerPinPos.IsValid() */ )
					return SCHED_ADVISOR_TOSS_PLAYER;
				else
					return SCHED_ADVISOR_COMBAT;
				
			}
			
			return SCHED_ADVISOR_IDLE_STAND;
		}
	}

	return BaseClass::SelectSchedule();
}


//-----------------------------------------------------------------------------
// return the position where an object should be staged before throwing
//-----------------------------------------------------------------------------
Vector CNPC_Advisor::GetThrowFromPos( CBaseEntity *pEnt )
{
	Assert(pEnt);
	Assert(pEnt->VPhysicsGetObject());
	const CCollisionProperty *cProp = pEnt->CollisionProp();
	Assert(cProp);

	float effecRadius = cProp->BoundingRadius(); // radius of object (important for kickout)
	float howFarInFront = advisor_throw_stage_distance.GetFloat() + effecRadius * 1.43f;// clamp(lenToPlayer - posDist + effecRadius,effecRadius*2,90.f + effecRadius);
	
	Vector fwd;
	GetVectors(&fwd,NULL,NULL);
	
	return GetAbsOrigin() + fwd*howFarInFront;
}
#endif


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::Precache()
{
	BaseClass::Precache();
	
	PrecacheModel( STRING( GetModelName() ) );

#if NPC_ADVISOR_HAS_BEHAVIOR
	PrecacheModel( "sprites/lgtning.vmt" );
#endif

	PrecacheScriptSound( "NPC_Advisor.Blast" );
	PrecacheScriptSound( "NPC_Advisor.Gib" );
	PrecacheScriptSound( "NPC_Advisor.Idle" );
	PrecacheScriptSound( "NPC_Advisor.Alert" );
	PrecacheScriptSound( "NPC_Advisor.Die" );
	PrecacheScriptSound( "NPC_Advisor.Pain" );
	PrecacheScriptSound( "NPC_Advisor.ObjectChargeUp" );
	PrecacheParticleSystem( "Advisor_Psychic_Beam" );
	PrecacheParticleSystem( "advisor_object_charge" );
	PrecacheModel("sprites/greenglow1.vmt");
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Advisor::IdleSound()
{
	EmitSound( "NPC_Advisor.Idle" );
}


void CNPC_Advisor::AlertSound()
{
	EmitSound( "NPC_Advisor.Alert" );
}


void CNPC_Advisor::PainSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Advisor.Pain" );
}


void CNPC_Advisor::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_Advisor.Die" );
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Advisor::DrawDebugTextOverlays()
{
	int nOffset = BaseClass::DrawDebugTextOverlays();
	return nOffset;
}


#if NPC_ADVISOR_HAS_BEHAVIOR
//-----------------------------------------------------------------------------
// Determines which sounds the advisor cares about.
//-----------------------------------------------------------------------------
int CNPC_Advisor::GetSoundInterests()
{
	return SOUND_WORLD | SOUND_COMBAT | SOUND_PLAYER | SOUND_DANGER;
}


//-----------------------------------------------------------------------------
// record the last time we heard a combat sound
//-----------------------------------------------------------------------------
bool CNPC_Advisor::QueryHearSound( CSound *pSound )
{
	// Disregard footsteps from our own class type
	CBaseEntity *pOwner = pSound->m_hOwner;
	if ( pOwner && pSound->IsSoundType( SOUND_COMBAT ) && pSound->SoundChannel() != SOUNDENT_CHANNEL_NPC_FOOTSTEP && pSound->m_hOwner.IsValid() && pOwner->IsPlayer() )
	{
		// Msg("Heard player combat.\n");
		m_flLastPlayerAttackTime = gpGlobals->curtime;
	}

	return BaseClass::QueryHearSound(pSound);
}

//-----------------------------------------------------------------------------
// designer hook for setting throw rate
//-----------------------------------------------------------------------------
void CNPC_Advisor::InputSetThrowRate( inputdata_t &inputdata )
{
	advisor_throw_rate.SetValue(inputdata.value.Float());
}

void CNPC_Advisor::InputSetStagingNum( inputdata_t &inputdata )
{
	m_iStagingNum = inputdata.value.Int();
}

// 
//  cause the player to be pinned to a point in space
// 
void CNPC_Advisor::InputPinPlayer( inputdata_t &inputdata )
{
	string_t targetname = inputdata.value.StringID();

	// null string means designer is trying to unpin the player
	if (!targetname)
	{
		m_hPlayerPinPos = NULL;
	}

	// otherwise try to look up the entity and make it a target.
	CBaseEntity *pEnt = gEntList.FindEntityByName(NULL,targetname);

	if (pEnt)
	{
		m_hPlayerPinPos = pEnt;
	}
	else
	{
		// if we couldn't find the target, just bail on the behavior.
		Warning("Advisor tried to pin player to %s but that does not exist.\n", targetname.ToCStr());
		m_hPlayerPinPos = NULL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Advisor::OnScheduleChange( void )
{
	Write_AllBeamsOff();
	m_hvStagedEnts.RemoveAll();
	BaseClass::OnScheduleChange();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Advisor::GatherConditions( void )
{
	BaseClass::GatherConditions();

	// Handle script state changes
	bool bInScript = IsInAScript();
	if ( ( m_bWasScripting && bInScript == false ) || ( m_bWasScripting == false && bInScript ) )
	{
		SetCondition( COND_ADVISOR_PHASE_INTERRUPT );
	}
	
	// Retain this
	m_bWasScripting = bInScript;
}

//-----------------------------------------------------------------------------
// designer hook for yanking an object into the air right now
//-----------------------------------------------------------------------------
void CNPC_Advisor::InputWrenchImmediate( inputdata_t &inputdata )
{
	string_t groupname = inputdata.value.StringID();

	Assert(!!groupname);

	// for all entities with that name that aren't floating, punt them at me and add them to the levitation

	CBaseEntity *pEnt = NULL;

	const Vector &myPos = GetAbsOrigin() + Vector(0,36.0f,0);

	// conditional assignment: find an entity by name and save it into pEnt. Bail out when none are left.
	while ( ( pEnt = gEntList.FindEntityByName(pEnt,groupname) ) != NULL )
	{
		// if I'm not already levitating it, and if I didn't just throw it
		if (!m_physicsObjects.HasElement(pEnt) )
		{
			// add to levitation
			IPhysicsObject *pPhys = pEnt->VPhysicsGetObject();
			if ( pPhys )
			{
				// if the object isn't moveable, make it so.
                if ( !pPhys->IsMoveable() )
				{
					pPhys->EnableMotion( true );
				}

				// first, kick it at me
				Vector objectToMe;
				pPhys->GetPosition(&objectToMe,NULL);
                objectToMe = myPos - objectToMe;
				// compute a velocity that will get it here in about a second
				objectToMe /= (1.5f * gpGlobals->frametime);

				objectToMe *= random->RandomFloat(0.25f,1.0f);

				pPhys->SetVelocity( &objectToMe, NULL );

				// add it to tracked physics objects
				m_physicsObjects.AddToTail( pEnt );

				m_pLevitateController->AttachObject( pPhys, false );
				pPhys->Wake();
			}
			else
			{
				Warning( "Advisor tried to wrench %s, but it is not moveable!", pEnt->GetEntityName().ToCStr());
			}
		}
	}

}



//-----------------------------------------------------------------------------
// write a message turning a beam on
//-----------------------------------------------------------------------------
void CNPC_Advisor::Write_BeamOn(  CBaseEntity *pEnt )
{
	Assert( pEnt );
	EntityMessageBegin( this, true );
		WRITE_BYTE( ADVISOR_MSG_START_BEAM );
		WRITE_LONG( pEnt->entindex() );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// write a message turning a beam off
//-----------------------------------------------------------------------------
void CNPC_Advisor::Write_BeamOff( CBaseEntity *pEnt )
{
	Assert( pEnt );
	EntityMessageBegin( this, true );
		WRITE_BYTE( ADVISOR_MSG_STOP_BEAM );
		WRITE_LONG( pEnt->entindex() );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// tell client to kill all beams
//-----------------------------------------------------------------------------
void CNPC_Advisor::Write_AllBeamsOff( void )
{
	EntityMessageBegin( this, true );
		WRITE_BYTE( ADVISOR_MSG_STOP_ALL_BEAMS );
	MessageEnd();
}

//-----------------------------------------------------------------------------
// input wrapper around Write_BeamOn
//-----------------------------------------------------------------------------
void CNPC_Advisor::InputTurnBeamOn( inputdata_t &inputdata )
{
	// inputdata should specify a target
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.StringID() );
	if ( pTarget )
	{
		Write_BeamOn( pTarget );
	}
	else
	{
		Warning("InputTurnBeamOn could not find object %s", inputdata.value.String() );
	}
}


//-----------------------------------------------------------------------------
// input wrapper around Write_BeamOff
//-----------------------------------------------------------------------------
void CNPC_Advisor::InputTurnBeamOff( inputdata_t &inputdata )
{
	// inputdata should specify a target
	CBaseEntity *pTarget = gEntList.FindEntityByName( NULL, inputdata.value.StringID() );
	if ( pTarget )
	{
		Write_BeamOff( pTarget );
	}
	else
	{
		Warning("InputTurnBeamOn could not find object %s", inputdata.value.String() );
	}
}


void CNPC_Advisor::InputElightOn( inputdata_t &inputdata )
{
	EntityMessageBegin( this, true );
	WRITE_BYTE( ADVISOR_MSG_START_ELIGHT );
	MessageEnd();
}

void CNPC_Advisor::InputElightOff( inputdata_t &inputdata )
{
	EntityMessageBegin( this, true );
	WRITE_BYTE( ADVISOR_MSG_STOP_ELIGHT );
	MessageEnd();
}
#endif


//==============================================================================================
// MOTION CALLBACK
//==============================================================================================
CAdvisorLevitate::simresult_e	CAdvisorLevitate::Simulate( IPhysicsMotionController *pController, IPhysicsObject *pObject, float deltaTime, Vector &linear, AngularImpulse &angular )
{
   	// this function can be optimized to minimize branching if necessary (PPE branch prediction) 
	CNPC_Advisor *pAdvisor = static_cast<CNPC_Advisor *>(m_Advisor.Get());
	Assert(pAdvisor);

	if ( !OldStyle() )
	{	// independent movement of all objects
		// if an object was recently thrown, just zero out its gravity.
		if (pAdvisor->DidThrow(static_cast<CBaseEntity *>(pObject->GetGameData())))
		{
			linear = Vector( 0, 0, GetCurrentGravity() );
			
			return SIM_GLOBAL_ACCELERATION;
		}
		else
		{
			Vector vel; AngularImpulse angvel;
			pObject->GetVelocity(&vel,&angvel);
			Vector pos;
			pObject->GetPosition(&pos,NULL);
			bool bMovingUp = vel.z > 0;

			// if above top limit and moving up, move down. if below bottom limit and moving down, move up.
			if (bMovingUp)
			{
				if (pos.z > m_vecGoalPos2.z)
				{
					// turn around move down
					linear = Vector( 0, 0, Square((1.0f - m_flFloat)) * GetCurrentGravity() );
					angular = Vector( 0, -5, 0 );
				}
				else
				{	// keep moving up 
					linear = Vector( 0, 0, (1.0f + m_flFloat) * GetCurrentGravity() );
					angular = Vector( 0, 0, 10 );
				}
			}
			else
			{
				if (pos.z < m_vecGoalPos1.z)
				{
					// turn around move up
					linear = Vector( 0, 0, Square((1.0f + m_flFloat)) * GetCurrentGravity() );
					angular = Vector( 0, 5, 0 );
				}
				else
				{	// keep moving down
					linear = Vector( 0, 0, (1.0f - m_flFloat) * GetCurrentGravity() );
					angular = Vector( 0, 0, 10 );
				}
			}
			
			return SIM_GLOBAL_ACCELERATION;
		}

		//NDebugOverlay::Cross3D(pos,24.0f,255,255,0,true,0.04f);
		
	}
	else // old stateless technique
	{
		Warning("Advisor using old-style object movement!\n");

		/* // obsolete
		CBaseEntity *pEnt = (CBaseEntity *)pObject->GetGameData();
		Vector vecDir1 = m_vecGoalPos1 - pEnt->GetAbsOrigin();
		VectorNormalize( vecDir1 );
		
		Vector vecDir2 = m_vecGoalPos2 - pEnt->GetAbsOrigin();
		VectorNormalize( vecDir2 );
		*/
	
		linear = Vector( 0, 0, m_flFloat * GetCurrentGravity() );// + m_flFloat * 0.5 * ( vecDir1 + vecDir2 );
		angular = Vector( 0, 0, 10 );
		
		return SIM_GLOBAL_ACCELERATION;
	}

}


//==============================================================================================
// ADVISOR PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t advisorLinearTable[] =
{
	{ 100*100,	10 },
	{ 250*250,	25 },
	{ 350*350,	50 },
	{ 500*500,	75 },
	{ 1000*1000,100 },
};

static impactentry_t advisorAngularTable[] =
{
	{  50* 50, 10 },
	{ 100*100, 25 },
	{ 150*150, 50 },
	{ 200*200, 75 },
};

static impactdamagetable_t gAdvisorImpactDamageTable =
{
	advisorLinearTable,
	advisorAngularTable,
	
	ARRAYSIZE(advisorLinearTable),
	ARRAYSIZE(advisorAngularTable),

	200*200,// minimum linear speed squared
	180*180,// minimum angular speed squared (360 deg/s to cause spin/slice damage)
	15,		// can't take damage from anything under 15kg

	10,		// anything less than 10kg is "small"
	5,		// never take more than 1 pt of damage from anything under 15kg
	128*128,// <15kg objects must go faster than 36 in/s to do damage

	45,		// large mass in kg 
	2,		// large mass scale (anything over 500kg does 4X as much energy to read from damage table)
	1,		// large mass falling scale
	0,		// my min velocity
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const impactdamagetable_t
//-----------------------------------------------------------------------------
const impactdamagetable_t &CNPC_Advisor::GetPhysicsImpactDamageTable( void )
{
	return advisor_use_impact_table.GetBool() ? gAdvisorImpactDamageTable : BaseClass::GetPhysicsImpactDamageTable();
}



#if NPC_ADVISOR_HAS_BEHAVIOR
//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------
AI_BEGIN_CUSTOM_NPC( npc_advisor, CNPC_Advisor )

	DECLARE_TASK( TASK_ADVISOR_FIND_OBJECTS )
	DECLARE_TASK( TASK_ADVISOR_LEVITATE_OBJECTS )
	/*
	DECLARE_TASK( TASK_ADVISOR_PICK_THROW_OBJECT )
	DECLARE_TASK( TASK_ADVISOR_THROW_OBJECT )
	*/

	DECLARE_CONDITION( COND_ADVISOR_PHASE_INTERRUPT )	// A stage has interrupted us

	DECLARE_TASK( TASK_ADVISOR_STAGE_OBJECTS ) // haul all the objects into the throw-from slots
	DECLARE_TASK( TASK_ADVISOR_BARRAGE_OBJECTS ) // hurl all the objects in sequence

	DECLARE_TASK( TASK_ADVISOR_PIN_PLAYER ) // pinion the player to a point in space
	
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ADVISOR_COMBAT,

		"	Tasks"
		"		TASK_ADVISOR_FIND_OBJECTS			0"
		"		TASK_ADVISOR_LEVITATE_OBJECTS		0"
		"		TASK_ADVISOR_STAGE_OBJECTS			1"
		"		TASK_ADVISOR_BARRAGE_OBJECTS		0"
		"	"
		"	Interrupts"
		"		COND_ADVISOR_PHASE_INTERRUPT"
		"		COND_ENEMY_DEAD"
	)

	//=========================================================
	DEFINE_SCHEDULE
	(
	SCHED_ADVISOR_IDLE_STAND,

		"	Tasks"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_WAIT				3"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_ADVISOR_PHASE_INTERRUPT"
	)
	
	DEFINE_SCHEDULE
	(
		SCHED_ADVISOR_TOSS_PLAYER,

		"	Tasks"
		"		TASK_ADVISOR_FIND_OBJECTS			0"
		"		TASK_ADVISOR_LEVITATE_OBJECTS		0"
		"		TASK_ADVISOR_PIN_PLAYER				0"
		"	"
		"	Interrupts"
	)

AI_END_CUSTOM_NPC()
#endif

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// npc_blob - experimental, cpu-intensive monster made of lots of smaller elements
//
//=============================================================================//
#include "cbase.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_schedule.h"
#include "ai_hull.h"
#include "soundent.h"
#include "game.h"
#include "npcevent.h"
#include "entitylist.h"
#include "activitylist.h"
#include "ai_basenpc.h"
#include "engine/IEngineSound.h"
#include "vstdlib/jobthread.h"
#include "saverestore_utlvector.h"
#include "eventqueue.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern float MOVE_HEIGHT_EPSILON;

#define BLOB_MAX_AVOID_ORIGINS 3

ConVar blob_mindist( "blob_mindist", "120.0" );
ConVar blob_element_speed( "blob_element_speed", "187" );
ConVar npc_blob_idle_speed_factor( "npc_blob_idle_speed_factor", "0.5" );

ConVar blob_numelements( "blob_numelements", "20" );
ConVar blob_batchpercent( "blob_batchpercent", "100" );

ConVar blob_radius( "blob_radius", "160" );

//ConVar blob_min_element_speed( "blob_min_element_speed", "50" );
//ConVar blob_max_element_speed( "blob_max_element_speed", "250" );

ConVar npc_blob_use_threading( "npc_blob_use_threading", "1" );

ConVar npc_blob_sin_amplitude( "npc_blob_sin_amplitude", "60.0f" );

ConVar npc_blob_show_centroid( "npc_blob_show_centroid", "0" );

ConVar npc_blob_straggler_dist( "npc_blob_straggler_dist", "240" );

ConVar npc_blob_use_orientation( "npc_blob_use_orientation", "1" );
ConVar npc_blob_use_model( "npc_blob_use_model", "2" );

ConVar npc_blob_think_interval( "npc_blob_think_interval", "0.025" );


#define NPC_BLOB_MODEL "models/headcrab.mdl"

//=========================================================
// Blob movement rules
//=========================================================
enum
{
	BLOB_MOVE_SWARM = 0,				// Just swarm with the rest of the group
	BLOB_MOVE_TO_TARGET_LOCATION,		// Move to a designated location
	BLOB_MOVE_TO_TARGET_ENTITY,			// Chase the designated entity
	BLOB_MOVE_DONT_MOVE,				// Sit still!!!!
};

//=========================================================
//=========================================================
class CBlobElement : public CBaseAnimating
{
public:
	void Precache();
	void Spawn();
	int	DrawDebugTextOverlays(void); 

	void	SetElementVelocity( Vector vecVelocity, bool bPlanarOnly );
	void	AddElementVelocity( Vector vecVelocityAdd, bool bPlanarOnly );
	void	ModifyVelocityForSurface( float flInterval, float flSpeed );

	void	SetSinePhase( float flPhase ) { m_flSinePhase = flPhase; }
	float	GetSinePhase() { return m_flSinePhase; }

	float	GetSineAmplitude() { return m_flSineAmplitude; }
	float	GetSineFrequency() { return m_flSineFrequency; }

	void	SetActiveMovementRule( int moveRule ) { m_iMovementRule = moveRule; }
	int		GetActiveMovementRule() { return m_iMovementRule; }

	void	MoveTowardsTargetEntity( float speed );
	void	SetTargetEntity( CBaseEntity *pEntity ) { m_hTargetEntity = pEntity; }
	CBaseEntity *GetTargetEntity() { return m_hTargetEntity.Get(); }

	void	MoveTowardsTargetLocation( float speed );
	void	SetTargetLocation( const Vector &vecLocation ) { m_vecTargetLocation = vecLocation; }

	void	ReconfigureRandomParams();
	void	EnforceSpeedLimits( float flMinSpeed, float flMaxSpeed );

	DECLARE_DATADESC();

public:
	Vector	m_vecPrevOrigin;	// Only exists for debugging (isolating stuck elements)
	int		m_iStuckCount;
	bool	m_bOnWall;
	float	m_flDistFromCentroidSqr;
	int		m_iElementNumber;
	Vector	m_vecTargetLocation;
	float	m_flRandomEightyPercent;

private:
	EHANDLE	m_hTargetEntity;
	float	m_flSinePhase;
	float	m_flSineAmplitude;
	float	m_flSineFrequency;
	int		m_iMovementRule;
};
LINK_ENTITY_TO_CLASS( blob_element, CBlobElement );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CBlobElement )

DEFINE_FIELD( m_vecPrevOrigin,			FIELD_POSITION_VECTOR ),
DEFINE_FIELD( m_iStuckCount,			FIELD_INTEGER ),
DEFINE_FIELD( m_bOnWall,				FIELD_BOOLEAN ),
DEFINE_FIELD( m_flDistFromCentroidSqr,	FIELD_FLOAT ),
DEFINE_FIELD( m_iElementNumber,			FIELD_INTEGER ),
DEFINE_FIELD( m_vecTargetLocation,		FIELD_POSITION_VECTOR ),
DEFINE_FIELD( m_hTargetEntity,			FIELD_EHANDLE ),
DEFINE_FIELD( m_flSinePhase,			FIELD_FLOAT ),
DEFINE_FIELD( m_flSineAmplitude,		FIELD_FLOAT ),
DEFINE_FIELD( m_flSineFrequency,		FIELD_FLOAT ),
DEFINE_FIELD( m_iMovementRule,			FIELD_INTEGER ),

END_DATADESC()


const char *pszBlobModels[] =
{
	"models/gibs/agibs.mdl",
	"models/props_junk/watermelon01.mdl",
	"models/w_squeak.mdl",
	"models/baby_headcrab.mdl"
};

const char *GetBlobModelName()
{
	int index = npc_blob_use_model.GetInt();

	return pszBlobModels[ index ];
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBlobElement::Precache()
{
	PrecacheModel( GetBlobModelName() );

	m_flRandomEightyPercent = random->RandomFloat( 0.8f, 1.0f );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CBlobElement::Spawn()
{
	Precache();
	
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_FLY );
	AddSolidFlags( FSOLID_NOT_STANDABLE | FSOLID_NOT_SOLID );

	SetModel( GetBlobModelName() );
	UTIL_SetSize( this, vec3_origin, vec3_origin );

	QAngle angles(0,0,0);
	angles.y = random->RandomFloat( 0, 180 );
	SetAbsAngles( angles );

	AddEffects( EF_NOSHADOW );

	ReconfigureRandomParams();
}

//---------------------------------------------------------
//---------------------------------------------------------
int CBlobElement::DrawDebugTextOverlays(void) 
{
	int text_offset = BaseClass::DrawDebugTextOverlays();
	if (m_debugOverlays & OVERLAY_TEXT_BIT) 
	{
		char tempstr[512];
		Q_snprintf(tempstr,sizeof(tempstr), "Element #:%d", m_iElementNumber );
		EntityText(text_offset,tempstr,0);
		text_offset++;
	}
	return text_offset;
}


//---------------------------------------------------------
// This is the official way to set velocity for an element
// Do not call SetAbsVelocity() directly, since we also
// need to record the last velocity we intended to give the
// element, so that we can detect changes after game physics
// runs.
//---------------------------------------------------------
void CBlobElement::SetElementVelocity( Vector vecVelocity, bool bPlanarOnly )
{
	SetAbsVelocity( vecVelocity );
}

//---------------------------------------------------------
// This is the official way to add velocity to an element. 
// See SetElementVelocity() for explanation.
//---------------------------------------------------------
void CBlobElement::AddElementVelocity( Vector vecVelocityAdd, bool bPlanarOnly )
{
	Vector vecSum = GetAbsVelocity() + vecVelocityAdd;
	SetAbsVelocity( vecSum );
}

//---------------------------------------------------------
// This function seeks to keep the blob element moving along
// multiple different types of surfaces (climbing walls, etc)
//---------------------------------------------------------
#define BLOB_TRACE_HEIGHT 8.0f
void CBlobElement::ModifyVelocityForSurface( float flInterval, float flSpeed )
{
	trace_t tr;
	Vector vecStart = GetAbsOrigin();
	Vector up = Vector( 0, 0, BLOB_TRACE_HEIGHT );

	Vector vecWishedGoal = vecStart + (GetAbsVelocity() * flInterval);

	UTIL_TraceLine( vecStart + up, vecWishedGoal + up, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 0, 0, false, 0.1f );

	m_bOnWall = false;

	if( tr.fraction == 1.0f )
	{
		UTIL_TraceLine( vecWishedGoal + up, vecWishedGoal - (up * 2.0f), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		//NDebugOverlay::Line( tr.startpos, tr.endpos, 255, 255, 0, false, 0.1f );
		tr.endpos.z += MOVE_HEIGHT_EPSILON;
	}
	else
	{
		//NDebugOverlay::Cross3D( GetAbsOrigin(), 16, 255, 255, 0, false, 0.025f );

		m_bOnWall = true;

		if( tr.m_pEnt != NULL && !tr.m_pEnt->IsWorld() )
		{
			IPhysicsObject *pPhysics = tr.m_pEnt->VPhysicsGetObject();

			if( pPhysics != NULL )
			{
				Vector vecMassCenter;
				Vector vecMassCenterWorld;

				vecMassCenter = pPhysics->GetMassCenterLocalSpace();
				pPhysics->LocalToWorld( &vecMassCenterWorld, vecMassCenter );

				if( tr.endpos.z > vecMassCenterWorld.z )
				{
					pPhysics->ApplyForceOffset( (-150.0f * m_flRandomEightyPercent) * tr.plane.normal, tr.endpos );
				}
			}
		}
	}

	Vector vecDir = tr.endpos - vecStart;
	VectorNormalize( vecDir );
	SetElementVelocity( vecDir * flSpeed, false );
}

//---------------------------------------------------------
// Set velocity that will carry me towards a specified entity
// Most often used to move along with the npc_blob that 
// is directing me.
//---------------------------------------------------------
void CBlobElement::MoveTowardsTargetEntity( float speed )
{
	CBaseEntity *pTarget = m_hTargetEntity.Get();

	if( pTarget != NULL )
	{
		// Try to attack my target's enemy directly if I can.
		CBaseEntity *pTargetEnemy = pTarget->GetEnemy();

		if( pTargetEnemy != NULL )
		{
			pTarget = pTargetEnemy;
		}

		Vector vecDir = pTarget->WorldSpaceCenter() - GetAbsOrigin();
		vecDir.NormalizeInPlace();
		SetElementVelocity( vecDir * speed, true );
	}
	else
	{
        SetElementVelocity( vec3_origin, true );
	}
}

//---------------------------------------------------------
// Set velocity that will take me towards a specified location.
// This is often used to send all blob elements to specific
// locations, causing the blob to appear as though it has
// formed a specific shape.
//---------------------------------------------------------
void CBlobElement::MoveTowardsTargetLocation( float speed )
{
	Vector vecDir = m_vecTargetLocation - GetAbsOrigin();
	float dist = VectorNormalize( vecDir );

	//!!!HACKHACK - how about a real way to tell if we've reached our goal?
	if( dist <= 8.0f )
	{
		SetActiveMovementRule( BLOB_MOVE_DONT_MOVE );
	}

	speed = MIN( dist, speed );

	SetElementVelocity( vecDir * speed, true );
}

//---------------------------------------------------------
// Pick new random numbers for the parameters that create
// variations in movement.
//---------------------------------------------------------
void CBlobElement::ReconfigureRandomParams()
{
	m_flSinePhase = random->RandomFloat( 0.01f, 0.9f );
	m_flSineFrequency = random->RandomFloat( 10.0f, 20.0f );
	m_flSineAmplitude = random->RandomFloat( 0.5f, 1.5f );
}

//---------------------------------------------------------
// Adjust velocity if this element is moving faster than 
// flMaxSpeed or slower than flMinSpeed
//---------------------------------------------------------
void CBlobElement::EnforceSpeedLimits( float flMinSpeed, float flMaxSpeed )
{
	Vector vecVelocity = GetAbsVelocity();
	float flSpeed = VectorNormalize( vecVelocity );

	if( flSpeed > flMaxSpeed )
	{
		SetElementVelocity( vecVelocity * flMaxSpeed, true );
	}
	else if( flSpeed < flMinSpeed )
	{
		SetElementVelocity( vecVelocity * flMinSpeed, true );
	}
}

//=========================================================
// Custom schedules
//=========================================================
enum
{
	SCHED_MYCUSTOMSCHEDULE = LAST_SHARED_SCHEDULE,
};

//=========================================================
// Custom tasks
//=========================================================
enum 
{
	TASK_MYCUSTOMTASK = LAST_SHARED_TASK,
};


//=========================================================
// Custom Conditions
//=========================================================
enum 
{
	COND_MYCUSTOMCONDITION = LAST_SHARED_CONDITION,
};


//=========================================================
//=========================================================
class CNPC_Blob : public CAI_BaseNPC
{
	DECLARE_CLASS( CNPC_Blob, CAI_BaseNPC );

public:
	CNPC_Blob();
	void	Precache( void );
	void	Spawn( void );
	Class_T Classify( void );
	void	RunAI();
	void	GatherConditions( void );
	int		SelectSchedule( void );
	int		GetSoundInterests( void ) { return (SOUND_BUGBAIT); }


	void	ComputeCentroid();

	void	DoBlobBatchedAI( int iStart, int iEnd );

	int		ComputeBatchSize();
	void	AdvanceBatch();
	int		GetBatchStart();
	int		GetBatchEnd();

	CBlobElement *CreateNewElement();
	void	InitializeElements();
	void	RecomputeIdealElementDist();

	void	RemoveAllElementsExcept( int iExempt );

	void	RemoveExcessElements( int iNumElements );
	void	AddNewElements( int iNumElements );

	void	FormShapeFromPath( string_t iszPathName );
	void	SetRadius( float flRadius );

	DECLARE_DATADESC();

	int		m_iNumElements;
	bool	m_bInitialized;
	int		m_iBatchStart;
	Vector	m_vecCentroid;
	float	m_flMinElementDist;

	CUtlVector<CHandle< CBlobElement > >m_Elements;

	DEFINE_CUSTOM_AI;

public:
	void InputFormPathShape( inputdata_t &inputdata );
	void InputSetRadius( inputdata_t &inputdata );
	void InputChaseEntity( inputdata_t &inputdata );
	void InputIsolateElement( inputdata_t &inputdata );
	void InputFormHemisphere( inputdata_t &inputdata );
	void InputFormTwoSpheres( inputdata_t &inputdata );

public:
	Vector	m_vecAvoidOrigin[ BLOB_MAX_AVOID_ORIGINS ];
	float	m_flAvoidRadiusSqr;

private:
	int		m_iReconfigureElement;
	int		m_iNumAvoidOrigins;

	bool	m_bEatCombineHack;
};

LINK_ENTITY_TO_CLASS( npc_blob, CNPC_Blob );
IMPLEMENT_CUSTOM_AI( npc_blob,CNPC_Blob );


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Blob )

DEFINE_FIELD( m_iNumElements, FIELD_INTEGER ),
DEFINE_FIELD( m_bInitialized, FIELD_BOOLEAN ),
DEFINE_FIELD( m_iBatchStart, FIELD_INTEGER ),
DEFINE_FIELD( m_vecCentroid, FIELD_POSITION_VECTOR ),
DEFINE_FIELD( m_flMinElementDist, FIELD_FLOAT ),
DEFINE_FIELD( m_iReconfigureElement, FIELD_INTEGER ),
DEFINE_UTLVECTOR( m_Elements, FIELD_EHANDLE ),

DEFINE_INPUTFUNC( FIELD_STRING, "FormPathShape", InputFormPathShape ),
DEFINE_INPUTFUNC( FIELD_FLOAT, "SetRadius", InputSetRadius ),
DEFINE_INPUTFUNC( FIELD_STRING, "ChaseEntity", InputChaseEntity ),
DEFINE_INPUTFUNC( FIELD_INTEGER, "IsolateElement", InputIsolateElement ),
DEFINE_INPUTFUNC( FIELD_VOID, "FormHemisphere", InputFormHemisphere ),
DEFINE_INPUTFUNC( FIELD_VOID, "FormTwoSpheres", InputFormTwoSpheres ),

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------
CNPC_Blob::CNPC_Blob()
{
	m_iNumElements = 0;
	m_bInitialized = false;
	m_iBatchStart = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Initialize the custom schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_Blob::InitCustomSchedules(void) 
{
	INIT_CUSTOM_AI(CNPC_Blob);

	ADD_CUSTOM_TASK(CNPC_Blob,		TASK_MYCUSTOMTASK);

	ADD_CUSTOM_SCHEDULE(CNPC_Blob,	SCHED_MYCUSTOMSCHEDULE);

	ADD_CUSTOM_CONDITION(CNPC_Blob,	COND_MYCUSTOMCONDITION);
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Blob::Precache( void )
{
	PrecacheModel( NPC_BLOB_MODEL );
	UTIL_PrecacheOther( "blob_element" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Blob::Spawn( void )
{
	Precache();

	SetModel( NPC_BLOB_MODEL );

	SetHullType(HULL_TINY);
	SetHullSizeNormal();

	SetSolid( SOLID_NONE );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	m_iHealth			= INT_MAX;
	m_flFieldOfView		= -1.0f;
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND );

	m_Elements.RemoveAll();

	NPCInit();

	AddEffects( EF_NODRAW );

	m_flMinElementDist = blob_mindist.GetFloat();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
// Output : 
//-----------------------------------------------------------------------------
Class_T	CNPC_Blob::Classify( void )
{
	return	CLASS_PLAYER_ALLY;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::RunAI()
{
	BaseClass::RunAI();

	if( !m_bInitialized )
	{
		// m_bInitialized is set to false in the constructor. So this bit of
		// code runs one time, the first time I think.
		Msg("I need to initialize\n");
		InitializeElements();
		m_bInitialized = true;
		return;
	}

	int iIdealNumElements = blob_numelements.GetInt();
	if( iIdealNumElements != m_iNumElements )
	{
		int delta = iIdealNumElements - m_iNumElements;

		if( delta < 0 )
		{
			delta = -delta;
			delta = MIN(delta, 5 );
			RemoveExcessElements( delta );
			
			if( m_iReconfigureElement > m_iNumElements )
			{
				// Start this index over at zero, if it is past the new end of the utlvector.
				m_iReconfigureElement = 0;
			}
		}
		else
		{
			delta = MIN(delta, 5 );
			AddNewElements( delta );
		}
	
		RecomputeIdealElementDist();
	}

	ComputeCentroid();

	if( npc_blob_show_centroid.GetBool() )
	{
		NDebugOverlay::Cross3D( m_vecCentroid + Vector( 0, 0, 12 ), 32, 0, 255, 0, false, 0.025f );
	}

	if( npc_blob_use_threading.GetBool() )
	{
		IterRangeParallel( this, &CNPC_Blob::DoBlobBatchedAI, 0, m_Elements.Count() );
	}
	else
	{
		DoBlobBatchedAI( 0, m_Elements.Count() );
	}

	if( GetEnemy() != NULL )
	{
		float flEnemyDistSqr = m_vecCentroid.DistToSqr( GetEnemy()->GetAbsOrigin() );

		if( flEnemyDistSqr <= Square( 32.0f ) )
		{
			if( GetEnemy()->Classify() == CLASS_COMBINE )
			{
				if( !m_bEatCombineHack )
				{
					variant_t var;

					var.SetFloat( 0 );
					g_EventQueue.AddEvent( GetEnemy(), "HitByBugBait", 0.0f, this, this );
					g_EventQueue.AddEvent( GetEnemy(), "SetHealth", var, 3.0f, this, this );
					m_bEatCombineHack = true;

					blob_radius.SetValue( 48.0f );
					RecomputeIdealElementDist();
				}
			}
			else
			{
				CTakeDamageInfo info;

				info.SetAttacker( this );
				info.SetInflictor( this );
				info.SetDamage( 5 );
				info.SetDamageType( DMG_SLASH );
				info.SetDamageForce( Vector( 0, 0, 1 ) );

				GetEnemy()->TakeDamage( info );
			}
		}
	}

	SetNextThink( gpGlobals->curtime + npc_blob_think_interval.GetFloat() );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::GatherConditions( void )
{
	if( m_bEatCombineHack )
	{
		// We just ate someone.
		if( !GetEnemy() || !GetEnemy()->IsAlive() )
		{
			m_bEatCombineHack = false;
			blob_radius.SetValue( 160.0f );
			RecomputeIdealElementDist();
		}
	}

	BaseClass::GatherConditions();

}

//-----------------------------------------------------------------------------
// Either stand still or chase the enemy, for now.
//-----------------------------------------------------------------------------
int CNPC_Blob::SelectSchedule( void )
{
	if( GetEnemy() == NULL )
		return SCHED_IDLE_STAND;

	return SCHED_CHASE_ENEMY;
}

//-----------------------------------------------------------------------------
// Average the origin of all elements to get the centroid for the group
//-----------------------------------------------------------------------------
void CNPC_Blob::ComputeCentroid()
{
	m_vecCentroid = vec3_origin;

	for( int i = 0 ; i < m_Elements.Count() ; i++ )
	{
		m_vecCentroid += m_Elements[ i ]->GetAbsOrigin();
	}

	m_vecCentroid /= m_Elements.Count();
}

//-----------------------------------------------------------------------------
// Run all of the AI for elements within the range iStart to iEnd 
//-----------------------------------------------------------------------------
void CNPC_Blob::DoBlobBatchedAI( int iStart, int iEnd )
{
	float flInterval = gpGlobals->curtime - GetLastThink();

	// Local fields for sin-wave movement variance
	float flMySine;
	float flAmplitude = npc_blob_sin_amplitude.GetFloat();
	float flMyAmplitude;
	Vector vecRight;
	Vector vecForward;

	// Local fields for attract/repel
	float minDistSqr = Square( m_flMinElementDist );
	float flBlobSpeed = blob_element_speed.GetFloat();
	float flSpeed;

	// Local fields for speed limiting
	float flMinSpeed = blob_element_speed.GetFloat() * 0.5f;
	float flMaxSpeed = blob_element_speed.GetFloat() * 1.5f;
	bool bEnforceSpeedLimit;
	bool bEnforceRelativePositions;
	bool bDoMovementVariation;
	bool bDoOrientation = npc_blob_use_orientation.GetBool();
	float flIdleSpeedFactor = npc_blob_idle_speed_factor.GetFloat();

	// Group cohesion
	float flBlobRadiusSqr = Square( blob_radius.GetFloat() + 48.0f ); // Four feet of fudge

	// Build a right-hand vector along which we'll add some sine wave data to give each
	// element a unique insect-like undulation along an axis perpendicular to their path,
	// which makes the entire group look far less orderly
	if( GetEnemy() != NULL )
	{
		// If I have an enemy, the right-hand vector is perpendicular to a straight line 
		// from the group's centroid to the enemy's origin.
		vecForward = GetEnemy()->GetAbsOrigin() - m_vecCentroid;
		VectorNormalize( vecForward );
		vecRight.x = vecForward.y;
		vecRight.y = -vecForward.x;
	}
	else
	{
		// If there is no enemy, wobble along the axis from the centroid to me.
		vecForward = GetAbsOrigin() - m_vecCentroid;
		VectorNormalize( vecForward );
		vecRight.x = vecForward.y;
		vecRight.y = -vecForward.x;
	}

	//--
	// MAIN LOOP - Run all of the elements in the set iStart to iEnd
	//--
	for( int i = iStart ; i < iEnd ; i++ )
	{
		CBlobElement *pThisElement = m_Elements[ i ];

		//--
		// Initial movement
		//--
		// Start out with bEnforceSpeedLimit set to false. This is because an element
		// can't overspeed if it's moving undisturbed towards its target entity or 
		// target location. An element can only under or overspeed when it is repelled 
		// by multiple other elements in the group. See "Relative Positions" below.
		//
		// Initialize some 'defaults' that may be changed for each iteration of this loop
		bEnforceSpeedLimit = false;
		bEnforceRelativePositions = true;
		bDoMovementVariation = true;
		flSpeed = flBlobSpeed;

		switch( pThisElement->GetActiveMovementRule() )
		{
		case BLOB_MOVE_DONT_MOVE:
			{
				pThisElement->SetElementVelocity( vec3_origin, true );

				trace_t tr;
				Vector vecOrigin = pThisElement->GetAbsOrigin();

				UTIL_TraceLine( vecOrigin, vecOrigin - Vector( 0, 0, 16), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

				if( tr.fraction < 1.0f )
				{
					QAngle angles;

					VectorAngles( tr.plane.normal, angles );

					float flSwap = angles.x;

					angles.x = -angles.y;
					angles.y = flSwap;

					pThisElement->SetAbsAngles( angles );
				}
			}
			continue;
			break;

		case BLOB_MOVE_TO_TARGET_LOCATION:
			{
				Vector vecDiff = pThisElement->GetAbsOrigin() - pThisElement->m_vecTargetLocation;

				if( vecDiff.Length2DSqr() <= Square(80.0f) )
				{
					// Don't shove this guy around any more, let him get to his goal position.
					flSpeed *= 0.5f;
					bEnforceRelativePositions = false;
					bDoMovementVariation = false;
				}

				pThisElement->MoveTowardsTargetLocation( flSpeed );
			}
			break;

		case BLOB_MOVE_TO_TARGET_ENTITY:
			{
				if( !IsMoving() && GetEnemy() == NULL )
				{
					if( pThisElement->GetAbsOrigin().DistToSqr( GetAbsOrigin() ) <= flBlobRadiusSqr )
					{
						flSpeed = (flSpeed * flIdleSpeedFactor) * pThisElement->m_flRandomEightyPercent;
					}
				}
				pThisElement->MoveTowardsTargetEntity( flSpeed );
			}
			break;

		default:
			Msg("ERROR: Blob Element with unspecified Movement Rule\n");
			break;
		}

		//---
		// Relative positions
		//--
		// Check this element against ALL other elements. If the two elements are closer
		// than the allowed minimum distance, repel this element away. (The other element
		// will repel when its AI runs). A single element can be repelled by many other 
		// elements. This is why bEnforceSpeedLimit is set to true if any of the repelling
		// code runs for this element. Multiple attempts to repel an element in the same
		// direction will cause overspeed. Conflicting attempts to repel an element in opposite
		// directions will cause underspeed.
		Vector vecDir = Vector( 0, 0, 0 );
		Vector vecThisElementOrigin = pThisElement->GetAbsOrigin();

		if( bEnforceRelativePositions )
		{
			for( int j = 0 ; j < m_Elements.Count() ; j++ )
			{
				// This is the innermost loop! We should optimize here, if anywhere.

				// If this element is on the wall, then don't be repelled by anyone. Repelling
				// elements that are trying to climb a wall usually make them look like they 
				// fall off the wall a few times while climbing.
				if( pThisElement->m_bOnWall )
					continue;

				CBlobElement *pThatElement = m_Elements[ j ];
				if( i != j )
				{
					Vector vecThatElementOrigin = pThatElement->GetAbsOrigin();
					float distSqr = vecThisElementOrigin.DistToSqr( vecThatElementOrigin );

					if( distSqr < minDistSqr )
					{
						// Too close to the other element. Move away.
						float flRepelSpeed;
						Vector vecRepelDir = ( vecThisElementOrigin - vecThatElementOrigin );

						vecRepelDir.NormalizeInPlace();
						flRepelSpeed = (flSpeed * ( 1.0f - ( distSqr / minDistSqr ) ) ) * pThatElement->GetSinePhase(); 
						pThisElement->AddElementVelocity( vecRepelDir * flRepelSpeed, true );

						// Since we altered this element's velocity after it was initially set, there's a chance
						// that the sums of multiple vectors will cause the element to over or underspeed, so 
						// mark it for speed limit enforcement
						bEnforceSpeedLimit = true;
					}
				}
			}
		}

		//--
		// Movement variation
		//--
		if( bDoMovementVariation )
		{
			flMySine = sin( gpGlobals->curtime * pThisElement->GetSineFrequency() );
			flMyAmplitude = flAmplitude * pThisElement->GetSineAmplitude();
			pThisElement->AddElementVelocity( vecRight * (flMySine * flMyAmplitude), true );
		}

		// Avoidance
		for( int a = 0 ; a < m_iNumAvoidOrigins ; a++ )
		{
			Vector vecAvoidDir = pThisElement->GetAbsOrigin() - m_vecAvoidOrigin[ a ];

			if( vecAvoidDir.LengthSqr() <= (m_flAvoidRadiusSqr * pThisElement->m_flRandomEightyPercent) )
			{
				VectorNormalize( vecAvoidDir );
				pThisElement->AddElementVelocity( vecAvoidDir * (flSpeed * 2.0f), true );
				break;
			}
		}

		//--
		// Speed limits
		//---
		if( bEnforceSpeedLimit == true )
		{
			pThisElement->EnforceSpeedLimits( flMinSpeed, flMaxSpeed );
		}

		//--
		// Wall crawling
		//--
		pThisElement->ModifyVelocityForSurface( flInterval, flSpeed );

		// For identifying stuck elements.
		pThisElement->m_vecPrevOrigin = pThisElement->GetAbsOrigin(); 

		pThisElement->m_flDistFromCentroidSqr = pThisElement->m_vecPrevOrigin.DistToSqr( m_vecCentroid );

		// Orientation
		if( bDoOrientation )
		{
			QAngle angles;
			VectorAngles( pThisElement->GetAbsVelocity(), angles );
			pThisElement->SetAbsAngles( angles );
		}

/*
		//--
		// Stragglers/Group integrity
		//
		if( pThisElement->m_flDistFromCentroidSqr > flStragglerDistSqr )
		{
			NDebugOverlay::Line( pThisElement->GetAbsOrigin(), m_vecCentroid, 255, 0, 0, false, 0.025f );
		}
*/
	}
}

//-----------------------------------------------------------------------------
// Throw out all elements and their entities except for the the specified 
// index into the UTILVector. This is useful for isolating elements that 
// get into a bad state.
//-----------------------------------------------------------------------------
void CNPC_Blob::RemoveAllElementsExcept( int iExempt )
{
	if( m_Elements.Count() == 1 )
		return;

	m_Elements[ 0 ].Set( m_Elements[ iExempt ].Get() );

	for( int i = 1 ; i < m_Elements.Count() ; i++ )
	{
		if( i != iExempt )
		{
			m_Elements[ i ]->SUB_Remove();
		}
	}

	m_Elements.RemoveMultiple( 1, m_Elements.Count() - 1 );

	m_iNumElements = 1;
}

//-----------------------------------------------------------------------------
// Purpose: The blob has too many elements. Locate good candidates and remove
// this many elements.
//-----------------------------------------------------------------------------
void CNPC_Blob::RemoveExcessElements( int iNumElements )
{
	// For now we're not assessing candidates, just blindly removing.
	int i;
	for( i = 0 ; i < iNumElements ; i++ )
	{
		int iLastElement = m_iNumElements - 1;
		
		// Nuke the associated entity
		m_Elements[ iLastElement ]->SUB_Remove();

		m_Elements.Remove( iLastElement );
		m_iNumElements--;
	}
}

//-----------------------------------------------------------------------------
// Purpose: This blob has too few elements. Add this many elements by stacking
// them on top of existing elements and allowing them to disperse themselves
// into the blob.
//-----------------------------------------------------------------------------
void CNPC_Blob::AddNewElements( int iNumElements )
{
	int i;
	
	// Keep track of how many elements we had when we came into this function.
	// Since the new elements copy their origins from existing elements, we only want
	// to copy origins from elements that existed before we came into this function. 
	// Otherwise, the more elements we create while in this function, the more likely it 
	// becomes that several of them will stack on the same origin.
	int iInitialElements = m_iNumElements;

	for( i = 0 ; i < iNumElements ; i++ )
	{
		CBlobElement *pElement = CreateNewElement();

		if( pElement != NULL )
		{
			// Copy the origin of some element that is not me. This will make the expansion
			// of the group easier on the eye, since this element will spawn inside of some
			// other element, and then be pushed out by the blob's repel rules.
			int iCopyElement = random->RandomInt( 0, iInitialElements - 1 );
			pElement->SetAbsOrigin( m_Elements[iCopyElement]->GetAbsOrigin() );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define BLOB_MAX_VERTS 128
void CNPC_Blob::FormShapeFromPath( string_t iszPathName )
{
	Vector vertex[ BLOB_MAX_VERTS ];

	int i;
	int iNumVerts = 0;

	for ( i = 0 ; i < BLOB_MAX_VERTS ; i++ )
	{
		if( iszPathName == NULL_STRING )
		{
			//Msg("Terminal path\n");
			break;
		}

		CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, iszPathName );

		if( pEntity != NULL )
		{
			bool bClosedPath = false;

			for( int j = 0 ; j < i ; j++ )
			{
				// Stop if we reach a vertex that's already in the array (closed path)
				if( vertex[ j ] == pEntity->GetAbsOrigin() )
				{
					//Msg("Closed path!\n");
					bClosedPath = true;
					break;
				}
			}

			vertex[ i ] = pEntity->GetAbsOrigin();
			iszPathName = pEntity->m_target;
			iNumVerts++;

			if( bClosedPath )
				break;
		}
	}

	//Msg("%d verts found in path!\n", iNumVerts);

	float flPathLength = 0.0f;
	float flDistribution;

	for( i = 0 ; i < iNumVerts - 1 ; i++ )
	{
		Vector vecDiff = vertex[ i ] - vertex[ i + 1 ];

		flPathLength += vecDiff.Length();
	}

	flDistribution = flPathLength / m_iNumElements;
	Msg("Path length is %f, distribution is %f\n", flPathLength, flDistribution );

	int element = 0;
	for( i = 0 ; i < iNumVerts - 1 ; i++ )
	{
		//NDebugOverlay::Line( vertex[ i ], vertex[ i + 1 ], 0, 255, 0, false, 10.0f );
		Vector vecDiff = vertex[ i + 1 ] - vertex[ i ];
		Vector vecStart = vertex[ i ];

		float flSegmentLength = VectorNormalize( vecDiff );

		float flStep;

		for( flStep = 0.0f ; flStep < flSegmentLength ; flStep += flDistribution )
		{
			//NDebugOverlay::Cross3D( vecStart + vecDiff * flStep, 16, 255, 255, 255, false, 10.0f );
			m_Elements[ element ]->SetTargetLocation( vecStart + vecDiff * flStep );
			m_Elements[ element ]->SetActiveMovementRule( BLOB_MOVE_TO_TARGET_LOCATION );
			element++;

			if( element == m_iNumElements )
				return;
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::SetRadius( float flRadius )
{
	blob_radius.SetValue( flRadius );
	RecomputeIdealElementDist();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::InputFormPathShape( inputdata_t &inputdata )
{
	string_t shape = inputdata.value.StringID();

	if( shape == NULL_STRING )
		return;

	//Msg("I'm supposed to form some shape called:%s\n", shape );

	FormShapeFromPath( shape );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::InputSetRadius( inputdata_t &inputdata )
{
	float flNewRadius = inputdata.value.Float();

	SetRadius( flNewRadius );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::InputChaseEntity( inputdata_t &inputdata )
{
	CBaseEntity *pEntity = gEntList.FindEntityByName( NULL, inputdata.value.StringID(), NULL, inputdata.pActivator, inputdata.pCaller );
	
	if ( pEntity )
	{
		for( int i = 0 ; i < m_Elements.Count() ; i++ )
		{
			CBlobElement *pElement = m_Elements[ i ];

			pElement->SetTargetEntity( pEntity );
			pElement->SetActiveMovementRule( BLOB_MOVE_TO_TARGET_ENTITY );
		}
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::InputIsolateElement( inputdata_t &inputdata )
{
	int iElement = inputdata.value.Int();

	RemoveAllElementsExcept( iElement );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::InputFormHemisphere( inputdata_t &inputdata )
{
	Vector center = GetAbsOrigin();
	const float flRadius = 240.0f;

	Vector vecDir;

	for( int i = 0 ; i < m_Elements.Count() ; i++ )
	{
		CBlobElement *pElement = m_Elements[ i ];

		// Compute a point around my center
		vecDir.x = random->RandomFloat( -1, 1 );
		vecDir.y = random->RandomFloat( -1, 1 );
		vecDir.z = random->RandomFloat( 0, 1 );

		VectorNormalize( vecDir );

		pElement->SetTargetLocation( center + vecDir * flRadius );
		pElement->SetActiveMovementRule( BLOB_MOVE_TO_TARGET_LOCATION );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::InputFormTwoSpheres( inputdata_t &inputdata )
{
	Vector center = GetAbsOrigin();
	Vector sphere1 = GetAbsOrigin() + Vector( 120.0f, 0, 120.0f );
	Vector sphere2 = GetAbsOrigin() + Vector( -120.0f, 0, 120.0f );
	const float flRadius = 100.0f;

	Vector vecDir;

	int batchSize = m_Elements.Count() / 2;

	for( int i = 0 ; i < batchSize ; i++ )
	{
		CBlobElement *pElement = m_Elements[ i ];

		// Compute a point around my center
		vecDir.x = random->RandomFloat( -1, 1 );
		vecDir.y = random->RandomFloat( -1, 1 );
		vecDir.z = random->RandomFloat( -1, 1 );

		VectorNormalize( vecDir );

		pElement->SetTargetLocation( sphere1 + vecDir * flRadius );
		pElement->SetActiveMovementRule( BLOB_MOVE_TO_TARGET_LOCATION );
	}

	for( int i = batchSize ; i < m_Elements.Count() ; i++ )
	{
		CBlobElement *pElement = m_Elements[ i ];

		// Compute a point around my center
		vecDir.x = random->RandomFloat( -1, 1 );
		vecDir.y = random->RandomFloat( -1, 1 );
		vecDir.z = random->RandomFloat( -1, 1 );

		VectorNormalize( vecDir );

		pElement->SetTargetLocation( sphere2 + vecDir * flRadius );
		pElement->SetActiveMovementRule( BLOB_MOVE_TO_TARGET_LOCATION );
	}

}

//-----------------------------------------------------------------------------
// Get the index of the element to start processing with for this batch.
//-----------------------------------------------------------------------------
int CNPC_Blob::GetBatchStart()
{
	return m_iBatchStart;
}

//-----------------------------------------------------------------------------
// Get the index of the element to stop processing with for this batch.
//-----------------------------------------------------------------------------
int CNPC_Blob::GetBatchEnd()
{
	int batchDone = m_iBatchStart + ComputeBatchSize();
	batchDone = MIN( batchDone, m_Elements.Count() );

	return batchDone;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int CNPC_Blob::ComputeBatchSize()
{
	int batchSize = m_Elements.Count() / ( 100 / blob_batchpercent.GetInt() );
	return batchSize;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

void CNPC_Blob::AdvanceBatch()
{
	m_iBatchStart += ComputeBatchSize();

	if( m_iBatchStart >= m_Elements.Count() )
		m_iBatchStart = 0;
}

//-----------------------------------------------------------------------------
// Creates a new blob element from scratch and adds it to the blob
//-----------------------------------------------------------------------------
CBlobElement *CNPC_Blob::CreateNewElement()
{
	CBlobElement *pElement = static_cast<CBlobElement*>(CreateEntityByName( "blob_element" ));

	if( pElement != NULL )
	{
		pElement->SetOwnerEntity( this );
		pElement->SetSinePhase( fabs( sin(((float)m_iNumElements)/10.0f) ) );
		pElement->SetActiveMovementRule( BLOB_MOVE_TO_TARGET_ENTITY );
		pElement->SetTargetEntity( this );

		pElement->m_iElementNumber = m_iNumElements;
		m_iNumElements++;
		pElement->Spawn();
		m_Elements.AddToTail( pElement );
		return pElement;
	}

	Warning("Blob could not spawn new element!\n");
	return NULL;
}

//-----------------------------------------------------------------------------
// Create, initialize, and distribute all blob elements
//-----------------------------------------------------------------------------
void CNPC_Blob::InitializeElements()
{
	// Squirt all of the elements out into a circle
	int i;
	QAngle angDistributor( 0, 0, 0 );

	int iNumElements = blob_numelements.GetInt();

	float step = 360.0f / ((float)iNumElements);
	for( i = 0 ; i < iNumElements ; i++ )
	{
		Vector vecDir;
		Vector vecDest;
		AngleVectors( angDistributor, &vecDir, NULL, NULL );
		vecDest = WorldSpaceCenter() + vecDir * 64.0f;

		CBlobElement *pElement = CreateNewElement();

		if( !pElement )
		{
			Msg("Blob could not create all elements!!\n");
			return;
		}

		trace_t tr;
		UTIL_TraceLine( vecDest, vecDest + Vector (0, 0, MIN_COORD_FLOAT), MASK_SHOT, pElement, COLLISION_GROUP_NONE, &tr );

		pElement->SetAbsOrigin( tr.endpos + Vector( 0, 0, 1 ) );

		angDistributor.y += step;
	}

	CBaseEntity *pEntity = gEntList.FindEntityByClassname( NULL, "info_target" );
	for( i = 0 ; i < BLOB_MAX_AVOID_ORIGINS ; i++ )
	{
		if( pEntity )
		{
			if( pEntity->NameMatches("avoid") )
			{
				m_vecAvoidOrigin[ i ] = pEntity->GetAbsOrigin();
				m_flAvoidRadiusSqr = Square( 120.0f );
				m_iNumAvoidOrigins++;
			}

			pEntity = gEntList.FindEntityByClassname( pEntity, "info_target" );
		}
		else
		{
			break;
		}
	}

	Msg("%d avoid origins\n", m_iNumAvoidOrigins );

	RecomputeIdealElementDist();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CNPC_Blob::RecomputeIdealElementDist()
{
	float radius = blob_radius.GetFloat();
	float area = M_PI * Square(radius);

	//Msg("Area of blob is: %f\n", area );

	//m_flMinElementDist =  2.75f * sqrt( area / m_iNumElements );
	m_flMinElementDist =  M_PI * sqrt( area / m_iNumElements );

	//Msg("New element dist: %f\n", m_flMinElementDist );
}


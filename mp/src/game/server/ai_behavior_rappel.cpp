//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ai_motor.h"
#include "ai_behavior_rappel.h"
#include "beam_shared.h"
#include "rope.h"
#include "eventqueue.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

BEGIN_DATADESC( CAI_RappelBehavior )
	DEFINE_FIELD( m_bWaitingToRappel, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bOnGround, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_hLine, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecRopeAnchor, FIELD_POSITION_VECTOR ),
END_DATADESC();

//=========================================================
//=========================================================
class CRopeAnchor : public CPointEntity
{
	DECLARE_CLASS( CRopeAnchor, CPointEntity );

public:
	void Spawn( void );
	void FallThink( void );
	void RemoveThink( void );
	EHANDLE m_hRope;

	DECLARE_DATADESC();
};

BEGIN_DATADESC( CRopeAnchor )
	DEFINE_FIELD( m_hRope, FIELD_EHANDLE ),

	DEFINE_THINKFUNC( FallThink ),
	DEFINE_THINKFUNC( RemoveThink ),
END_DATADESC();

LINK_ENTITY_TO_CLASS( rope_anchor, CRopeAnchor );

//---------------------------------------------------------
//---------------------------------------------------------
#define RAPPEL_ROPE_WIDTH 1
void CRopeAnchor::Spawn()
{
	BaseClass::Spawn();

	// Decent enough default in case something happens to our owner!
	float flDist = 384;

	if( GetOwnerEntity() )
	{
		flDist = fabs( GetOwnerEntity()->GetAbsOrigin().z - GetAbsOrigin().z );
	}

	m_hRope = CRopeKeyframe::CreateWithSecondPointDetached( this, -1, flDist, RAPPEL_ROPE_WIDTH, "cable/cable.vmt", 5, true );

	ASSERT( m_hRope != NULL );

	SetThink( &CRopeAnchor::FallThink );
	SetNextThink( gpGlobals->curtime + 0.2 );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRopeAnchor::FallThink()
{
	SetMoveType( MOVETYPE_FLYGRAVITY );

	Vector vecVelocity = GetAbsVelocity();

	vecVelocity.x = random->RandomFloat( -30.0f, 30.0f );
	vecVelocity.y = random->RandomFloat( -30.0f, 30.0f );

	SetAbsVelocity( vecVelocity );

	SetThink( &CRopeAnchor::RemoveThink );
	SetNextThink( gpGlobals->curtime + 3.0 );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CRopeAnchor::RemoveThink()
{
	UTIL_Remove( m_hRope );	
	SetThink( &CRopeAnchor::SUB_Remove );
	SetNextThink( gpGlobals->curtime );
}

//=========================================================
//=========================================================



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAI_RappelBehavior::CAI_RappelBehavior()
{
	m_hLine = NULL;
	m_bWaitingToRappel = false;
	m_bOnGround = true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool CAI_RappelBehavior::KeyValue( const char *szKeyName, const char *szValue )
{
	if( FStrEq( szKeyName, "waitingtorappel" ) )
	{
		m_bWaitingToRappel = ( atoi(szValue) != 0);
		m_bOnGround = !m_bWaitingToRappel;
		return true;
	}

	return BaseClass::KeyValue( szKeyName, szValue );
}

void CAI_RappelBehavior::Precache()
{
	CBaseEntity::PrecacheModel( "cable/cable.vmt" );
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#define RAPPEL_MAX_SPEED	600	// Go this fast if you're really high.
#define RAPPEL_MIN_SPEED	60 // Go no slower than this.
#define RAPPEL_DECEL_DIST	(20.0f * 12.0f)	// Start slowing down when you're this close to the ground.
void CAI_RappelBehavior::SetDescentSpeed()
{
	// Trace to the floor and see how close we're getting. Slow down if we're close.
	// STOP if there's an NPC under us.
	trace_t tr;
	AI_TraceLine( GetOuter()->GetAbsOrigin(), GetOuter()->GetAbsOrigin() - Vector( 0, 0, 8192 ), MASK_SHOT, GetOuter(), COLLISION_GROUP_NONE, &tr );

	float flDist = fabs( GetOuter()->GetAbsOrigin().z - tr.endpos.z );

	float speed = RAPPEL_MAX_SPEED;

	if( flDist <= RAPPEL_DECEL_DIST )
	{
		float factor;
		factor = flDist / RAPPEL_DECEL_DIST;

		speed = MAX( RAPPEL_MIN_SPEED, speed * factor );
	}

	Vector vecNewVelocity = vec3_origin;
	vecNewVelocity.z = -speed;
	GetOuter()->SetAbsVelocity( vecNewVelocity );
}


void CAI_RappelBehavior::CleanupOnDeath( CBaseEntity *pCulprit, bool bFireDeathOutput )
{
	BaseClass::CleanupOnDeath( pCulprit, bFireDeathOutput );

	//This will remove the beam and create a rope if the NPC dies while rappeling down.
	if ( m_hLine )
	{
		 CAI_BaseNPC *pNPC = GetOuter();

		 if ( pNPC )
		 {
			 CutZipline();
		 }
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_RappelBehavior::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_MOVE_AWAY_PATH:
		GetOuter()->GetMotor()->SetIdealYaw( UTIL_AngleMod( GetOuter()->GetLocalAngles().y - 180.0f ) );
		BaseClass::StartTask( pTask );
		break;

	case TASK_RANGE_ATTACK1:
		BaseClass::StartTask( pTask );
		break;

	case TASK_RAPPEL:
		{
			CreateZipline();
			SetDescentSpeed();
		}
		break;

	case TASK_HIT_GROUND:
		m_bOnGround = true;

		if( GetOuter()->GetGroundEntity() != NULL && GetOuter()->GetGroundEntity()->IsNPC() && GetOuter()->GetGroundEntity()->m_iClassname == GetOuter()->m_iClassname )
		{
			// Although I tried to get NPC's out from under me, I landed on one. Kill it, so long as it's the same type of character as me.
			variant_t val;
			val.SetFloat( 0 );
			g_EventQueue.AddEvent( GetOuter()->GetGroundEntity(), "sethealth", val, 0, GetOuter(), GetOuter() );
		}

		TaskComplete();
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CAI_RappelBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_RAPPEL:
		{
			// If we don't do this, the beam won't show up sometimes. Ideally, all beams would update their
			// bboxes correctly, but we're close to shipping and we can't change that now.
			if ( m_hLine )
			{
				m_hLine->RelinkBeam();
			}

			if( GetEnemy() )
			{
				// Face the enemy if there's one.
				Vector vecEnemyLKP = GetEnemyLKP();
				GetOuter()->GetMotor()->SetIdealYawToTargetAndUpdate( vecEnemyLKP );
			}

			SetDescentSpeed();
			if( GetOuter()->GetFlags() & FL_ONGROUND )
			{
				CBaseEntity *pGroundEnt = GetOuter()->GetGroundEntity();

				if( pGroundEnt && pGroundEnt->IsPlayer() )
				{
					// try to shove the player in the opposite direction as they are facing (so they'll see me)
					Vector vecForward;
					pGroundEnt->GetVectors( &vecForward, NULL, NULL );
					pGroundEnt->SetAbsVelocity( vecForward * -500 );
					break;
				}

				GetOuter()->m_OnRappelTouchdown.FireOutput( GetOuter(), GetOuter(), 0 );
				GetOuter()->RemoveFlag( FL_FLY );
				
				CutZipline();

				TaskComplete();
			}
		}
		break;

	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_RappelBehavior::CanSelectSchedule()
{
	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( m_bWaitingToRappel )
		return true;

	if ( m_bOnGround )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_RappelBehavior::GatherConditions()
{
	BaseClass::GatherConditions();

	if( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
	{
		// Shoot at the enemy so long as I'm six feet or more above them.
		if( (GetAbsOrigin().z - GetEnemy()->GetAbsOrigin().z >= 36.0f) && GetOuter()->GetShotRegulator()->ShouldShoot() )
		{
			Activity activity = GetOuter()->TranslateActivity( ACT_GESTURE_RANGE_ATTACK1 );
			Assert( activity != ACT_INVALID );
			GetOuter()->AddGesture( activity );
			// FIXME: this seems a bit wacked
			GetOuter()->Weapon_SetActivity( GetOuter()->Weapon_TranslateActivity( ACT_RANGE_ATTACK1 ), 0 );

			GetOuter()->OnRangeAttack1();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAI_RappelBehavior::SelectSchedule()
{
	if ( HasCondition( COND_BEGIN_RAPPEL ) )
	{
		m_bWaitingToRappel = false;
		return SCHED_RAPPEL;
	}

	if ( m_bWaitingToRappel )
	{
		return SCHED_RAPPEL_WAIT;
	}
	else
	{
		return SCHED_RAPPEL;
	}
	
	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_RappelBehavior::BeginRappel()
{
	// Send the message to begin rappeling!
	SetCondition( COND_BEGIN_RAPPEL );

	m_vecRopeAnchor = GetOuter()->GetAbsOrigin();

	trace_t tr;

	UTIL_TraceEntity( GetOuter(), GetAbsOrigin(), GetAbsOrigin()-Vector(0,0,4096), MASK_SHOT, GetOuter(), COLLISION_GROUP_NONE, &tr );

	if( tr.m_pEnt != NULL && tr.m_pEnt->IsNPC() )
	{
		Vector forward;
		GetOuter()->GetVectors( &forward, NULL, NULL );

		CSoundEnt::InsertSound( SOUND_DANGER, tr.m_pEnt->EarPosition() - forward * 12.0f, 32.0f, 0.2f, GetOuter() );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_RappelBehavior::CutZipline()
{
	if( m_hLine )
	{
		UTIL_Remove( m_hLine );
	}

	CBaseEntity *pAnchor = CreateEntityByName( "rope_anchor" );
	pAnchor->SetOwnerEntity( GetOuter() ); // Boy, this is a hack!!
	pAnchor->SetAbsOrigin( m_vecRopeAnchor );
	pAnchor->Spawn();
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAI_RappelBehavior::CreateZipline()
{
#if 1
	if( !m_hLine )
	{
		int attachment = GetOuter()->LookupAttachment( "zipline" );

		if( attachment > 0 )
		{
			CBeam *pBeam;
			pBeam = CBeam::BeamCreate( "cable/cable.vmt", 1 );
			pBeam->SetColor( 150, 150, 150 );
			pBeam->SetWidth( 0.3 );
			pBeam->SetEndWidth( 0.3 );

			CAI_BaseNPC *pNPC = GetOuter();
			pBeam->PointEntInit( pNPC->GetAbsOrigin() + Vector( 0, 0, 80 ), pNPC );

			pBeam->SetEndAttachment( attachment );

			m_hLine.Set( pBeam );
		}
	}
#endif
}


AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( CAI_RappelBehavior )

	DECLARE_TASK( TASK_RAPPEL )
	DECLARE_TASK( TASK_HIT_GROUND )

	DECLARE_CONDITION( COND_BEGIN_RAPPEL )

	//===============================================
	//===============================================
	DEFINE_SCHEDULE
	(
		SCHED_RAPPEL_WAIT,

		"	Tasks"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_RAPPEL_LOOP"
		"		TASK_WAIT_INDEFINITE			0"
		""
		"	Interrupts"
		"		COND_BEGIN_RAPPEL"
	);

	//===============================================
	//===============================================
	DEFINE_SCHEDULE
	(
		SCHED_RAPPEL,

		"	Tasks"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_RAPPEL_LOOP"
		"		TASK_RAPPEL				0"
		"		TASK_SET_SCHEDULE		SCHEDULE:SCHED_CLEAR_RAPPEL_POINT"
		""
		"	Interrupts"
		""
		"		COND_NEW_ENEMY"	// Only so the enemy selection code will pick an enemy!
	);

	//===============================================
	//===============================================
	DEFINE_SCHEDULE
	(
		SCHED_CLEAR_RAPPEL_POINT,

		"	Tasks"
		"		TASK_HIT_GROUND			0"
		"		TASK_MOVE_AWAY_PATH		128"	// Clear this spot for other rappellers
		"		TASK_RUN_PATH			0"
		"		TASK_WAIT_FOR_MOVEMENT	0"
		""
		"	Interrupts"
		""
	);

AI_END_CUSTOM_SCHEDULE_PROVIDER()

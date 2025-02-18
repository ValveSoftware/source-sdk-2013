//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "cbase.h"
#include "npc_basescanner.h"
#include "baseprojectile.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_wpnscanner_health( "sk_wpnscanner_health","30");
ConVar	sk_wpnscanner_proj_dmg( "sk_wpnscanner_proj_dmg","5");
ConVar	sk_wpnscanner_proj_speed( "sk_wpnscanner_proj_speed","1300");

//-----------------------------------------------------------------------------
// Purpose: Scanner with a gun
//-----------------------------------------------------------------------------
class CNPC_WpnScanner : public CNPC_BaseScanner
{
	DECLARE_CLASS( CNPC_WpnScanner, CNPC_BaseScanner );
public: 
	void		Spawn( void );
	void		Precache( void );

	int			TranslateSchedule( int scheduleType );
	void		StartTask( const Task_t *pTask );
	void		RunTask( const Task_t *pTask );

	float		MinGroundDist( void );
	void		MoveToAttack(float flInterval);

private:
	string_t	m_iszProjectileModel;
	int			m_iMuzzleAttachment;

private:
	DEFINE_CUSTOM_AI;

	/*
	// Custom interrupt conditions
	enum
	{
		COND_CSCANNER_HAVE_INSPECT_TARGET = BaseClass::NEXT_CONDITION,

		NEXT_CONDITION,
	};
	*/

	// Custom schedules
	enum
	{
		SCHED_WPNSCANNER_ATTACK = BaseClass::NEXT_SCHEDULE,

		NEXT_SCHEDULE,
	};

	// Custom tasks
	enum
	{
		TASK_WPNSCANNER_ATTACK = BaseClass::NEXT_TASK,

		NEXT_TASK,
	};
};

LINK_ENTITY_TO_CLASS( npc_wpnscanner, CNPC_WpnScanner );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_WpnScanner::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	SetHealth( sk_wpnscanner_health.GetFloat() );
	SetMaxHealth( GetHealth() );

	CapabilitiesAdd( bits_CAP_INNATE_MELEE_ATTACK1 );

	m_iMuzzleAttachment = LookupAttachment( "light" );

	BaseClass::Spawn();

	m_flAttackNearDist = 250;
	m_flAttackFarDist = 700;
	m_flAttackRange = 750;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CNPC_WpnScanner::Precache()
{
	if( !GetModelName() )
	{
		SetModelName( MAKE_STRING( "models/combine_scanner.mdl" ) );
	}

	PrecacheModel( STRING( GetModelName() ) );

	m_iszProjectileModel = MAKE_STRING( "models/crossbow_bolt.mdl" );
	PrecacheModel( STRING(m_iszProjectileModel) );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : Type - 
//-----------------------------------------------------------------------------
int CNPC_WpnScanner::TranslateSchedule( int scheduleType ) 
{
	switch ( scheduleType )
	{
	case SCHED_IDLE_STAND:
		return SCHED_SCANNER_PATROL;

	case SCHED_SCANNER_ATTACK:
		return SCHED_WPNSCANNER_ATTACK;
	}
	return BaseClass::TranslateSchedule(scheduleType);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_WpnScanner::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_WPNSCANNER_ATTACK:
		{
			// Windup
			break;
		}

	default:
		{
			BaseClass::StartTask(pTask);
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTask - 
//-----------------------------------------------------------------------------
void CNPC_WpnScanner::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_WPNSCANNER_ATTACK:
		{
			CBaseEntity *pEnemy = GetEnemy();
			if ( !pEnemy )
			{
				TaskFail( FAIL_NO_ENEMY );
				return;
			}

			if ( m_flNextAttack > gpGlobals->curtime )
				return;
			m_flNextAttack = gpGlobals->curtime + 0.2;
			
			Vector vecFirePos;
			QAngle vecAngles;
			GetAttachment( m_iMuzzleAttachment, vecFirePos, vecAngles );
			Vector vecTarget = GetEnemy()->BodyTarget( vecFirePos );
			Vector vecToTarget = (vecTarget - vecFirePos);
			VectorNormalize( vecToTarget );

			VectorAngles( vecToTarget, vecAngles );
			Vector vecRight;
			Vector vecUp;
			AngleVectors( vecAngles, &vecToTarget, &vecRight, &vecUp );

			// Add some inaccuracy
			float x, y, z;
			do {
				x = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
				y = random->RandomFloat(-0.5,0.5) + random->RandomFloat(-0.5,0.5);
				z = x*x+y*y;
			} while (z > 1);

			vecToTarget = vecToTarget + x * VECTOR_CONE_20DEGREES * vecRight + y * VECTOR_CONE_20DEGREES * vecUp;
			vecToTarget *= sk_wpnscanner_proj_speed.GetFloat();

			baseprojectilecreate_t newProj;
			newProj.vecOrigin = vecFirePos;
			newProj.vecVelocity = vecToTarget;
			newProj.pOwner = this;
			newProj.iszModel = m_iszProjectileModel;
			newProj.flDamage = sk_wpnscanner_proj_dmg.GetFloat();
			newProj.iDamageType = DMG_ENERGYBEAM;
			newProj.flDamageScale = 1.0;
			CBaseProjectile::Create( newProj );

			break;
		}

	default:
		{
			BaseClass::RunTask(pTask);
		}
	}
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
float CNPC_WpnScanner::MinGroundDist( void )
{
	if ( m_nFlyMode == SCANNER_FLY_ATTACK  )
		return 16;

	return BaseClass::MinGroundDist();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flInterval - 
//-----------------------------------------------------------------------------
void CNPC_WpnScanner::MoveToAttack(float flInterval)
{
	if (GetEnemy() == NULL)
		return;

	if ( flInterval <= 0 )
		return;

	Vector vTargetPos = GetEnemyLKP();
	Vector idealPos = IdealGoalForMovement( vTargetPos, GetAbsOrigin(), GetGoalDistance(), 128 );

	NDebugOverlay::Box( idealPos, -Vector(4,4,4), Vector(4,4,4), 0,255,0,8, 0.1 );

	MoveToTarget( flInterval, idealPos );

	//FIXME: Re-implement?

	/*
	// ---------------------------------------------------------
	//  Add evasion if I have taken damage recently
	// ---------------------------------------------------------
	if ((m_flLastDamageTime + SCANNER_EVADE_TIME) > gpGlobals->curtime)
	{
	vFlyDirection = vFlyDirection + VelocityToEvade(GetEnemyCombatCharacterPointer());
	}
	*/
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_wpnscanner, CNPC_WpnScanner )

	DECLARE_TASK( TASK_WPNSCANNER_ATTACK )

	//DECLARE_CONDITION(COND_CSCANNER_HAVE_INSPECT_TARGET)

	//=========================================================
	// > SCHED_WPNSCANNER_ATTACK
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_WPNSCANNER_ATTACK,

		"	Tasks"
		"		TASK_SCANNER_SET_FLY_ATTACK			0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_WPNSCANNER_ATTACK				0"
		"		TASK_WAIT_RANDOM					0.5"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_OCCLUDED"
		"		COND_TOO_FAR_TO_ATTACK"
		"		COND_NOT_FACING_ATTACK"
		"		COND_SCANNER_GRABBED_BY_PHYSCANNON"
	)

AI_END_CUSTOM_NPC()

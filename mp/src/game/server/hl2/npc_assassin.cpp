//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "ammodef.h"
#include "AI_Hint.h"
#include "AI_Navigator.h"
#include "npc_Assassin.h"
#include "game.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "ai_squad.h"
#include "AI_SquadSlot.h"
#include "ai_moveprobe.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	sk_assassin_health( "sk_assassin_health","150");
ConVar	g_debug_assassin( "g_debug_assassin", "0" );

//=========================================================
// Anim Events	
//=========================================================
#define	ASSASSIN_AE_FIRE_PISTOL_RIGHT	1
#define	ASSASSIN_AE_FIRE_PISTOL_LEFT	2
#define	ASSASSIN_AE_KICK_HIT			3

int AE_ASSASIN_FIRE_PISTOL_RIGHT;
int AE_ASSASIN_FIRE_PISTOL_LEFT;
int AE_ASSASIN_KICK_HIT;

//=========================================================
// Assassin activities
//=========================================================
int ACT_ASSASSIN_FLIP_LEFT;
int ACT_ASSASSIN_FLIP_RIGHT;
int ACT_ASSASSIN_FLIP_BACK;
int ACT_ASSASSIN_FLIP_FORWARD;
int ACT_ASSASSIN_PERCH;

//=========================================================
// Flip types
//=========================================================
enum 
{
	FLIP_LEFT,
	FLIP_RIGHT,
	FLIP_FORWARD,
	FLIP_BACKWARD,
	NUM_FLIP_TYPES,
};

//=========================================================
// Private conditions
//=========================================================
enum Assassin_Conds
{
	COND_ASSASSIN_ENEMY_TARGETTING_ME = LAST_SHARED_CONDITION,
};

//=========================================================
// Assassin schedules
//=========================================================
enum
{
	SCHED_ASSASSIN_FIND_VANTAGE_POINT = LAST_SHARED_SCHEDULE,
	SCHED_ASSASSIN_EVADE,
	SCHED_ASSASSIN_STALK_ENEMY,
	SCHED_ASSASSIN_LUNGE,
};

//=========================================================
// Assassin tasks
//=========================================================
enum 
{
	TASK_ASSASSIN_GET_PATH_TO_VANTAGE_POINT = LAST_SHARED_TASK,
	TASK_ASSASSIN_EVADE,
	TASK_ASSASSIN_SET_EYE_STATE,
	TASK_ASSASSIN_LUNGE,
};


//-----------------------------------------------------------------------------
// Purpose: Class Constructor
//-----------------------------------------------------------------------------
CNPC_Assassin::CNPC_Assassin( void )
{
}

//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CLASS( npc_assassin, CNPC_Assassin );

#if 0
//---------------------------------------------------------
// Custom Client entity
//---------------------------------------------------------
IMPLEMENT_SERVERCLASS_ST(CNPC_Assassin, DT_NPC_Assassin)
END_SEND_TABLE()

#endif

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CNPC_Assassin )
	DEFINE_FIELD( m_nNumFlips,	FIELD_INTEGER ),
	DEFINE_FIELD( m_nLastFlipType, FIELD_INTEGER ),
	DEFINE_FIELD( m_flNextFlipTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextLungeTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextShotTime, FIELD_TIME ),
	DEFINE_FIELD( m_bEvade,		FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bAggressive, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bBlinkState, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_pEyeSprite,	FIELD_CLASSPTR ),
	DEFINE_FIELD( m_pEyeTrail,	FIELD_CLASSPTR ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Assassin::Precache( void )
{
	PrecacheModel( "models/fassassin.mdl" );

	PrecacheScriptSound( "NPC_Assassin.ShootPistol" );
	PrecacheScriptSound( "Zombie.AttackHit" );
	PrecacheScriptSound( "Assassin.AttackMiss" );
	PrecacheScriptSound( "NPC_Assassin.Footstep" );

	PrecacheModel( "sprites/redglow1.vmt" );

	BaseClass::Precache();
}


//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
void CNPC_Assassin::Spawn( void )
{
	Precache();

	SetModel( "models/fassassin.mdl" );

	SetHullType(HULL_HUMAN);
	SetHullSizeNormal();

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );
	SetBloodColor( BLOOD_COLOR_RED );
	
	m_iHealth			= sk_assassin_health.GetFloat();
	m_flFieldOfView		= 0.1;
	m_NPCState			= NPC_STATE_NONE;

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_CLIMB | bits_CAP_MOVE_GROUND | bits_CAP_MOVE_JUMP );
	CapabilitiesAdd( bits_CAP_SQUAD | bits_CAP_USE_WEAPONS | bits_CAP_AIM_GUN | bits_CAP_INNATE_RANGE_ATTACK1 | bits_CAP_INNATE_RANGE_ATTACK2 | bits_CAP_INNATE_MELEE_ATTACK1 );

	//Turn on our guns
	SetBodygroup( 1, 1 );

	int attachment = LookupAttachment( "Eye" );

	// Start up the eye glow
	m_pEyeSprite = CSprite::SpriteCreate( "sprites/redglow1.vmt", GetLocalOrigin(), false );

	if ( m_pEyeSprite != NULL )
	{
		m_pEyeSprite->SetAttachment( this, attachment );
		m_pEyeSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 200, kRenderFxNone );
		m_pEyeSprite->SetScale( 0.25f );
	}

	// Start up the eye trail
	m_pEyeTrail	= CSpriteTrail::SpriteTrailCreate( "sprites/bluelaser1.vmt", GetLocalOrigin(), false );

	if ( m_pEyeTrail != NULL )
	{
		m_pEyeTrail->SetAttachment( this, attachment );
		m_pEyeTrail->SetTransparency( kRenderTransAdd, 255, 0, 0, 200, kRenderFxNone );
		m_pEyeTrail->SetStartWidth( 8.0f );
		m_pEyeTrail->SetLifeTime( 0.75f );
	}

	NPCInit();

	m_bEvade = false;
	m_bAggressive = false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if a reasonable jumping distance
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CNPC_Assassin::IsJumpLegal(const Vector &startPos, const Vector &apex, const Vector &endPos) const
{
	const float MAX_JUMP_RISE		= 256.0f;
	const float MAX_JUMP_DISTANCE	= 256.0f;
	const float MAX_JUMP_DROP		= 512.0f;

	return BaseClass::IsJumpLegal( startPos, apex, endPos, MAX_JUMP_RISE, MAX_JUMP_DROP, MAX_JUMP_DISTANCE );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int CNPC_Assassin::MeleeAttack1Conditions
//-----------------------------------------------------------------------------
int CNPC_Assassin::MeleeAttack1Conditions ( float flDot, float flDist )
{
	if ( flDist > 84 )
		return COND_TOO_FAR_TO_ATTACK;
	
	if ( flDot < 0.7f )
		return 0;

	if ( GetEnemy() == NULL )
		return 0;

	return COND_CAN_MELEE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int CNPC_Assassin::RangeAttack1Conditions
//-----------------------------------------------------------------------------
int CNPC_Assassin::RangeAttack1Conditions ( float flDot, float flDist )
{
	if ( flDist < 84 )
		return COND_TOO_CLOSE_TO_ATTACK;

	if ( flDist > 1024 )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDot < 0.5f )
		return COND_NOT_FACING_ATTACK;

	return COND_CAN_RANGE_ATTACK1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : flDot - 
//			flDist - 
// Output : int CNPC_Assassin::RangeAttack1Conditions
//-----------------------------------------------------------------------------
int CNPC_Assassin::RangeAttack2Conditions ( float flDot, float flDist )
{
	if ( m_flNextLungeTime > gpGlobals->curtime )
		return 0;

	float lungeRange = GetSequenceMoveDist( SelectWeightedSequence( (Activity) ACT_ASSASSIN_FLIP_FORWARD ) );

	if ( flDist < lungeRange * 0.25f )
		return COND_TOO_CLOSE_TO_ATTACK;

	if ( flDist > lungeRange * 1.5f )
		return COND_TOO_FAR_TO_ATTACK;

	if ( flDot < 0.75f )
		return COND_NOT_FACING_ATTACK;

	if ( GetEnemy() == NULL )
		return 0;

	// Check for a clear path
	trace_t	tr;
	UTIL_TraceHull( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );
	
	if ( tr.fraction == 1.0f || tr.m_pEnt == GetEnemy() )
		return COND_CAN_RANGE_ATTACK2;

	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : hand - 
//-----------------------------------------------------------------------------
void CNPC_Assassin::FirePistol( int hand )
{
	if ( m_flNextShotTime > gpGlobals->curtime )
		return;

	m_flNextShotTime = gpGlobals->curtime + random->RandomFloat( 0.05f, 0.15f );

	Vector	muzzlePos;
	QAngle	muzzleAngle;

	const char *handName = ( hand ) ? "LeftMuzzle" : "RightMuzzle";

	GetAttachment( handName, muzzlePos, muzzleAngle );

	Vector	muzzleDir;
	
	if ( GetEnemy() == NULL )
	{
		AngleVectors( muzzleAngle, &muzzleDir );
	}
	else
	{
		muzzleDir = GetEnemy()->BodyTarget( muzzlePos ) - muzzlePos;
		VectorNormalize( muzzleDir );
	}

	int bulletType = GetAmmoDef()->Index( "Pistol" );

	FireBullets( 1, muzzlePos, muzzleDir, VECTOR_CONE_5DEGREES, 1024, bulletType, 2 );

	UTIL_MuzzleFlash( muzzlePos, muzzleAngle, 0.5f, 1 );

	CPASAttenuationFilter filter( this );
	EmitSound( filter, entindex(), "NPC_Assassin.ShootPistol" );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Assassin::HandleAnimEvent( animevent_t *pEvent )
{
	
	if ( pEvent->event == AE_ASSASIN_FIRE_PISTOL_RIGHT )
	{
		FirePistol( 0 );
		return;
	}

	if ( pEvent->event == AE_ASSASIN_FIRE_PISTOL_LEFT )
	{
		FirePistol( 1 );
		return;
	}
	
	if ( pEvent->event == AE_ASSASIN_KICK_HIT )
	{
		Vector	attackDir = BodyDirection2D();
		Vector	attackPos = WorldSpaceCenter() + ( attackDir * 64.0f );

		trace_t	tr;
		UTIL_TraceHull( WorldSpaceCenter(), attackPos, -Vector(8,8,8), Vector(8,8,8), MASK_SHOT_HULL, this, COLLISION_GROUP_NONE, &tr );

		if ( ( tr.m_pEnt != NULL ) && ( tr.DidHitWorld() == false ) )
		{
			if ( tr.m_pEnt->m_takedamage != DAMAGE_NO )
			{
				CTakeDamageInfo info( this, this, 5, DMG_CLUB );
				CalculateMeleeDamageForce( &info, (tr.endpos - tr.startpos), tr.endpos );
				tr.m_pEnt->TakeDamage( info );

				CBasePlayer	*pPlayer = ToBasePlayer( tr.m_pEnt );

				if ( pPlayer != NULL )
				{
					//Kick the player angles
					pPlayer->ViewPunch( QAngle( -30, 40, 10 ) );
				}

				EmitSound( "Zombie.AttackHit" );
				//EmitSound( "Assassin.AttackHit" );
			}
		}
		else
		{
			EmitSound( "Assassin.AttackMiss" );
			//EmitSound( "Assassin.AttackMiss" );
		}

		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

//-----------------------------------------------------------------------------
// Purpose: Causes the assassin to prefer to run away, rather than towards her target
//-----------------------------------------------------------------------------
bool CNPC_Assassin::MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost )
{
	if ( GetEnemy() == NULL )
		return true;

	float	multiplier = 1.0f;

	Vector	moveDir = ( vecEnd - vecStart );
	VectorNormalize( moveDir );

	Vector	enemyDir = ( GetEnemy()->GetAbsOrigin() - vecStart );
	VectorNormalize( enemyDir );

	// If we're moving towards our enemy, then the cost is much higher than normal
	if ( DotProduct( enemyDir, moveDir ) > 0.5f )
	{
		multiplier = 16.0f;
	}

	*pCost *= multiplier;

	return ( multiplier != 1 );
}

//---------------------------------------------------------
//---------------------------------------------------------
int CNPC_Assassin::SelectSchedule ( void )
{
	switch	( m_NPCState )
	{
	case NPC_STATE_IDLE:
	case NPC_STATE_ALERT:
		{
			if ( HasCondition ( COND_HEAR_DANGER ) )
				 return SCHED_TAKE_COVER_FROM_BEST_SOUND;
				
			if ( HasCondition ( COND_HEAR_COMBAT ) )
				return SCHED_INVESTIGATE_SOUND;
		}
		break;

	case NPC_STATE_COMBAT:
		{
			// dead enemy
			if ( HasCondition( COND_ENEMY_DEAD ) )
			{
				// call base class, all code to handle dead enemies is centralized there.
				return BaseClass::SelectSchedule();
			}

			// Need to move
			if ( /*(	HasCondition( COND_SEE_ENEMY ) && HasCondition( COND_ASSASSIN_ENEMY_TARGETTING_ME ) && random->RandomInt( 0, 32 ) == 0 && m_flNextFlipTime < gpGlobals->curtime ) )*/
					( m_nNumFlips > 0 ) || 
					( ( HasCondition ( COND_LIGHT_DAMAGE ) && random->RandomInt( 0, 2 ) == 0 ) ) || ( HasCondition ( COND_HEAVY_DAMAGE ) ) )
			{
				if ( m_nNumFlips <= 0 )
				{
					m_nNumFlips = random->RandomInt( 1, 2 );
				}

				return SCHED_ASSASSIN_EVADE;
			}

			// Can kick
			if ( HasCondition( COND_CAN_MELEE_ATTACK1 ) )
				return SCHED_MELEE_ATTACK1;

			// Can shoot
			if ( HasCondition( COND_CAN_RANGE_ATTACK2 ) )
			{
				m_flNextLungeTime	= gpGlobals->curtime + 2.0f;
				m_nLastFlipType		= FLIP_FORWARD;

				return SCHED_ASSASSIN_LUNGE;
			}

			// Can shoot
			if ( HasCondition( COND_CAN_RANGE_ATTACK1 ) )
				return SCHED_RANGE_ATTACK1;

			// Face our enemy
			if ( HasCondition( COND_SEE_ENEMY ) )
				return SCHED_COMBAT_FACE;

			// new enemy
			if ( HasCondition( COND_NEW_ENEMY ) )
				return SCHED_TAKE_COVER_FROM_ENEMY;

			// ALERT( at_console, "stand\n");
			return SCHED_ASSASSIN_FIND_VANTAGE_POINT;
		}
		break;
	}

	return BaseClass::SelectSchedule();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Assassin::PrescheduleThink( void )
{
	if ( GetActivity() == ACT_RUN || GetActivity() == ACT_WALK)
	{
		CPASAttenuationFilter filter( this );

		static int iStep = 0;
		iStep = ! iStep;
		if (iStep)
		{
			EmitSound( filter, entindex(), "NPC_Assassin.Footstep" );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : right - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CNPC_Assassin::CanFlip( int flipType, Activity &activity, const Vector *avoidPosition )
{
	Vector		testDir;
	Activity	act = ACT_INVALID;

	switch( flipType )
	{
	case FLIP_RIGHT:
		GetVectors( NULL, &testDir, NULL );
		act = NPC_TranslateActivity( (Activity) ACT_ASSASSIN_FLIP_RIGHT ); 
		break;

	case FLIP_LEFT:
		GetVectors( NULL, &testDir, NULL );
		testDir.Negate();
		act = NPC_TranslateActivity( (Activity) ACT_ASSASSIN_FLIP_LEFT );
		break;

	case FLIP_FORWARD:
		GetVectors( &testDir, NULL, NULL );
		act = NPC_TranslateActivity( (Activity) ACT_ASSASSIN_FLIP_FORWARD );
		break;
	
	case FLIP_BACKWARD:
		GetVectors( &testDir, NULL, NULL );
		testDir.Negate();
		act = NPC_TranslateActivity( (Activity) ACT_ASSASSIN_FLIP_BACK );
		break;

	default:
		assert(0); //NOTENOTE: Invalid flip type
		activity = ACT_INVALID;
		return false;
		break;
	}

	// Make sure we don't flip towards our avoidance position/
	if ( avoidPosition != NULL )
	{
		Vector	avoidDir = (*avoidPosition) - GetAbsOrigin();
		VectorNormalize( avoidDir );

		if ( DotProduct( avoidDir, testDir ) > 0.0f )
			return false;
	}

	int seq = SelectWeightedSequence( act );

	// Find out the length of this sequence
	float	testDist = GetSequenceMoveDist( seq );
	
	// Find the resulting end position from the sequence's movement
	Vector	endPos = GetAbsOrigin() + ( testDir * testDist );

	trace_t	tr;

	if ( ( flipType != FLIP_BACKWARD ) && ( avoidPosition != NULL ) )
	{
		UTIL_TraceLine( (*avoidPosition), endPos, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
		
		if ( tr.fraction == 1.0f )
			return false;
	}

	/*
	UTIL_TraceHull( GetAbsOrigin(), endPos, NAI_Hull::Mins(m_eHull) + Vector( 0, 0, StepHeight() ), NAI_Hull::Maxs(m_eHull), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

	// See if we're hit an obstruction in that direction
	if ( tr.fraction < 1.0f )
	{
		if ( g_debug_assassin.GetBool() )
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), NAI_Hull::Mins(m_eHull) + Vector( 0, 0, StepHeight() ), NAI_Hull::Maxs(m_eHull) + Vector( testDist, 0, StepHeight() ), testDir, 255, 0, 0, true, 2.0f );
		}

		return false;
	}

#define NUM_STEPS 2

	float	stepLength = testDist / NUM_STEPS;

	for ( int i = 1; i <= NUM_STEPS; i++ )
	{
		endPos = GetAbsOrigin() + ( testDir * (stepLength*i) );
		
		// Also check for a cliff edge
		UTIL_TraceHull( endPos, endPos - Vector( 0, 0, StepHeight() * 4.0f ), NAI_Hull::Mins(m_eHull) + Vector( 0, 0, StepHeight() ), NAI_Hull::Maxs(m_eHull), MASK_NPCSOLID, this, COLLISION_GROUP_NONE, &tr );

		if ( tr.fraction == 1.0f )
		{
			if ( g_debug_assassin.GetBool() )
			{
				NDebugOverlay::BoxDirection( endPos, NAI_Hull::Mins(m_eHull) + Vector( 0, 0, StepHeight() ), NAI_Hull::Maxs(m_eHull) + Vector( StepHeight() * 4.0f, 0, StepHeight() ), Vector(0,0,-1), 255, 0, 0, true, 2.0f );
			}

			return false;
		}
	}

	if ( g_debug_assassin.GetBool() )
	{
		NDebugOverlay::BoxDirection( GetAbsOrigin(), NAI_Hull::Mins(m_eHull) + Vector( 0, 0, StepHeight() ), NAI_Hull::Maxs(m_eHull) + Vector( testDist, 0, StepHeight() ), testDir, 0, 255, 0, true, 2.0f );
	}
	*/
	
	AIMoveTrace_t moveTrace;
	GetMoveProbe()->TestGroundMove( GetAbsOrigin(), endPos, MASK_NPCSOLID, AITGM_DEFAULT, &moveTrace );

	if ( moveTrace.fStatus != AIMR_OK )
		return false;

	// Return the activity to use
	activity = (Activity) act;

	return true;
}

//---------------------------------------------------------
// Purpose: 
//---------------------------------------------------------
void CNPC_Assassin::StartTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ASSASSIN_SET_EYE_STATE:
		{
			SetEyeState( (eyeState_t) ( (int) pTask->flTaskData ) );
			TaskComplete();
		}
		break;

	case TASK_ASSASSIN_EVADE:
		{
			Activity flipAct = ACT_INVALID;

			const Vector *avoidPos = ( GetEnemy() != NULL ) ? &(GetEnemy()->GetAbsOrigin()) : NULL;

			for ( int i = FLIP_LEFT; i < NUM_FLIP_TYPES; i++ )
			{
				if ( CanFlip( i, flipAct, avoidPos ) )
				{
					// Don't flip back to where we just were
					if ( ( ( i == FLIP_LEFT ) && ( m_nLastFlipType == FLIP_RIGHT ) ) ||
						 ( ( i == FLIP_RIGHT ) && ( m_nLastFlipType == FLIP_LEFT ) ) ||
						 ( ( i == FLIP_FORWARD ) && ( m_nLastFlipType == FLIP_BACKWARD ) ) ||
						 ( ( i == FLIP_BACKWARD ) && ( m_nLastFlipType == FLIP_FORWARD ) ) )
					{
						flipAct = ACT_INVALID;
						continue;
					}

					m_nNumFlips--;
					ResetIdealActivity( flipAct );
					m_flNextFlipTime = gpGlobals->curtime + 2.0f;
					m_nLastFlipType = i;
					break;
				}
			}

			if ( flipAct == ACT_INVALID )
			{
				m_nNumFlips = 0;
				m_nLastFlipType = -1;
				m_flNextFlipTime = gpGlobals->curtime + 2.0f;
				TaskFail( "Unable to find flip evasion direction!\n" );
			}
		}
		break;

	case TASK_ASSASSIN_GET_PATH_TO_VANTAGE_POINT:
		{
			assert( GetEnemy() != NULL );
			if ( GetEnemy() == NULL )
				break;

			Vector	goalPos;

			CHintCriteria	hint;

			// Find a disadvantage node near the player, but away from ourselves
			hint.SetHintType( HINT_TACTICAL_ENEMY_DISADVANTAGED );
			hint.AddExcludePosition( GetAbsOrigin(), 256 );
			hint.AddExcludePosition( GetEnemy()->GetAbsOrigin(), 256 );

			if ( ( m_pSquad != NULL ) && ( m_pSquad->NumMembers() > 1 ) )
			{
				AISquadIter_t iter;
				for ( CAI_BaseNPC *pSquadMember = m_pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = m_pSquad->GetNextMember( &iter ) )
				{
					if ( pSquadMember == NULL )
						continue;

					hint.AddExcludePosition( pSquadMember->GetAbsOrigin(), 128 );
				}
			}
	
			hint.SetFlag( bits_HINT_NODE_NEAREST );

			CAI_Hint *pHint = CAI_HintManager::FindHint( this, GetEnemy()->GetAbsOrigin(), &hint );

			if ( pHint == NULL )
			{
				TaskFail( "Unable to find vantage point!\n" );
				break;
			}

			pHint->GetPosition( this, &goalPos );

			AI_NavGoal_t goal( goalPos );
			
			//Try to run directly there
			if ( GetNavigator()->SetGoal( goal ) == false )
			{
				TaskFail( "Unable to find path to vantage point!\n" );
				break;
			}
			
			TaskComplete();
		}
		break;

	default:
		BaseClass::StartTask( pTask );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
//
//-----------------------------------------------------------------------------
float CNPC_Assassin::MaxYawSpeed( void )
{
	switch( GetActivity() )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 160;
		break;
	case ACT_RUN:
		return 900;
		break;
	case ACT_RANGE_ATTACK1:
		return 0;
		break;
	default:
		return 60;
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
void CNPC_Assassin::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
	case TASK_ASSASSIN_EVADE:

		AutoMovement();

		if ( IsActivityFinished() )
		{
			TaskComplete();
		}

		break;
		
	default:
		BaseClass::RunTask( pTask );
		break;
	}
}


//---------------------------------------------------------
//---------------------------------------------------------
bool CNPC_Assassin::FValidateHintType ( CAI_Hint *pHint )
{
	switch( pHint->HintType() )
	{
	case HINT_TACTICAL_ENEMY_DISADVANTAGED:
		{
			Vector	hintPos;
			pHint->GetPosition( this, &hintPos );

			// Verify that we can see the target from that position
			hintPos += GetViewOffset();

			trace_t	tr;
			UTIL_TraceLine( hintPos, GetEnemy()->BodyTarget( hintPos, true ), MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

			// Check for seeing our target at the new location
			if ( ( tr.fraction == 1.0f ) || ( tr.m_pEnt == GetEnemy() ) )
				return false;

			return true;
			break;
		}

	default:
		return false;
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const Vector
//-----------------------------------------------------------------------------
const Vector &CNPC_Assassin::GetViewOffset( void )
{
	static Vector eyeOffset;

	//FIXME: Use eye attachment?
	// If we're crouching, offset appropriately
	if ( ( GetActivity() == ACT_ASSASSIN_PERCH ) ||
		 ( GetActivity() == ACT_RANGE_ATTACK1 ) )
	{
		eyeOffset = Vector( 0, 0, 24.0f );
	}
	else
	{
		eyeOffset = BaseClass::GetViewOffset();
	}

	return eyeOffset;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Assassin::OnScheduleChange( void )
{
	//TODO: Change eye state?

	BaseClass::OnScheduleChange();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CNPC_Assassin::SetEyeState( eyeState_t state )
{
	//Must have a valid eye to affect
	if ( ( m_pEyeSprite == NULL ) || ( m_pEyeTrail == NULL ) )
		return;

	//Set the state
	switch( state )
	{
	default:
	case ASSASSIN_EYE_SEE_TARGET: //Fade in and scale up
		m_pEyeSprite->SetColor( 255, 0, 0 );
		m_pEyeSprite->SetBrightness( 164, 0.1f );
		m_pEyeSprite->SetScale( 0.4f, 0.1f );

		m_pEyeTrail->SetColor( 255, 0, 0 );
		m_pEyeTrail->SetScale( 8.0f );
		m_pEyeTrail->SetBrightness( 164 );

		break;

	case ASSASSIN_EYE_SEEKING_TARGET: //Ping-pongs
		
		//Toggle our state
		m_bBlinkState = !m_bBlinkState;
		m_pEyeSprite->SetColor( 255, 128, 0 );

		if ( m_bBlinkState )
		{
			//Fade up and scale up
			m_pEyeSprite->SetScale( 0.25f, 0.1f );
			m_pEyeSprite->SetBrightness( 164, 0.1f );
		}
		else
		{
			//Fade down and scale down
			m_pEyeSprite->SetScale( 0.2f, 0.1f );
			m_pEyeSprite->SetBrightness( 64, 0.1f );
		}

		break;

	case ASSASSIN_EYE_DORMANT: //Fade out and scale down
		m_pEyeSprite->SetScale( 0.5f, 0.5f );
		m_pEyeSprite->SetBrightness( 64, 0.5f );
		
		m_pEyeTrail->SetScale( 2.0f );
		m_pEyeTrail->SetBrightness( 64 );
		break;

	case ASSASSIN_EYE_DEAD: //Fade out slowly
		m_pEyeSprite->SetColor( 255, 0, 0 );
		m_pEyeSprite->SetScale( 0.1f, 5.0f );
		m_pEyeSprite->SetBrightness( 0, 5.0f );

		m_pEyeTrail->SetColor( 255, 0, 0 );
		m_pEyeTrail->SetScale( 0.1f );
		m_pEyeTrail->SetBrightness( 0 );
		break;

	case ASSASSIN_EYE_ACTIVE:
		m_pEyeSprite->SetColor( 255, 0, 0 );
		m_pEyeSprite->SetScale( 0.1f );
		m_pEyeSprite->SetBrightness( 0 );
		break;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Assassin::GatherEnemyConditions( CBaseEntity *pEnemy )
{
	ClearCondition( COND_ASSASSIN_ENEMY_TARGETTING_ME );

	BaseClass::GatherEnemyConditions( pEnemy );

	// See if we're being targetted specifically
	if ( HasCondition( COND_ENEMY_FACING_ME ) )
	{
		Vector	enemyDir = GetAbsOrigin() - pEnemy->GetAbsOrigin();
		VectorNormalize( enemyDir );

		Vector	enemyBodyDir;
		CBasePlayer	*pPlayer = ToBasePlayer( pEnemy );

		if ( pPlayer != NULL )
		{
			enemyBodyDir = pPlayer->BodyDirection3D();
		}
		else
		{
			AngleVectors( pEnemy->GetAbsAngles(), &enemyBodyDir );
		}

		float	enemyDot = DotProduct( enemyBodyDir, enemyDir );

		//FIXME: Need to refine this a bit
		if ( enemyDot > 0.97f )
		{
			SetCondition( COND_ASSASSIN_ENEMY_TARGETTING_ME );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Assassin::BuildScheduleTestBits( void )
{
	SetNextThink( gpGlobals->curtime + 0.05 );

		//Don't allow any modifications when scripted
	if ( m_NPCState == NPC_STATE_SCRIPT )
		return;

	//Become interrupted if we're targetted when shooting an enemy
	if ( IsCurSchedule( SCHED_RANGE_ATTACK1 ) )
	{
		SetCustomInterruptCondition( COND_ASSASSIN_ENEMY_TARGETTING_ME );
	}
	
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &info - 
//-----------------------------------------------------------------------------
void CNPC_Assassin::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	// Turn off the eye
	SetEyeState( ASSASSIN_EYE_DEAD );
	
	// Turn off the pistols
	SetBodygroup( 1, 0 );

	// Spawn her guns
}

//-----------------------------------------------------------------------------
//
// Schedules
//
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_NPC( npc_assassin, CNPC_Assassin )

	DECLARE_ACTIVITY(ACT_ASSASSIN_FLIP_LEFT)
	DECLARE_ACTIVITY(ACT_ASSASSIN_FLIP_RIGHT)
	DECLARE_ACTIVITY(ACT_ASSASSIN_FLIP_BACK)
	DECLARE_ACTIVITY(ACT_ASSASSIN_FLIP_FORWARD)
	DECLARE_ACTIVITY(ACT_ASSASSIN_PERCH)

	//Adrian: events go here
	DECLARE_ANIMEVENT( AE_ASSASIN_FIRE_PISTOL_RIGHT )
	DECLARE_ANIMEVENT( AE_ASSASIN_FIRE_PISTOL_LEFT )
	DECLARE_ANIMEVENT( AE_ASSASIN_KICK_HIT )

	DECLARE_TASK(TASK_ASSASSIN_GET_PATH_TO_VANTAGE_POINT)
	DECLARE_TASK(TASK_ASSASSIN_EVADE)
	DECLARE_TASK(TASK_ASSASSIN_SET_EYE_STATE)
	DECLARE_TASK(TASK_ASSASSIN_LUNGE)

	DECLARE_CONDITION(COND_ASSASSIN_ENEMY_TARGETTING_ME)

	//=========================================================
	// ASSASSIN_STALK_ENEMY
	//=========================================================

	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_STALK_ENEMY,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_PLAY_SEQUENCE_FACE_ENEMY			ACTIVITY:ACT_ASSASSIN_PERCH"
		"	"
		"	Interrupts"
		"		COND_ASSASSIN_ENEMY_TARGETTING_ME"
		"		COND_SEE_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// > ASSASSIN_FIND_VANTAGE_POINT
	//=========================================================

	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_FIND_VANTAGE_POINT,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_TAKE_COVER_FROM_ENEMY"
		"		TASK_STOP_MOVING						0"
		"		TASK_ASSASSIN_GET_PATH_TO_VANTAGE_POINT	0"
		"		TASK_RUN_PATH							0"
		"		TASK_WAIT_FOR_MOVEMENT					0"
		"		TASK_SET_SCHEDULE						SCHEDULE:SCHED_ASSASSIN_STALK_ENEMY"
		"	"
		"	Interrupts"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_TASK_FAILED"
	)

	//=========================================================
	// Assassin needs to avoid the player
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_EVADE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_ASSASSIN_FIND_VANTAGE_POINT"
		"		TASK_STOP_MOVING			0"
		"		TASK_ASSASSIN_EVADE			0"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
	)	

	//=========================================================
	// Assassin needs to avoid the player
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ASSASSIN_LUNGE,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_ASSASSIN_FIND_VANTAGE_POINT"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ASSASSIN_FLIP_FORWARD"
		"	"
		"	Interrupts"
		"		COND_TASK_FAILED"
	)	

AI_END_CUSTOM_NPC()

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "ai_utils.h"
#include "ai_memory.h"
#include "ai_basenpc.h"
#include "ai_senses.h"
#include "ai_moveprobe.h"
#include "vphysics/object_hash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_MoveMonitor )
	DEFINE_FIELD( m_vMark, FIELD_POSITION_VECTOR ), 
	DEFINE_FIELD( m_flMarkTolerance, FIELD_FLOAT )
END_DATADESC()

//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_ShotRegulator )
	DEFINE_FIELD( m_flNextShotTime, FIELD_TIME ),
	DEFINE_FIELD( m_bInRestInterval, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nBurstShotsRemaining, FIELD_SHORT ),
	DEFINE_FIELD( m_nMinBurstShots, FIELD_SHORT ),
	DEFINE_FIELD( m_nMaxBurstShots, FIELD_SHORT ),
	DEFINE_FIELD( m_flMinRestInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxRestInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMinBurstInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxBurstInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_bDisabled, FIELD_BOOLEAN ),
END_DATADESC()

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
// Constructor
//-----------------------------------------------------------------------------
CAI_ShotRegulator::CAI_ShotRegulator() : m_nMinBurstShots(1), m_nMaxBurstShots(1)
{
	m_flMinRestInterval = 0.0f;
	m_flMaxRestInterval = 0.0f;
	m_flMinBurstInterval = 0.0f;
	m_flMaxBurstInterval = 0.0f;
	m_flNextShotTime = -1;
	m_nBurstShotsRemaining = 1;
	m_bInRestInterval = false;
	m_bDisabled = false;
}


//-----------------------------------------------------------------------------
// For backward compatibility
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::SetParameters( int minShotsPerBurst, int maxShotsPerBurst, float minRestTime, float maxRestTime )
{
	SetBurstShotCountRange( minShotsPerBurst, maxShotsPerBurst );
	SetRestInterval( minRestTime, maxRestTime );
	Reset( false );
}


//-----------------------------------------------------------------------------
// Sets the number of shots to shoot in a single burst
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::SetBurstShotCountRange( int minShotsPerBurst, int maxShotsPerBurst )
{
	m_nMinBurstShots = minShotsPerBurst;
	m_nMaxBurstShots = maxShotsPerBurst;
}


//-----------------------------------------------------------------------------
// How much time should I rest between bursts?
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::SetRestInterval( float flMinRestInterval, float flMaxRestInterval )
{
	m_flMinRestInterval = flMinRestInterval;
	m_flMaxRestInterval = flMaxRestInterval;
}


//-----------------------------------------------------------------------------
// How much time should I wait in between shots in a single burst?
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::SetBurstInterval( float flMinBurstInterval, float flMaxBurstInterval )
{
	m_flMinBurstInterval = flMinBurstInterval;
	m_flMaxBurstInterval = flMaxBurstInterval;
}


//-----------------------------------------------------------------------------
// Poll the current parameters
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::GetBurstShotCountRange( int *pMinShotsPerBurst, int *pMaxShotsPerBurst ) const
{
	*pMinShotsPerBurst = m_nMinBurstShots;
	*pMaxShotsPerBurst = m_nMaxBurstShots;
}

void CAI_ShotRegulator::GetRestInterval( float *pMinRestInterval, float *pMaxRestInterval ) const
{
	*pMinRestInterval = m_flMinRestInterval;
	*pMaxRestInterval = m_flMaxRestInterval;
}

void CAI_ShotRegulator::GetBurstInterval( float *pMinBurstInterval, float *pMaxBurstInterval ) const
{
	*pMinBurstInterval = m_flMinBurstInterval;
	*pMaxBurstInterval = m_flMaxBurstInterval;
}


//-----------------------------------------------------------------------------
// Resets the shot regulator to start a new burst
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::Reset( bool bStartShooting )
{
	m_bDisabled = false;
	m_nBurstShotsRemaining = random->RandomInt( m_nMinBurstShots, m_nMaxBurstShots );
	if ( bStartShooting )
	{
		m_flNextShotTime = gpGlobals->curtime;
		m_bInRestInterval = false;
	}
	else
	{
		m_flNextShotTime = gpGlobals->curtime + random->RandomFloat( m_flMinRestInterval, m_flMaxRestInterval );
		m_bInRestInterval = true;
	}
}


//-----------------------------------------------------------------------------
// Should we shoot?
//-----------------------------------------------------------------------------
bool CAI_ShotRegulator::ShouldShoot() const
{ 
	return ( !m_bDisabled && (m_flNextShotTime <= gpGlobals->curtime) ); 
}


//-----------------------------------------------------------------------------
// Am I in the middle of a burst?
//-----------------------------------------------------------------------------
bool CAI_ShotRegulator::IsInRestInterval() const
{
	return (m_bInRestInterval && !ShouldShoot()); 
}


//-----------------------------------------------------------------------------
// When will I shoot next?
//-----------------------------------------------------------------------------
float CAI_ShotRegulator::NextShotTime() const
{
	return m_flNextShotTime;
}


//-----------------------------------------------------------------------------
// Causes us to potentially delay our shooting time
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::FireNoEarlierThan( float flTime )
{
	if ( flTime > m_flNextShotTime )
	{
		m_flNextShotTime = flTime;
	}
}


//-----------------------------------------------------------------------------
// Burst shot count accessors
//-----------------------------------------------------------------------------
int CAI_ShotRegulator::GetBurstShotsRemaining() const				
{ 
	return m_nBurstShotsRemaining; 
}

void CAI_ShotRegulator::SetBurstShotsRemaining( int shots )	
{
	m_nBurstShotsRemaining = shots;
}


//-----------------------------------------------------------------------------
// We fired the weapon! Update the next shot time
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::OnFiredWeapon()
{
	--m_nBurstShotsRemaining;
	if ( m_nBurstShotsRemaining <= 0 )
	{
		Reset( false );
	}
	else
	{
		m_bInRestInterval = false;
		m_flNextShotTime += random->RandomFloat( m_flMinBurstInterval, m_flMaxBurstInterval );
		if ( m_flNextShotTime < gpGlobals->curtime )
		{
			m_flNextShotTime = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::EnableShooting( void )
{
	m_bDisabled = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::DisableShooting( void )
{
	m_bDisabled = true;
}


//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_AccelDecay )
	DEFINE_FIELD( m_velocity,		FIELD_FLOAT ),
	DEFINE_FIELD( m_maxVelocity,	FIELD_FLOAT ),
	DEFINE_FIELD( m_minVelocity,	FIELD_FLOAT ),
	DEFINE_FIELD( m_invDecay,		FIELD_FLOAT ),
	DEFINE_FIELD( m_decayTime,		FIELD_FLOAT ),
	DEFINE_FIELD( m_accel,			FIELD_FLOAT ),
END_DATADESC()


void CAI_AccelDecay::SetParameters( float minVelocity, float maxVelocity, float accelPercentPerTick, float decelPercentPerTick )
{
	m_minVelocity = minVelocity;
	m_maxVelocity = maxVelocity;

	m_accel = accelPercentPerTick;
	m_invDecay = 1.0 - decelPercentPerTick;

	m_decayTime = 0.0;
	float d = 1.0;

	int i = 0;
	while (d * m_maxVelocity > m_minVelocity && i < 10)
	{
		d = d * m_invDecay;
		m_decayTime = m_decayTime + 0.1 * d; // appox interval call
		i++;
	}
}

//-----------------------------------------------------------------------------
//
//-----------------------------------------------------------------------------

float CAI_AccelDecay::Update( float flCurrent, float flTarget, float flInterval )
{
	float delta = flTarget - flCurrent;
	float deltaSign = ( delta < 0 ) ? -1 : 1;
	delta = fabsf( delta );

	float curVelocity = m_velocity;

	if ( delta > 0.01 )
	{
		if (fabsf( m_velocity ) < m_minVelocity)
			m_velocity = m_minVelocity * deltaSign;

		if (delta < m_velocity * deltaSign * m_decayTime )
		{
			m_velocity = m_velocity * m_invDecay;

			if (delta < m_velocity * deltaSign * flInterval)
			{
				m_velocity = delta * deltaSign / flInterval;
			}
		}
		else
		{
			m_velocity = m_velocity * (1.0f - m_accel) + m_maxVelocity * m_accel * deltaSign;
			if (delta < m_velocity * deltaSign * m_decayTime)
			{
				m_velocity = delta * deltaSign / m_decayTime;
			}
		}

		float newValue = flCurrent + (curVelocity + m_velocity) * 0.5 * flInterval;
		return newValue;
	}

	return flTarget;
}



void CAI_AccelDecay::ResetVelocity( float flVelocity )
{
	m_velocity = flVelocity;
}


void CAI_AccelDecay::SetMaxVelocity( float maxVelocity )
{
	if (maxVelocity != m_maxVelocity)
	{
		SetParameters( m_minVelocity, maxVelocity, m_accel, 1.0 - m_invDecay );
	}
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

ConVar free_pass_peek_debug( "free_pass_peek_debug", "0" );

BEGIN_SIMPLE_DATADESC( AI_FreePassParams_t )

	DEFINE_KEYFIELD( timeToTrigger,			FIELD_FLOAT, "freepass_timetotrigger"),
	DEFINE_KEYFIELD( duration,				FIELD_FLOAT, "freepass_duration"),
	DEFINE_KEYFIELD( moveTolerance,			FIELD_FLOAT, "freepass_movetolerance"),
	DEFINE_KEYFIELD( refillRate,			FIELD_FLOAT, "freepass_refillrate"),
	DEFINE_FIELD(	 coverDist,				FIELD_FLOAT),
	DEFINE_KEYFIELD( peekTime,				FIELD_FLOAT, "freepass_peektime"),
	DEFINE_FIELD(	 peekTimeAfterDamage,	FIELD_FLOAT),
	DEFINE_FIELD(	 peekEyeDist,			FIELD_FLOAT),
	DEFINE_FIELD(	 peekEyeDistZ,			FIELD_FLOAT),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( CAI_FreePass )

	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_FreePassTimeRemaining,	FIELD_FLOAT ),
	DEFINE_EMBEDDED( m_FreePassMoveMonitor ), 
	DEFINE_EMBEDDED( m_Params ), 

END_DATADESC()

//---------------------------------------------------------
//---------------------------------------------------------

void CAI_FreePass::Reset( float passTime, float moveTolerance )
{
	CBaseEntity *pTarget = GetPassTarget();

	if ( !pTarget || m_Params.duration < 0.1 )
		return;

	if ( passTime == -1 )
	{
		m_FreePassTimeRemaining = m_Params.duration;
	}
	else
	{
		m_FreePassTimeRemaining = passTime;
	}

	if ( moveTolerance == -1  )
	{
		m_FreePassMoveMonitor.SetMark( pTarget, m_Params.moveTolerance );
	}
	else
	{
		m_FreePassMoveMonitor.SetMark( pTarget, moveTolerance );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------

void CAI_FreePass::Update( )
{
	CBaseEntity *pTarget = GetPassTarget();
	if ( !pTarget || m_Params.duration < 0.1 )
		return;

	//---------------------------------
	//
	// Free pass logic
	//
	AI_EnemyInfo_t *pTargetInfo = GetOuter()->GetEnemies()->Find( pTarget );

	// This works with old data because need to do before base class so as to not choose as enemy
	if ( !HasPass() )
	{
		float timePlayerLastSeen = (pTargetInfo) ? pTargetInfo->timeLastSeen : AI_INVALID_TIME;
		float lastTimeDamagedBy = (pTargetInfo) ? pTargetInfo->timeLastReceivedDamageFrom : AI_INVALID_TIME;

		if ( timePlayerLastSeen == AI_INVALID_TIME || gpGlobals->curtime - timePlayerLastSeen > .15 ) // If didn't see the player last think
		{
			trace_t tr;
			UTIL_TraceLine( pTarget->EyePosition(), GetOuter()->EyePosition(), MASK_BLOCKLOS, GetOuter(), COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.0 && tr.m_pEnt != pTarget )
			{
				float dist = (tr.endpos - tr.startpos).Length() * tr.fraction;

				if ( dist < m_Params.coverDist )
				{
					if ( ( timePlayerLastSeen == AI_INVALID_TIME || gpGlobals->curtime - timePlayerLastSeen > m_Params.timeToTrigger ) &&
						 ( lastTimeDamagedBy == AI_INVALID_TIME || gpGlobals->curtime - lastTimeDamagedBy > m_Params.timeToTrigger ) )
					{
						m_FreePassTimeRemaining = m_Params.duration;
						m_FreePassMoveMonitor.SetMark( pTarget, m_Params.moveTolerance );
					}
				}
			}
		}
	}
	else
	{
		float temp = m_FreePassTimeRemaining;
		m_FreePassTimeRemaining = 0;
		CAI_Senses *pSenses = GetOuter()->GetSenses();
		bool bCanSee = ( pSenses && pSenses->ShouldSeeEntity( pTarget ) && pSenses->CanSeeEntity( pTarget ) );
		m_FreePassTimeRemaining = temp;

		if ( bCanSee )
		{
			if ( !m_FreePassMoveMonitor.TargetMoved( pTarget ) )
				m_FreePassTimeRemaining -= 0.1;
			else
				Revoke( true );
		}
		else
		{
			m_FreePassTimeRemaining += 0.1 * m_Params.refillRate;
			if ( m_FreePassTimeRemaining > m_Params.duration )
				m_FreePassTimeRemaining = m_Params.duration;
			m_FreePassMoveMonitor.SetMark( pTarget, m_Params.moveTolerance );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_FreePass::HasPass()
{
	return ( m_FreePassTimeRemaining > 0 );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_FreePass::Revoke( bool bUpdateMemory )
{
	m_FreePassTimeRemaining = 0;
	if ( bUpdateMemory && GetPassTarget() )
	{
		GetOuter()->UpdateEnemyMemory( GetPassTarget(), GetPassTarget()->GetAbsOrigin() );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_FreePass::ShouldAllowFVisible(bool bBaseResult )
{
	CBaseEntity *	pTarget 	= GetPassTarget();
	AI_EnemyInfo_t *pTargetInfo = GetOuter()->GetEnemies()->Find( pTarget );
	
	if ( !bBaseResult || HasPass() )
		return false;
		
	bool bIsVisible = true;
		
	// Peek logic
	if ( m_Params.peekTime > 0.1 )
	{
		float lastTimeSeen = (pTargetInfo) ? pTargetInfo->timeLastSeen : AI_INVALID_TIME;
		float lastTimeDamagedBy = (pTargetInfo) ? pTargetInfo->timeLastReceivedDamageFrom : AI_INVALID_TIME;
		
		if ( ( lastTimeSeen == AI_INVALID_TIME || gpGlobals->curtime - lastTimeSeen > m_Params.peekTime ) &&
			 ( lastTimeDamagedBy == AI_INVALID_TIME || gpGlobals->curtime - lastTimeDamagedBy > m_Params.peekTimeAfterDamage ) )
		{
			Vector vToTarget;

			VectorSubtract( pTarget->EyePosition(), GetOuter()->EyePosition(), vToTarget );
			vToTarget.z = 0.0f;
			VectorNormalize( vToTarget );

			Vector vecRight( -vToTarget.y, vToTarget.x, 0.0f );
			trace_t	tr;

			UTIL_TraceLine( GetOuter()->EyePosition(), pTarget->EyePosition() + (vecRight * m_Params.peekEyeDist - Vector( 0, 0, m_Params.peekEyeDistZ )), MASK_BLOCKLOS, GetOuter(), COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.0 && tr.m_pEnt != pTarget )
			{
				if ( free_pass_peek_debug.GetBool() )
					NDebugOverlay::Line( tr.startpos, tr.endpos - Vector( 0, 0, 2), 0, 255, 0, false, 0.1 );
				bIsVisible = false;
			}
			
			if ( bIsVisible )
			{
				UTIL_TraceLine( GetOuter()->EyePosition(), pTarget->EyePosition() + (-vecRight * m_Params.peekEyeDist - Vector( 0, 0, m_Params.peekEyeDistZ )), MASK_BLOCKLOS, GetOuter(), COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction != 1.0 && tr.m_pEnt != pTarget )
				{
					if ( free_pass_peek_debug.GetBool() )
						NDebugOverlay::Line( tr.startpos, tr.endpos - Vector( 0, 0, 2), 0, 255, 0, false, 0.1 );
					bIsVisible = false;
				}
			}
		}
		
		if ( bIsVisible && free_pass_peek_debug.GetBool() )
			NDebugOverlay::Line( GetOuter()->EyePosition(), pTarget->EyePosition() - Vector( 0, 0, 2), 255, 0, 0, false, 0.1 );
	}

	return bIsVisible;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------

string_t g_iszFuncBrushClassname = NULL_STRING;

//-----------------------------------------------------------------------------
CTraceFilterNav::CTraceFilterNav( CAI_BaseNPC *pProber, bool bIgnoreTransientEntities, const IServerEntity *passedict, int collisionGroup, bool bAllowPlayerAvoid ) : 
	CTraceFilterSimple( passedict, collisionGroup ),
	m_pProber(pProber),
	m_bIgnoreTransientEntities(bIgnoreTransientEntities),
	m_bAllowPlayerAvoid(bAllowPlayerAvoid)
{
	m_bCheckCollisionTable = g_EntityCollisionHash->IsObjectInHash( pProber );
}

//-----------------------------------------------------------------------------
bool CTraceFilterNav::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	IServerEntity *pServerEntity = (IServerEntity*)pHandleEntity;
	CBaseEntity *pEntity = (CBaseEntity *)pServerEntity;

	if ( m_pProber == pEntity )
		return false;

	if ( m_pProber->GetMoveProbe()->ShouldBrushBeIgnored( pEntity ) == true )
		return false;

#ifdef HL1_DLL 
	if ( ( contentsMask & CONTENTS_MOVEABLE ) == 0 )
	{
		if ( pEntity->ClassMatches( "func_pushable" ) )
			return false;
	}
#endif

	if ( m_bIgnoreTransientEntities && (pEntity->IsPlayer() || pEntity->IsNPC() ) )
		return false;

	//Adrian - If I'm flagged as using the new collision method, then ignore the player when trying
	//to check if I can get somewhere.
	if ( m_bAllowPlayerAvoid && m_pProber->ShouldPlayerAvoid() && pEntity->IsPlayer() )
		return false;

	if ( pEntity->IsNavIgnored() )
		return false;

	if ( m_bCheckCollisionTable )
	{
		if ( g_EntityCollisionHash->IsObjectPairInHash( m_pProber, pEntity ) )
			return false;
	}

	if ( m_pProber->ShouldProbeCollideAgainstEntity( pEntity ) == false )
		return false;

	return CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

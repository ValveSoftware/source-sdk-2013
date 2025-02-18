//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"

#include "movevars_shared.h"

#include "ai_blended_movement.h"
#include "ai_route.h"
#include "ai_navigator.h"
#include "ai_moveprobe.h"
#include "KeyValues.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
//
// class CAI_BlendedMotor
//

BEGIN_SIMPLE_DATADESC( CAI_BlendedMotor )
	// DEFINE_FIELD( m_bDeceleratingToGoal, FIELD_BOOLEAN ),

	// DEFINE_FIELD( m_iPrimaryLayer, FIELD_INTEGER ),
	// DEFINE_FIELD( m_iSecondaryLayer, FIELD_INTEGER ),

	// DEFINE_FIELD( m_nPrimarySequence, FIELD_INTEGER ),
	// DEFINE_FIELD( m_nSecondarySequence, FIELD_INTEGER ),
	// DEFINE_FIELD( m_flSecondaryWeight, FIELD_FLOAT ),

	// DEFINE_CUSTOM_FIELD( m_nSavedGoalActivity, ActivityDataOps() ),
	// DEFINE_CUSTOM_FIELD( m_nSavedTranslatedGoalActivity, ActivityDataOps() ),
	// DEFINE_FIELD( m_nGoalSequence, FIELD_INTEGER ),

	// DEFINE_FIELD( m_nPrevMovementSequence, FIELD_INTEGER ),
	// DEFINE_FIELD( m_nInteriorSequence, FIELD_INTEGER ),
	// DEFINE_FIELD( m_flCurrRate, FIELD_FLOAT ),
	// DEFINE_FIELD( m_flStartCycle, FIELD_FLOAT ),

	//			m_scriptMove
	//			m_scriptTurn

	//	DEFINE_FIELD( m_flNextTurnGesture, FIELD_TIME ),
	//	DEFINE_FIELD( m_prevYaw, FIELD_FLOAT ),
	//	DEFINE_FIELD( m_doTurn, FIELD_FLOAT ),
	//	DEFINE_FIELD( m_doLeft, FIELD_FLOAT ),
	//	DEFINE_FIELD( m_doRight, FIELD_FLOAT ),
	//	DEFINE_FIELD( m_flNextTurnAct, FIELD_TIME ),
	//	DEFINE_FIELD( m_flPredictiveSpeedAdjust, FIELD_FLOAT ),
	//	DEFINE_FIELD( m_flReactiveSpeedAdjust, FIELD_FLOAT ),
	//	DEFINE_FIELD( m_vecPrevOrigin1, FIELD_POSITION ),
	//	DEFINE_FIELD( m_vecPrevOrigin2, FIELD_POSITION ),

END_DATADESC()

//-------------------------------------

void CAI_BlendedMotor::ResetMoveCalculations()
{
	BaseClass::ResetMoveCalculations();
	m_scriptMove.RemoveAll();
	m_scriptTurn.RemoveAll();
}

//-------------------------------------

void CAI_BlendedMotor::MoveStart()
{ 
	AI_PROFILE_SCOPE(CAI_BlendedMotor_MoveStart);

	if (m_nPrimarySequence == -1)
	{
		m_nPrimarySequence = GetSequence();
		m_flStartCycle = GetCycle();
		m_flCurrRate = 0.4;

		// Assert( !GetOuter()->HasMovement( m_nStartSequence ) );

		m_nSecondarySequence = -1;

		m_iPrimaryLayer = AddLayeredSequence( m_nPrimarySequence, 0 );
		SetLayerWeight( m_iPrimaryLayer, 0.0 );
		SetLayerPlaybackRate( m_iPrimaryLayer, 0.0 );
		SetLayerNoRestore( m_iPrimaryLayer, true );
		SetLayerCycle( m_iPrimaryLayer, m_flStartCycle, m_flStartCycle );

		m_flSecondaryWeight = 0.0;
	}
	else
	{
		// suspect that MoveStop() wasn't called when the previous route finished
		// Assert( 0 );
	}


	if (m_nGoalSequence == ACT_INVALID)
	{
		ResetGoalSequence();
	}

	m_vecPrevOrigin2 = GetAbsOrigin();
	m_vecPrevOrigin1 = GetAbsOrigin();

	m_bDeceleratingToGoal = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------

void CAI_BlendedMotor::ResetGoalSequence( void )
{

	m_nSavedGoalActivity = GetNavigator()->GetArrivalActivity( );
	if (m_nSavedGoalActivity == ACT_INVALID)
	{
		m_nSavedGoalActivity = GetOuter()->GetStoppedActivity();
	}

	m_nSavedTranslatedGoalActivity = GetOuter()->NPC_TranslateActivity( m_nSavedGoalActivity );

	m_nGoalSequence = GetNavigator()->GetArrivalSequence( m_nPrimarySequence );
	// Msg("Start %s end %s\n", GetOuter()->GetSequenceName( m_nPrimarySequence ), GetOuter()->GetSequenceName( m_nGoalSequence ) );

	m_nGoalSequence = GetInteriorSequence( m_nPrimarySequence );

	Assert( m_nGoalSequence != ACT_INVALID );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------


void CAI_BlendedMotor::MoveStop()
{ 
	AI_PROFILE_SCOPE(CAI_BlendedMotor_MoveStop);

	CAI_Motor::MoveStop();

	if (m_iPrimaryLayer != -1)
	{
		RemoveLayer( m_iPrimaryLayer, 0.2, 0.1 );
		m_iPrimaryLayer = -1;
	}
	if (m_iSecondaryLayer != -1)
	{
		RemoveLayer( m_iSecondaryLayer, 0.2, 0.1 );
		m_iSecondaryLayer = -1;
	}
	m_nPrimarySequence = ACT_INVALID;
	m_nSecondarySequence = ACT_INVALID;
	m_nPrevMovementSequence = ACT_INVALID;
	m_nInteriorSequence = ACT_INVALID;

	// 	int nNextSequence = FindTransitionSequence(GetSequence(), m_nIdealSequence, NULL);
}

void CAI_BlendedMotor::MovePaused()
{
	CAI_Motor::MovePaused();
	SetMoveScriptAnim( 0.0 );
}


void CAI_BlendedMotor::MoveContinue()
{ 
	AI_PROFILE_SCOPE(CAI_BlendedMotor_MoveContinue);

	m_nPrimarySequence = GetInteriorSequence( ACT_INVALID );
	m_nGoalSequence = m_nPrimarySequence;

	Assert( m_nPrimarySequence != ACT_INVALID );

	if (m_nPrimarySequence == ACT_INVALID)
		return;

	m_flStartCycle = 0.0;

	m_iPrimaryLayer = AddLayeredSequence( m_nPrimarySequence, 0 );
	SetLayerWeight( m_iPrimaryLayer, 0.0 );
	SetLayerPlaybackRate( m_iPrimaryLayer, 0.0 );
	SetLayerNoRestore( m_iPrimaryLayer, true );
	SetLayerCycle( m_iPrimaryLayer, m_flStartCycle, m_flStartCycle );

	m_bDeceleratingToGoal = false;
}


//-----------------------------------------------------------------------------
// Purpose: for the MoveInterval, interpolate desired speed, calc actual distance traveled
//-----------------------------------------------------------------------------
float CAI_BlendedMotor::GetMoveScriptDist( float &flNewSpeed )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_GetMoveScriptDist);

	int i;
	float flTotalDist = 0;
	float t = GetMoveInterval();

	Assert( m_scriptMove.Count() > 1);

	flNewSpeed = 0;
	for (i = 0; i < m_scriptMove.Count()-1; i++)
	{
		if (t < m_scriptMove[i].flTime)
		{
			// get new velocity
			float a = t / m_scriptMove[i].flTime;
			flNewSpeed = m_scriptMove[i].flMaxVelocity * (1 - a) + m_scriptMove[i+1].flMaxVelocity * a;
			
			// get distance traveled over this entry
			flTotalDist += (m_scriptMove[i].flMaxVelocity + flNewSpeed) * 0.5 * t; 
			break;
		}
		else
		{
			// used all of entries time, get entries total movement
			flNewSpeed = m_scriptMove[i+1].flMaxVelocity;
			flTotalDist += m_scriptMove[i].flDist;
			t -= m_scriptMove[i].flTime;
		}
	}

	return flTotalDist;
}


//-----------------------------------------------------------------------------
// Purpose: return the total time that the move script covers
//-----------------------------------------------------------------------------

float CAI_BlendedMotor::GetMoveScriptTotalTime()
{
	float flDist = GetNavigator()->GetArrivalDistance();

	int i = m_scriptMove.Count() - 1;

	if (i < 0)
		return -1;

	while (i > 0 && flDist > 1)
	{
		flDist -= m_scriptMove[i].flDist;
		i--;
	}
	return m_scriptMove[i].flElapsedTime;
}


//-----------------------------------------------------------------------------
// Purpose: for the MoveInterval, interpolate desired angle
//-----------------------------------------------------------------------------

float CAI_BlendedMotor::GetMoveScriptYaw( void )
{
	int i;

	// interpolate desired angle
	float flNewYaw = GetAbsAngles().y;
	float t = GetMoveInterval();
	for (i = 0; i < m_scriptTurn.Count()-1; i++)
	{
		if (t < m_scriptTurn[i].flTime)
		{
			// get new direction
			float a = t / m_scriptTurn[i].flTime;
			float deltaYaw = UTIL_AngleDiff( m_scriptTurn[i+1].flYaw, m_scriptTurn[i].flYaw );
			flNewYaw = UTIL_AngleMod( m_scriptTurn[i].flYaw + a * deltaYaw );
			break;
		}
		else
		{
			t -= m_scriptTurn[i].flTime;
		}
	}

	return flNewYaw;
}

//-----------------------------------------------------------------------------
// Purpose: blend in the "idle" or "arrival" animation depending on speed
//-----------------------------------------------------------------------------

void CAI_BlendedMotor::SetMoveScriptAnim( float flNewSpeed )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_SetMoveScriptAnim);

	// don't bother if the npc is dead
	if (!GetOuter()->IsAlive())
		return;

	// insert ideal layers
	// FIXME: needs full transitions, as well as starting vs stopping sequences, leaning, etc.

	CAI_Navigator *pNavigator = GetNavigator();

	SetPlaybackRate( m_flCurrRate );
	// calc weight of idle animation layer that suppresses the run animation
	float flWeight = 0.0f;
	if (GetIdealSpeed() > 0.0f)
	{
		flWeight = 1.0f - (flNewSpeed / (GetIdealSpeed()  * GetPlaybackRate()));
	}
	if (flWeight < 0.0f)
	{
		m_flCurrRate = flNewSpeed / GetIdealSpeed();
		m_flCurrRate = clamp( m_flCurrRate, 0.0f, 1.0f );
		SetPlaybackRate( m_flCurrRate );
		flWeight = 0.0;
	}
	// Msg("weight %.3f rate %.3f\n", flWeight, m_flCurrRate );
	m_flCurrRate = MIN( m_flCurrRate + (1.0 - m_flCurrRate) * 0.8f, 1.0f );

	if (m_nSavedGoalActivity == ACT_INVALID)
	{
		ResetGoalSequence();
	}

	// detect state change
	Activity activity = GetOuter()->NPC_TranslateActivity( m_nSavedGoalActivity );
	if ( activity != m_nSavedTranslatedGoalActivity )
	{
		m_nSavedTranslatedGoalActivity = activity;
		m_nInteriorSequence = ACT_INVALID;
		m_nGoalSequence = pNavigator->GetArrivalSequence( m_nPrimarySequence );
	}

	if (m_bDeceleratingToGoal)
	{
		// find that sequence to play when at goal
		m_nGoalSequence = pNavigator->GetArrivalSequence( m_nPrimarySequence );

		if (m_nGoalSequence == ACT_INVALID)
		{
			m_nGoalSequence = GetInteriorSequence( m_nPrimarySequence );
		}

		Assert( m_nGoalSequence != ACT_INVALID );
	}

	if (m_flSecondaryWeight == 1.0 || (m_iSecondaryLayer != -1 && m_nPrimarySequence == m_nSecondarySequence))
	{
		// secondary layer at full strength last time, delete the primary and shift down
		RemoveLayer( m_iPrimaryLayer, 0.0, 0.0 );

		m_iPrimaryLayer = m_iSecondaryLayer;
		m_nPrimarySequence = m_nSecondarySequence;
		m_iSecondaryLayer = -1;
		m_nSecondarySequence = ACT_INVALID;
		m_flSecondaryWeight = 0.0;
	}

	// look for transition sequence if needed
	if (m_nSecondarySequence == ACT_INVALID)
	{
		if (!m_bDeceleratingToGoal && m_nGoalSequence != GetInteriorSequence( m_nPrimarySequence ))
		{
			// strob interior sequence in case it changed
			m_nGoalSequence = GetInteriorSequence( m_nPrimarySequence );
		}

		if (m_nGoalSequence != ACT_INVALID && m_nPrimarySequence != m_nGoalSequence)
		{
			// Msg("From %s to %s\n", GetOuter()->GetSequenceName( m_nPrimarySequence ), GetOuter()->GetSequenceName( m_nGoalSequence ) );
			m_nSecondarySequence = GetOuter()->FindTransitionSequence(m_nPrimarySequence, m_nGoalSequence, NULL);
			if (m_nSecondarySequence == ACT_INVALID)
				m_nSecondarySequence = m_nGoalSequence;
		}
	}

	// set blending for 
	if (m_nSecondarySequence != ACT_INVALID)
	{
		if (m_iSecondaryLayer == -1)
		{
			m_iSecondaryLayer = AddLayeredSequence( m_nSecondarySequence, 0 );
			SetLayerWeight( m_iSecondaryLayer, 0.0 );
			if (m_nSecondarySequence == m_nGoalSequence)
			{
				SetLayerPlaybackRate( m_iSecondaryLayer, 0.0 );
			}
			else
			{
				SetLayerPlaybackRate( m_iSecondaryLayer, 1.0 );
			}
			SetLayerNoRestore( m_iSecondaryLayer, true );
			m_flSecondaryWeight = 0.0;
		}

		m_flSecondaryWeight = MIN( m_flSecondaryWeight + 0.3, 1.0 );

		if (m_flSecondaryWeight < 1.0)
		{
			SetLayerWeight( m_iPrimaryLayer, (flWeight - m_flSecondaryWeight * flWeight) / (1.0f - m_flSecondaryWeight * flWeight) );
			SetLayerWeight( m_iSecondaryLayer, flWeight * m_flSecondaryWeight );
		}
		else
		{
			SetLayerWeight( m_iPrimaryLayer, 0.0f );
			SetLayerWeight( m_iSecondaryLayer, flWeight );
		}
	}
	else
	{
		// recreate layer if missing
		if (m_iPrimaryLayer == -1)
		{
			MoveContinue();
		}

		// try to catch a stale layer
		if (m_iSecondaryLayer != -1)
		{
			// secondary layer at full strength last time, delete the primary and shift down
			RemoveLayer( m_iSecondaryLayer, 0.0, 0.0 );
			m_iSecondaryLayer = -1;
			m_nSecondarySequence = ACT_INVALID;
			m_flSecondaryWeight = 0.0;
		}

		// debounce
		// flWeight = flWeight * 0.5 + 0.5 * GetOuter()->GetLayerWeight( m_iPrimaryLayer );
		SetLayerWeight( m_iPrimaryLayer, flWeight );
	}
}


//-----------------------------------------------------------------------------
// Purpose: get the "idle" animation to play as the compliment to the movement animation
//-----------------------------------------------------------------------------
int CAI_BlendedMotor::GetInteriorSequence( int fromSequence )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_GetInteriorSequence);

	// FIXME: add interior activity to path, just like arrival activity.
	int  sequence = GetNavigator()->GetMovementSequence();

	if (m_nInteriorSequence != ACT_INVALID && sequence == m_nPrevMovementSequence)
	{
		return m_nInteriorSequence;
	}

	m_nPrevMovementSequence = sequence;

	KeyValues *seqKeyValues = GetOuter()->GetSequenceKeyValues( sequence );
	// Msg("sequence %d : %s (%d)\n", sequence,  GetOuter()->GetSequenceName( sequence ), seqKeyValues != NULL );
	if (seqKeyValues)
	{
		KeyValues *pkvInterior = seqKeyValues->FindKey("interior");
		if (pkvInterior)
		{
			const char *szActivity = pkvInterior->GetString();
		
			Activity activity = ( Activity )GetOuter()->LookupActivity( szActivity );
			if ( activity != ACT_INVALID )
			{
				m_nInteriorSequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ), fromSequence );
			}
			else
			{
				activity = (Activity)GetOuter()->GetActivityID( szActivity );
				if ( activity != ACT_INVALID )
				{
					m_nInteriorSequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ), fromSequence );
				}
			}

			if (activity == ACT_INVALID || m_nInteriorSequence == ACT_INVALID)
			{
				m_nInteriorSequence = GetOuter()->LookupSequence( szActivity );
			}
		}
	}

	if (m_nInteriorSequence == ACT_INVALID)
	{
		Activity activity = GetNavigator()->GetMovementActivity();
		if (activity == ACT_WALK_AIM || activity == ACT_RUN_AIM)
		{
			activity = ACT_IDLE_ANGRY;
		}
		else
		{
			activity = ACT_IDLE;
		}
		m_nInteriorSequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ), fromSequence );

		Assert( m_nInteriorSequence != ACT_INVALID );
	}

	return m_nInteriorSequence;
}




//-----------------------------------------------------------------------------
// Purpose: Move the npc to the next location on its route.
//-----------------------------------------------------------------------------

AIMotorMoveResult_t CAI_BlendedMotor::MoveGroundExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_MoveGroundExecute);

	if ( move.curExpectedDist < 0.001 )
	{
  		AIMotorMoveResult_t result = BaseClass::MoveGroundExecute( move, pTraceResult );
  		// Msg(" BaseClass::MoveGroundExecute() - remaining %.2f\n", GetMoveInterval() );
  		SetMoveScriptAnim( 0.0 );
  		return result;
	}

	BuildMoveScript( move, pTraceResult );

	float flNewSpeed = GetCurSpeed();
	float flTotalDist = GetMoveScriptDist( flNewSpeed );

	Assert( move.maxDist < 0.01 || flTotalDist > 0.0 );

	// --------------------------------------------
	// turn in the direction of movement
	// --------------------------------------------
	
	float flNewYaw = GetMoveScriptYaw( );

	// get facing based on movement yaw
	AILocalMoveGoal_t move2 = move;
	move2.facing = UTIL_YawToVector( flNewYaw );

	// turn in the direction needed
	MoveFacing( move2 );

	// reset actual "sequence" ground speed based current movement sequence, orientation

	// FIXME: this should be based on 

	GetOuter()->m_flGroundSpeed = GetSequenceGroundSpeed( GetSequence());



	/*
	if (1 || flNewSpeed > GetIdealSpeed())
	{
		// DevMsg( "%6.2f : Speed %.1f : %.1f (%.1f) :  %d\n", gpGlobals->curtime, flNewSpeed, move.maxDist, move.transitionDist, GetOuter()->m_pHintNode != NULL );
		// DevMsg( "%6.2f : Speed %.1f : %.1f\n", gpGlobals->curtime, flNewSpeed, GetIdealSpeed() );
	}
	*/

	SetMoveScriptAnim( flNewSpeed );

	/*
	if ((GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
	{
		DevMsg( "%6.2f : Speed %.1f : %.1f : %.2f\n", gpGlobals->curtime, flNewSpeed, GetIdealSpeed(), flNewSpeed / GetIdealSpeed() );
	}
	*/

	AIMotorMoveResult_t result = MoveGroundExecuteWalk( move, flNewSpeed, flTotalDist, pTraceResult );

	return result;

}




AIMotorMoveResult_t CAI_BlendedMotor::MoveFlyExecute( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_MoveFlyExecute);

	if ( move.curExpectedDist < 0.001 )
		return BaseClass::MoveFlyExecute( move, pTraceResult );

	BuildMoveScript( move, pTraceResult );

	float flNewSpeed = GetCurSpeed();
	float flTotalDist = GetMoveScriptDist( flNewSpeed );

	Assert( move.maxDist < 0.01 || flTotalDist > 0.0 );

	// --------------------------------------------
	// turn in the direction of movement
	// --------------------------------------------
	
	float flNewYaw = GetMoveScriptYaw( );

	// get facing based on movement yaw
	AILocalMoveGoal_t move2 = move;
	move2.facing = UTIL_YawToVector( flNewYaw );

	// turn in the direction needed
	MoveFacing( move2 );

	GetOuter()->m_flGroundSpeed = GetSequenceGroundSpeed( GetSequence());

	SetMoveScriptAnim( flNewSpeed );

	// DevMsg( "%6.2f : Speed %.1f : %.1f\n", gpGlobals->curtime, flNewSpeed, GetIdealSpeed() );

	// reset actual "sequence" ground speed based current movement sequence, orientation

	// FIXME: the above is redundant with MoveGroundExecute, and the below is a mix of MoveGroundExecuteWalk and MoveFlyExecute

	bool bReachingLocalGoal = ( flTotalDist > move.maxDist );

	// can I move farther in this interval than I'm supposed to?
	if ( bReachingLocalGoal )
	{
		if ( !(move.flags & AILMG_CONSUME_INTERVAL) )
		{
			// only use a portion of the time interval
			SetMoveInterval( GetMoveInterval() * (1 - move.maxDist / flTotalDist) );
		}
		else
			SetMoveInterval( 0 );
		flTotalDist = move.maxDist;
	}
	else
	{
		// use all the time
		SetMoveInterval( 0 );
	}

	SetMoveVel( move.dir * flNewSpeed );

	// orig
	Vector vecStart, vecEnd;
	vecStart = GetLocalOrigin();
	VectorMA( vecStart, flTotalDist, move.dir, vecEnd );

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_FLY, vecStart, vecEnd, MASK_NPCSOLID, NULL, &moveTrace );
	if ( pTraceResult )
		*pTraceResult = moveTrace;
	
	// Check for total blockage
	if (fabs(moveTrace.flDistObstructed - flTotalDist) <= 1e-1)
	{
		// But if we bumped into our target, then we succeeded!
		if ( move.pMoveTarget && (moveTrace.pObstruction == move.pMoveTarget) )
			return AIM_PARTIAL_HIT_TARGET;

		return AIM_FAILED;
	}

	// The true argument here causes it to touch all triggers
	// in the volume swept from the previous position to the current position
	UTIL_SetOrigin(GetOuter(), moveTrace.vEndPosition, true);

	return (IsMoveBlocked(moveTrace.fStatus)) ? AIM_PARTIAL_HIT_WORLD : AIM_SUCCESS;
}




float CAI_BlendedMotor::OverrideMaxYawSpeed( Activity activity )
{
	// Don't do this is we're locked
	if ( IsYawLocked() )
		return 0.0f;

	switch( activity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 45;
		break;
	default:
		if (GetOuter()->IsMoving())
		{
			return 15;
		}
		return 45; // too fast?
		break;
	}
	return -1;
}



void CAI_BlendedMotor::UpdateYaw( int speed )
{
	// Don't do this is we're locked
	if ( IsYawLocked() )
		return;

	GetOuter()->UpdateTurnGesture( );
	BaseClass::UpdateYaw( speed );
}



void CAI_BlendedMotor::RecalculateYawSpeed() 
{ 
	// Don't do this is we're locked
	if ( IsYawLocked() )
	{
		SetYawSpeed( 0.0f );
		return;
	}

	if (GetOuter()->HasMemory( bits_MEMORY_TURNING ))
		return;

	SetYawSpeed( CalcYawSpeed() ); 
}


//-------------------------------------


void CAI_BlendedMotor::MoveClimbStart(  const Vector &climbDest, const Vector &climbDir, float climbDist, float yaw )
{
	// TODO: merge transitions with movement script
	if (m_iPrimaryLayer != -1)
	{
		SetLayerWeight( m_iPrimaryLayer, 0 );
	}
	if (m_iSecondaryLayer != -1)
	{
		SetLayerWeight( m_iSecondaryLayer, 0 );
	}

	BaseClass::MoveClimbStart( climbDest, climbDir, climbDist, yaw );
}


//-------------------------------------


void CAI_BlendedMotor::MoveJumpStart( const Vector &velocity )
{
	// TODO: merge transitions with movement script
	if (m_iPrimaryLayer != -1)
	{
		SetLayerWeight( m_iPrimaryLayer, 0 );
	}
	if (m_iSecondaryLayer != -1)
	{
		SetLayerWeight( m_iSecondaryLayer, 0 );
	}

	BaseClass::MoveJumpStart( velocity );
}


//-------------------------------------

void CAI_BlendedMotor::BuildMoveScript( const AILocalMoveGoal_t &move, AIMoveTrace_t *pTraceResult )
{
	m_scriptMove.RemoveAll();
	m_scriptTurn.RemoveAll();

	BuildVelocityScript( move );
	BuildTurnScript( move );

/*
	if (GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT)
	{
		int i;
#if 1

		for (i = 1; i < m_scriptMove.Count(); i++)
		{
			NDebugOverlay::Line( m_scriptMove[i-1].vecLocation, m_scriptMove[i].vecLocation, 255,255,255, true, 0.1 );

			NDebugOverlay::Box( m_scriptMove[i].vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0,255,255, 0, 0.1 );

			//NDebugOverlay::Line( m_scriptMove[i].vecLocation, m_scriptMove[i].vecLocation + Vector( 0,0,m_scriptMove[i].flMaxVelocity), 0,255,255, true, 0.1 );

			Vector vecMidway = m_scriptMove[i].vecLocation + ((m_scriptMove[i-1].vecLocation - m_scriptMove[i].vecLocation) * 0.5);
			NDebugOverlay::Text( vecMidway, UTIL_VarArgs( "%d", i ), false, 0.1 );
		}
#endif
#if 0
		for (i = 1; i < m_scriptTurn.Count(); i++)
		{
			NDebugOverlay::Line( m_scriptTurn[i-1].vecLocation, m_scriptTurn[i].vecLocation, 255,255,255, true, 0.1 );

			NDebugOverlay::Box( m_scriptTurn[i].vecLocation, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255,0,0, 0, 0.1 );

			NDebugOverlay::Line( m_scriptTurn[i].vecLocation + Vector( 0,0,1), m_scriptTurn[i].vecLocation + Vector( 0,0,1) + UTIL_YawToVector( m_scriptTurn[i].flYaw ) * 32, 255,0,0, true, 0.1 );
		}
#endif
	}
*/
}	


#define YAWSPEED	150


void CAI_BlendedMotor::BuildTurnScript( const AILocalMoveGoal_t &move  )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_BuildTurnScript);

	int i;

	AI_Movementscript_t script;
	script.Init();

	// current location
	script.vecLocation = GetAbsOrigin();
	script.flYaw = GetAbsAngles().y;
	m_scriptTurn.AddToTail( script );

	//-------------------------

	// insert default turn parameters, try to turn 80% to goal at all corners before getting there
	int prev = 0;
	for (i = 0; i < m_scriptMove.Count(); i++)
	{
		AI_Waypoint_t *pCurWaypoint = m_scriptMove[i].pWaypoint;
		if (pCurWaypoint)
		{
			script.Init();
			script.vecLocation = pCurWaypoint->vecLocation;
			script.pWaypoint = pCurWaypoint;
			script.flElapsedTime = m_scriptMove[i].flElapsedTime;

			m_scriptTurn[prev].flTime = script.flElapsedTime - m_scriptTurn[prev].flElapsedTime;

			if (pCurWaypoint->GetNext())
			{
				Vector d1 = pCurWaypoint->GetNext()->vecLocation - script.vecLocation;
				Vector d2 = script.vecLocation - m_scriptTurn[prev].vecLocation;
				
				d1.z = 0;
				VectorNormalize( d1 );
				d2.z = 0;
				VectorNormalize( d2 );

				float y1 = UTIL_VecToYaw( d1 );
				float y2 = UTIL_VecToYaw( d2 );

				float deltaYaw = fabs( UTIL_AngleDiff( y1, y2 ) );

				if (deltaYaw > 0.1)
				{
					// turn to 80% of goal
					script.flYaw = UTIL_ApproachAngle( y1, y2, deltaYaw * 0.8 );
					m_scriptTurn.AddToTail( script );
					// DevMsg("turn waypoint %.1f %.1f %.1f\n", script.vecLocation.x, script.vecLocation.y, script.vecLocation.z );
					prev++;
				}
			}
			else
			{
				Vector vecDir = GetNavigator()->GetArrivalDirection();
				script.flYaw = UTIL_VecToYaw( vecDir );
				m_scriptTurn.AddToTail( script );
				// DevMsg("turn waypoint %.1f %.1f %.1f\n", script.vecLocation.x, script.vecLocation.y, script.vecLocation.z );
				prev++;
			}
		}
	}

	// propagate ending facing back over any nearby nodes
	// FIXME: this needs to minimize total turning, not just local/end turning.
	// depending on waypoint spacing, complexity, it may turn the wrong way!
	for (i = m_scriptTurn.Count()-1; i > 1; i--)
	{
		float deltaYaw = UTIL_AngleDiff( m_scriptTurn[i-1].flYaw, m_scriptTurn[i].flYaw );
	
		float maxYaw = YAWSPEED * m_scriptTurn[i-1].flTime;

		if (fabs(deltaYaw) > maxYaw)
		{
			m_scriptTurn[i-1].flYaw = UTIL_ApproachAngle( m_scriptTurn[i-1].flYaw, m_scriptTurn[i].flYaw, maxYaw );
		}
	}

	for (i = 0; i < m_scriptTurn.Count() - 1; )
	{
		i = i + BuildTurnScript( i, i + 1 ) + 1;
	}
	//-------------------------
}



int CAI_BlendedMotor::BuildTurnScript( int i, int j )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_BuildTurnScript2);

	int k;

	Vector vecDir = m_scriptTurn[j].vecLocation - m_scriptTurn[i].vecLocation;
	float interiorYaw = UTIL_VecToYaw( vecDir );

	float deltaYaw;

	deltaYaw = fabs( UTIL_AngleDiff( interiorYaw, m_scriptTurn[i].flYaw ) );
	float t1 = deltaYaw / YAWSPEED;

	deltaYaw = fabs( UTIL_AngleDiff( m_scriptTurn[j].flYaw, interiorYaw ) );
	float t2 = deltaYaw / YAWSPEED;

	float totalTime = m_scriptTurn[j].flElapsedTime - m_scriptTurn[i].flElapsedTime;

	Assert( totalTime >  0 );

	if (t1 < 0.01)
	{
		if (t2 > totalTime * 0.8)
		{
			// too close, nothing to do
			return 0;
		}

		// go ahead and force yaw
		m_scriptTurn[i].flYaw = interiorYaw;

		// we're already aiming close enough to the interior yaw, set the point where we need to blend out
		k = BuildInsertNode( i, totalTime - t2 );
		m_scriptTurn[k].flYaw = interiorYaw;

		return 1;
	}
	else if (t2 < 0.01)
	{
		if (t1 > totalTime * 0.8)
		{
			// too close, nothing to do
			return 0;
		}

 		// we'll finish up aiming close enough to the interior yaw, set the point where we need to blend in
		k = BuildInsertNode( i, t1 );
		m_scriptTurn[k].flYaw = interiorYaw;
		
		return 1;
	}
	else if (t1 + t2 > totalTime)
	{
		// don't bother with interior node
		return 0;
		
		// waypoints need to much turning, ignore interior yaw
		float a = (t1 / (t1 + t2));
		t1 = a * totalTime;

		k = BuildInsertNode( i, t1 );

		deltaYaw = UTIL_AngleDiff( m_scriptTurn[j].flYaw, m_scriptTurn[i].flYaw );
		m_scriptTurn[k].flYaw = UTIL_ApproachAngle( m_scriptTurn[j].flYaw, m_scriptTurn[i].flYaw, deltaYaw * (1 - a) );

		return 1;
	}
	else if (t1 + t2 < totalTime * 0.8)
	{
		// turn to face interior, run a ways, then turn away
		k = BuildInsertNode( i, t1 );
		m_scriptTurn[k].flYaw = interiorYaw;

		k = BuildInsertNode( i, t2 );
		m_scriptTurn[k].flYaw = interiorYaw;

		return 2;
	}
	return 0;
}


int CAI_BlendedMotor::BuildInsertNode( int i, float flTime )
{
	AI_Movementscript_t script;
	script.Init();

	Assert( flTime > 0.0 );

	for (i; i < m_scriptTurn.Count() - 1; i++)
	{
		if (m_scriptTurn[i].flTime < flTime)
		{
			flTime -= m_scriptTurn[i].flTime;
		}
		else
		{
			float a = flTime / m_scriptTurn[i].flTime;

			script.flTime = (m_scriptTurn[i].flTime - flTime);

			m_scriptTurn[i].flTime = flTime;

			script.flElapsedTime = m_scriptTurn[i].flElapsedTime * (1 - a) + m_scriptTurn[i+1].flElapsedTime * a;

			script.vecLocation = m_scriptTurn[i].vecLocation * (1 - a) + m_scriptTurn[i+1].vecLocation * a;

			m_scriptTurn.InsertAfter( i, script );

			return i + 1;
		}
	}
	Assert( 0 );
	return 0;
}


ConVar ai_path_insert_pause_at_obstruction( "ai_path_insert_pause_at_obstruction", "1" );
ConVar ai_path_adjust_speed_on_immediate_turns( "ai_path_adjust_speed_on_immediate_turns", "1" );
ConVar ai_path_insert_pause_at_est_end( "ai_path_insert_pause_at_est_end", "1" );

#define MIN_VELOCITY 0.0f
#define MIN_STEER_DOT 0.0f

void CAI_BlendedMotor::BuildVelocityScript( const AILocalMoveGoal_t &move )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_BuildVelocityScript);

	int i;
	float a;

	float idealVelocity = GetIdealSpeed();
	if (idealVelocity == 0)
	{
		idealVelocity = 50;
	}

	float idealAccel = GetIdealAccel();
	if (idealAccel == 0)
	{
		idealAccel = 100;
	}

	AI_Movementscript_t script;

	// set current location as start of script
	script.vecLocation = GetAbsOrigin();
	script.flMaxVelocity = GetCurSpeed();
	m_scriptMove.AddToTail( script );

	//-------------------------

	extern ConVar npc_height_adjust;
	if (npc_height_adjust.GetBool() && move.bHasTraced && move.directTrace.flTotalDist != move.thinkTrace.flTotalDist)
	{
		float flDist = (move.directTrace.vEndPosition - m_scriptMove[0].vecLocation).Length2D();
		float flHeight = move.directTrace.vEndPosition.z - m_scriptMove[0].vecLocation.z;
		float flDelta;

		if (flDist > 0)
		{
			flDelta = flHeight / flDist;
		}
		else
		{
			flDelta = 0;
		}

		m_flPredictiveSpeedAdjust = 1.1 - fabs( flDelta );
		m_flPredictiveSpeedAdjust = clamp( m_flPredictiveSpeedAdjust, (flHeight > 0.0f) ? 0.5f : 0.8f, 1.0f );

		/*
		if ((GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			Msg("m_flPredictiveSpeedAdjust %.3f  %.1f %.1f\n", m_flPredictiveSpeedAdjust, flHeight, flDist );
			NDebugOverlay::Box( move.directTrace.vEndPosition, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 0,255,255, 0, 0.12 );
		}
		*/
	}
	if (npc_height_adjust.GetBool())
	{
		float flDist = (move.thinkTrace.vEndPosition - m_vecPrevOrigin2).Length2D();
		float flHeight = move.thinkTrace.vEndPosition.z - m_vecPrevOrigin2.z;
		float flDelta;

		if (flDist > 0)
		{
			flDelta = flHeight / flDist;
		}
		else
		{
			flDelta = 0;
		}

		float newSpeedAdjust = 1.1 - fabs( flDelta );
		newSpeedAdjust = clamp( newSpeedAdjust, (flHeight > 0.0f) ? 0.5f : 0.8f, 1.0f );

		// debounce speed adjust
		if (newSpeedAdjust < m_flReactiveSpeedAdjust)
		{
			m_flReactiveSpeedAdjust = m_flReactiveSpeedAdjust * 0.2f + newSpeedAdjust * 0.8f;
		}
		else
		{
			m_flReactiveSpeedAdjust = m_flReactiveSpeedAdjust * 0.5f + newSpeedAdjust * 0.5f;
		}

		// filter through origins
		m_vecPrevOrigin2 = m_vecPrevOrigin1;
		m_vecPrevOrigin1 = GetAbsOrigin();

		/*
		if ((GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			NDebugOverlay::Box( m_vecPrevOrigin2, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255,0,255, 0, 0.12 );
			NDebugOverlay::Box( move.thinkTrace.vEndPosition, Vector( -2, -2, -2 ), Vector( 2, 2, 2 ), 255,0,255, 0, 0.12 );
			Msg("m_flReactiveSpeedAdjust %.3f  %.1f %.1f\n", m_flReactiveSpeedAdjust, flHeight, flDist );
		}
		*/
	}

	idealVelocity = idealVelocity * MIN( m_flReactiveSpeedAdjust, m_flPredictiveSpeedAdjust );

	//-------------------------

	bool bAddedExpected = false;

	// add all waypoint locations and velocities
	AI_Waypoint_t *pCurWaypoint = GetNavigator()->GetPath()->GetCurWaypoint();

	// there has to be at least one waypoint
	Assert( pCurWaypoint );

	while (pCurWaypoint && (pCurWaypoint->NavType() == NAV_GROUND || pCurWaypoint->NavType() == NAV_FLY) /*&& flTotalDist / idealVelocity < 3.0*/) // limit lookahead to 3 seconds
	{
		script.Init();
		AI_Waypoint_t *pNext = pCurWaypoint->GetNext();

		if (ai_path_adjust_speed_on_immediate_turns.GetBool() && !bAddedExpected)
		{
			// hack in next expected immediate location for move
			script.vecLocation = GetAbsOrigin() + move.dir * move.curExpectedDist;
			bAddedExpected = true;
			pNext = pCurWaypoint;
		}
		else
		{
			script.vecLocation = pCurWaypoint->vecLocation;
			script.pWaypoint = pCurWaypoint;
		}

		//DevMsg("waypoint %.1f %.1f %.1f\n", script.vecLocation.x, script.vecLocation.y, script.vecLocation.z );

		if (pNext)
		{
			switch( pNext->NavType())
			{
			case NAV_GROUND:
			case NAV_FLY:
				{
					Vector d1 = pNext->vecLocation - script.vecLocation;
					Vector d2 = script.vecLocation - m_scriptMove[m_scriptMove.Count()-1].vecLocation;
					
					// remove very short, non terminal ground links
					// FIXME: is this safe?  Maybe just check for co-located ground points?
					if (d1.Length2D() < 1.0)
					{
						/*
						if (m_scriptMove.Count() > 1)
						{
							int i = m_scriptMove.Count() - 1;
							m_scriptMove[i].vecLocation = pCurWaypoint->vecLocation;
							m_scriptMove[i].pWaypoint = pCurWaypoint;
						}
						*/
						pCurWaypoint = pNext;
						continue;
					}

					d1.z = 0;
					VectorNormalize( d1 );
					d2.z = 0;
					VectorNormalize( d2 );

					// figure velocity
					float dot = (DotProduct( d1, d2 ) + 0.2);
					if (dot > 0)
					{
						dot = clamp( dot, 0.0f, 1.0f );
						script.flMaxVelocity = idealVelocity * dot;
					}
					else
					{
						script.flMaxVelocity = 0;
					}
				}
				break;
			case NAV_JUMP:

				// FIXME: information about what the jump should look like isn't stored in the waypoints
				// this'll need to call 
				//    GetMoveProbe()->MoveLimit( NAV_JUMP, GetLocalOrigin(), GetPath()->CurWaypointPos(), MASK_NPCSOLID, GetNavTargetEntity(), &moveTrace );
				// to get how far/fast the jump will be, but this is also stateless, so it'd call it per frame.
				// So far it's not clear that the moveprobe doesn't also call this.....

				{
					float minJumpHeight = 0;
					float maxHorzVel = MAX( GetCurSpeed(), 100 );
					float gravity = GetCurrentGravity() * GetOuter()->GetGravity();
					Vector vecApex;
					Vector rawJumpVel = GetMoveProbe()->CalcJumpLaunchVelocity(script.vecLocation, pNext->vecLocation, gravity, &minJumpHeight, maxHorzVel, &vecApex );

					script.flMaxVelocity = rawJumpVel.Length2D();
					// Msg("%.1f\n", script.flMaxVelocity );
				}
				break;
			case NAV_CLIMB:
				{
					/*
					CAI_Node *pClimbNode = GetNavigator()->GetNetwork()->GetNode(pNext->iNodeID);

					check: pClimbNode->m_eNodeInfo
						bits_NODE_CLIMB_BOTTOM, 
						bits_NODE_CLIMB_ON, 
						bits_NODE_CLIMB_OFF_FORWARD, 
						bits_NODE_CLIMB_OFF_LEFT, 
						bits_NODE_CLIMB_OFF_RIGHT
					*/

					script.flMaxVelocity = 0;
				}
				break;
			/*
			case NAV_FLY:
				// FIXME: can there be a NAV_GROUND -> NAV_FLY transition?
				script.flMaxVelocity = 0;
				break;
			*/
			}
		}
		else
		{
			script.flMaxVelocity = GetNavigator()->GetArrivalSpeed();
			// Assert( script.flMaxVelocity == 0 );
		}

		m_scriptMove.AddToTail( script );
		pCurWaypoint = pNext;
	}


	//-------------------------

	// update distances
	float flTotalDist = 0;
	for (i = 0; i < m_scriptMove.Count() - 1; i++ )
	{
		flTotalDist += m_scriptMove[i].flDist = (m_scriptMove[i+1].vecLocation - m_scriptMove[i].vecLocation).Length2D();
	}

	//-------------------------

	if ( !m_bDeceleratingToGoal && m_scriptMove.Count() && flTotalDist > 0 )
	{
		float flNeededAccel = DeltaV( m_scriptMove[0].flMaxVelocity, m_scriptMove[m_scriptMove.Count() - 1].flMaxVelocity, flTotalDist );
		m_bDeceleratingToGoal =  (flNeededAccel < -idealAccel);
		//Assert( flNeededAccel != idealAccel);
	}

	//-------------------------

	// insert slowdown points due to blocking
	if (ai_path_insert_pause_at_obstruction.GetBool() && move.directTrace.pObstruction)
	{
		float distToObstruction = (move.directTrace.vEndPosition - m_scriptMove[0].vecLocation).Length2D();

		// HACK move obstruction out "stepsize" to account for it being based on stand position and not a trace
		distToObstruction = distToObstruction + 16;

		InsertSlowdown( distToObstruction, idealAccel, false );
	}

	if (ai_path_insert_pause_at_est_end.GetBool() && GetNavigator()->GetArrivalDistance() > 0.0)
	{
		InsertSlowdown( flTotalDist - GetNavigator()->GetArrivalDistance(), idealAccel, true );
	}

	// calc initial velocity based on immediate direction changes
	if ( ai_path_adjust_speed_on_immediate_turns.GetBool() && m_scriptMove.Count() > 1)
	{
		/*
		if ((GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT))
		{
			Vector tmp = m_scriptMove[1].vecLocation - m_scriptMove[0].vecLocation;
			VectorNormalize( tmp );
			NDebugOverlay::Line( m_scriptMove[0].vecLocation + Vector( 0, 0, 10 ), m_scriptMove[0].vecLocation + tmp * 32 + Vector( 0, 0, 10 ), 255,255,255, true, 0.1 );
			
			NDebugOverlay::Line( m_scriptMove[0].vecLocation + Vector( 0, 0, 10 ), m_scriptMove[1].vecLocation + Vector( 0, 0, 10 ), 255,0,0, true, 0.1 );

			tmp = GetCurVel();
			VectorNormalize( tmp );
			NDebugOverlay::Line( m_scriptMove[0].vecLocation + Vector( 0, 0, 10 ), m_scriptMove[0].vecLocation + tmp * 32 + Vector( 0, 0, 10 ), 0,0,255, true, 0.1 );
		}
		*/

		Vector d1 = m_scriptMove[1].vecLocation - m_scriptMove[0].vecLocation;
		d1.z = 0;
		VectorNormalize( d1 );

		Vector d2 = GetCurVel();
		d2.z = 0;
		VectorNormalize( d2 );

		float dot = (DotProduct( d1, d2 ) + MIN_STEER_DOT);
		dot = clamp( dot, 0.0f, 1.0f );
		m_scriptMove[0].flMaxVelocity = m_scriptMove[0].flMaxVelocity * dot;
	}

	// clamp forward velocities
	for (i = 0; i < m_scriptMove.Count() - 1; i++ )
	{
		// find needed acceleration
		float dv = m_scriptMove[i+1].flMaxVelocity - m_scriptMove[i].flMaxVelocity;

		if (dv > 0.0)
		{
			// find time, distance to accel to next max vel
			float t1 = dv / idealAccel;
			float d1 = m_scriptMove[i].flMaxVelocity * t1 + 0.5 * (idealAccel) * t1 * t1;

			// is there enough distance
			if (d1 > m_scriptMove[i].flDist)
			{
				float r1, r2;

				// clamp the next velocity to the possible accel in the given distance
				if (SolveQuadratic( 0.5 * idealAccel, m_scriptMove[i].flMaxVelocity, -m_scriptMove[i].flDist, r1, r2 ))
				{
					m_scriptMove[i+1].flMaxVelocity = m_scriptMove[i].flMaxVelocity + idealAccel * r1;
				}
			}
		}
	}

	// clamp decel velocities
	for (i = m_scriptMove.Count() - 1; i > 0; i-- )
	{
		// find needed deceleration
		float dv = m_scriptMove[i].flMaxVelocity - m_scriptMove[i-1].flMaxVelocity;

		if (dv < 0.0)
		{
			// find time, distance to decal to next max vel
			float t1 = -dv / idealAccel;
			float d1 = m_scriptMove[i].flMaxVelocity * t1 + 0.5 * (idealAccel) * t1 * t1;

			// is there enough distance
			if (d1 > m_scriptMove[i-1].flDist)
			{
				float r1, r2;
				
				// clamp the next velocity to the possible decal in the given distance
				if (SolveQuadratic( 0.5 * idealAccel, m_scriptMove[i].flMaxVelocity, -m_scriptMove[i-1].flDist, r1, r2 ))
				{
					m_scriptMove[i-1].flMaxVelocity = m_scriptMove[i].flMaxVelocity + idealAccel * r1;
				}
			}
		}
	}

	/*
	for (i = 0; i < m_scriptMove.Count(); i++)
	{
		NDebugOverlay::Text( m_scriptMove[i].vecLocation, (const char *)CFmtStr( "%.2f ", m_scriptMove[i].flMaxVelocity  ), false, 0.1 );
		// DevMsg("%.2f ", m_scriptMove[i].flMaxVelocity );
	}
	// DevMsg("\n");
	*/

	// insert intermediate ideal velocities
	for (i = 0; i < m_scriptMove.Count() - 1;)
	{
		// accel to ideal
		float t1 = (idealVelocity - m_scriptMove[i].flMaxVelocity) / idealAccel;
		float d1 = m_scriptMove[i].flMaxVelocity * t1 + 0.5 * (idealAccel) * t1 * t1;

		// decel from ideal
		float t2 = (idealVelocity - m_scriptMove[i+1].flMaxVelocity) / idealAccel;
		float d2 = m_scriptMove[i+1].flMaxVelocity * t2 + 0.5 * (idealAccel) * t2 * t2;

		m_scriptMove[i].flDist = (m_scriptMove[i+1].vecLocation - m_scriptMove[i].vecLocation).Length2D();

		// is it possible to accel and decal to idealVelocity between next two nodes
		if (d1 + d2 < m_scriptMove[i].flDist)
		{
			Vector start =  m_scriptMove[i].vecLocation;
			Vector end = m_scriptMove[i+1].vecLocation;
			float dist = m_scriptMove[i].flDist;

			// insert the two points needed to end accel and start decel
			if (d1 > 1.0 && t1 > 0.1)
			{
				a = d1 / dist;

				script.Init();
				script.vecLocation = end * a + start * (1 - a);
				script.flMaxVelocity = idealVelocity;
				m_scriptMove.InsertAfter( i, script );
				i++;
			}

			if (dist - d2 > 1.0 && t2 > 0.1)
			{
				// DevMsg("%.2f : ", a );

				a = (dist - d2) / dist;

				script.Init();
				script.vecLocation = end * a + start * (1 - a);
				script.flMaxVelocity = idealVelocity;
				m_scriptMove.InsertAfter( i, script );
				i++;
			}

			i++;
		}
		else
		{
			// check to see if the amount of change needed to reach target is less than the ideal acceleration
			float flNeededAccel = fabs( DeltaV( m_scriptMove[i].flMaxVelocity, m_scriptMove[i+1].flMaxVelocity, m_scriptMove[i].flDist ) );
			if (flNeededAccel < idealAccel)
			{
				// if so, they it's possible to get a bit towards the ideal velocity
				float v1 = m_scriptMove[i].flMaxVelocity;
				float v2 = m_scriptMove[i+1].flMaxVelocity;
				float dist = m_scriptMove[i].flDist;

				// based on solving:
				//		v1+A*t1-v2-A*t2=0
				//		v1*t1+0.5*A*t1*t1+v2*t2+0.5*A*t2*t2-D=0

				float tmp = idealAccel*dist+0.5*v1*v1+0.5*v2*v2;
				Assert( tmp >= 0 );
				t1 = (-v1+sqrt( tmp )) / idealAccel;
				t2 = (v1+idealAccel*t1-v2)/idealAccel;

				// if this assert hits, write down the v1, v2, dist, and idealAccel numbers and send them to me (Ken).
				// go ahead the comment it out, it's safe, but I'd like to know a test case where it's happening
				//Assert( t1 > 0 && t2 > 0 );

				// check to make sure it's really worth it
				if (t1 > 0.0 && t2 > 0.0)
				{
					d1 = v1 * t1 + 0.5 * idealAccel * t1 * t1;
					
					/*
					d2 = v2 * t2 + 0.5 * idealAccel * t2 * t2;
					Assert( fabs( d1 + d2 - dist ) < 0.001 );
					*/

					float a = d1 / m_scriptMove[i].flDist;
					script.Init();
					script.vecLocation = m_scriptMove[i+1].vecLocation * a + m_scriptMove[i].vecLocation * (1 - a);
					script.flMaxVelocity = m_scriptMove[i].flMaxVelocity + idealAccel * t1;

					if (script.flMaxVelocity < idealVelocity)
					{
						// DevMsg("insert %.2f %.2f %.2f\n", m_scriptMove[i].flMaxVelocity, script.flMaxVelocity, m_scriptMove[i+1].flMaxVelocity ); 
						m_scriptMove.InsertAfter( i, script );
						i += 1;
					}
				}
			}
			i += 1;
		}
	}

	// clamp min velocities
	for (i = 0; i < m_scriptMove.Count(); i++)
	{
		m_scriptMove[i].flMaxVelocity = MAX( m_scriptMove[i].flMaxVelocity, MIN_VELOCITY );
	}

	// rebuild fields
	m_scriptMove[0].flElapsedTime = 0;
	for (i = 0; i < m_scriptMove.Count() - 1; )
	{
		m_scriptMove[i].flDist = (m_scriptMove[i+1].vecLocation - m_scriptMove[i].vecLocation).Length2D();

		if (m_scriptMove[i].flMaxVelocity == 0 && m_scriptMove[i+1].flMaxVelocity == 0)
		{
			// force a minimum velocity 
			Assert( 0 );
			m_scriptMove[i+1].flMaxVelocity = 1.0;
		}

		float t = m_scriptMove[i].flDist / (0.5 * (m_scriptMove[i].flMaxVelocity + m_scriptMove[i+1].flMaxVelocity));
		m_scriptMove[i].flTime = t;

		/*
		if (m_scriptMove[i].flDist < 0.01)
		{
			// Assert( m_scriptMove[i+1].pWaypoint == NULL );

			m_scriptMove.Remove( i + 1 );
			continue;
		}
		*/

		m_scriptMove[i+1].flElapsedTime = m_scriptMove[i].flElapsedTime + m_scriptMove[i].flTime;

		i++;
	}

	/*
	for (i = 0; i < m_scriptMove.Count(); i++)
	{
		DevMsg("(%.2f : %.2f : %.2f)", m_scriptMove[i].flMaxVelocity, m_scriptMove[i].flDist, m_scriptMove[i].flTime );
		// DevMsg("(%.2f:%.2f)", m_scriptMove[i].flTime, m_scriptMove[i].flElapsedTime );
	}
	DevMsg("\n");
	*/
}



void CAI_BlendedMotor::InsertSlowdown( float distToObstruction, float idealAccel, bool bAlwaysSlowdown )
{
	int i;
	AI_Movementscript_t script;

	if (distToObstruction <= 0.0)
		return;

	for (i = 0; i <  m_scriptMove.Count() - 1; i++)
	{
		if (m_scriptMove[i].flDist > 0 && distToObstruction - m_scriptMove[i].flDist < 0)
		{
			float a = distToObstruction / m_scriptMove[i].flDist;
			Assert( a >= 0 && a <= 1);
			script.vecLocation = (1 - a) * m_scriptMove[i].vecLocation + a * m_scriptMove[i+1].vecLocation;

			//NDebugOverlay::Line( m_scriptMove[i].vecLocation + Vector( 0, 0, 5 ), script.vecLocation + Vector( 0, 0, 5 ), 0,255,0, true, 0.1 );
			//NDebugOverlay::Line( script.vecLocation + Vector( 0, 0, 5 ), m_scriptMove[i+1].vecLocation + Vector( 0, 0, 5 ), 0,0,255, true, 0.1 );

			float r1, r2;

			// clamp the next velocity to the possible accel in the given distance
			if (!bAlwaysSlowdown && SolveQuadratic( -0.5 * idealAccel, m_scriptMove[0].flMaxVelocity, -distToObstruction, r1, r2 ))
			{
				script.flMaxVelocity = MAX( 10, m_scriptMove[0].flMaxVelocity - idealAccel * r1 );
			}
			else
			{
				script.flMaxVelocity = 10.0;
			}

			script.flMaxVelocity = 1.0; // as much as reasonable
			script.pWaypoint = NULL;
			script.flDist = m_scriptMove[i].flDist - distToObstruction;
			m_scriptMove[i].flDist = distToObstruction;
			m_scriptMove.InsertAfter( i, script );
			break;
		}
		else
		{
			distToObstruction -= m_scriptMove[i].flDist;
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: issues turn gestures when it detects that the body has turned but the feet haven't compensated
//-----------------------------------------------------------------------------


void CAI_BlendedMotor::MaintainTurnActivity( void )
{
	AI_PROFILE_SCOPE(CAI_BlendedMotor_MaintainTurnActivity);

	if (m_flNextTurnGesture > gpGlobals->curtime || m_flNextTurnAct > gpGlobals->curtime || GetOuter()->IsMoving() )
	{
		// clear out turn detection if currently turing or moving
		m_doTurn = m_doRight = m_doLeft = 0;
		if ( GetOuter()->IsMoving())
		{
			m_flNextTurnAct = gpGlobals->curtime + 0.3;
		}
	}
	else 
	{
		// detect undirected turns
		if (m_prevYaw != GetAbsAngles().y)
		{
			float diff = UTIL_AngleDiff( m_prevYaw, GetAbsAngles().y );
			if (diff < 0.0)
			{
				m_doLeft += -diff;
			}
			else
			{
				m_doRight += diff;
			}
			m_prevYaw = GetAbsAngles().y;
		}
		// accumulate turn angle, delay response for short turns
		m_doTurn += m_doRight + m_doLeft;
		// accumulate random foot stick clearing
		m_doTurn += random->RandomFloat( 0.4, 0.6 );
	}

	if (m_doTurn > 15.0f)
	{
		// mostly a foot stick clear
		int iSeq = ACT_INVALID;
		if (m_doLeft > m_doRight)
		{
			iSeq = SelectWeightedSequence( ACT_GESTURE_TURN_LEFT );
		}
		else
		{
			iSeq = SelectWeightedSequence( ACT_GESTURE_TURN_RIGHT );
		}
		m_doLeft = 0;
		m_doRight = 0;

		if (iSeq != ACT_INVALID)
		{
			int iLayer = GetOuter()->AddGestureSequence( iSeq );
			if (iLayer != -1)
			{
				GetOuter()->SetLayerPriority( iLayer, 100 );
				// increase speed if we're getting behind or they're turning quickly
				float rate = random->RandomFloat( 0.8, 1.2 );
				if (m_doTurn > 90.0)
				{
					rate *= 1.5;
				}
				GetOuter()->SetLayerPlaybackRate( iLayer, rate );
				// disable turing for the duration of the gesture
				m_flNextTurnAct = gpGlobals->curtime + GetOuter()->GetLayerDuration( iLayer );
			}
			else
			{
				// too many active gestures, try again in half a second
				m_flNextTurnAct = gpGlobals->curtime + 0.3;
			}
		}
		m_doTurn = m_doRight = m_doLeft = 0;
	}
}

ConVar scene_flatturn( "scene_flatturn", "1" );

bool CAI_BlendedMotor::AddTurnGesture( float flYD )
{

	// some funky bug with human turn gestures, disable for now
	return false;

	// try using a turn gesture
	Activity activity = ACT_INVALID;
	float weight = 1.0;
	float turnCompletion = 1.0;

	if (m_flNextTurnGesture > gpGlobals->curtime)
	{
		/*
		if ( GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT )
		{
			Msg( "%.1f : [ %.2f ]\n", flYD, m_flNextTurnAct - gpGlobals->curtime );
		}
		*/
		return false;
	}

	if ( GetOuter()->IsMoving() || GetOuter()->IsCrouching() )
	{
		return false;
	}

	if (fabs( flYD ) < 15)
	{
		return false;
	}
	else if (flYD < -45)
	{
		activity = ACT_GESTURE_TURN_RIGHT90;
		weight = flYD / -90;
		turnCompletion = 0.36;
	}
	else if (flYD < 0)
	{
		activity = ACT_GESTURE_TURN_RIGHT45;
		weight = flYD / -45;
		turnCompletion = 0.4;
	}
	else if (flYD <= 45)
	{
		activity = ACT_GESTURE_TURN_LEFT45;
		weight = flYD / 45;
		turnCompletion = 0.4;
	}
	else
	{
		activity = ACT_GESTURE_TURN_LEFT90;
		weight = flYD / 90;
		turnCompletion = 0.36;
	}

	int seq = SelectWeightedSequence( activity );

	if (scene_flatturn.GetBool() && GetOuter()->IsCurSchedule( SCHED_SCENE_GENERIC ))
	{
		Activity flatactivity = activity;

		if (activity == ACT_GESTURE_TURN_RIGHT90)
		{
			flatactivity = ACT_GESTURE_TURN_RIGHT90_FLAT;
		}
		else if (activity == ACT_GESTURE_TURN_RIGHT45)
		{
			flatactivity = ACT_GESTURE_TURN_RIGHT45_FLAT;
		}
		else if (activity == ACT_GESTURE_TURN_LEFT90)
		{
			flatactivity = ACT_GESTURE_TURN_LEFT90_FLAT;
		}
		else if (activity == ACT_GESTURE_TURN_LEFT45)
		{
			flatactivity = ACT_GESTURE_TURN_LEFT45_FLAT;
		}

		if (flatactivity != activity)
		{
			int newseq = SelectWeightedSequence( flatactivity );
			if (newseq != ACTIVITY_NOT_AVAILABLE)
			{
				seq = newseq;
			}
		}
	}

	if (seq != ACTIVITY_NOT_AVAILABLE)
	{
		int iLayer = GetOuter()->AddGestureSequence( seq );
		if (iLayer != -1)
		{
			GetOuter()->SetLayerPriority( iLayer, 100 );
			// vary the playback a bit
			SetLayerPlaybackRate( iLayer, 1.0 );
			float actualDuration = GetOuter()->GetLayerDuration( iLayer );

			float rate = random->RandomFloat( 0.5f, 1.1f );
			float diff = fabs( flYD );
			float speed = (diff / (turnCompletion * actualDuration / rate)) * 0.1f;

			speed = clamp( speed, 15.f, 35.f );
			speed = MIN( speed, diff );

			actualDuration = (diff / (turnCompletion * speed)) * 0.1 ;

			GetOuter()->SetLayerDuration( iLayer, actualDuration );

			SetLayerWeight( iLayer, weight );

			SetYawSpeed( speed );

			Remember( bits_MEMORY_TURNING );

			// don't overlap the turn portion of the gestures, and don't play them too often
			m_flNextTurnGesture = gpGlobals->curtime + MAX( turnCompletion * actualDuration, 0.3 );

			/*
			if ( GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT )
			{
				Msg( "%.1f : %.2f %.2f : %.2f (%.2f)\n", flYD, weight, speed, actualDuration, turnCompletion * actualDuration );
			}
			*/
			return true;
		}
		else
		{
			return false;
		}
	}
	return false;
}


//-------------------------------------



#if 0
Activity CAI_BlendedMotor::GetTransitionActivity( )
{
	AI_Waypoint_t *waypoint = GetNavigator()->GetPath()->GetTransitionWaypoint();

	if ( waypoint->Flags() & bits_WP_TO_GOAL )
	{
		if ( waypoint->activity != ACT_INVALID)
		{
			return waypoint->activity;
		}

		return GetStoppedActivity( );
	}

	if (waypoint)
		waypoint = waypoint->GetNext();

	switch(waypoint->NavType() )
	{
	case NAV_JUMP:
		return ACT_JUMP; // are jumps going to get a movement track added to them?

	case NAV_GROUND:
		return GetNavigator()->GetMovementActivity(); // yuck

	case NAV_CLIMB:
		return ACT_CLIMB_UP; // depends on specifics of climb node

	default:
		return ACT_IDLE;
	}
}
#endif

//-------------------------------------
// Purpose:	return a velocity that should be hit at the end of the interval to match goal
// Input  : flInterval - time interval to consider
//        : flGoalDistance - distance to goal
//        : flGoalVelocity - desired velocity at goal
//        : flCurVelocity - current velocity
//        : flIdealVelocity - velocity to go at if goal is too far away
//        : flAccelRate - maximum acceleration/deceleration rate
// Output : target velocity at time t+flInterval
//-------------------------------------

float ChangeDistance( float flInterval, float flGoalDistance, float flGoalVelocity, float flCurVelocity, float flIdealVelocity, float flAccelRate, float &flNewDistance, float &flNewVelocity )
{
	float scale = 1.0;
	if (flGoalDistance < 0)
	{
		flGoalDistance = - flGoalDistance;
		flCurVelocity = -flCurVelocity;
		scale = -1.0;
	}

	flNewVelocity = flCurVelocity;
	flNewDistance = 0.0;

	// if I'm too close, just go ahead and set the velocity
	if (flGoalDistance < 0.01)
	{
		return flGoalVelocity * scale;
	}

	float flGoalAccel = DeltaV( flCurVelocity, flGoalVelocity, flGoalDistance );

	flNewVelocity = flCurVelocity;

	// --------------------------------------------
	// if goal is close enough try to match the goal velocity, else try to go ideal velocity
	// --------------------------------------------
	if (flGoalAccel < 0 && flGoalAccel < -flAccelRate)
	{
		// I need to slow down;
		flNewVelocity = flCurVelocity + flGoalAccel * flInterval;
		if (flNewVelocity < 0)
			flNewVelocity = 0;
	}
	else if (flGoalAccel > 0 && flGoalAccel >= flAccelRate)
	{
		// I need to speed up
		flNewVelocity = flCurVelocity + flGoalAccel * flInterval;
		if (flNewVelocity > flGoalVelocity)
			flNewVelocity = flGoalVelocity;
	}
	else if (flNewVelocity < flIdealVelocity)
	{
		// speed up to ideal velocity;
		flNewVelocity = flCurVelocity + flAccelRate * flInterval;
		if (flNewVelocity > flIdealVelocity)
			flNewVelocity = flIdealVelocity;
		// don't overshoot
		if (0.5*(flNewVelocity + flCurVelocity) * flInterval > flGoalDistance)
		{
			flNewVelocity = 0.5 * (2 * flGoalDistance / flInterval - flCurVelocity);
		}
	}
	else if (flNewVelocity > flIdealVelocity)
	{
		// slow down to ideal velocity;
		flNewVelocity = flCurVelocity - flAccelRate * flInterval;
		if (flNewVelocity < flIdealVelocity)
			flNewVelocity = flIdealVelocity;
	}

	float flDist = 0.5*(flNewVelocity + flCurVelocity) * flInterval;

	if (flDist > flGoalDistance)
	{
		flDist = flGoalDistance;
		flNewVelocity = flGoalVelocity;
	}

	flNewVelocity = flNewVelocity * scale;

	flNewDistance = (flGoalDistance - flDist) * scale;
	
	return 0.0;
}

//-----------------------------------------------------------------------------

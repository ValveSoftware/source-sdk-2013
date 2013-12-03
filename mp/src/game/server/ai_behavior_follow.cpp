//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "cbase.h"
#include "tier1/utllinkedlist.h"
#include "bitstring.h"
#include "utlvector.h"
#include "ai_navigator.h"
#include "scripted.h"
#include "ai_hint.h"
#include "ai_behavior_follow.h"
#include "ai_memory.h"
#include "ai_squad.h"
#include "ai_tacticalservices.h"
#include "ndebugoverlay.h"
#include "ai_senses.h"

#ifdef HL2_EPISODIC
	#include "info_darknessmode_lightsource.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	ai_debug_follow( "ai_debug_follow", "0" );
ConVar	ai_follow_use_points( "ai_follow_use_points", "1" );
ConVar	ai_follow_use_points_when_moving( "ai_follow_use_points_when_moving", "1" );
#define FollowMsg(s) if ( !GetOuter() || !ai_debug_follow.GetBool() ) ; else DevMsg( GetOuter(), "Follow: " s )

#define WAIT_HINT_MIN_DIST		(16*16)		// Was: Square(GetHullWidth())

//-----------------------------------------------------------------------------
//
// Purpose: Formation management
//
//			Right now, this is in a very preliminary sketch state. (toml 03-03-03)
//-----------------------------------------------------------------------------

struct AI_FollowSlot_t;
struct AI_FollowFormation_t;
struct AI_FollowGroup_t;

struct AI_Follower_t
{
	AI_Follower_t()
	{
		slot = -1;
		memset( &navInfo, 0, sizeof(navInfo) );
		pGroup = NULL;
	}

	AIHANDLE 			hFollower;
	int					slot;
	AI_FollowNavInfo_t	navInfo;
	AI_FollowGroup_t *	pGroup;	// backpointer for efficiency
};

struct AI_FollowGroup_t
{
	AI_FollowFormation_t *	pFormation;
	EHANDLE 				hFollowTarget;
	CUtlFixedLinkedList<AI_Follower_t>	followers;
	CVarBitVec				slotUsage;
};


//-------------------------------------

class CAI_FollowManager
{
public:
	~CAI_FollowManager()
	{
		for ( int i = 0; i < m_groups.Count(); i++ )
			delete m_groups[i];
	}

	bool AddFollower( CBaseEntity *pTarget, CAI_BaseNPC *pFollower, AI_Formations_t formation, AI_FollowManagerInfoHandle_t *pHandle );
	void ChangeFormation( AI_FollowManagerInfoHandle_t &handle, AI_Formations_t formation );
	void RemoveFollower( AI_FollowManagerInfoHandle_t &handle );
	bool CalcFollowPosition( AI_FollowManagerInfoHandle_t &handle, AI_FollowNavInfo_t *pNavInfo );

	int CountFollowersInGroup( CAI_BaseNPC *pMember )
	{
		AI_FollowGroup_t *pGroup = FindFollowerGroup( pMember );

		if( !pGroup )
		{
			return 0;
		}

		return pGroup->followers.Count();
	}

	int CountFollowers( CBaseEntity *pFollowTarget, string_t iszClassname )
	{
		AI_FollowGroup_t *pGroup = FindGroup( pFollowTarget );

		if( !pGroup )
		{
			return 0;
		}

		if ( iszClassname == NULL_STRING )
		{
			return pGroup->followers.Count();
		}
		else
		{
			int result = 0;
			for ( int i = pGroup->followers.Head(); i != pGroup->followers.InvalidIndex(); i = pGroup->followers.Next( i ) )
			{
				if ( pGroup->followers[i].hFollower && pGroup->followers[i].hFollower->ClassMatches( iszClassname ) )
				{
					result++;
				}
			}
			return result;
		}
	}

	int GetFollowerSlot( CAI_BaseNPC *pFollower )
	{
		AI_FollowGroup_t *pGroup = FindFollowerGroup( pFollower );

		if( !pGroup )
		{
			return 0;
		}

		int h = pGroup->followers.Head();

		while( h != pGroup->followers.InvalidIndex() )
		{
			AI_Follower_t *it = &pGroup->followers[h];
			if ( it->hFollower.Get() == pFollower )
			{
				return it->slot;
			}

			h = pGroup->followers.Next( h );
		}

		return 0;
	}

private:
	bool RedistributeSlots( AI_FollowGroup_t *pGroup );
	int FindBestSlot( AI_FollowGroup_t *pGroup );
	void CalculateFieldsFromSlot( AI_FollowSlot_t *pSlot, AI_FollowNavInfo_t *pFollowerInfo );

	AI_FollowGroup_t *FindCreateGroup( CBaseEntity *pTarget, AI_Formations_t formation );
	AI_FollowGroup_t *FindGroup( CBaseEntity *pTarget );
	AI_FollowGroup_t *FindFollowerGroup( CBaseEntity *pFollower );
	void RemoveGroup( AI_FollowGroup_t * );
	
	//---------------------------------
	
	CUtlVector<AI_FollowGroup_t *> m_groups;
};

//-------------------------------------

CAI_FollowManager g_AIFollowManager;

//-----------------------------------------------------------------------------

int AIGetNumFollowers( CBaseEntity *pEntity, string_t iszClassname )
{
	return g_AIFollowManager.CountFollowers( pEntity, iszClassname );
}

//-----------------------------------------------------------------------------
//
// CAI_FollowBehavior
//
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( AI_FollowNavInfo_t )
	DEFINE_FIELD( flags, FIELD_INTEGER ),
	DEFINE_FIELD( position, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( range, FIELD_FLOAT ),
	DEFINE_FIELD( Zrange, FIELD_FLOAT ),
	DEFINE_FIELD( tolerance, FIELD_FLOAT ),
	DEFINE_FIELD( followPointTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( targetMoveTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( repathOnRouteTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( walkTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( coverTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( enemyLOSTolerance, FIELD_FLOAT ),
	DEFINE_FIELD( chaseEnemyTolerance, FIELD_FLOAT ),
END_DATADESC();

BEGIN_SIMPLE_DATADESC( AI_FollowParams_t )
	DEFINE_FIELD( formation, FIELD_INTEGER ),
	DEFINE_FIELD( bNormalMemoryDiscard, FIELD_BOOLEAN ),

END_DATADESC();

BEGIN_DATADESC( CAI_FollowBehavior )
	DEFINE_FIELD( m_hFollowTarget, FIELD_EHANDLE ),
	DEFINE_EMBEDDED( m_FollowNavGoal ),
	DEFINE_FIELD( m_flTimeUpdatedFollowPosition, FIELD_TIME ),
	DEFINE_FIELD( m_bFirstFacing, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flTimeFollowTargetVisible, FIELD_TIME ),
	DEFINE_EMBEDDED( m_TargetMonitor ),
	DEFINE_FIELD( m_bTargetUnreachable, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bFollowNavFailed, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMovingToCover, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_flOriginalEnemyDiscardTime, FIELD_FLOAT ),
	DEFINE_FIELD( m_SavedDistTooFar, FIELD_FLOAT ),
	DEFINE_EMBEDDED( m_FollowDelay ),
	DEFINE_EMBEDDED( m_RepathOnFollowTimer ),
	DEFINE_CUSTOM_FIELD( m_CurrentFollowActivity,	ActivityDataOps() ),
	DEFINE_EMBEDDED( m_TimeBlockUseWaitPoint ),
	DEFINE_EMBEDDED( m_TimeCheckForWaitPoint ),
	DEFINE_FIELD( m_pInterruptWaitPoint, FIELD_CLASSPTR ),
	DEFINE_EMBEDDED( m_TimeBeforeSpreadFacing ),
	DEFINE_EMBEDDED( m_TimeNextSpreadFacing ),
	//				m_hFollowManagerInfo	(reset on load)
	DEFINE_EMBEDDED( m_params ),
	DEFINE_FIELD( m_hFollowGoalEnt, FIELD_EHANDLE ),
	DEFINE_FIELD( m_nFailedFollowAttempts, FIELD_INTEGER ),
	DEFINE_FIELD( m_flTimeFailFollowStarted, FIELD_TIME ),
	DEFINE_FIELD( m_vFollowMoveAnchor, FIELD_POSITION_VECTOR ),
END_DATADESC();

//-------------------------------------

CAI_FollowBehavior::CAI_FollowBehavior( const AI_FollowParams_t &params )
{
	memset( &m_FollowNavGoal, 0, sizeof( m_FollowNavGoal ) );
	
	m_FollowDelay.Set( 1.0, 3.0 );
	m_hFollowManagerInfo.m_pGroup = NULL;
	m_hFollowManagerInfo.m_hFollower = 0;
	
	m_TimeBlockUseWaitPoint.Set( 0.5, 1.5 );
	m_TimeCheckForWaitPoint.Set( 1.0 );
	m_pInterruptWaitPoint = NULL;

	m_TimeBeforeSpreadFacing.Set( 2.0, 4.0 );
	m_TimeNextSpreadFacing.Set( 3.0, 12.0 );

	m_params = params;

	NoteSuccessfulFollow();
}

//-------------------------------------

CAI_FollowBehavior::~CAI_FollowBehavior()
{
	Assert( !m_hFollowManagerInfo.m_pGroup );
}

//-----------------------------------------------------------------------------
// Purpose: Draw any text overlays
// Input  : Previous text offset from the top
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_FollowBehavior::DrawDebugTextOverlays( int text_offset )
{
	char			tempstr[ 512 ];
	int				offset;
	CBaseEntity *	followEnt;

	offset = BaseClass::DrawDebugTextOverlays( text_offset );
	if ( GetOuter()->m_debugOverlays & OVERLAY_TEXT_BIT )
	{	
		followEnt = GetFollowTarget();
		if ( followEnt != NULL )
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Follow: (%d) %s (%s)", followEnt->entindex(), followEnt->GetDebugName(), followEnt->GetClassname() );
		}
		else 
		{
			Q_snprintf( tempstr, sizeof(tempstr), "Follow: NULL" );
		}
		GetOuter()->EntityText( offset, tempstr, 0 );
		offset++;
	}

	return offset;
}


void CAI_FollowBehavior::DrawDebugGeometryOverlays()
{
	if ( GetFollowTarget() )
	{
		Vector vecFollowPos = GetGoalPosition();
		NDebugOverlay::HorzArrow( GetOuter()->GetAbsOrigin(), vecFollowPos, 16.0f, 0, 255, 0, 0, true, 0 );
	}
}


//-------------------------------------

void CAI_FollowBehavior::SetParameters( const AI_FollowParams_t &params )
{
	m_params = params;

	if ( m_hFollowManagerInfo.m_pGroup )
	{
		g_AIFollowManager.ChangeFormation( m_hFollowManagerInfo, params.formation );
		m_flTimeUpdatedFollowPosition = 0;
	}
}

//-------------------------------------

CBaseEntity * CAI_FollowBehavior::GetFollowTarget()
{ 
	return m_hFollowTarget;
}

//-------------------------------------

// Returns true if the NPC is actively following a target.
bool CAI_FollowBehavior::IsActive( void )
{
	if ( IsRunning() && GetFollowTarget() ) 
	{
		// Only true if we're running a follow schedule
		return IsCurScheduleFollowSchedule();
	}

	return false;
}

//-------------------------------------

void CAI_FollowBehavior::SetFollowTarget( CBaseEntity *pLeader, bool fFinishCurSchedule ) 
{ 
	if ( pLeader == m_hFollowTarget )
		return;

	if ( !GetOuter()->IsAlive() )
	{
		return;
	}

	m_flTimeUpdatedFollowPosition = 0;

	if ( m_hFollowTarget )
	{
		g_AIFollowManager.RemoveFollower( m_hFollowManagerInfo );
		m_hFollowTarget = NULL;
		m_hFollowManagerInfo.m_pGroup = NULL;
		if ( IsRunning() )
		{
			if ( GetNavigator()->GetGoalType() == GOALTYPE_TARGETENT )
			{
				GetNavigator()->StopMoving(); // Stop him from walking toward the player
			}
			
			if ( GetEnemy() != NULL )
			{
				GetOuter()->SetIdealState( NPC_STATE_COMBAT );
			}
		}
	}

	if ( pLeader ) 
	{
		if ( g_AIFollowManager.AddFollower( pLeader, GetOuter(), m_params.formation, &m_hFollowManagerInfo ) )
		{
			m_hFollowTarget = pLeader;
			m_bFirstFacing = true;
			m_flTimeFollowTargetVisible = 0;
			SetCondition( COND_TARGET_MOVED_FROM_MARK );
			m_TargetMonitor.ClearMark();
			NoteSuccessfulFollow();
		}
	}

	NotifyChangeBehaviorStatus(fFinishCurSchedule);
}

//-------------------------------------
void CAI_FollowBehavior::SetFollowGoalDirect( CAI_FollowGoal *pGoal )
{
	m_hFollowGoalEnt = pGoal;
	m_flTimeUpdatedFollowPosition = 0;
}

//-------------------------------------

bool CAI_FollowBehavior::SetFollowGoal( CAI_FollowGoal *pGoal, bool fFinishCurSchedule )
{
	if ( GetOuter()->ShouldAcceptGoal( this, pGoal ) )
	{
		GetOuter()->ClearCommandGoal();

		if( hl2_episodic.GetBool() )
		{
			// Poke the NPC to interrupt any stubborn schedules
			GetOuter()->SetCondition(COND_PROVOKED);
		}

		SetFollowTarget( pGoal->GetGoalEntity() );
		Assert( pGoal->m_iFormation == AIF_SIMPLE || pGoal->m_iFormation == AIF_WIDE || pGoal->m_iFormation == AIF_MEDIUM || pGoal->m_iFormation == AIF_SIDEKICK || pGoal->m_iFormation == AIF_VORTIGAUNT );
		SetParameters( AI_FollowParams_t( (AI_Formations_t)pGoal->m_iFormation ) );
		m_hFollowGoalEnt = pGoal;
		m_flTimeUpdatedFollowPosition = 0;
		return true;
	}
	return false;
}

//-------------------------------------

void CAI_FollowBehavior::ClearFollowGoal( CAI_FollowGoal *pGoal )
{
	GetOuter()->OnClearGoal( this, pGoal );
	if ( pGoal == m_hFollowGoalEnt )
	{
		SetFollowTarget( NULL );
		m_hFollowGoalEnt = NULL;
		m_flTimeUpdatedFollowPosition = 0;
	}
}

//-------------------------------------

bool CAI_FollowBehavior::UpdateFollowPosition()
{
	AI_PROFILE_SCOPE( CAI_FollowBehavior_UpdateFollowPosition );

	if ( m_flTimeUpdatedFollowPosition == gpGlobals->curtime )
	{
		return true;
	}

	if (m_hFollowTarget == NULL)
		return false;
	
	if ( !g_AIFollowManager.CalcFollowPosition( m_hFollowManagerInfo, &m_FollowNavGoal ) )
	{
		return false;
	}

	CBaseEntity *pFollowTarget = GetFollowTarget();

	if ( pFollowTarget->GetParent() )
	{
		if ( pFollowTarget->GetParent()->GetServerVehicle() )
		{
			m_FollowNavGoal.targetMoveTolerance *= 1.5;
			m_FollowNavGoal.range += pFollowTarget->GetParent()->BoundingRadius() * 0.333;
		}
	}

#if TODO
	// @TODO (toml 07-27-03): this is too simplistic. fails when the new point is an inappropriate target
	CBasePlayer *pPlayer = dynamic_cast<CBasePlayer *>(m_hFollowTarget.Get());
	Vector targetVelocity = pPlayer->GetSmoothedVelocity();
	m_FollowNavGoal.position += targetVelocity * 0.5;
#endif
	
	m_flTimeUpdatedFollowPosition = gpGlobals->curtime;

	return true;
}

//-------------------------------------

bool CAI_FollowBehavior::IsMovingToFollowTarget()
{
	return ( IsRunning() && ( IsCurSchedule(SCHED_FOLLOW, false) || IsCurSchedule(SCHED_FOLLOWER_GO_TO_WAIT_POINT, false) ) );
}

//-------------------------------------

bool CAI_FollowBehavior::CanSelectSchedule()
{
	if ( !GetOuter()->IsInterruptable() )
		return false;

	if ( !ShouldFollow() )
	{
		return false;
	}

	return true;
}

//-------------------------------------

bool CAI_FollowBehavior::PlayerIsPushing()
{ 
	return (m_hFollowTarget && m_hFollowTarget->IsPlayer() && HasCondition( COND_PLAYER_PUSHING ) ); 
}

//-------------------------------------

bool CAI_FollowBehavior::IsFollowTargetInRange( float rangeMultiplier )
{
	if ( !GetFollowTarget()->IsPlayer() && HasCondition( COND_RECEIVED_ORDERS ) )
		return false;

	if( GetNpcState() == NPC_STATE_COMBAT )
	{
		if( IsFollowGoalInRange( MAX( m_FollowNavGoal.coverTolerance, m_FollowNavGoal.enemyLOSTolerance ) * rangeMultiplier, GetGoalZRange(), GetGoalFlags() ) )
		{
			return true;
		}
	}
	else
	{
		if( IsFollowGoalInRange( MAX( m_FollowNavGoal.tolerance, GetGoalRange() ) * rangeMultiplier, GetGoalZRange(), GetGoalFlags() ) )
		{
			if ( m_FollowNavGoal.flags & AIFF_REQUIRE_LOS_OUTSIDE_COMBAT )
			{
				//trace_t tr;
				//AI_TraceLOS( vecStart, vecStart + vecDir * 8192, m_hFollowTarget, &tr );
				//if ( AI_TraceLOS m_FollowNavGoal.position
				if ( !HasCondition(COND_SEE_PLAYER) )
					return false;
			}

			return true;
		}
	}
	return false;
}

//-------------------------------------

bool CAI_FollowBehavior::IsFollowGoalInRange( float tolerance, float zTolerance, int flags )
{
	const Vector &origin = WorldSpaceCenter();
	const Vector &goal = GetGoalPosition();
	if ( zTolerance == -1 )
		zTolerance = GetHullHeight();
	float distanceSq = ( goal.AsVector2D() - origin.AsVector2D() ).LengthSqr();
	tolerance += 0.1;

	// Increase Z tolerance slightly as XY distance decreases
	float flToleranceSq = (tolerance*tolerance);
	float flIncreaseRange = flToleranceSq * 0.25;
	zTolerance += zTolerance * clamp((distanceSq / flIncreaseRange), 0.f, 1.f );
	if ( fabs( origin.z - goal.z ) > zTolerance )
		return false;

	if ( distanceSq > flToleranceSq )
		return false;

	if ( flags & AIFF_REQUIRE_LOS_OUTSIDE_COMBAT && m_hFollowTarget.Get() )
	{
		if ( !GetOuter()->GetSenses()->DidSeeEntity( m_hFollowTarget ) )
			return false;
	}

	return true;
}

//-------------------------------------

bool CAI_FollowBehavior::IsChaseGoalInRange() 
{ 
	if ( GetEnemy() && ( GetEnemy()->WorldSpaceCenter() - m_FollowNavGoal.position ).LengthSqr() > Square( m_FollowNavGoal.chaseEnemyTolerance ) )
		return false;

	return true; 
}

//-------------------------------------

void CAI_FollowBehavior::NoteFailedFollow()
{
	m_nFailedFollowAttempts++;
	if ( m_flTimeFailFollowStarted == FLT_MAX )
		m_flTimeFailFollowStarted = gpGlobals->curtime;

	if ( GetOuter() && ai_debug_follow.GetBool() ) 
		DevMsg( GetOuter(), "Follow: NoteFailedFollow() (%d, %f)\n", m_nFailedFollowAttempts, m_flTimeFailFollowStarted );
}

//-------------------------------------

void CAI_FollowBehavior::NoteSuccessfulFollow()
{
	m_nFailedFollowAttempts = 0;
	m_flTimeFailFollowStarted = FLT_MAX;
	FollowMsg( "NoteSuccessfulFollow()\n" );
}

//-------------------------------------

void CAI_FollowBehavior::BeginScheduleSelection()
{
	if ( GetOuter()->m_hCine )
		GetOuter()->m_hCine->CancelScript();

	m_TimeBeforeSpreadFacing.Reset();

	SetCondition( COND_TARGET_MOVED_FROM_MARK );
	m_TargetMonitor.ClearMark();
	NoteSuccessfulFollow();

	if ( !m_params.bNormalMemoryDiscard )
	{
		// Forget about enemies that I haven't seen for >5 seconds
		m_flOriginalEnemyDiscardTime = GetOuter()->GetEnemies()->GetEnemyDiscardTime();
		GetOuter()->GetEnemies()->SetEnemyDiscardTime( 5.0f );
	}

	m_SavedDistTooFar = GetOuter()->m_flDistTooFar;
	if ( GetFollowTarget() && GetFollowTarget()->IsPlayer() )
	{
		GetOuter()->m_flDistTooFar = FLT_MAX;
	}

	BaseClass::BeginScheduleSelection();
}

//-------------------------------------

void CAI_FollowBehavior::EndScheduleSelection()
{
	if ( !m_params.bNormalMemoryDiscard )
	{
		// Restore our original enemy discard time
		GetOuter()->GetEnemies()->SetEnemyDiscardTime( m_flOriginalEnemyDiscardTime );
	}

	if ( m_SavedDistTooFar > 0.1 ) // backward savefile compatability
	{
		GetOuter()->m_flDistTooFar = m_SavedDistTooFar;
	}

	BaseClass::EndScheduleSelection();
}

//-------------------------------------

void CAI_FollowBehavior::CleanupOnDeath( CBaseEntity *pCulprit, bool bFireDeathOutput )
{
	if ( m_hFollowManagerInfo.m_pGroup )
	{
		g_AIFollowManager.RemoveFollower( m_hFollowManagerInfo );
		m_hFollowManagerInfo.m_pGroup = NULL;
		m_hFollowTarget = NULL;
	}
	BaseClass::CleanupOnDeath( pCulprit, bFireDeathOutput );
}

//-------------------------------------

void CAI_FollowBehavior::Precache()
{
	if ( m_hFollowTarget != NULL && m_hFollowManagerInfo.m_pGroup  == NULL )
	{
		// Post load fixup
		if ( !g_AIFollowManager.AddFollower( m_hFollowTarget, GetOuter(), m_params.formation, &m_hFollowManagerInfo ) )
		{
			m_hFollowTarget = NULL;
		}
	}
}

//-------------------------------------

void CAI_FollowBehavior::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( !GetFollowTarget() )
	{
		ClearCondition( COND_FOLLOW_PLAYER_IS_LIT );
		ClearCondition( COND_FOLLOW_PLAYER_IS_NOT_LIT );
		ClearCondition( COND_FOLLOW_TARGET_VISIBLE );
		ClearCondition( COND_FOLLOW_TARGET_NOT_VISIBLE );
		ClearCondition( COND_FOLLOW_DELAY_EXPIRED );
		ClearCondition( COND_TARGET_MOVED_FROM_MARK );
		ClearFollowPoint();
		m_pInterruptWaitPoint = NULL;
		m_bTargetUnreachable = false;
		m_flTimeFollowTargetVisible = 0;

		if ( IsRunning() )
		{
			GetOuter()->ClearSchedule( "Follow target gone" );
		}
		return;
	}

	if ( !m_TargetMonitor.IsMarkSet() )
	{
		FollowMsg( "No mark set\n" );
	}
		
	if ( m_FollowDelay.IsRunning() && m_FollowDelay.Expired())
	{
		SetCondition( COND_FOLLOW_DELAY_EXPIRED );
		m_FollowDelay.Stop();
	}
	
	if ( m_TargetMonitor.TargetMoved2D( GetFollowTarget() ) )
	{
		FollowMsg( "Target moved\n" );
		m_TargetMonitor.ClearMark();
		SetCondition( COND_TARGET_MOVED_FROM_MARK );
		m_bTargetUnreachable = false;
	}

	if ( !m_TargetMonitor.IsMarkSet() )
		m_bTargetUnreachable = false;

	m_pInterruptWaitPoint = NULL;

	if ( GetHintNode() == NULL )
	{
		if ( ShouldUseFollowPoints() && m_TimeBlockUseWaitPoint.Expired() && m_TimeCheckForWaitPoint.Expired() )
		{
			m_TimeCheckForWaitPoint.Reset();
			m_pInterruptWaitPoint = FindFollowPoint();
			if ( m_pInterruptWaitPoint )
				SetCondition( COND_FOUND_WAIT_POINT );
		}
	}

	if ( m_flTimeUpdatedFollowPosition == 0 || gpGlobals->curtime - m_flTimeUpdatedFollowPosition > 2.0 )
		UpdateFollowPosition();

	if ( IsFollowTargetInRange() )
	{
		NoteSuccessfulFollow();
	} 
	else if ( GetOuter()->GetTask() && !IsCurScheduleFollowSchedule() )
	{
		if ( !m_FollowDelay.IsRunning() || m_FollowDelay.Expired() )
		{
			switch ( GetOuter()->GetTask()->iTask )
			{
			case TASK_WAIT_RANDOM:
			case TASK_WAIT_INDEFINITE:
			case TASK_WAIT:
			case TASK_WAIT_FACE_ENEMY:
			case TASK_WAIT_FACE_ENEMY_RANDOM:
				{
					m_TargetMonitor.ClearMark();
					if ( !HasCondition(COND_FOLLOW_PLAYER_IS_NOT_LIT) )
					{
						SetCondition( COND_TARGET_MOVED_FROM_MARK );
					}
				}
			}
		}
	}

#if 0
	else if ( !IsFollowPointInRange() )
	{
		GetHintNode()->Unlock();
		SetHintNode( NULL );
	}
#endif

#ifdef HL2_EPISODIC
	// Let followers know if the player is lit in the darkness
	if ( GetFollowTarget()->IsPlayer() && HL2GameRules()->IsAlyxInDarknessMode() )
	{
		if ( LookerCouldSeeTargetInDarkness( GetOuter(), GetFollowTarget() ) )
		{
			SetCondition( COND_FOLLOW_PLAYER_IS_LIT );
			ClearCondition( COND_FOLLOW_PLAYER_IS_NOT_LIT );
		}
		else
		{
			SetCondition( COND_FOLLOW_PLAYER_IS_NOT_LIT );
			ClearCondition( COND_FOLLOW_PLAYER_IS_LIT );
		}
	}
#endif

	// Set our follow target visibility state
	if ( (GetFollowTarget()->IsPlayer() && HasCondition( COND_SEE_PLAYER )) || GetOuter()->FVisible( GetFollowTarget()) )
	{
		SetCondition( COND_FOLLOW_TARGET_VISIBLE );
		ClearCondition( COND_FOLLOW_TARGET_NOT_VISIBLE );
		m_flTimeFollowTargetVisible = gpGlobals->curtime;
	}
	else
	{
		ClearCondition( COND_FOLLOW_TARGET_VISIBLE );
		SetCondition( COND_FOLLOW_TARGET_NOT_VISIBLE );
	}

	if ( HasFollowPoint() && ( m_flTimeFollowTargetVisible != 0 && gpGlobals->curtime - m_flTimeFollowTargetVisible > 5.0 ) )
		SetCondition( COND_FOLLOW_WAIT_POINT_INVALID );
	else
		ClearCondition( COND_FOLLOW_WAIT_POINT_INVALID );
}

//-------------------------------------

int CAI_FollowBehavior::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	if ( failedTask == TASK_MOVE_TO_FOLLOW_POSITION || failedTask == TASK_GET_PATH_TO_FOLLOW_POSITION )
	{
		if ( m_hFollowTarget )
		{
			m_TargetMonitor.SetMark( m_hFollowTarget, m_FollowNavGoal.targetMoveTolerance * 0.5 );	
			m_FollowDelay.Start();
			NoteFailedFollow();
		}
	}
	
	return BaseClass::SelectFailSchedule( failedSchedule, failedTask, taskFailCode );
}

//-------------------------------------

bool CAI_FollowBehavior::ShouldFollow()
{
	if ( !GetFollowTarget() )
		return false;

	if ( GetFollowTarget()->GetFlags() & FL_NOTARGET )
		return false;

	// If we recently failed to build a follow path, wait a while to
	// give other schedules a chance to run.
	if ( m_bFollowNavFailed && m_FollowDelay.IsRunning() && !m_FollowDelay.Expired() )
	{
		return false;
	}
		
	m_bFollowNavFailed = false;

	return true;	
}

//-------------------------------------

bool CAI_FollowBehavior::ShouldMoveToFollowTarget()
{
	if ( GetFollowTarget() == NULL )
		return false;

	if( m_bTargetUnreachable )
		return false;

#ifdef HL2_EPISODIC
	if ( HL2GameRules()->IsAlyxInDarknessMode() )
	{
		// If we're in darkness mode, the player needs to be lit by
		// darkness, but we don't need line of sight to him.
		if ( HasCondition(COND_FOLLOW_PLAYER_IS_NOT_LIT) )
			return false;
	}
#endif

	if ( HasFollowPoint() )
	{
		if ( IsFollowPointInRange() )
			return false;
	}
	else if ( IsFollowTargetInRange() )
		return false;

	if( m_FollowDelay.IsRunning() && !m_FollowDelay.Expired() && !HasCondition( COND_TARGET_MOVED_FROM_MARK ) )
		return false;

	return true;
}

//-------------------------------------

int CAI_FollowBehavior::SelectScheduleManagePosition()
{
	if ( PlayerIsPushing() )
		return SCHED_MOVE_AWAY;

	if ( !UpdateFollowPosition() )
		return SCHED_FAIL;

	return SCHED_NONE;
}
	
//-------------------------------------

bool CAI_FollowBehavior::ShouldUseFollowPoints()
{
	if ( !ai_follow_use_points.GetBool() || GetEnemy() != NULL )
		return false;

	return true;
}

//-------------------------------------

bool CAI_FollowBehavior::HasFollowPoint()
{
	return ( GetHintNode() && GetHintNode()->HintType() == HINT_FOLLOW_WAIT_POINT );
}

//-------------------------------------

void CAI_FollowBehavior::ClearFollowPoint()
{
	if ( GetHintNode() && GetHintNode()->HintType() == HINT_FOLLOW_WAIT_POINT )
	{
		GetHintNode()->Unlock();
		SetHintNode( NULL );
	}
}

//-------------------------------------

const Vector &CAI_FollowBehavior::GetFollowPoint()
{
	static Vector invalid = vec3_invalid;
	if ( GetHintNode() && GetHintNode()->HintType() == HINT_FOLLOW_WAIT_POINT )
		return GetHintNode()->GetAbsOrigin();
	return invalid;
}

//-------------------------------------

CAI_Hint *CAI_FollowBehavior::FindFollowPoint()
{
	if ( !m_TimeBlockUseWaitPoint.Expired() )
		return NULL;

	CHintCriteria hintCriteria;
	hintCriteria.SetHintType( HINT_FOLLOW_WAIT_POINT );
	hintCriteria.SetFlag( bits_HINT_NODE_VISIBLE | bits_HINT_NODE_NEAREST );

	// Add the search position
	hintCriteria.AddIncludePosition( GetGoalPosition(), MAX( m_FollowNavGoal.followPointTolerance, GetGoalRange() ) );
	hintCriteria.AddExcludePosition( GetGoalPosition(), (GetFollowTarget()->WorldAlignMins().AsVector2D() - GetFollowTarget()->WorldAlignMaxs().AsVector2D()).Length());

	return CAI_HintManager::FindHint( GetOuter(), hintCriteria );
}

//-------------------------------------

bool CAI_FollowBehavior::IsFollowPointInRange()
{
	return ( GetHintNode() && 
			 GetHintNode()->HintType() == HINT_FOLLOW_WAIT_POINT && 
			 (GetHintNode()->GetAbsOrigin() - GetFollowTarget()->GetAbsOrigin()).LengthSqr() < Square(MAX(m_FollowNavGoal.followPointTolerance, GetGoalRange())) );
}


//-------------------------------------

bool CAI_FollowBehavior::ShouldIgnoreFollowPointFacing()
{
	if ( !GetHintNode() )
		return true;

	HintIgnoreFacing_t hintSetting = GetHintNode()->GetIgnoreFacing();

	if ( hintSetting == HIF_DEFAULT )
		return ( GetHintNode()->HintActivityName() == NULL_STRING );

	return ( hintSetting == HIF_YES );
}

//-------------------------------------

void CAI_FollowBehavior::SetFollowPoint( CAI_Hint *pHintNode )
{
	if ( !pHintNode )
		return;

	Assert( pHintNode->HintType() == HINT_FOLLOW_WAIT_POINT );
	
	if ( GetHintNode() == pHintNode )
		return;

	if ( GetHintNode() )
		GetHintNode()->Unlock();

	if ( !pHintNode->Lock( GetOuter() ) )
	{
		SetHintNode( NULL );
		m_TimeBlockUseWaitPoint.Reset();
	}
	else
		SetHintNode( pHintNode );
}

//-------------------------------------

int CAI_FollowBehavior::SelectScheduleFollowPoints()
{
	bool bShouldUseFollowPoints = ( ShouldUseFollowPoints() && IsFollowGoalInRange( m_FollowNavGoal.followPointTolerance + 0.1, GetGoalZRange(), GetGoalFlags() ) );
	float distSqToPoint = FLT_MAX;
	bool bHasFollowPoint = HasFollowPoint();

	if ( bHasFollowPoint )
	{
		distSqToPoint = (GetHintNode()->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
		if ( !bShouldUseFollowPoints || 
			distSqToPoint > Square(2.0 * GetHullWidth()) ||
			HasCondition( COND_FOLLOW_WAIT_POINT_INVALID ) )
		{
			GetHintNode()->Unlock();
			SetHintNode( NULL );
			m_TimeBlockUseWaitPoint.Reset();
			bShouldUseFollowPoints = false;
		}
	}

	if ( bShouldUseFollowPoints )
	{
		bool bNewHint = false;
		if ( GetHintNode() && !bHasFollowPoint )
		{
			GetHintNode()->Unlock();
			SetHintNode( NULL );
		}

		if (!GetHintNode())
		{
			bNewHint = true;
			SetFollowPoint( ( m_pInterruptWaitPoint ) ? m_pInterruptWaitPoint : FindFollowPoint() );
			
			if ( GetHintNode() )
				distSqToPoint = (GetHintNode()->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
		}
		
		if ( GetHintNode() )
		{
			if ( bNewHint || distSqToPoint > WAIT_HINT_MIN_DIST )
				return SCHED_FOLLOWER_GO_TO_WAIT_POINT;
			if ( !ShouldIgnoreFollowPointFacing() )
				return SCHED_FOLLOWER_STAND_AT_WAIT_POINT;
		}
	}
	else
		ClearFollowPoint();
	
	return SCHED_NONE;
}

//-------------------------------------

int CAI_FollowBehavior::SelectScheduleMoveToFormation()
{
	if( ( GetNpcState() != NPC_STATE_COMBAT	&& !( HasCondition( COND_LIGHT_DAMAGE ) || HasCondition( COND_HEAVY_DAMAGE ))) ||
		!IsFollowGoalInRange( GetGoalRange(), GetGoalZRange(), GetGoalFlags() ) )
	{
		AISquadIter_t iter;
		CAI_Squad *pSquad = GetOuter()->GetSquad();
		if ( pSquad )
		{
			for ( CAI_BaseNPC *pSquadMember = pSquad->GetFirstMember( &iter ); pSquadMember; pSquadMember = pSquad->GetNextMember( &iter ) )
			{
				if ( pSquadMember->HasCondition( COND_PLAYER_PUSHING ) )
				{
					return SCHED_NONE;
				}
			}
		}
		if ( ShouldMoveToFollowTarget() || m_bFirstFacing )
		{
			return SCHED_TARGET_FACE; // Code for "SCHED_MOVE_TO_FACE_FOLLOW_TARGET". Used by Talker clients to interject comment
		}
	}
	return SCHED_NONE;
}

//-------------------------------------

int CAI_FollowBehavior::SelectSchedule()
{
	// Allow a range attack if we need to do it
	if ( hl2_episodic.GetBool() )
	{
		// Range attack
		if ( GetOuter()->ShouldMoveAndShoot() == false && HasCondition( COND_CAN_RANGE_ATTACK1 ) )
			return SCHED_RANGE_ATTACK1;
	}

	if ( GetFollowTarget() )
	{
		if ( !GetFollowTarget()->IsAlive() )
		{
			// UNDONE: Comment about the recently dead player here?
			SetFollowTarget( NULL );
		}
		else if ( ShouldFollow() )
		{
			int result = SCHED_NONE;
			
			result = SelectScheduleManagePosition();
			if ( result != SCHED_NONE )
				return result;
			
			result = SelectScheduleFollowPoints();
			if ( result != SCHED_NONE )
				return result;
				
			result = SelectScheduleMoveToFormation();
			if ( result != SCHED_NONE )
				return result;
				
			if ( HasCondition ( COND_NO_PRIMARY_AMMO ) && HaveSequenceForActivity( GetOuter()->TranslateActivity( ACT_RUN_AIM ) ) )
				return SCHED_HIDE_AND_RELOAD;
		}

		if ( PlayerIsPushing() )	
			return SCHED_MOVE_AWAY;
	}
	else
	{
		// Should not have landed here. Follow target ent must have been destroyed
		NotifyChangeBehaviorStatus();
	}

	if ( HasCondition( COND_TARGET_MOVED_FROM_MARK ) )
	{
		m_TargetMonitor.SetMark( m_hFollowTarget, m_FollowNavGoal.targetMoveTolerance * 0.5 );
	}

	return FollowCallBaseSelectSchedule();
}

//-------------------------------------

int CAI_FollowBehavior::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
		case SCHED_FOLLOWER_IDLE_STAND:
			// If we have an enemy, at least face them!
			if ( GetEnemy() )
				return SCHED_FOLLOWER_COMBAT_FACE;
			
			break;

		case SCHED_IDLE_STAND:
		{
			if ( ShouldMoveToFollowTarget() && !IsFollowGoalInRange( GetGoalRange(), GetGoalZRange(), GetGoalFlags() ) )
			{
				return SCHED_MOVE_TO_FACE_FOLLOW_TARGET;			
			}
			if ( HasFollowPoint() && !ShouldIgnoreFollowPointFacing() )
				return SCHED_FOLLOWER_GO_TO_WAIT_POINT;
			
			// If we have an enemy, at least face them!
			if ( GetEnemy() )
				return SCHED_FOLLOWER_COMBAT_FACE;

			return SCHED_FOLLOWER_IDLE_STAND;
		}

		case SCHED_COMBAT_STAND:
		case SCHED_ALERT_STAND:
		{
			if ( ShouldMoveToFollowTarget() && !IsFollowGoalInRange( GetGoalRange(), GetGoalZRange(), GetGoalFlags() ) )
			{
				return SCHED_MOVE_TO_FACE_FOLLOW_TARGET;			
			}
			break;
		}

		case SCHED_TARGET_FACE:
		{
			if ( ( ShouldMoveToFollowTarget() || m_bFirstFacing ) && !IsFollowGoalInRange( GetGoalRange(), GetGoalZRange(), GetGoalFlags() ) )
			{
				return SCHED_MOVE_TO_FACE_FOLLOW_TARGET;			
			}
			if ( HasFollowPoint() && !ShouldIgnoreFollowPointFacing() )
				return SCHED_FOLLOWER_GO_TO_WAIT_POINT;
			if ( !m_TargetMonitor.IsMarkSet() )
				m_TargetMonitor.SetMark( m_hFollowTarget, m_FollowNavGoal.targetMoveTolerance );
			return SCHED_FACE_FOLLOW_TARGET; // @TODO (toml 03-03-03): should select a facing sched
		}

		case SCHED_TARGET_CHASE:
		{
			return SCHED_FOLLOW;
		}

		// SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK just tells the NPC to chase their enemy, so
		// forbid this unless the destination is acceptable within the parameters of the follow behavior.
		case SCHED_CHASE_ENEMY:
		case SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK:
		{
			if ( IsChaseGoalInRange() == false )
				return SCHED_FOLLOWER_IDLE_STAND;
			break;
		}

		case SCHED_RANGE_ATTACK1:
		{
			if ( GetOuter()->GetShotRegulator()->IsInRestInterval() )
			{
				if ( GetEnemy() )
					return SCHED_FOLLOWER_COMBAT_FACE;
			
				return SCHED_FOLLOWER_IDLE_STAND; // @TODO (toml 07-02-03): Should do something more tactically sensible
			}
			break;
		}

		case SCHED_CHASE_ENEMY_FAILED:
		{
			if (HasMemory(bits_MEMORY_INCOVER))
			{
				// Make sure I don't get too far from the player
				if ( GetFollowTarget() )
				{
					float fDist = (GetLocalOrigin() - GetFollowTarget()->GetAbsOrigin()).Length();
					if (fDist > 500)
					{
						return SCHED_FOLLOW;
					}
				}
			}
			break;
		}

		case SCHED_MOVE_AWAY_FAIL:
		{
			return SCHED_FOLLOWER_MOVE_AWAY_FAIL;
		}
		case SCHED_MOVE_AWAY_END:
		{
			return SCHED_FOLLOWER_MOVE_AWAY_END;
		}
	}
	return BaseClass::TranslateSchedule( scheduleType );
}

//-------------------------------------

void CAI_FollowBehavior::OnStartSchedule( int scheduleType )
{
	if ( !IsRunning() && HasFollowPoint() )
	{
		ClearHintNode( 0.5 );
	}

	if ( !m_TargetMonitor.IsMarkSet() && !IsCurScheduleFollowSchedule() )
	{
		m_TargetMonitor.SetMark( m_hFollowTarget, m_FollowNavGoal.targetMoveTolerance );
	}
}

//-------------------------------------

void CAI_FollowBehavior::GetFollowTargetViewLoc( Vector *pResult )
{
	if ( !dynamic_cast<CPointEntity *>(m_hFollowTarget.Get()) )
	{
		trace_t tr;
		Vector vecStart, vecDir;

		ASSERT( m_hFollowTarget != NULL );

		vecStart = m_hFollowTarget->EyePosition();

		CBasePlayer *pPlayer;

		pPlayer = dynamic_cast<CBasePlayer *>(m_hFollowTarget.Get());

		if( pPlayer )
		{
			// Follow target is a player.
			pPlayer->EyeVectors( &vecDir, NULL, NULL );
		}
		else
		{
			// Not a player. 
			m_hFollowTarget->GetVectors( &vecDir, NULL, NULL );
		}

		AI_TraceLOS( vecStart, vecStart + vecDir * 8192, m_hFollowTarget, &tr );

		*pResult = tr.endpos;		
	}
	else
		*pResult = m_hFollowTarget->GetAbsOrigin();
}

//-------------------------------------

bool CAI_FollowBehavior::ValidateFaceTarget( Vector *pFaceTarget )
{
	if ( *pFaceTarget == vec3_invalid )
	{
		if ( m_hFollowTarget != NULL )
		{
			*pFaceTarget = m_hFollowTarget->GetAbsOrigin();
		}
		return false;
	}

	Vector testPoint = *pFaceTarget - GetAbsOrigin();
	testPoint.z = 0;
	VectorNormalize( testPoint );
	testPoint *= 48;
	testPoint += GetOuter()->EyePosition();

	trace_t tr;
	AI_TraceLine( GetOuter()->EyePosition(), testPoint, MASK_BLOCKLOS, m_hFollowTarget, COLLISION_GROUP_NONE, &tr );

	if ( tr.fraction < 1.0 )
	{
		*pFaceTarget = m_hFollowTarget->GetAbsOrigin();
		return false;
	}
	return true;
}

//-------------------------------------

bool CAI_FollowBehavior::FindCoverFromEnemyAtFollowTarget( float coverRadius, Vector *pResult )
{
	CBaseEntity *pEntity = GetEnemy();

	return GetOuter()->FindCoverPosInRadius( pEntity, m_FollowNavGoal.position, coverRadius, pResult );
}

//-------------------------------------

void CAI_FollowBehavior::StartTask( const Task_t *pTask )
{
	AI_PROFILE_SCOPE( CAI_FollowBehavior_StartTask );

	switch ( pTask->iTask )
	{
		case TASK_RANGE_ATTACK1:
			BaseClass::StartTask( pTask );
			break;

		case TASK_GET_PATH_TO_FOLLOW_POSITION:
		{
			if ( !UpdateFollowPosition() )
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				m_TargetMonitor.SetMark( m_hFollowTarget, m_FollowNavGoal.targetMoveTolerance );
				m_bMovingToCover = false;
				GetOuter()->m_vInterruptSavePosition = vec3_invalid;
			}
			
			break;
		}

		case TASK_CANT_FOLLOW:
		{
			SetFollowTarget( NULL, true );
			TaskComplete();
			break;
		}

		case TASK_FOLLOWER_FACE_TACTICAL:
		case TASK_FACE_FOLLOW_TARGET:
		{
			if ( !m_TimeBeforeSpreadFacing.Expired() )
			{
				m_TimeNextSpreadFacing.Reset();
			}

			Vector faceTarget = vec3_invalid;
			bool bFollowingPoint = ( dynamic_cast<CPointEntity *>(m_hFollowTarget.Get()) != NULL );
			if ( GetNpcState() == NPC_STATE_COMBAT )
			{
				if( gpGlobals->curtime - GetOuter()->GetEnemyLastTimeSeen() < 5.0 )
				{
					faceTarget = GetEnemyLKP();
				}
				else if ( !bFollowingPoint )
				{
					GetFollowTargetViewLoc( &faceTarget );
				}
			}
			else if ( m_hFollowTarget && !bFollowingPoint )
			{
				if ( m_bFirstFacing && m_hFollowTarget->IsPlayer() )
				{
					faceTarget = m_hFollowTarget->GetAbsOrigin();
				}
				else if ( m_TimeNextSpreadFacing.Expired() )
				{
					m_TimeNextSpreadFacing.Reset();

					bool bIsEpisodicVitalAlly;
					
#ifdef HL2_DLL
					bIsEpisodicVitalAlly = (hl2_episodic.GetBool() && GetOuter()->Classify() == CLASS_PLAYER_ALLY_VITAL);
#else
					bIsEpisodicVitalAlly = false;
#endif//HL2_DLL

					if( bIsEpisodicVitalAlly )
					{
						faceTarget = m_hFollowTarget->GetAbsOrigin();
					}
					else
					{
						int roll = random->RandomInt(1, 4);
						if ( roll == 1 )
						{
							GetFollowTargetViewLoc( &faceTarget );
						}
						else if ( roll == 2 )
						{
							faceTarget = m_hFollowTarget->GetAbsOrigin();
						}
						else
						{
							// Fan out and face to cover all directions.
							int count = g_AIFollowManager.CountFollowersInGroup( GetOuter() );

							if( count > 0 )
							{
								// Slice up the directions among followers and leader. ( +1 because we count the leader!)
								float flSlice = 360.0 / (count + 1);

								// Add one to slots so then are 1 to N instead of 0 to N - 1.
								int slot = random->RandomInt( 0, count );

								QAngle angle = m_hFollowTarget->GetAbsAngles();

								// split up the remaining angles among followers in my group.
								angle.y = UTIL_AngleMod( angle.y + ( flSlice * slot ) );

								Vector vecDir;
								AngleVectors( angle, &vecDir );

								faceTarget = GetOuter()->GetAbsOrigin() + vecDir * 128;
							}
						}
					}
				}
				else
				{
					// Stay where we are
					TaskComplete();
					break;
				}
			}

			m_bFirstFacing = false;

			if ( ValidateFaceTarget( &faceTarget ) )
			{
				Assert( faceTarget != vec3_invalid );

				if ( !GetOuter()->FInAimCone( faceTarget ) )
				{
					GetMotor()->SetIdealYawToTarget( faceTarget, 30 );
					GetOuter()->SetTurnActivity(); 
				}
				else
					TaskComplete();
			}
			else
				ChainStartTask( TASK_FACE_REASONABLE );

			break;
		}
			
		case TASK_MOVE_TO_FOLLOW_POSITION:
		{
			if ( m_hFollowTarget == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else if ( (m_hFollowTarget->GetAbsOrigin() - GetAbsOrigin()).Length() < 1 )
			{
				TaskComplete();
			}
			else if ( !GetNavigator()->IsGoalActive() )
			{
				TaskFail(FAIL_NO_ROUTE);
			}
			else
			{
				m_vFollowMoveAnchor = GetAbsOrigin();
				m_CurrentFollowActivity = ACT_INVALID;
				m_RepathOnFollowTimer.Force();
			}
			break;
		}
		
		case TASK_SET_FOLLOW_TARGET_MARK:
		{
			if ( m_hFollowTarget == NULL)
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				FollowMsg( "TASK_SET_FOLLOW_TARGET_MARK\n" );
				m_TargetMonitor.SetMark( m_hFollowTarget, m_FollowNavGoal.targetMoveTolerance );
				TaskComplete();
			}
			break;
		}

		case TASK_SET_FOLLOW_DELAY:
		{
			m_FollowDelay.Start( pTask->flTaskData );
			TaskComplete();
			break;
		}

		case TASK_FIND_COVER_FROM_ENEMY:
		{
			CBaseEntity *pLeader = GetFollowTarget();
			if ( pLeader )
			{
				Vector coverPos = vec3_invalid;
				float coverRadius = MIN( GetOuter()->CoverRadius(), m_FollowNavGoal.coverTolerance );
				
				if ( FindCoverFromEnemyAtFollowTarget( coverRadius, &coverPos ) )
				{
					AI_NavGoal_t goal(GOALTYPE_COVER, coverPos, ACT_RUN, AIN_HULL_TOLERANCE, AIN_DEF_FLAGS);
					GetNavigator()->SetGoal( goal );

					GetOuter()->m_flMoveWaitFinished = gpGlobals->curtime + pTask->flTaskData;
					TaskComplete();
				}
				else
					TaskFail(FAIL_NO_COVER);
			}
			else
				BaseClass::StartTask( pTask );
			break;
		}
				
		case TASK_GET_PATH_TO_FOLLOW_POINT:
		{
			ChainStartTask( TASK_GET_PATH_TO_HINTNODE, ShouldIgnoreFollowPointFacing() );
			break;
		}

		case TASK_ARRIVE_AT_FOLLOW_POINT:
		{
			if ( GetHintNode() && !ShouldIgnoreFollowPointFacing() )
				ChainStartTask( TASK_FACE_HINTNODE, 0 );
			else
				TaskComplete();
			break;
		}

		case TASK_SET_FOLLOW_POINT_STAND_SCHEDULE:
		{
			if ( GetHintNode() && !ShouldIgnoreFollowPointFacing() )
			{
				float distSqToPoint = (GetHintNode()->GetAbsOrigin() - GetAbsOrigin()).LengthSqr();
				if ( distSqToPoint < WAIT_HINT_MIN_DIST )
				{
					GetOuter()->SetSchedule( SCHED_FOLLOWER_STAND_AT_WAIT_POINT );
				}
				else
				{
					GetHintNode()->Unlock();
					SetHintNode( NULL );
					m_TimeBlockUseWaitPoint.Reset();
					TaskFail("Couldn't get to wait node." );
				}
			}
			else
			{
				GetOuter()->SetSchedule( SCHED_FACE_FOLLOW_TARGET );
			}
			break;
		}

		case TASK_BEGIN_STAND_AT_WAIT_POINT:
		{
			if ( !m_TargetMonitor.IsMarkSet() && IsFollowPointInRange() )
				m_TargetMonitor.SetMark( m_hFollowTarget, m_FollowNavGoal.targetMoveTolerance );
			if ( GetHintNode() && !ShouldIgnoreFollowPointFacing() )
				ChainStartTask( TASK_FACE_HINTNODE, 0 );
			else
				TaskComplete();
			break;
		}

		default:
			BaseClass::StartTask( pTask );
	}
}

//-------------------------------------

void CAI_FollowBehavior::RunTask( const Task_t *pTask )
{
	switch( pTask->iTask )
	{
		case TASK_GET_PATH_TO_FOLLOW_POSITION:
		{
			switch( GetOuter()->GetTaskInterrupt() )
			{
			case 0:
				{
					if ( GetEnemy() )
					{
						Assert( GetOuter()->m_vInterruptSavePosition == vec3_invalid );
						Vector coverPos = vec3_invalid;
						float coverRadius = MIN( (float)12*12, m_FollowNavGoal.coverTolerance );
						if ( FindCoverFromEnemyAtFollowTarget( coverRadius, &coverPos ) )
						{
							GetOuter()->m_vInterruptSavePosition = coverPos;
						}
						GetOuter()->TaskInterrupt();
						break;
					}
				}
				// Fall through...

			case 1:
				{
					if ( GetOuter()->m_vInterruptSavePosition != vec3_invalid )
					{
						AI_NavGoal_t goal(GOALTYPE_COVER, GetOuter()->m_vInterruptSavePosition, ACT_RUN, AIN_HULL_TOLERANCE, AIN_DEF_FLAGS);
						if ( GetNavigator()->SetGoal( goal, AIN_NO_PATH_TASK_FAIL ) )
						{
							TaskComplete();
							m_bMovingToCover = true;
						}
						else
						{
							GetOuter()->TaskInterrupt();
						}
						break;
					}
				// Fall through...
				}

			case 2:
				{
					Assert( !m_bMovingToCover );
					Vector vGoalPosition;
					if ( HasFollowPoint() && IsFollowPointInRange() )
						vGoalPosition = GetFollowPoint();
					else
						vGoalPosition = GetGoalPosition();

					AI_NavGoal_t goal( vGoalPosition, AIN_DEF_ACTIVITY, GetGoalTolerance() );
					if ( !m_hFollowTarget->GetParent() || !m_hFollowTarget->GetParent()->GetServerVehicle() )
					{
						goal.pTarget = m_hFollowTarget;
					}
					else
					{
						goal.pTarget = m_hFollowTarget->GetParent();
					}

					bool bSuccess = true;
					if ( !GetNavigator()->SetGoal( goal, AIN_NO_PATH_TASK_FAIL ) )
					{
						const Vector &vTarget = GetFollowTarget()->WorldSpaceCenter();
						Vector vToGoal = vGoalPosition - vTarget;
						if ( vToGoal.Length2DSqr() > 6*12 )
						{
							goal.dest = vTarget + vToGoal * 0.5;
							if ( !GetNavigator()->SetGoal( goal, AIN_NO_PATH_TASK_FAIL ) )
							{
								bSuccess = false;
								m_FollowDelay.Start( 2.0, 5.0 );
							}
						}
						else
						{
							bSuccess = false;
							m_FollowDelay.Start( 2.0, 5.0 );
						}
					}

					if ( !bSuccess )
					{
						m_bFollowNavFailed = true;
						TaskFail( FAIL_NO_ROUTE );
					}
					else
					{
						TaskComplete();
					}
				}
			}
			
			break;
		}
		
		case TASK_FOLLOWER_FACE_TACTICAL:
		case TASK_FACE_FOLLOW_TARGET:
		{
			ChainRunTask( TASK_FACE_REASONABLE );
			break;
		}

		case TASK_MOVE_TO_FOLLOW_POSITION:
		{
			if ( m_hFollowTarget == NULL )
			{
				TaskFail(FAIL_NO_TARGET);
			}
			else
			{
				if ( m_bMovingToCover )
				{
					ChainRunTask( TASK_WAIT_FOR_MOVEMENT );
					NoteSuccessfulFollow();
					return;
				}

				// Re-evaluate when you think your finished, or the target has moved too far
				if ( !UpdateFollowPosition() )
				{
					TaskFail(FAIL_NO_TARGET);
					break;
				}
				
				if ( ShouldUseFollowPoints() && ai_follow_use_points_when_moving.GetBool() )
				{
					if ( HasFollowPoint() )
					{
						if ( !IsFollowPointInRange() )
						{
							ClearFollowPoint();
							GetNavigator()->SetArrivalDirection( vec3_origin );
							GetNavigator()->SetArrivalActivity( ACT_INVALID );
							m_TimeBlockUseWaitPoint.Reset();
							m_TimeCheckForWaitPoint.Reset();
						}
					}
					if ( GetNavigator()->GetNavType() != NAV_JUMP && !HasFollowPoint() && m_pInterruptWaitPoint )
					{
						SetFollowPoint( m_pInterruptWaitPoint );
					}
				}
				else
				{
					ClearFollowPoint();
					if ( GetNavigator()->IsGoalActive() )
					{
						GetNavigator()->SetArrivalDirection( vec3_origin );
						GetNavigator()->SetArrivalActivity( ACT_INVALID );
					}
				}
				
				if ( !GetNavigator()->IsGoalActive() )
				{
					// What this probably means is that the navigation failed but within tolerance
					// So for now, just call it good and block another attempt for a bit
					TaskComplete();
					if ( !IsFollowPointInRange() )
						ClearFollowPoint();
					if ( !IsFollowGoalInRange( m_FollowNavGoal.tolerance, GetGoalZRange(), GetGoalFlags() ) )
						m_FollowDelay.Start( 0.25, 0.75 );
					else
					{
						m_TargetMonitor.SetMark( GetFollowTarget(), m_FollowNavGoal.targetMoveTolerance );
						m_bTargetUnreachable = false;
					}
					break;
				}
				
				if ( !HasFollowPoint() )
				{
					float range = GetGoalRange();

					Vector vVelocity =- GetFollowTarget()->GetSmoothedVelocity();
					bool bDoSlowdown = ( vVelocity.LengthSqr() < Square(4*12) );
					if ( bDoSlowdown )
					{
						range += GetMotor()->MinStoppingDist(12) - 12;
					}

					if ( IsFollowGoalInRange( range, GetGoalZRange(), GetGoalFlags() ) )
					{
						m_TimeBeforeSpreadFacing.Reset();
						TaskComplete();
						GetNavigator()->StopMoving( !bDoSlowdown );		// Stop moving
						m_TargetMonitor.SetMark( GetFollowTarget(), m_FollowNavGoal.targetMoveTolerance );
						break;
					}

					// Update the nav goal if needed
					if ( m_RepathOnFollowTimer.Expired() )
					{
						if ( (GetNavigator()->GetGoalPos() - GetGoalPosition()).LengthSqr() > Square( m_FollowNavGoal.repathOnRouteTolerance ) )
						{
							if ( GetNavigator()->GetNavType() != NAV_JUMP )
							{
								m_RepathOnFollowTimer.Set( .5 );
								if ( !GetNavigator()->UpdateGoalPos( GetGoalPosition() ) )
								{
									bool bSuccess = false;
									const Vector &vTarget = GetFollowTarget()->WorldSpaceCenter();
									Vector vToGoal = GetGoalPosition() - vTarget;
									if ( vToGoal.Length2DSqr() > 6*12 )
									{
										if ( GetNavigator()->UpdateGoalPos( vTarget + vToGoal * 0.5 ) )
										{
											bSuccess = true;
										}
									}

									if ( !bSuccess )
									{
										TaskFail(FAIL_NO_ROUTE);
										m_bTargetUnreachable = true;
									}
									break;
								}
								NoteSuccessfulFollow();
							}
						}
					}
				}
				else
				{
					const Vector &vFollowPoint = GetFollowPoint();
					if ( GetNavigator()->GetGoalPos() != vFollowPoint )
					{
						if ( !GetNavigator()->UpdateGoalPos( vFollowPoint ) )
						{
							TaskFail(FAIL_NO_ROUTE);
							m_bTargetUnreachable = true;
							break;
						}
						NoteSuccessfulFollow();

						if ( !ShouldIgnoreFollowPointFacing() )
							GetNavigator()->SetArrivalDirection( GetHintNode()->GetDirection() );
						if ( GetHintNode()->HintActivityName() != NULL_STRING )
						{
							Activity hintActivity = (Activity)CAI_BaseNPC::GetActivityID( STRING(GetHintNode()->HintActivityName()) );
							if ( hintActivity != ACT_INVALID )
							{
								GetNavigator()->SetArrivalActivity( GetOuter()->GetHintActivity(GetHintNode()->HintType(), hintActivity  ) );
							}
							else
							{
								int iSequence = GetOuter()->LookupSequence(STRING(GetHintNode()->HintActivityName()));
								if ( iSequence != ACT_INVALID )
								{
									GetNavigator()->SetArrivalSequence( iSequence );
								}
							}
						}
					}
				}

				// Set the appropriate activity based on an overlapping range
				// overlap the range to prevent oscillation
				// BUGBUG: this is checking linear distance (ie. through walls) and not path distance or even visibility

				// Never stop running once started
				if ( m_CurrentFollowActivity != ACT_RUN )
				{
					float distToTargetSq = ( GetNavigator()->GetGoalPos() - GetLocalOrigin() ).Length2DSqr();
					
					// Pick the right movement activity.
					Activity followActivity = ( distToTargetSq < Square(m_FollowNavGoal.walkTolerance) && GetOuter()->GetState() != NPC_STATE_COMBAT ) ? ACT_WALK : ACT_RUN;

					// If we're supposed to have LOS, run to catch up
					if ( m_FollowNavGoal.flags & AIFF_REQUIRE_LOS_OUTSIDE_COMBAT )
					{
						if ( !GetOuter()->GetSenses()->DidSeeEntity( m_hFollowTarget ) )
						{
							followActivity = ACT_RUN;
						}
					}

					if ( followActivity != m_CurrentFollowActivity )
					{
						m_CurrentFollowActivity = followActivity;
						GetNavigator()->SetMovementActivity(followActivity);
					}
				}

				if ( ( m_vFollowMoveAnchor - GetAbsOrigin() ).LengthSqr() > Square( 15.0 * 12.0 ) )
				{
					m_vFollowMoveAnchor = GetAbsOrigin();
					NoteSuccessfulFollow();
				}

			}
			break;
		}
			
		case TASK_ARRIVE_AT_FOLLOW_POINT:
		{
			ChainRunTask( TASK_FACE_HINTNODE, 0 );
			break;
		}

		case TASK_BEGIN_STAND_AT_WAIT_POINT:
		{
			ChainRunTask( TASK_FACE_HINTNODE, 0 );
			break;
		}

		default:
			BaseClass::RunTask( pTask );
	}
}

//-------------------------------------

void CAI_FollowBehavior::TaskComplete( bool fIgnoreSetFailedCondition )
{
	const Task_t *pTask = GetCurTask();
	if ( pTask->iTask == TASK_MOVE_TO_FOLLOW_POSITION || pTask->iTask == TASK_GET_PATH_TO_FOLLOW_POSITION )
		NoteSuccessfulFollow();
	BaseClass::TaskComplete( fIgnoreSetFailedCondition );
}

//-------------------------------------

void CAI_FollowBehavior::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();
	bool bIsTakeCover = false;
	bool bIsHideAndReload = false;
	bool bIsReload = false;
	bool bIgnoreMovedMark = false;

	if ( ( GetOuter()->ConditionInterruptsCurSchedule( COND_GIVE_WAY ) || 
		   GetOuter()->ConditionInterruptsCurSchedule( COND_IDLE_INTERRUPT ) ||
		   ( bIsHideAndReload = IsCurSchedule(SCHED_HIDE_AND_RELOAD ) ) == true || 
		   ( bIsReload = IsCurSchedule(SCHED_RELOAD ) ) == true || 
		   IsCurSchedule(SCHED_STANDOFF ) || 
		   ( bIsTakeCover = IsCurSchedule(SCHED_TAKE_COVER_FROM_ENEMY ) ) == true || 
		   IsCurSchedule(SCHED_COMBAT_FACE ) || 
		   IsCurSchedule(SCHED_ALERT_FACE )  ||
		   IsCurSchedule(SCHED_COMBAT_STAND ) || 
		   IsCurSchedule(SCHED_ALERT_STAND) ) ||
		   IsCurSchedule(SCHED_ALERT_FACE_BESTSOUND ) )
	{
#ifdef HL2_EPISODIC
		if( IsCurSchedule(SCHED_RELOAD, false) && GetOuter()->Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			// Alyx and Barney do not stop reloading because the player has moved. 
			// Citizens and other regular allies do.
			bIgnoreMovedMark = true;
		}
#endif//HL2_EPISODIC

		if( !bIgnoreMovedMark )
		{
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_TARGET_MOVED_FROM_MARK ) );
		}

		if ( !bIsTakeCover && !bIsHideAndReload && !bIsReload )
			GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_FOLLOW_DELAY_EXPIRED) );
	}

	// Add logic for NPCs not able to move and shoot
	if ( hl2_episodic.GetBool() )
	{
		if ( IsCurScheduleFollowSchedule() && GetOuter()->ShouldMoveAndShoot() == false )
		{
			GetOuter()->SetCustomInterruptCondition( COND_CAN_RANGE_ATTACK1 );
		}

#ifdef HL2_EPISODIC
		// In Alyx darkness mode, break on the player turning their flashlight off
		if ( HL2GameRules()->IsAlyxInDarknessMode() )
		{
			if ( IsCurSchedule(SCHED_FOLLOW, false) || IsCurSchedule(SCHED_MOVE_TO_FACE_FOLLOW_TARGET, false) ||
				 IsCurSchedule(SCHED_FACE_FOLLOW_TARGET, false) )
			{
				GetOuter()->SetCustomInterruptCondition( GetClassScheduleIdSpace()->ConditionLocalToGlobal( COND_FOLLOW_PLAYER_IS_NOT_LIT ) );
			}
		}
#endif // HL2_EPISODIC
	}

	if ( GetNpcState() == NPC_STATE_COMBAT && IsCurScheduleFollowSchedule() )
	{
		GetOuter()->ClearCustomInterruptCondition( COND_LIGHT_DAMAGE );
	}
}

//-------------------------------------

Activity CAI_FollowBehavior::NPC_TranslateActivity( Activity activity )
{
	if ( activity == ACT_IDLE && HasFollowPoint() && GetHintNode()->HintActivityName() != NULL_STRING )
	{
		return GetOuter()->GetHintActivity(GetHintNode()->HintType(), (Activity)CAI_BaseNPC::GetActivityID( STRING(GetHintNode()->HintActivityName()) ) );
	}
	return BaseClass::NPC_TranslateActivity( activity );
}

//-------------------------------------

bool CAI_FollowBehavior::IsCurScheduleFollowSchedule()
{
	int curScheduleId = ( GetOuter()->GetCurSchedule() ) ? GetOuter()->GetCurSchedule()->GetId() : SCHED_NONE;
	if ( curScheduleId >= GetClassScheduleIdSpace()->ScheduleLocalToGlobal( SCHED_FOLLOWER_MOVE_AWAY_FAIL ) &&
		 curScheduleId <= GetClassScheduleIdSpace()->ScheduleLocalToGlobal( SCHED_FOLLOWER_STAND_AT_WAIT_POINT ) )
	{
		return true;
	}
	return false;
}

//-------------------------------------

bool CAI_FollowBehavior::IsCurTaskContinuousMove()
{
	const Task_t *pCurTask = GetCurTask();
	if ( pCurTask && pCurTask->iTask == TASK_MOVE_TO_FOLLOW_POSITION )
		return true;
	return BaseClass::IsCurTaskContinuousMove();
}

//-------------------------------------

void CAI_FollowBehavior::OnMovementFailed()
{
	float acceptDist = m_FollowNavGoal.range;
	if ( m_FollowNavGoal.tolerance > acceptDist )
		acceptDist = m_FollowNavGoal.tolerance;

	if ( GetNpcState() == NPC_STATE_COMBAT )
	{
		if ( m_FollowNavGoal.coverTolerance > acceptDist )
			acceptDist = m_FollowNavGoal.coverTolerance;
		if (m_FollowNavGoal.enemyLOSTolerance > acceptDist )
			acceptDist = m_FollowNavGoal.enemyLOSTolerance;
	}

	float flZRange = GetGoalZRange();
	if ( GetGoalZRange() == -1 )
	{
		flZRange = GetHullHeight() * 2;
	}

	if ( IsFollowGoalInRange( acceptDist * 1.5, flZRange, GetGoalFlags() ) )
		m_bTargetUnreachable = true;
	else
		m_FollowDelay.Start();
}

//-------------------------------------

void CAI_FollowBehavior::OnMovementComplete()
{
	if ( !IsCurSchedule(SCHED_FOLLOWER_GO_TO_WAIT_POINT) )
		m_TimeBeforeSpreadFacing.Reset();
	else
	{
		m_TimeBeforeSpreadFacing.Force();
		m_TimeNextSpreadFacing.Force();
	}
}

//-------------------------------------

bool CAI_FollowBehavior::FValidateHintType( CAI_Hint *pHint )
{
	if ( pHint->HintType() == HINT_FOLLOW_WAIT_POINT )
	{
		if ( GetFollowTarget() && GetFollowTarget()->FVisible( pHint->GetAbsOrigin() + Vector( 0, 0, 0.1 ) )  )
			return true;
		else
			return false;
	}
	return BaseClass::FValidateHintType( pHint );
}

//-------------------------------------

bool CAI_FollowBehavior::IsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	if ( (vLocation - m_FollowNavGoal.position).LengthSqr() > Square( m_FollowNavGoal.coverTolerance + 0.1 ) )
		return false;
	return BaseClass::IsValidCover( vLocation, pHint );
}

//-------------------------------------

bool CAI_FollowBehavior::IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint )
{
	if ( (vLocation - m_FollowNavGoal.position).LengthSqr() > Square( m_FollowNavGoal.enemyLOSTolerance + 0.1 ) )
		return false;
	return BaseClass::IsValidShootPosition( vLocation, pNode, pHint );
}

//-------------------------------------

bool CAI_FollowBehavior::ShouldAlwaysThink()
{
	return ( m_hFollowTarget && m_hFollowTarget->IsPlayer() );
}


//-----------------------------------------------------------------------------
//
// CAI_FollowGoal
//
// Purpose: A level tool to control the follow behavior. Use is not required
//			in order to use behavior.
//
//-----------------------------------------------------------------------------

BEGIN_DATADESC( CAI_FollowGoal )
	DEFINE_KEYFIELD(	m_iFormation, FIELD_INTEGER, "Formation" ),

#ifdef HL2_EPISODIC
	DEFINE_INPUTFUNC( FIELD_VOID, "OutsideTransition",	InputOutsideTransition ),
#endif
END_DATADESC()

//-------------------------------------

LINK_ENTITY_TO_CLASS( ai_goal_follow, CAI_FollowGoal );

//-------------------------------------

void CAI_FollowGoal::EnableGoal( CAI_BaseNPC *pAI )
{
	CAI_FollowBehavior *pBehavior;
	if ( !pAI->GetBehavior( &pBehavior ) )
		return;
	
	CBaseEntity *pGoalEntity = GetGoalEntity();
	if ( !pGoalEntity && AI_IsSinglePlayer() )
	{
		if ( pAI->IRelationType(UTIL_GetLocalPlayer()) == D_LI )
		{
			pGoalEntity = UTIL_GetLocalPlayer();
			SetGoalEntity( pGoalEntity );
		}
	}

	if ( pGoalEntity )
		pBehavior->SetFollowGoal( this );
}

//-------------------------------------

void CAI_FollowGoal::DisableGoal( CAI_BaseNPC *pAI  )
{ 
	CAI_FollowBehavior *pBehavior;
	if ( !pAI || !pAI->GetBehavior( &pBehavior ) )
		return;
	
	pBehavior->ClearFollowGoal( this );
}

//-------------------------------------

#ifdef HL2_EPISODIC
void CAI_FollowGoal::InputOutsideTransition( inputdata_t &inputdata )
{
	EnterDormant();
}
#endif

//-----------------------------------------------------------------------------
//
// CAI_FollowManager
//
//-----------------------------------------------------------------------------

//-------------------------------------
//
// Purpose: Formation definitions
//

// @TODO (toml 11-21-03): rework follow so we don't have to have class specifc formations in this file

struct AI_FollowSlot_t
{
	int 		priority;

	TableVector position;
	float		positionVariability;
	
	float		rangeMin;
	float		rangeMax;

	float		Zrange;

	float		tolerance;

	// @Q (toml 02-28-03): facing?
};

struct AI_FollowFormation_t
{
	const char *		pszName;
	unsigned 			flags;
	int					nSlots;

	// Range within which can exit formation to seek a follow point
	float				followPointTolerance;

	// Distance target must move to reset formation
	float				targetMoveTolerance;

	// Distance from current move goal target must move to force a repathfind
	float				repathOnRouteTolerance;

	// Distance from target within which should walk, not run to formation
	float				walkTolerance;

	// Distance within which can exit formation to seek cover
	float				coverTolerance;

	// Distance within which can exit formation to seek LOS to enemy
	float				enemyLOSTolerance;

	// Distance within which can exit formation to chase enemy
	float				chaseEnemyTolerance;

	AI_FollowSlot_t *	pSlots;
};

//-------------------------------------

static AI_FollowSlot_t g_SimpleFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 96, 120, -1, 128 },
};

static AI_FollowFormation_t g_SimpleFollowFormation = 
{
	"Simple",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_SimpleFollowFormationSlots),
	168,						// followPointTolerance
	36,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	190,						// walkTolerance
	300,						// coverTolerance
	300,						// enemyLOSTolerance
	300,						// chaseEnemyTolerance
	g_SimpleFollowFormationSlots,
};


//-------------------------------------

static AI_FollowSlot_t g_WideFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 240, -1, 128 },
};

static AI_FollowFormation_t g_WideFollowFormation = 
{
	"Wide",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_WideFollowFormationSlots),
	168,						// followPointTolerance
	72,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	190,						// walkTolerance
	600,						// coverTolerance
	600,						// enemyLOSTolerance
	600,						// chaseEnemyTolerance
	g_WideFollowFormationSlots,
};

//---------------------------------------------
// Antlion use very loose following criteria

static AI_FollowSlot_t g_AntlionFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 150, 250, -1, 128 },
};

static AI_FollowFormation_t g_AntlionFollowFormation = 
{
	"Antlion",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_AntlionFollowFormationSlots),
	168,						// followPointTolerance
	36,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	190,						// walkTolerance
	1024,						// coverTolerance
	1024,						// enemyLOSTolerance
	1024,						// chaseEnemyTolerance
	g_AntlionFollowFormationSlots,
};

//-------------------------------------

#define COMMANDER_TOLERANCE (13.0 * 1.415)

static AI_FollowSlot_t g_CommanderFollowFormationSlots[] = 
{
	{ 2, { 0, 0, 0 }, 0, COMMANDER_TOLERANCE, COMMANDER_TOLERANCE, -1, 48 },
	{ 1, { 0, 0, 0 }, 0, COMMANDER_TOLERANCE, COMMANDER_TOLERANCE, -1, 48 },
	{ 1, { 0, 0, 0 }, 0, COMMANDER_TOLERANCE, COMMANDER_TOLERANCE, -1, 48 },
	{ 1, { 0, 0, 0 }, 0, COMMANDER_TOLERANCE, COMMANDER_TOLERANCE, -1, 48 },
};

static AI_FollowFormation_t g_CommanderFollowFormation = 
{
	"Commander",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_CommanderFollowFormationSlots),
	168,						// followPointTolerance
	6,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	12,							// walkTolerance
	300,						// coverTolerance
	300,						// enemyLOSTolerance
	300,						// chaseEnemyTolerance
	g_CommanderFollowFormationSlots,
};

//-------------------------------------

static AI_FollowSlot_t g_TightFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 0,  0, -1, 48 },
	{ 1, { 0, 0, 0 }, 0, 0,  0, -1, 48 },
	{ 1, { 0, 0, 0 }, 0, 0,  0, -1, 48 },
	{ 1, { 0, 0, 0 }, 0, 0,  0, -1, 48 },
};

static AI_FollowFormation_t g_TightFollowFormation = 
{
	"Tight",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_CommanderFollowFormationSlots),
	48,							// followPointTolerance
	6,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	12,							// walkTolerance
	300,						// coverTolerance
	32,							// enemyLOSTolerance
	32,							// chaseEnemyTolerance
	g_TightFollowFormationSlots,
};

//-------------------------------------

static AI_FollowSlot_t g_MediumFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
	{ 1, { 0, 0, 0 }, 0, 156, 156, -1, 128 },
};

static AI_FollowFormation_t g_MediumFollowFormation = 
{
	"Medium",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_MediumFollowFormationSlots),
	168,						// followPointTolerance
	36,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	190,						// walkTolerance
	300,						// coverTolerance
	300,						// enemyLOSTolerance
	300,						// chaseEnemyTolerance
	g_MediumFollowFormationSlots,
};

//-------------------------------------

static AI_FollowSlot_t g_SidekickFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
};

static AI_FollowFormation_t g_SidekickFollowFormation = 
{
	"Sidekick",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS | AIFF_REQUIRE_LOS_OUTSIDE_COMBAT,
	ARRAYSIZE(g_SidekickFollowFormationSlots),
	168,						// followPointTolerance
	36,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	190,						// walkTolerance
	300,						// coverTolerance
	300,						// enemyLOSTolerance
	300,						// chaseEnemyTolerance
	g_SidekickFollowFormationSlots,
};


//-------------------------------------
// Used for hunters following striders
//-------------------------------------
static AI_FollowSlot_t g_HunterFollowFormationSlots[] = 
{
	{ 3, { 480, -240, -400 }, 0, 48, 64, 1000, 60 },
	{ 3, { 480, 240, -400 }, 0, 48, 64, 1000, 60 },
	{ 2, { 480, 0, -400 }, 0, 48, 64, 1000, 60 },
	{ 1, { -240, 0, -400 }, 0, 48, 64, 1000, 60 },
};

static AI_FollowFormation_t g_HunterFollowFormation = 
{
	"Hunter",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS,
	ARRAYSIZE(g_HunterFollowFormationSlots),
	48,							// followPointTolerance
	48,							// targetMoveTolerance
	60,//180,						// repathOnRouteTolerance
	0,							// walkTolerance
	960,						// coverTolerance
	960,						// enemyLOSTolerance
	1920,						// chaseEnemyTolerance
	g_HunterFollowFormationSlots,
};


//-------------------------------------

static AI_FollowSlot_t g_VortigauntFollowFormationSlots[] = 
{
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
	{ 1, { 0, 0, 0 }, 0, 120, 160, 256, 128 },
};

static AI_FollowFormation_t g_VortigauntFollowFormation = 
{
	"Vortigaunt",
	AIFF_DEFAULT | AIFF_USE_FOLLOW_POINTS | AIFF_REQUIRE_LOS_OUTSIDE_COMBAT,
	ARRAYSIZE(g_VortigauntFollowFormationSlots),
	168,						// followPointTolerance
	36,							// targetMoveTolerance
	60,							// repathOnRouteTolerance
	190,						// walkTolerance
	300,						// coverTolerance
	(50*12),					// enemyLOSTolerance
	(50*12),					// chaseEnemyTolerance
	g_VortigauntFollowFormationSlots,
};


//-----------------------------------------------------------------------------
// NOTE: these must correspond with the AI_Formations_t enumeration in AI_Behavior_Follow.h!!
//-----------------------------------------------------------------------------
AI_FollowFormation_t *g_AI_Formations[] =
{
	&g_SimpleFollowFormation,
	&g_WideFollowFormation,
	&g_AntlionFollowFormation,
	&g_CommanderFollowFormation,
	&g_TightFollowFormation,
	&g_MediumFollowFormation,
	&g_SidekickFollowFormation,
	&g_HunterFollowFormation,
	&g_VortigauntFollowFormation,
};

AI_FollowFormation_t *AIGetFormation( AI_Formations_t formation )
{
	if ( formation < 0 )
		formation = (AI_Formations_t)0;
	else if ( formation >= ARRAYSIZE( g_AI_Formations ) )
		formation = (AI_Formations_t)(ARRAYSIZE( g_AI_Formations ) - 1 );
		
	return g_AI_Formations[formation];
}

//---------------------------------------------------------

bool CAI_FollowManager::AddFollower( CBaseEntity *pTarget, CAI_BaseNPC *pFollower, AI_Formations_t formation, AI_FollowManagerInfoHandle_t *pHandle )
{
	AI_FollowGroup_t *pGroup = FindCreateGroup( pTarget, formation );
	int slot = FindBestSlot( pGroup );

	if ( slot != -1 )
	{
		MEM_ALLOC_CREDIT();

		AI_FollowSlot_t *pSlot 		= &pGroup->pFormation->pSlots[slot];

		int i = pGroup->followers.AddToTail( );

		AI_Follower_t *iterNode = &pGroup->followers[i];
		iterNode->hFollower 	= pFollower;
		iterNode->slot 			= slot;
		iterNode->pGroup		= pGroup;

		pGroup->slotUsage.Set( slot );
		
		CalculateFieldsFromSlot( pSlot, &iterNode->navInfo );
		
		pHandle->m_hFollower = i;
		pHandle->m_pGroup = pGroup;
		return true;
	}

	pHandle->m_hFollower = 0;
	pHandle->m_pGroup = NULL;
	return false;
}

//-------------------------------------

bool CAI_FollowManager::CalcFollowPosition( AI_FollowManagerInfoHandle_t& hInfo, AI_FollowNavInfo_t *pNavInfo )
{
	if ( hInfo.m_pGroup && hInfo.m_hFollower )
	{
		AI_FollowGroup_t *pGroup = hInfo.m_pGroup;
		Assert( pGroup->hFollowTarget.Get() );
		CBaseEntity *pTarget = pGroup->hFollowTarget;

		AI_Follower_t *iterNode = &pGroup->followers[hInfo.m_hFollower];
		if ( iterNode->navInfo.position != vec3_origin )
		{
			QAngle angles = pTarget->GetLocalAngles();
			angles.x = angles.z = 0;
				
			matrix3x4_t fRotateMatrix;
			AngleMatrix(angles, fRotateMatrix);
			
			VectorRotate( iterNode->navInfo.position, fRotateMatrix, pNavInfo->position);
			pNavInfo->position += pTarget->WorldSpaceCenter();
		}
		else
		{
			pNavInfo->position = iterNode->navInfo.position + pTarget->WorldSpaceCenter();
		}
			
		pNavInfo->tolerance 			= iterNode->navInfo.tolerance;
		pNavInfo->range 				= iterNode->navInfo.range;
		pNavInfo->Zrange 				= iterNode->navInfo.Zrange;
		pNavInfo->flags					= pGroup->pFormation->flags;
		pNavInfo->followPointTolerance 	= pGroup->pFormation->followPointTolerance;
		pNavInfo->targetMoveTolerance 	= pGroup->pFormation->targetMoveTolerance;
		pNavInfo->repathOnRouteTolerance = pGroup->pFormation->repathOnRouteTolerance;
		pNavInfo->walkTolerance 		= pGroup->pFormation->walkTolerance;
		pNavInfo->coverTolerance		= pGroup->pFormation->coverTolerance;
		pNavInfo->enemyLOSTolerance		= pGroup->pFormation->enemyLOSTolerance;
		pNavInfo->chaseEnemyTolerance	= pGroup->pFormation->chaseEnemyTolerance;
		return true;
	}		
	return false;
}

//-------------------------------------

bool CAI_FollowManager::RedistributeSlots( AI_FollowGroup_t *pGroup )
{
	bool result = false;

	CUtlRBTree<CBaseEntity *> movedFollowers;
	SetDefLessFunc( movedFollowers );

	const Vector &originFollowed = pGroup->hFollowTarget->GetAbsOrigin();
	int 		  bestSlot;

	while ( ( bestSlot = FindBestSlot( pGroup ) ) != -1 && ((int)movedFollowers.Count() < pGroup->followers.Count()) )
	{
		AI_FollowSlot_t *  pSlot 	  = &pGroup->pFormation->pSlots[bestSlot];
		Vector			   slotPos	  = originFollowed + pSlot->position;
		int  h			= pGroup->followers.Head();
		int  hBest 		= pGroup->followers.InvalidIndex();
		float 			   distSqBest = FLT_MAX;
		
		while ( h != pGroup->followers.InvalidIndex() )
		{
			AI_Follower_t *p = &pGroup->followers[h];

			if ( movedFollowers.Find( p->hFollower ) == movedFollowers.InvalidIndex() && 
				 ( p->slot == -1 || pSlot->priority > pGroup->pFormation->pSlots[p->slot].priority ) )
			{
				float distSqCur = ( p->hFollower->GetAbsOrigin() - slotPos ).LengthSqr();
				if ( distSqCur < distSqBest )
				{
					hBest = h;
				}
			}

			h = pGroup->followers.Next( h );
		}
		
		if ( hBest == pGroup->followers.InvalidIndex() )
			break;
		
		AI_Follower_t *pBest = &pGroup->followers[hBest];
		if ( pBest->slot != -1 )
		{
			pGroup->slotUsage.Clear( pBest->slot );
		}
		pBest->slot = bestSlot;
		CalculateFieldsFromSlot( pSlot, &pBest->navInfo );
		pGroup->slotUsage.Set( bestSlot );
		movedFollowers.Insert( pBest->hFollower );
		result = true;
	}
	return result;
}

//-------------------------------------

void CAI_FollowManager::ChangeFormation( AI_FollowManagerInfoHandle_t& hInfo, AI_Formations_t formation )
{
	if ( !hInfo.m_pGroup || !hInfo.m_hFollower )
		return;

	AI_FollowGroup_t *pGroup = hInfo.m_pGroup;
	AI_FollowFormation_t *pNewFormation = AIGetFormation( formation );
	if ( pNewFormation == pGroup->pFormation )
		return;

	int h = pGroup->followers.Head();
		
	while ( h != pGroup->followers.InvalidIndex() )
	{
		CAI_FollowBehavior *pFollowBehavior;
		
		AI_Follower_t *p = &pGroup->followers[h];
		p->slot = -1;
		p->hFollower->GetBehavior( &pFollowBehavior );
		Assert( pFollowBehavior );
		if ( pFollowBehavior )
		{
			pFollowBehavior->m_params.formation = formation;
			pFollowBehavior->m_TargetMonitor.ClearMark();
			pFollowBehavior->SetCondition( CAI_FollowBehavior::COND_TARGET_MOVED_FROM_MARK );
			pFollowBehavior->m_bTargetUnreachable = false;
		}
		
		h = pGroup->followers.Next( h );
	}
	
	pGroup->slotUsage.ClearAll();
	pGroup->pFormation = pNewFormation;
	pGroup->slotUsage.Resize( pGroup->pFormation->nSlots );
	
	RedistributeSlots( pGroup );
	
#ifdef DEBUG
	h = pGroup->followers.Head();
	while ( h != pGroup->followers.InvalidIndex() )
	{
		AI_Follower_t *p = &pGroup->followers[h];
		Assert( p->slot != -1 );
		h = pGroup->followers.Next( h );
	}
#endif
}

//-------------------------------------

void CAI_FollowManager::RemoveFollower( AI_FollowManagerInfoHandle_t& hInfo )
{
	if ( hInfo.m_pGroup && hInfo.m_hFollower )
	{
		AI_FollowGroup_t *pGroup = hInfo.m_pGroup;
		AI_Follower_t* iterNode = &pGroup->followers[hInfo.m_hFollower];

		int slot = iterNode->slot;
		pGroup->slotUsage.Clear( slot );
		pGroup->followers.Remove( hInfo.m_hFollower );
		if ( pGroup->followers.Count() == 0 )
		{
			RemoveGroup( pGroup );
		}
		else
		{
			if ( pGroup->hFollowTarget != NULL ) // NULL on level unload
			{
				RedistributeSlots( pGroup );
			}
		}
	}		
}

//-------------------------------------

int CAI_FollowManager::FindBestSlot( AI_FollowGroup_t *pGroup )
{
	// @TODO (toml 02-28-03): crude placeholder
	int nSlots = pGroup->pFormation->nSlots;
	
	int best = -1;
	int bestPriority = -1;
	
	for ( int i = 0; i < nSlots; i++ )
	{
		if ( !pGroup->slotUsage.IsBitSet( i ) && pGroup->pFormation->pSlots[i].priority > bestPriority )
		{
			bestPriority = pGroup->pFormation->pSlots[i].priority;
			best = i;
		}
	}
	return best;
}

//-------------------------------------

void CAI_FollowManager::CalculateFieldsFromSlot( AI_FollowSlot_t *pSlot, AI_FollowNavInfo_t *pFollowerInfo )
{
	// @TODO (toml 02-28-03): placeholder. Force break if someone tries to actually use
	Assert( pSlot->positionVariability == 0.0 );
	//Assert( pSlot->tolerance == AIN_DEF_TOLERANCE );

	pFollowerInfo->position		= pSlot->position;
	pFollowerInfo->range 		= random->RandomFloat( pSlot->rangeMin, pSlot->rangeMax );
	pFollowerInfo->Zrange		= pSlot->Zrange;
	pFollowerInfo->tolerance 	= pSlot->tolerance;
}

//-------------------------------------

AI_FollowGroup_t *CAI_FollowManager::FindCreateGroup( CBaseEntity *pTarget, AI_Formations_t formation )
{
	AI_FollowGroup_t *pGroup = FindGroup( pTarget );
	
	if ( !pGroup )
	{
		{
		MEM_ALLOC_CREDIT();
		pGroup = new AI_FollowGroup_t;
		}
		
		pGroup->pFormation = AIGetFormation( formation );
		pGroup->slotUsage.Resize( pGroup->pFormation->nSlots );
		pGroup->hFollowTarget = pTarget;
		
		m_groups.AddToHead( pGroup );
	}
	
	return pGroup;
}

//-------------------------------------

void CAI_FollowManager::RemoveGroup( AI_FollowGroup_t *pGroup )
{
	for ( int i = 0; i < m_groups.Count(); i++ )
	{
		if ( m_groups[i] == pGroup )
		{
			delete m_groups[i];
			m_groups.FastRemove(i);
			return;
		}
	}
}

//-------------------------------------

AI_FollowGroup_t *CAI_FollowManager::FindGroup( CBaseEntity *pTarget )
{
	for ( int i = 0; i < m_groups.Count(); i++ )
	{
		if ( m_groups[i]->hFollowTarget == pTarget )
			return m_groups[i];
	}
	return NULL;
}

//-------------------------------------

AI_FollowGroup_t *CAI_FollowManager::FindFollowerGroup( CBaseEntity *pFollower )
{
	for ( int i = 0; i < m_groups.Count(); i++ )
	{
		int h = m_groups[i]->followers.Head();
		while( h != m_groups[i]->followers.InvalidIndex() )
		{
			AI_Follower_t *p = &m_groups[i]->followers[h];
			if ( p->hFollower.Get() == pFollower )
				return m_groups[i];
			h = m_groups[i]->followers.Next( h );
		}
	}
	return NULL;
}
	
//-----------------------------------------------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_FollowBehavior)

	DECLARE_TASK(TASK_CANT_FOLLOW)
	DECLARE_TASK(TASK_FACE_FOLLOW_TARGET)
	DECLARE_TASK(TASK_MOVE_TO_FOLLOW_POSITION)
	DECLARE_TASK(TASK_GET_PATH_TO_FOLLOW_POSITION)
	DECLARE_TASK(TASK_SET_FOLLOW_TARGET_MARK)
	DECLARE_TASK(TASK_FOLLOWER_FACE_TACTICAL)
	DECLARE_TASK(TASK_SET_FOLLOW_DELAY)
	DECLARE_TASK(TASK_GET_PATH_TO_FOLLOW_POINT)
	DECLARE_TASK(TASK_ARRIVE_AT_FOLLOW_POINT)
	DECLARE_TASK(TASK_BEGIN_STAND_AT_WAIT_POINT)
	DECLARE_TASK(TASK_SET_FOLLOW_POINT_STAND_SCHEDULE)

	DECLARE_CONDITION(COND_TARGET_MOVED_FROM_MARK)
	DECLARE_CONDITION(COND_FOUND_WAIT_POINT)
	DECLARE_CONDITION(COND_FOLLOW_DELAY_EXPIRED)
	DECLARE_CONDITION(COND_FOLLOW_TARGET_VISIBLE)
	DECLARE_CONDITION(COND_FOLLOW_TARGET_NOT_VISIBLE)
	DECLARE_CONDITION(COND_FOLLOW_WAIT_POINT_INVALID)
	DECLARE_CONDITION(COND_FOLLOW_PLAYER_IS_LIT)
	DECLARE_CONDITION(COND_FOLLOW_PLAYER_IS_NOT_LIT)

	//=========================================================
	// > SCHED_FOLLOWER_MOVE_AWAY_END
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FOLLOWER_MOVE_AWAY_END,

		"	Tasks"
		"		 TASK_SET_FAIL_SCHEDULE					SCHEDULE:SCHED_FOLLOWER_MOVE_AWAY_FAIL "
		"		 TASK_STOP_MOVING						0"
		"		 TASK_FACE_FOLLOW_TARGET				0"
		"		 TASK_SET_FOLLOW_DELAY					2"
		""
		"	Interrupts"
		"		COND_PLAYER_PUSHING"
	)

	//=========================================================
	// > SCHED_FOLLOWER_MOVE_AWAY_FAIL
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FOLLOWER_MOVE_AWAY_FAIL,

		"	Tasks"
		"		 TASK_STOP_MOVING						0"
		"		 TASK_FACE_FOLLOW_TARGET				0"
		"		 TASK_SET_FOLLOW_DELAY					2"
		""
		"	Interrupts"
		"		COND_PLAYER_PUSHING"
	)

	//=========================================================
	// > SCHED_FOLLOW
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_FOLLOW,

		"	Tasks"
		"		TASK_GET_PATH_TO_FOLLOW_POSITION 0"
		"		TASK_MOVE_TO_FOLLOW_POSITION	0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_SET_SCHEDULE				SCHEDULE:SCHED_TARGET_FACE "
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
		"		COND_BETTER_WEAPON_AVAILABLE"
	);

	//=========================================================
	// > SCHED_MOVE_TO_FACE_FOLLOW_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_MOVE_TO_FACE_FOLLOW_TARGET,

		"	Tasks"
//		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
//		"		TASK_FACE_FOLLOW_TARGET				0"
//		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_FOLLOW"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
	)
	
	//=========================================================
	// > SCHED_FACE_FOLLOW_TARGET
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_FACE_FOLLOW_TARGET,

		"	Tasks"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_FACE_FOLLOW_TARGET				0"
		"		TASK_SET_ACTIVITY					ACTIVITY:ACT_IDLE"
		"		TASK_SET_SCHEDULE					SCHEDULE:SCHED_FOLLOWER_IDLE_STAND "
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
		"		COND_GIVE_WAY"
	)

	//=========================================================
	// > SCHED_FOLLOWER_GO_TO_WAIT_POINT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FOLLOWER_GO_TO_WAIT_POINT,

		"	Tasks"
		"		TASK_LOCK_HINTNODE			0		" // this will fail the schedule if no hint node or not already lockable
		"		TASK_SET_FAIL_SCHEDULE		SCHEDULE:SCHED_FOLLOWER_GO_TO_WAIT_POINT_FAIL"
		"		TASK_SET_TOLERANCE_DISTANCE	4"
		"		TASK_GET_PATH_TO_FOLLOW_POINT	0"
		"		TASK_SET_FOLLOW_TARGET_MARK 0"
		"		TASK_WALK_PATH				0"
		"		TASK_WAIT_FOR_MOVEMENT		0"
		"		TASK_ARRIVE_AT_FOLLOW_POINT	0"
		"		TASK_SET_FOLLOW_POINT_STAND_SCHEDULE 0"

		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
		"		COND_TARGET_MOVED_FROM_MARK"
	)

	//=========================================================
	// > SCHED_FOLLOWER_GO_TO_WAIT_POINT_FAIL
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FOLLOWER_GO_TO_WAIT_POINT_FAIL,

		"	Tasks"
		"		TASK_CLEAR_HINTNODE			.5"
		"		TASK_SET_FOLLOW_DELAY		1"
		""
		"	Interrupts"
	)

	//=========================================================
	// > SCHED_FOLLOWER_STAND_AT_WAIT_POINT
	//=========================================================
	DEFINE_SCHEDULE 
	(
		SCHED_FOLLOWER_STAND_AT_WAIT_POINT,

		"	Tasks"
		"		TASK_BEGIN_STAND_AT_WAIT_POINT 0"
		"		TASK_PLAY_HINT_ACTIVITY		0"
		"		TASK_SET_SCHEDULE			SCHEDULE:SCHED_FOLLOWER_STAND_AT_WAIT_POINT "
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_HEAR_DANGER"
		"		COND_PROVOKED"
		"		COND_PLAYER_PUSHING"
		"		COND_TARGET_MOVED_FROM_MARK"
		"		COND_GIVE_WAY"
		"		COND_FOLLOW_WAIT_POINT_INVALID"
//		"		COND_IDLE_INTERRUPT"
	)

	DEFINE_SCHEDULE
	(
		SCHED_FOLLOWER_IDLE_STAND,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
//		"		TASK_SET_FOLLOW_TARGET_MARK		0"
		"		TASK_WAIT						2.5"
		"		TASK_FACE_FOLLOW_TARGET			0"
		"		TASK_SET_ACTIVITY				ACTIVITY:ACT_IDLE"
		"		TASK_WAIT						3"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_GIVE_WAY"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_BULLET_IMPACT"
		"		COND_PLAYER_PUSHING"
		"		COND_TARGET_MOVED_FROM_MARK"
		"		COND_FOLLOW_DELAY_EXPIRED"
		"		COND_FOUND_WAIT_POINT"
		"		COND_IDLE_INTERRUPT"
		"		COND_BETTER_WEAPON_AVAILABLE"
	)

	DEFINE_SCHEDULE
	(
	SCHED_FOLLOWER_COMBAT_FACE,

		"	Tasks"
		"		TASK_STOP_MOVING		0"
		"		TASK_SET_ACTIVITY		ACTIVITY:ACT_IDLE"
		"		TASK_FACE_ENEMY			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_SEE_FEAR"
		"		COND_CAN_RANGE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_NO_PRIMARY_AMMO"
		"		COND_LIGHT_DAMAGE"
		"		COND_HEAVY_DAMAGE"
		"		COND_SMELL"
		"		COND_PROVOKED"
		"		COND_GIVE_WAY"
		"		COND_HEAR_DANGER"
		"		COND_HEAR_COMBAT"
		"		COND_HEAR_BULLET_IMPACT"
		"		COND_PLAYER_PUSHING"
		"		COND_TARGET_MOVED_FROM_MARK"
		"		COND_FOLLOW_DELAY_EXPIRED"
		"		COND_FOUND_WAIT_POINT"
		"		COND_BETTER_WEAPON_AVAILABLE"
	)

	AI_END_CUSTOM_SCHEDULE_PROVIDER()

//=============================================================================

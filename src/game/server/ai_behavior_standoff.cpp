//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Combat behaviors for AIs in a relatively self-preservationist mode.
//			Lots of cover taking and attempted shots out of cover.
//
//=============================================================================//

#include "cbase.h"

#include "ai_hint.h"
#include "ai_node.h"
#include "ai_navigator.h"
#include "ai_tacticalservices.h"
#include "ai_behavior_standoff.h"
#include "ai_senses.h"
#include "ai_squad.h"
#include "ai_goalentity.h"
#include "ndebugoverlay.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define GOAL_POSITION_INVALID	Vector( FLT_MAX, FLT_MAX, FLT_MAX )

ConVar DrawBattleLines( "ai_drawbattlelines", "0", FCVAR_CHEAT );


// XXX(JohnS): The old parameters below triggered a warning -- fPlayerIsBattleline field used to be "1.5" which
//             implicitly cast to true.  Given that there are two floats followed by three ints, it seems all these
//             fields are off-by-one, but this code hasn't been touched in a very long time so I'm going to avoid
//             changing its behavior drastically now.  It probably has never used the originally intended values.  The
//             new values are just expanding what they would've been implicitly filled with.
//
// static AI_StandoffParams_t AI_DEFAULT_STANDOFF_PARAMS = { AIHCR_MOVE_ON_COVER, true, 1.5, 2.5, 1, 3, 25, 0 };
static AI_StandoffParams_t AI_DEFAULT_STANDOFF_PARAMS = { AIHCR_MOVE_ON_COVER, true, true, 2.5, 1., 3, 25, 0, false, 0.f };
// Suspected originally intended values:
//
// static AI_StandoffParams_t AI_DEFAULT_STANDOFF_PARAMS = { AIHCR_MOVE_ON_COVER, true, true(?), 1.5, 2.5, 1, 3, 25, false(?), 0.(?) };

#define MAKE_ACTMAP_KEY( posture, activity ) ( (((unsigned)(posture)) << 16) | ((unsigned)(activity)) )

// #define DEBUG_STANDOFF 1


#ifdef DEBUG_STANDOFF
#define StandoffMsg( msg ) 					DevMsg( GetOuter(), msg )
#define StandoffMsg1( msg, a ) 				DevMsg( GetOuter(), msg, a )
#define StandoffMsg2( msg, a, b ) 			DevMsg( GetOuter(), msg, a, b )
#define StandoffMsg3( msg, a, b, c ) 		DevMsg( GetOuter(), msg, a, b, c )
#define StandoffMsg4( msg, a, b, c, d ) 	DevMsg( GetOuter(), msg, a, b, c, d )
#define StandoffMsg5( msg, a, b, c, d, e )	DevMsg( GetOuter(), msg, a, b, c, d, e )
#else
#define StandoffMsg( msg ) 					((void)0)
#define StandoffMsg1( msg, a ) 				((void)0)
#define StandoffMsg2( msg, a, b ) 			((void)0)
#define StandoffMsg3( msg, a, b, c ) 		((void)0)
#define StandoffMsg4( msg, a, b, c, d ) 	((void)0)
#define StandoffMsg5( msg, a, b, c, d, e )	((void)0)
#endif

//-----------------------------------------------------------------------------
//
// CAI_BattleLine
//
//-----------------------------------------------------------------------------

const float AIBL_THINK_INTERVAL = 0.3;

class CAI_BattleLine : public CBaseEntity
{
	DECLARE_CLASS( CAI_BattleLine, CBaseEntity );

public:
	string_t		m_iszActor;
	bool			m_fActive;
	bool			m_fStrict;

	void Spawn()
	{
		if ( m_fActive )
		{
			SetThink(&CAI_BattleLine::MovementThink);
			SetNextThink( gpGlobals->curtime + AIBL_THINK_INTERVAL );
			m_SelfMoveMonitor.SetMark( this, 60 );
		}
	}

	virtual void InputActivate( inputdata_t &inputdata )		
	{ 
		if ( !m_fActive )
		{
			m_fActive = true; 
			NotifyChangeTacticalConstraints(); 

			SetThink(&CAI_BattleLine::MovementThink);
			SetNextThink( gpGlobals->curtime + AIBL_THINK_INTERVAL );
			m_SelfMoveMonitor.SetMark( this, 60 );
		}
	}
	
	virtual void InputDeactivate( inputdata_t &inputdata )	
	{ 
		if ( m_fActive )
		{
			m_fActive = false; 
			NotifyChangeTacticalConstraints(); 

			SetThink(NULL);
		}
	}
	
	void UpdateOnRemove()
	{
		if ( m_fActive )
		{
			m_fActive = false; 
			NotifyChangeTacticalConstraints(); 
		}
		BaseClass::UpdateOnRemove();
	}

	bool Affects( CAI_BaseNPC *pNpc )
	{
		const char *pszNamedActor = STRING( m_iszActor );

		if ( pNpc->NameMatches( pszNamedActor ) ||
			 pNpc->ClassMatches( pszNamedActor ) ||
			 ( pNpc->GetSquad() && stricmp( pNpc->GetSquad()->GetName(), pszNamedActor ) == 0 ) )
		{
			return true;
		}
		return false;
	}
	
	void MovementThink()
	{
		if ( m_SelfMoveMonitor.TargetMoved( this ) )
		{
			NotifyChangeTacticalConstraints();
			m_SelfMoveMonitor.SetMark( this, 60 );
		}
		SetNextThink( gpGlobals->curtime + AIBL_THINK_INTERVAL );
	}

private:
	void NotifyChangeTacticalConstraints()
	{
		for ( int i = 0; i < g_AI_Manager.NumAIs(); i++ )
		{
			CAI_BaseNPC *pNpc = (g_AI_Manager.AccessAIs())[i];
			if ( Affects( pNpc ) )
			{
				CAI_StandoffBehavior *pBehavior;
				if ( pNpc->GetBehavior( &pBehavior ) )
				{
					pBehavior->OnChangeTacticalConstraints();
				}
			}
		}
	}

	CAI_MoveMonitor m_SelfMoveMonitor;
	
	DECLARE_DATADESC();
};

//-------------------------------------

LINK_ENTITY_TO_CLASS( ai_battle_line, CAI_BattleLine );

BEGIN_DATADESC( CAI_BattleLine )
	DEFINE_KEYFIELD(	m_iszActor,				FIELD_STRING, 	"Actor"					),
	DEFINE_KEYFIELD(	m_fActive,				FIELD_BOOLEAN,  "Active"				),
	DEFINE_KEYFIELD(	m_fStrict,				FIELD_BOOLEAN,  "Strict"				),
	DEFINE_EMBEDDED( 	m_SelfMoveMonitor ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", 		InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate",		InputDeactivate ),
	
	DEFINE_THINKFUNC( MovementThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
//
// CAI_StandoffBehavior
//
//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( AI_StandoffParams_t )
	DEFINE_FIELD( hintChangeReaction,	FIELD_INTEGER ),
	DEFINE_FIELD( fPlayerIsBattleline,	FIELD_BOOLEAN ),
	DEFINE_FIELD( fCoverOnReload,		FIELD_BOOLEAN ),
	DEFINE_FIELD( minTimeShots,			FIELD_FLOAT ),
	DEFINE_FIELD( maxTimeShots,			FIELD_FLOAT ),
	DEFINE_FIELD( minShots,				FIELD_INTEGER ),
	DEFINE_FIELD( maxShots,				FIELD_INTEGER ),
	DEFINE_FIELD( oddsCover,			FIELD_INTEGER ),
	DEFINE_FIELD( fStayAtCover,			FIELD_BOOLEAN ),
	DEFINE_FIELD( flAbandonTimeLimit,	FIELD_FLOAT ),
END_DATADESC();

BEGIN_DATADESC( CAI_StandoffBehavior )
	DEFINE_FIELD( 		m_fActive, 						FIELD_BOOLEAN ),
	DEFINE_FIELD(		m_fTestNoDamage,				FIELD_BOOLEAN ),
	DEFINE_FIELD( 		m_vecStandoffGoalPosition, 		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( 		m_posture, 						FIELD_INTEGER ),
	DEFINE_EMBEDDED(	m_params ),
	DEFINE_FIELD(		m_hStandoffGoal,				FIELD_EHANDLE ),
	DEFINE_FIELD( 		m_fTakeCover, 					FIELD_BOOLEAN ),
	DEFINE_FIELD( 		m_SavedDistTooFar, 				FIELD_FLOAT ),
	DEFINE_FIELD( 		m_fForceNewEnemy, 				FIELD_BOOLEAN ),
	DEFINE_EMBEDDED( 	m_PlayerMoveMonitor ),
	DEFINE_EMBEDDED( 	m_TimeForceCoverHint ),
	DEFINE_EMBEDDED( 	m_TimePreventForceNewEnemy ),
	DEFINE_EMBEDDED( 	m_RandomCoverChangeTimer ),
	// 											m_UpdateBattleLinesSemaphore 	(not saved, only an in-think item)
	// 											m_BattleLines 					(not saved, rebuilt)
	DEFINE_FIELD( 		m_fIgnoreFronts, 				FIELD_BOOLEAN ),
	//											m_ActivityMap 					(not saved, rebuilt)
	//											m_bHasLowCoverActivity			(not saved, rebuilt)

	DEFINE_FIELD( 		m_nSavedMinShots, 				FIELD_INTEGER ),
	DEFINE_FIELD( 		m_nSavedMaxShots, 				FIELD_INTEGER ),
	DEFINE_FIELD( 		m_flSavedMinRest, 				FIELD_FLOAT ),
	DEFINE_FIELD( 		m_flSavedMaxRest, 				FIELD_FLOAT ),

END_DATADESC();

//-------------------------------------

CAI_StandoffBehavior::CAI_StandoffBehavior( CAI_BaseNPC *pOuter )
 :	CAI_MappedActivityBehavior_Temporary( pOuter )
{
	m_fActive = false;
	SetParameters( AI_DEFAULT_STANDOFF_PARAMS );
	SetPosture( AIP_STANDING );
	m_SavedDistTooFar = FLT_MAX;
	m_fForceNewEnemy = false;
	m_TimePreventForceNewEnemy.Set( 3.0, 6.0 );
	m_fIgnoreFronts = false;
	m_bHasLowCoverActivity = false;
}

//-------------------------------------

void CAI_StandoffBehavior::SetActive( bool fActive )
{
	if ( fActive !=	m_fActive )
	{
		if ( fActive )
		{
			GetOuter()->SpeakSentence( STANDOFF_SENTENCE_BEGIN_STANDOFF );
		}
		else
		{
			GetOuter()->SpeakSentence( STANDOFF_SENTENCE_END_STANDOFF );
		}

		m_fActive = fActive;
		NotifyChangeBehaviorStatus();
	}
}

//-------------------------------------

void CAI_StandoffBehavior::SetParameters( const AI_StandoffParams_t &params, CAI_GoalEntity *pGoalEntity )
{
	m_params = params;
	m_hStandoffGoal = pGoalEntity;
	m_vecStandoffGoalPosition = GOAL_POSITION_INVALID;
	if ( GetOuter() && GetOuter()->GetShotRegulator() )
	{
		GetOuter()->GetShotRegulator()->SetBurstShotCountRange( m_params.minShots, m_params.maxShots );
		GetOuter()->GetShotRegulator()->SetRestInterval( m_params.minTimeShots, m_params.maxTimeShots );
	}
}

//-------------------------------------

bool CAI_StandoffBehavior::CanSelectSchedule()
{
	if ( !m_bHasLowCoverActivity )
		m_fActive = false;

	if ( !m_fActive )
		return false;
		
	return ( GetNpcState() == NPC_STATE_COMBAT && GetOuter()->GetActiveWeapon() != NULL );
}

//-------------------------------------

void CAI_StandoffBehavior::Spawn()
{
	BaseClass::Spawn();
	UpdateTranslateActivityMap();
}

//-------------------------------------

void CAI_StandoffBehavior::BeginScheduleSelection()
{
	m_fTakeCover = true;

	// FIXME: Improve!!!
	GetOuter()->GetShotRegulator()->GetBurstShotCountRange( &m_nSavedMinShots, &m_nSavedMaxShots );
	GetOuter()->GetShotRegulator()->GetRestInterval( &m_flSavedMinRest, &m_flSavedMaxRest );

	GetOuter()->GetShotRegulator()->SetBurstShotCountRange( m_params.minShots, m_params.maxShots );
	GetOuter()->GetShotRegulator()->SetRestInterval( m_params.minTimeShots, m_params.maxTimeShots );
	GetOuter()->GetShotRegulator()->Reset();

	m_SavedDistTooFar = GetOuter()->m_flDistTooFar;
	GetOuter()->m_flDistTooFar = FLT_MAX;
	
	m_TimeForceCoverHint.Set( 8, false );
	m_RandomCoverChangeTimer.Set( 8, 16, false );
	UpdateTranslateActivityMap();
}


void CAI_StandoffBehavior::OnUpdateShotRegulator()
{
	GetOuter()->GetShotRegulator()->SetBurstShotCountRange( m_params.minShots, m_params.maxShots );
	GetOuter()->GetShotRegulator()->SetRestInterval( m_params.minTimeShots, m_params.maxTimeShots );
}


//-------------------------------------

void CAI_StandoffBehavior::EndScheduleSelection()
{
	UnlockHintNode();

	m_vecStandoffGoalPosition = GOAL_POSITION_INVALID;
	GetOuter()->m_flDistTooFar = m_SavedDistTooFar;

	// FIXME: Improve!!!
	GetOuter()->GetShotRegulator()->SetBurstShotCountRange( m_nSavedMinShots, m_nSavedMaxShots );
	GetOuter()->GetShotRegulator()->SetRestInterval( m_flSavedMinRest, m_flSavedMaxRest );
}

//-------------------------------------

void CAI_StandoffBehavior::PrescheduleThink()
{
	VPROF_BUDGET( "CAI_StandoffBehavior::PrescheduleThink", VPROF_BUDGETGROUP_NPCS );

	BaseClass::PrescheduleThink();
	
	if( DrawBattleLines.GetInt() != 0 )
	{
		CBaseEntity *pEntity = NULL;
		while ((pEntity = gEntList.FindEntityByClassname( pEntity, "ai_battle_line" )) != NULL)
		{
			// Visualize the battle line and its normal.
			CAI_BattleLine *pLine = dynamic_cast<CAI_BattleLine *>(pEntity);

			if( pLine->m_fActive )
			{
				Vector normal;

				pLine->GetVectors( &normal, NULL, NULL );

				NDebugOverlay::Line( pLine->GetAbsOrigin() - Vector( 0, 0, 64 ), pLine->GetAbsOrigin() + Vector(0,0,64), 0,255,0, false, 0.1 );
			}
		}
	}
}

//-------------------------------------

void CAI_StandoffBehavior::GatherConditions()
{
	CBaseEntity *pLeader = GetPlayerLeader();
	if ( pLeader && m_TimeForceCoverHint.Expired() )
	{
		if ( m_PlayerMoveMonitor.IsMarkSet() )
		{
			if (m_PlayerMoveMonitor.TargetMoved( pLeader ) )
			{
				OnChangeTacticalConstraints();
				m_PlayerMoveMonitor.ClearMark();
			}
		}
		else
		{
			m_PlayerMoveMonitor.SetMark( pLeader, 60 );
		}
	}

	if ( m_fForceNewEnemy )
	{
		m_TimePreventForceNewEnemy.Reset();
		GetOuter()->SetEnemy( NULL );
	}
	BaseClass::GatherConditions();
	m_fForceNewEnemy = false;

	ClearCondition( COND_ABANDON_TIME_EXPIRED );

	bool bAbandonStandoff = false;
	CAI_Squad *pSquad = GetOuter()->GetSquad();
	AISquadIter_t iter;
	if ( GetEnemy() )
	{
		AI_EnemyInfo_t *pEnemyInfo = GetOuter()->GetEnemies()->Find( GetEnemy() );
		if ( pEnemyInfo &&
			 m_params.flAbandonTimeLimit > 0 && 
			 ( ( pEnemyInfo->timeAtFirstHand != AI_INVALID_TIME && 
			     gpGlobals->curtime  - pEnemyInfo->timeLastSeen > m_params.flAbandonTimeLimit ) ||
			   ( pEnemyInfo->timeAtFirstHand == AI_INVALID_TIME && 
			     gpGlobals->curtime  - pEnemyInfo->timeFirstSeen > m_params.flAbandonTimeLimit * 2 ) ) )
		{
			SetCondition( COND_ABANDON_TIME_EXPIRED );

			bAbandonStandoff = true;

			if ( pSquad )
			{
				for ( CAI_BaseNPC *pSquadMate = pSquad->GetFirstMember( &iter ); pSquadMate; pSquadMate = pSquad->GetNextMember( &iter ) )
				{
					if ( pSquadMate->IsAlive() && pSquadMate != GetOuter() )
					{
						CAI_StandoffBehavior *pSquadmateStandoff;
						pSquadMate->GetBehavior( &pSquadmateStandoff );
						if ( pSquadmateStandoff && pSquadmateStandoff->IsActive() && 
							 pSquadmateStandoff->m_hStandoffGoal == m_hStandoffGoal &&
							 !pSquadmateStandoff->HasCondition( COND_ABANDON_TIME_EXPIRED ) )
						{
							bAbandonStandoff = false;
							break;
						}
					}
				}
			}
		}
	}

	if ( bAbandonStandoff )
	{
		if ( pSquad )
		{
			for ( CAI_BaseNPC *pSquadMate = pSquad->GetFirstMember( &iter ); pSquadMate; pSquadMate = pSquad->GetNextMember( &iter ) )
			{
				CAI_StandoffBehavior *pSquadmateStandoff;
				pSquadMate->GetBehavior( &pSquadmateStandoff );
				if ( pSquadmateStandoff && pSquadmateStandoff->IsActive() && pSquadmateStandoff->m_hStandoffGoal == m_hStandoffGoal )
					pSquadmateStandoff->SetActive( false );
			}
		}
		else
			SetActive( false );
	}
	else if ( GetOuter()->m_debugOverlays & OVERLAY_NPC_SELECTED_BIT )
	{
		if( DrawBattleLines.GetInt() != 0 )
		{
			if ( IsBehindBattleLines( GetAbsOrigin() ) )
			{
				NDebugOverlay::Box( GetOuter()->GetAbsOrigin(), -Vector(48,48,4), Vector(48,48,4), 255,0,0,8, 0.1 );
			}
			else
			{
				NDebugOverlay::Box( GetOuter()->GetAbsOrigin(), -Vector(48,48,4), Vector(48,48,4), 0,255,0,8, 0.1 );
			}
		}
	}
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleUpdateWeapon( void )
{
	// Check if need to reload
	if ( HasCondition ( COND_NO_PRIMARY_AMMO ) || HasCondition ( COND_LOW_PRIMARY_AMMO ))
	{
		StandoffMsg( "Out of ammo, reloading\n" );
		if ( m_params.fCoverOnReload )
		{
			GetOuter()->SpeakSentence( STANDOFF_SENTENCE_OUT_OF_AMMO );
			return SCHED_HIDE_AND_RELOAD;
		}
		
		return SCHED_RELOAD;
	}
	
	// Otherwise, update planned shots to fire before taking cover again
	if ( HasCondition( COND_LIGHT_DAMAGE ) )
	{
		// if hurt:
		int iPercent = random->RandomInt(0,99);

		if ( iPercent <= m_params.oddsCover && GetEnemy() != NULL )
		{
			SetReuseCurrentCover();
			StandoffMsg( "Hurt, firing one more shot before cover\n" );
			if ( !GetOuter()->GetShotRegulator()->IsInRestInterval() )
			{
				GetOuter()->GetShotRegulator()->SetBurstShotsRemaining( 1 );
			}
		}
	}

	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleCheckCover( void )
{
	if ( m_fTakeCover )
	{
		m_fTakeCover = false;
		if ( GetEnemy() )
		{
			GetOuter()->SpeakSentence( STANDOFF_SENTENCE_FORCED_TAKE_COVER );
			StandoffMsg( "Taking forced cover\n" );
			return SCHED_TAKE_COVER_FROM_ENEMY;
		}
	}
		
	if ( GetOuter()->GetShotRegulator()->IsInRestInterval() )
	{
		StandoffMsg( "Regulated to not shoot\n" );
		if ( GetHintType() == HINT_TACTICAL_COVER_LOW )
			SetPosture( AIP_CROUCHING );
		else
			SetPosture( AIP_STANDING );

		if ( random->RandomInt(0,99) < 80 )
			SetReuseCurrentCover();
		return SCHED_TAKE_COVER_FROM_ENEMY;
	}
	
	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleEstablishAim( void )
{
	if ( HasCondition( COND_ENEMY_OCCLUDED ) )
	{
		if ( GetPosture() == AIP_CROUCHING )
		{
			// force a stand up, just in case
			GetOuter()->SpeakSentence( STANDOFF_SENTENCE_STAND_CHECK_TARGET );
			StandoffMsg( "Crouching, standing up to gain LOS\n" );
			SetPosture( AIP_PEEKING );
			return SCHED_STANDOFF;
		}
		else if ( GetPosture() == AIP_PEEKING )
		{
			if ( m_TimePreventForceNewEnemy.Expired() )
			{
				// Look for a new enemy
				m_fForceNewEnemy = true;
				StandoffMsg( "Looking for enemy\n" );
			}
		}
#if 0
		else
		{
			return SCHED_ESTABLISH_LINE_OF_FIRE;
		}
#endif
	}

	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectScheduleAttack( void )
{
	if ( GetPosture() == AIP_PEEKING || GetPosture() == AIP_STANDING )
	{
		if ( !HasCondition( COND_CAN_RANGE_ATTACK1 ) && 
			 !HasCondition( COND_CAN_MELEE_ATTACK1 ) &&
			  HasCondition( COND_TOO_FAR_TO_ATTACK ) )
		{
			if ( GetOuter()->GetActiveWeapon() && ( GetOuter()->GetActiveWeapon()->CapabilitiesGet() & bits_CAP_WEAPON_RANGE_ATTACK1 ) )
			{
				if ( !HasCondition( COND_ENEMY_OCCLUDED ) || random->RandomInt(0,99) < 50 )
					// Don't advance, just fire anyway
					return SCHED_RANGE_ATTACK1;
			}
		}
	}

	return SCHED_NONE;
}

//-------------------------------------

int CAI_StandoffBehavior::SelectSchedule( void )
{
	switch ( GetNpcState() )
	{
		case NPC_STATE_COMBAT:
		{
			int schedule = SCHED_NONE;
			
			schedule = SelectScheduleUpdateWeapon();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			schedule = SelectScheduleCheckCover();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			schedule = SelectScheduleEstablishAim();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			schedule = SelectScheduleAttack();
			if ( schedule != SCHED_NONE )
				return schedule;
				
			break;
		}
	}

	return BaseClass::SelectSchedule();
}

//-------------------------------------

int CAI_StandoffBehavior::TranslateSchedule( int schedule )
{
	if ( schedule == SCHED_CHASE_ENEMY )
	{
		StandoffMsg( "trying SCHED_ESTABLISH_LINE_OF_FIRE\n" );
		return SCHED_ESTABLISH_LINE_OF_FIRE;
	}
	return BaseClass::TranslateSchedule( schedule );
}

//-------------------------------------

void CAI_StandoffBehavior::BuildScheduleTestBits()
{
	BaseClass::BuildScheduleTestBits();

	if ( IsCurSchedule( SCHED_TAKE_COVER_FROM_ENEMY ) )
		GetOuter()->ClearCustomInterruptCondition( COND_NEW_ENEMY );
}

//-------------------------------------

Activity CAI_MappedActivityBehavior_Temporary::GetMappedActivity( AI_Posture_t posture, Activity activity )
{
	if ( posture != AIP_STANDING )
	{
		unsigned short iActivityTranslation = m_ActivityMap.Find( MAKE_ACTMAP_KEY( posture, activity ) );
		if ( iActivityTranslation != m_ActivityMap.InvalidIndex() )
		{
			Activity result = m_ActivityMap[iActivityTranslation];
			return result;
		}
	}
	return ACT_INVALID;
}

//-------------------------------------

Activity CAI_StandoffBehavior::NPC_TranslateActivity( Activity activity )
{
	Activity coverActivity = GetCoverActivity();
	if ( coverActivity != ACT_INVALID )
	{
		if ( activity == ACT_IDLE )
			activity = coverActivity;
		if ( GetPosture() == AIP_STANDING && coverActivity == ACT_COVER_LOW )
			SetPosture( AIP_CROUCHING );
	}
	
	Activity result = GetMappedActivity( GetPosture(), activity );
	if ( result != ACT_INVALID)
		return result;

	return BaseClass::NPC_TranslateActivity( activity );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecPos - 
//-----------------------------------------------------------------------------
void CAI_StandoffBehavior::SetStandoffGoalPosition( const Vector &vecPos )
{
	m_vecStandoffGoalPosition = vecPos;
	UpdateBattleLines();
	OnChangeTacticalConstraints();
	GetOuter()->ClearSchedule( "Standoff goal position changed" );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vecPos - 
//-----------------------------------------------------------------------------
void CAI_StandoffBehavior::ClearStandoffGoalPosition()
{
	if ( m_vecStandoffGoalPosition != GOAL_POSITION_INVALID )
	{
		m_vecStandoffGoalPosition = GOAL_POSITION_INVALID;
		UpdateBattleLines();
		OnChangeTacticalConstraints();
		GetOuter()->ClearSchedule( "Standoff goal position cleared" );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Vector
//-----------------------------------------------------------------------------
Vector CAI_StandoffBehavior::GetStandoffGoalPosition() 
{
	if( m_vecStandoffGoalPosition != GOAL_POSITION_INVALID )
	{
		return m_vecStandoffGoalPosition;
	}
	else if( PlayerIsLeading() )
	{
		return UTIL_GetLocalPlayer()->GetAbsOrigin();
	}
	else
	{
		CAI_BattleLine *pBattleLine = NULL;
		for (;;)
		{
			pBattleLine = (CAI_BattleLine *)gEntList.FindEntityByClassname( pBattleLine, "ai_battle_line" );
			
			if ( !pBattleLine )
				break;
				
			if ( pBattleLine->m_fActive && pBattleLine->Affects( GetOuter() ) )
			{
				StandoffMsg1( "Using battleline %s as goal\n", STRING( pBattleLine->GetEntityName() ) );
				return pBattleLine->GetAbsOrigin();
			}
		}
	}

	return GetAbsOrigin();
}

//-------------------------------------

void CAI_StandoffBehavior::UpdateBattleLines()
{
	if ( m_UpdateBattleLinesSemaphore.EnterThink() )
	{
		// @TODO (toml 06-19-03): This is the quick to code thing. Could use some optimization/caching to not recalc everything (up to) each think
		m_BattleLines.RemoveAll();

		bool bHaveGoalPosition = ( m_vecStandoffGoalPosition != GOAL_POSITION_INVALID );

		if ( bHaveGoalPosition )
		{
			// If we have a valid standoff goal position, it takes precendence.
			const float DIST_GOAL_PLANE = 180;
			
			BattleLine_t goalLine;

			if ( GetDirectionOfStandoff( &goalLine.normal ) )
			{
				goalLine.point = GetStandoffGoalPosition() + goalLine.normal * DIST_GOAL_PLANE;
				m_BattleLines.AddToTail( goalLine );
			}
		}
		else if ( PlayerIsLeading() && GetEnemy() )
		{
			if ( m_params.fPlayerIsBattleline )
			{
				const float DIST_PLAYER_PLANE = 180;
				CBaseEntity *pPlayer = UTIL_GetLocalPlayer();
				
				BattleLine_t playerLine;

				if ( GetDirectionOfStandoff( &playerLine.normal ) )
				{
					playerLine.point = pPlayer->GetAbsOrigin() + playerLine.normal * DIST_PLAYER_PLANE;
					m_BattleLines.AddToTail( playerLine );
				}
			}
		}
		
		CAI_BattleLine *pBattleLine = NULL;
		for (;;)
		{
			pBattleLine = (CAI_BattleLine *)gEntList.FindEntityByClassname( pBattleLine, "ai_battle_line" );
			
			if ( !pBattleLine )
				break;
				
			if ( pBattleLine->m_fActive && (pBattleLine->m_fStrict || !bHaveGoalPosition ) && pBattleLine->Affects( GetOuter() ) )
			{
				BattleLine_t battleLine;
				
				battleLine.point = pBattleLine->GetAbsOrigin();
				battleLine.normal = UTIL_YawToVector( pBattleLine->GetAbsAngles().y );

				m_BattleLines.AddToTail( battleLine );
			}
				
		}
	}
}

//-------------------------------------

bool CAI_StandoffBehavior::IsBehindBattleLines( const Vector &point )
{
	UpdateBattleLines();

	Vector vecToPoint;
	
	for ( int i = 0; i < m_BattleLines.Count(); i++ )
	{
		vecToPoint = point - m_BattleLines[i].point;
		VectorNormalize( vecToPoint );
		vecToPoint.z = 0;
		
		if ( DotProduct( m_BattleLines[i].normal, vecToPoint ) > 0 )
		{
			if( DrawBattleLines.GetInt() != 0 )
			{
				NDebugOverlay::Box( point, -Vector(48,48,4), Vector(48,48,4), 0,255,0,8, 1 );
				NDebugOverlay::Line( point, GetOuter()->GetAbsOrigin(), 0,255,0,true, 1 );
			}
			return false;
		}
	}
	
	if( DrawBattleLines.GetInt() != 0 )
	{
		NDebugOverlay::Box( point, -Vector(48,48,4), Vector(48,48,4), 255,0,0,8, 1 );
		NDebugOverlay::Line( point, GetOuter()->GetAbsOrigin(), 255,0,0,true, 1 );
	}

	return true;
}

//-------------------------------------

bool CAI_StandoffBehavior::IsValidCover( const Vector &vecCoverLocation, const CAI_Hint *pHint )
{
	if ( !BaseClass::IsValidCover( vecCoverLocation, pHint ) )
		return false;

	if ( IsCurSchedule( SCHED_TAKE_COVER_FROM_BEST_SOUND ) )
		return true;

	return ( m_fIgnoreFronts || IsBehindBattleLines( vecCoverLocation ) );
}

//-------------------------------------

bool CAI_StandoffBehavior::IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, const CAI_Hint *pHint )
{
	if ( !BaseClass::IsValidShootPosition( vLocation, pNode, pHint ) )
		return false;

	return ( m_fIgnoreFronts || IsBehindBattleLines( vLocation ) );
}

//-------------------------------------

void CAI_StandoffBehavior::StartTask( const Task_t *pTask )
{
	bool fCallBase = false;
	
	switch ( pTask->iTask )
	{
		case TASK_RANGE_ATTACK1:
		{
			fCallBase = true;
			break;
		}
		
		case TASK_FIND_COVER_FROM_ENEMY:
		{
			StandoffMsg( "TASK_FIND_COVER_FROM_ENEMY\n" );

			// If within time window to force change
			if ( !m_params.fStayAtCover && (!m_TimeForceCoverHint.Expired() || m_RandomCoverChangeTimer.Expired()) )
			{
				m_TimeForceCoverHint.Force();
				m_RandomCoverChangeTimer.Set( 8, 16, false );

				// @TODO (toml 03-24-03):  clean this up be tool-izing base tasks. Right now, this is here to force to not use lateral cover search
				CBaseEntity *pEntity = GetEnemy();

				if ( pEntity == NULL )
				{
					// Find cover from self if no enemy available
					pEntity = GetOuter();
				}

				CBaseEntity *pLeader = GetPlayerLeader();
				if ( pLeader )
				{
					m_PlayerMoveMonitor.SetMark( pLeader, 60 );
				}

				Vector					coverPos			= vec3_origin;
				CAI_TacticalServices *	pTacticalServices	= GetTacticalServices();
				const Vector &			enemyPos			= pEntity->GetAbsOrigin();
				Vector					enemyEyePos			= pEntity->EyePosition();
				float					coverRadius			= GetOuter()->CoverRadius();
				const Vector &			goalPos				= GetStandoffGoalPosition();
				bool					bTryGoalPosFirst	= true;

				if( pLeader && m_vecStandoffGoalPosition == GOAL_POSITION_INVALID )
				{
					if( random->RandomInt(1, 100) <= 50 )
					{
						// Half the time, if the player is leading, try to find a spot near them
						bTryGoalPosFirst = false;
						StandoffMsg( "Not trying goal pos\n" );
					}
				}

				if( bTryGoalPosFirst )
				{
					// Firstly, try to find cover near the goal position.
					pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, 0, 15*12, &coverPos );

					if ( coverPos == vec3_origin )
						pTacticalServices->FindCoverPos( goalPos, enemyPos, enemyEyePos, 15*12-0.1, 40*12, &coverPos );

					StandoffMsg1( "Trying goal pos, %s\n", ( coverPos == vec3_origin  ) ? "failed" :  "succeeded" );
				}

				if ( coverPos == vec3_origin  ) 
				{
					// Otherwise, find a node near to self
					StandoffMsg( "Looking for near cover\n" );
					if ( !GetTacticalServices()->FindCoverPos( enemyPos, enemyEyePos, 0, coverRadius, &coverPos ) ) 
					{
						// Try local lateral cover
						if ( !GetTacticalServices()->FindLateralCover( enemyEyePos, 0, &coverPos ) )
						{
							// At this point, try again ignoring front lines. Any cover probably better than hanging out in the open
							m_fIgnoreFronts = true;
							if ( !GetTacticalServices()->FindCoverPos( enemyPos, enemyEyePos, 0, coverRadius, &coverPos ) ) 
							{
								if ( !GetTacticalServices()->FindLateralCover( enemyEyePos, 0, &coverPos ) )
								{
									Assert( coverPos == vec3_origin );
								}
							}
							m_fIgnoreFronts = false;
						}
					}
				}

				if ( coverPos != vec3_origin )
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
			{
				fCallBase = true;
			}
			break;
		}

		default:
		{
			fCallBase = true;
		}
	}
	
	if ( fCallBase )
		BaseClass::StartTask( pTask );
}

//-------------------------------------

void CAI_StandoffBehavior::OnChangeHintGroup( string_t oldGroup, string_t newGroup )
{
	OnChangeTacticalConstraints();
}

//-------------------------------------

void CAI_StandoffBehavior::OnChangeTacticalConstraints()
{
	if ( m_params.hintChangeReaction > AIHCR_DEFAULT_AI )
		m_TimeForceCoverHint.Set( 8.0, false );
	if ( m_params.hintChangeReaction == AIHCR_MOVE_IMMEDIATE )
		m_fTakeCover = true;
}

//-------------------------------------

bool CAI_StandoffBehavior::PlayerIsLeading()
{
	CBaseEntity *pPlayer = AI_GetSinglePlayer();
	return ( pPlayer && GetOuter()->IRelationType( pPlayer ) == D_LI );
}

//-------------------------------------

CBaseEntity *CAI_StandoffBehavior::GetPlayerLeader()
{
	CBaseEntity *pPlayer = AI_GetSinglePlayer();
	if ( pPlayer && GetOuter()->IRelationType( pPlayer ) == D_LI )
		return pPlayer;
	return NULL;
}

//-------------------------------------

bool CAI_StandoffBehavior::GetDirectionOfStandoff( Vector *pDir )
{
	if ( GetEnemy() )
	{
		*pDir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
		VectorNormalize( *pDir );
		pDir->z = 0;
		return true;
	}
	return false;
}

//-------------------------------------

Hint_e CAI_StandoffBehavior::GetHintType()
{
	CAI_Hint *pHintNode = GetHintNode();
	if ( pHintNode )
		return pHintNode->HintType();
	return HINT_NONE;
}

//-------------------------------------

void CAI_StandoffBehavior::SetReuseCurrentCover()
{
	CAI_Hint *pHintNode = GetHintNode();
	if ( pHintNode && pHintNode->GetNode() && pHintNode->GetNode()->IsLocked() )
		pHintNode->GetNode()->Unlock();
}

//-------------------------------------

void CAI_StandoffBehavior::UnlockHintNode()
{
	CAI_Hint *pHintNode = GetHintNode();
	if ( pHintNode )
	{
		if ( pHintNode->IsLocked() && pHintNode->IsLockedBy( GetOuter() ) )
			pHintNode->Unlock();
		CAI_Node *pNode = pHintNode->GetNode();
		if ( pNode && pNode->IsLocked() )
			pNode->Unlock();
		ClearHintNode();
	}
}


//-------------------------------------

Activity CAI_StandoffBehavior::GetCoverActivity()
{
	CAI_Hint *pHintNode = GetHintNode();
	if ( pHintNode && pHintNode->HintType() == HINT_TACTICAL_COVER_LOW )
		return GetOuter()->GetCoverActivity( pHintNode );
	return ACT_INVALID;
}


//-------------------------------------

struct AI_ActivityMapping_t
{
	AI_Posture_t	posture;
	Activity		activity;
	const char *	pszWeapon;
	Activity		translation;
};

void CAI_MappedActivityBehavior_Temporary::UpdateTranslateActivityMap()
{
	AI_ActivityMapping_t mappings[] =		// This array cannot be static, as some activity values are set on a per-map-load basis
	{
		{	AIP_CROUCHING, 	ACT_IDLE, 				NULL, 				ACT_COVER_LOW, 				},
		{	AIP_CROUCHING, 	ACT_IDLE_ANGRY,			NULL, 				ACT_COVER_LOW, 				},
		{	AIP_CROUCHING, 	ACT_WALK, 				NULL, 				ACT_WALK_CROUCH, 			},
		{	AIP_CROUCHING, 	ACT_RUN, 				NULL, 				ACT_RUN_CROUCH, 			},
		{	AIP_CROUCHING, 	ACT_WALK_AIM, 			NULL, 				ACT_WALK_CROUCH_AIM, 		},
		{	AIP_CROUCHING, 	ACT_RUN_AIM, 			NULL, 				ACT_RUN_CROUCH_AIM, 		},
		{	AIP_CROUCHING,	ACT_RELOAD,				NULL, 				ACT_RELOAD_LOW,				},
		{	AIP_CROUCHING,	ACT_RANGE_ATTACK_SMG1,	NULL,				ACT_RANGE_ATTACK_SMG1_LOW,	},
		{	AIP_CROUCHING,	ACT_RANGE_ATTACK_AR2,	NULL,				ACT_RANGE_ATTACK_AR2_LOW,	},
		
		//----
		{	AIP_PEEKING, 	ACT_IDLE,				NULL,				ACT_RANGE_AIM_LOW,			},
		{	AIP_PEEKING, 	ACT_IDLE_ANGRY,			NULL,				ACT_RANGE_AIM_LOW,			},
		{	AIP_PEEKING, 	ACT_COVER_LOW,			NULL,				ACT_RANGE_AIM_LOW,			},
		{	AIP_PEEKING, 	ACT_RANGE_ATTACK1,		NULL, 				ACT_RANGE_ATTACK1_LOW,		},
		{	AIP_PEEKING,	ACT_RELOAD, 			NULL, 				ACT_RELOAD_LOW,				},
	};

	m_ActivityMap.RemoveAll();
	
	CBaseCombatWeapon *pWeapon = GetOuter()->GetActiveWeapon();
	const char *pszWeaponClass = ( pWeapon ) ? pWeapon->GetClassname() : "";
	for ( int i = 0; i < ARRAYSIZE(mappings); i++ )
	{
		if ( !mappings[i].pszWeapon || stricmp( mappings[i].pszWeapon, pszWeaponClass ) == 0 )
		{
			if ( HaveSequenceForActivity( mappings[i].translation ) || HaveSequenceForActivity( GetOuter()->Weapon_TranslateActivity( mappings[i].translation ) ) )
			{
				Assert( m_ActivityMap.Find( MAKE_ACTMAP_KEY( mappings[i].posture, mappings[i].activity ) ) == m_ActivityMap.InvalidIndex() );
				m_ActivityMap.Insert( MAKE_ACTMAP_KEY( mappings[i].posture, mappings[i].activity ), mappings[i].translation );
			}
		}
	}
}

void CAI_StandoffBehavior::UpdateTranslateActivityMap()
{
	BaseClass::UpdateTranslateActivityMap();
	
	Activity lowCoverActivity = GetMappedActivity( AIP_CROUCHING, ACT_COVER_LOW );
	if ( lowCoverActivity == ACT_INVALID )
		lowCoverActivity = ACT_COVER_LOW;
		
	m_bHasLowCoverActivity = ( ( CapabilitiesGet() & bits_CAP_DUCK ) && (GetOuter()->TranslateActivity( lowCoverActivity ) != ACT_INVALID));

	CBaseCombatWeapon *pWeapon = GetOuter()->GetActiveWeapon();
	if ( pWeapon && (GetOuter()->TranslateActivity( lowCoverActivity ) == ACT_INVALID ))
		DevMsg( "Note: NPC class %s lacks ACT_COVER_LOW, therefore cannot participate in standoff\n", GetOuter()->GetClassname() );
}

//-------------------------------------

void CAI_MappedActivityBehavior_Temporary::OnChangeActiveWeapon( CBaseCombatWeapon *pOldWeapon, CBaseCombatWeapon *pNewWeapon )
{
	UpdateTranslateActivityMap();
}

//-------------------------------------

void CAI_StandoffBehavior::OnRestore()
{
	UpdateTranslateActivityMap();
}

//-------------------------------------

AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER(CAI_StandoffBehavior)

	DECLARE_CONDITION( COND_ABANDON_TIME_EXPIRED )

AI_END_CUSTOM_SCHEDULE_PROVIDER()

//-----------------------------------------------------------------------------
//
// CAI_StandoffGoal
//
// Purpose: A level tool to control the standoff behavior. Use is not required
//			in order to use behavior.
//
//-----------------------------------------------------------------------------

AI_StandoffParams_t g_StandoffParamsByAgression[] =
{
	//	hintChangeReaction,		fCoverOnReload, PlayerBtlLn,	minTimeShots,	maxTimeShots, 	minShots, 	maxShots, 	oddsCover			flAbandonTimeLimit
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			4.0, 			8.0, 			2, 			4, 			50,			false,	30 		},	// AGGR_VERY_LOW
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			2.0, 			5.0, 			3, 			5, 			25,			false, 	20		},	// AGGR_LOW
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			0.6, 			2.5, 			3, 			6, 			25,			false, 	10		},	// AGGR_MEDIUM
	{ 	AIHCR_MOVE_ON_COVER,	true,			true, 			0.2, 			1.5, 			5, 			8, 			10,			false, 	10		},	// AGGR_HIGH
	{ 	AIHCR_MOVE_ON_COVER,	false,			true, 			0, 				0, 				100,		100, 		0,			false, 	5		},	// AGGR_VERY_HIGH
};

//-------------------------------------

class CAI_StandoffGoal : public CAI_GoalEntity
{
	DECLARE_CLASS( CAI_StandoffGoal, CAI_GoalEntity );

public:
	CAI_StandoffGoal()
	{
		m_aggressiveness = AGGR_MEDIUM;	
		m_fPlayerIsBattleline = true;
		m_HintChangeReaction = AIHCR_DEFAULT_AI;
		m_fStayAtCover = false;
		m_bAbandonIfEnemyHides = false;
		m_customParams = AI_DEFAULT_STANDOFF_PARAMS;
	}

	//---------------------------------

	void EnableGoal( CAI_BaseNPC *pAI )	
	{
		CAI_StandoffBehavior *pBehavior;
		if ( !pAI->GetBehavior( &pBehavior ) )
			return;
		
		pBehavior->SetActive( true );
		SetBehaviorParams( pBehavior);
	}

	void DisableGoal( CAI_BaseNPC *pAI  ) 
	{
		// @TODO (toml 04-07-03): remove the no damage spawn flag once stable. The implementation isn't very good.
		CAI_StandoffBehavior *pBehavior;
		if ( !pAI->GetBehavior( &pBehavior ) )
			return;
		pBehavior->SetActive( false );
		SetBehaviorParams( pBehavior);
	}

	void InputActivate( inputdata_t &inputdata )
	{
		ValidateAggression();
		BaseClass::InputActivate( inputdata );
	}
	
	void InputDeactivate( inputdata_t &inputdata ) 	
	{
		ValidateAggression();
		BaseClass::InputDeactivate( inputdata );
	}
	
	void InputSetAggressiveness( inputdata_t &inputdata )
	{
		int newVal = inputdata.value.Int();
		
		m_aggressiveness = (Aggressiveness_t)newVal;
		ValidateAggression();
		
		UpdateActors();

		const CUtlVector<AIHANDLE> &actors = AccessActors();
		for ( int i = 0; i < actors.Count(); i++ )
		{
			CAI_BaseNPC *pAI = actors[i];
			CAI_StandoffBehavior *pBehavior;
			if ( !pAI->GetBehavior( &pBehavior ) )
				continue;
			SetBehaviorParams( pBehavior);
		}
	}

	void SetBehaviorParams( CAI_StandoffBehavior *pBehavior )
	{
		AI_StandoffParams_t params;

		if ( m_aggressiveness != AGGR_CUSTOM )
			params = g_StandoffParamsByAgression[m_aggressiveness];
		else
			params = m_customParams;

		params.hintChangeReaction = m_HintChangeReaction;
		params.fPlayerIsBattleline = m_fPlayerIsBattleline;
		params.fStayAtCover = m_fStayAtCover;
		if ( !m_bAbandonIfEnemyHides )
			params.flAbandonTimeLimit = 0;

		pBehavior->SetParameters( params, this );
		pBehavior->OnChangeTacticalConstraints();
		if ( pBehavior->IsRunning() )
			pBehavior->GetOuter()->ClearSchedule( "Standoff behavior parms changed" );
	}
	
	void ValidateAggression()
	{
		if ( m_aggressiveness < AGGR_VERY_LOW || m_aggressiveness > AGGR_VERY_HIGH )
		{
			if ( m_aggressiveness != AGGR_CUSTOM )
			{
				DevMsg( "Invalid aggressiveness value %d\n", m_aggressiveness );
				
				if ( m_aggressiveness < AGGR_VERY_LOW )
					m_aggressiveness = AGGR_VERY_LOW;
				else if ( m_aggressiveness > AGGR_VERY_HIGH )
					m_aggressiveness = AGGR_VERY_HIGH;
			}
		}
	}

private:
	//---------------------------------

	DECLARE_DATADESC();

	enum Aggressiveness_t
	{
		AGGR_VERY_LOW,
		AGGR_LOW,
		AGGR_MEDIUM,
		AGGR_HIGH,
		AGGR_VERY_HIGH,
		
		AGGR_CUSTOM,
	};

	Aggressiveness_t 		m_aggressiveness;	
	AI_HintChangeReaction_t m_HintChangeReaction;
	bool					m_fPlayerIsBattleline;
	bool					m_fStayAtCover;
	bool					m_bAbandonIfEnemyHides;
	AI_StandoffParams_t		m_customParams;
};

//-------------------------------------

LINK_ENTITY_TO_CLASS( ai_goal_standoff, CAI_StandoffGoal );

BEGIN_DATADESC( CAI_StandoffGoal )
	DEFINE_KEYFIELD( m_aggressiveness,				FIELD_INTEGER, 	"Aggressiveness" ),
	//								   m_customParams  (individually)
	DEFINE_KEYFIELD( m_HintChangeReaction,			FIELD_INTEGER, 	"HintGroupChangeReaction" ),
	DEFINE_KEYFIELD( m_fPlayerIsBattleline,			FIELD_BOOLEAN,	"PlayerBattleline" ),
	DEFINE_KEYFIELD( m_fStayAtCover,				FIELD_BOOLEAN,	"StayAtCover" ),
	DEFINE_KEYFIELD( m_bAbandonIfEnemyHides,		FIELD_BOOLEAN, 	"AbandonIfEnemyHides" ),
	DEFINE_KEYFIELD( m_customParams.fCoverOnReload,	FIELD_BOOLEAN, 	"CustomCoverOnReload" ),
	DEFINE_KEYFIELD( m_customParams.minTimeShots,	FIELD_FLOAT, 	"CustomMinTimeShots" ),
	DEFINE_KEYFIELD( m_customParams.maxTimeShots,	FIELD_FLOAT, 	"CustomMaxTimeShots" ),
	DEFINE_KEYFIELD( m_customParams.minShots,		FIELD_INTEGER, 	"CustomMinShots" ),
	DEFINE_KEYFIELD( m_customParams.maxShots,		FIELD_INTEGER, 	"CustomMaxShots" ),
	DEFINE_KEYFIELD( m_customParams.oddsCover,		FIELD_INTEGER, 	"CustomOddsCover" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetAggressiveness", InputSetAggressiveness ),
END_DATADESC()

///-----------------------------------------------------------------------------

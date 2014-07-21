//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "isaverestore.h"
#include "ai_behavior.h"
#include "scripted.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool g_bBehaviorHost_PreventBaseClassGatherConditions;

//-----------------------------------------------------------------------------
// CAI_BehaviorBase
//-----------------------------------------------------------------------------

BEGIN_DATADESC_NO_BASE( CAI_BehaviorBase )

END_DATADESC()

//-------------------------------------

CAI_ClassScheduleIdSpace *CAI_BehaviorBase::GetClassScheduleIdSpace()
{
	return GetOuter()->GetClassScheduleIdSpace();
}

//-----------------------------------------------------------------------------
// Purpose: Draw any text overlays (override in subclass to add additional text)
// Input  : Previous text offset from the top
// Output : Current text offset from the top
//-----------------------------------------------------------------------------
int CAI_BehaviorBase::DrawDebugTextOverlays( int text_offset )
{
	char	tempstr[ 512 ];
	int		offset = text_offset;

	if ( GetOuter()->m_debugOverlays & OVERLAY_TEXT_BIT )
	{
		Q_snprintf( tempstr, sizeof( tempstr ), "Behv: %s, ", GetName() );
		GetOuter()->EntityText( offset, tempstr, 0 );
		offset++;
	}

	return offset;
}

//-------------------------------------

void CAI_BehaviorBase::GatherConditions()
{
	Assert( m_pBackBridge != NULL );
	
	m_pBackBridge->BackBridge_GatherConditions();
}

//-------------------------------------

void CAI_BehaviorBase::PrescheduleThink()
{
}

//-------------------------------------

void CAI_BehaviorBase::OnScheduleChange()
{
}

//-------------------------------------

void CAI_BehaviorBase::OnStartSchedule( int scheduleType )
{
}

//-------------------------------------

int CAI_BehaviorBase::SelectSchedule()
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_SelectSchedule();
}

//-------------------------------------

int CAI_BehaviorBase::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	m_fOverrode = false; 
	return SCHED_NONE;
}

//-------------------------------------

void CAI_BehaviorBase::StartTask( const Task_t *pTask )
{
	m_fOverrode = false;
}

//-------------------------------------

void CAI_BehaviorBase::RunTask( const Task_t *pTask )
{
	m_fOverrode = false;
}

//-------------------------------------

void CAI_BehaviorBase::AimGun( void )
{
	m_fOverrode = false;
}

//-------------------------------------

int CAI_BehaviorBase::TranslateSchedule( int scheduleType )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_TranslateSchedule( scheduleType );
}

//-------------------------------------

CAI_Schedule *CAI_BehaviorBase::GetSchedule(int schedule)
{
	if (!GetClassScheduleIdSpace()->IsGlobalBaseSet())
	{
		Warning("ERROR: %s missing schedule!\n", GetSchedulingErrorName());
		return g_AI_SchedulesManager.GetScheduleFromID(SCHED_IDLE_STAND);
	}
	if ( AI_IdIsLocal( schedule ) )
	{
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);
	}

	if ( schedule == -1 )
		return NULL;

	return g_AI_SchedulesManager.GetScheduleFromID( schedule );
}

//-------------------------------------

bool CAI_BehaviorBase::IsCurSchedule( int schedule, bool fIdeal )
{
	if ( AI_IdIsLocal( schedule ) )
		schedule = GetClassScheduleIdSpace()->ScheduleLocalToGlobal(schedule);

	return GetOuter()->IsCurSchedule( schedule, fIdeal );
}

//-------------------------------------

const char *CAI_BehaviorBase::GetSchedulingErrorName()
{
	return "CAI_Behavior";
}

//-------------------------------------

Activity CAI_BehaviorBase::NPC_TranslateActivity( Activity activity )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_NPC_TranslateActivity( activity );
}

//-------------------------------------

bool CAI_BehaviorBase::IsCurTaskContinuousMove()
{
	m_fOverrode = false;
	return false;
}

//-------------------------------------

float CAI_BehaviorBase::GetDefaultNavGoalTolerance()
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_GetDefaultNavGoalTolerance();
}

//-------------------------------------

bool CAI_BehaviorBase::FValidateHintType( CAI_Hint *pHint )
{
	m_fOverrode = false;
	return false;
}

//-------------------------------------

bool CAI_BehaviorBase::IsValidEnemy( CBaseEntity *pEnemy )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_IsValidEnemy( pEnemy );
}

//-------------------------------------

CBaseEntity *CAI_BehaviorBase::BestEnemy( void )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_BestEnemy();
}

//-------------------------------------

bool CAI_BehaviorBase::IsValidCover( const Vector &vLocation, CAI_Hint const *pHint )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_IsValidCover( vLocation, pHint );
}

//-------------------------------------

bool CAI_BehaviorBase::IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CAI_Hint const *pHint )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_IsValidShootPosition( vLocation, pNode, pHint );
}

//-------------------------------------

float CAI_BehaviorBase::GetMaxTacticalLateralMovement( void )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_GetMaxTacticalLateralMovement();
}

//-------------------------------------

bool CAI_BehaviorBase::ShouldIgnoreSound( CSound *pSound )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_ShouldIgnoreSound( pSound );
}

//-------------------------------------

void CAI_BehaviorBase::OnSeeEntity( CBaseEntity *pEntity )
{
	Assert( m_pBackBridge != NULL );

	m_pBackBridge->BackBridge_OnSeeEntity( pEntity );
}

//-------------------------------------

void CAI_BehaviorBase::OnFriendDamaged( CBaseCombatCharacter *pSquadmate, CBaseEntity *pAttacker )
{
	Assert( m_pBackBridge != NULL );

	m_pBackBridge->BackBridge_OnFriendDamaged( pSquadmate, pAttacker );
}

//-------------------------------------

bool CAI_BehaviorBase::IsInterruptable( void )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_IsInterruptable();
}

//-------------------------------------

bool CAI_BehaviorBase::IsNavigationUrgent( void )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_IsNavigationUrgent();
}

//-------------------------------------

bool CAI_BehaviorBase::CanFlinch( void )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_CanFlinch();
}

//-------------------------------------

bool CAI_BehaviorBase::IsCrouching( void )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_IsCrouching();
}

//-------------------------------------

bool CAI_BehaviorBase::IsCrouchedActivity( Activity activity )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_IsCrouchedActivity( activity );
}

//-------------------------------------

bool CAI_BehaviorBase::QueryHearSound( CSound *pSound )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_QueryHearSound( pSound );
}

//-------------------------------------

bool CAI_BehaviorBase::CanRunAScriptedNPCInteraction( bool bForced )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_CanRunAScriptedNPCInteraction( bForced );
}

//-------------------------------------

bool CAI_BehaviorBase::ShouldPlayerAvoid( void )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_ShouldPlayerAvoid();
}

//-------------------------------------

int CAI_BehaviorBase::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_OnTakeDamage_Alive( info );
}

//-------------------------------------

float CAI_BehaviorBase::GetReasonableFacingDist( void )
{
	Assert( m_pBackBridge != NULL );
	
	return m_pBackBridge->BackBridge_GetReasonableFacingDist();
}

//-------------------------------------

bool CAI_BehaviorBase::ShouldAlwaysThink()
{
	m_fOverrode = false;
	return false;
}

//-------------------------------------

Activity CAI_BehaviorBase::GetFlinchActivity( bool bHeavyDamage, bool bGesture )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_GetFlinchActivity( bHeavyDamage, bGesture );
}

//-------------------------------------

bool CAI_BehaviorBase::OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_OnCalcBaseMove( pMoveGoal, distClear, pResult );
}

//-------------------------------------

void CAI_BehaviorBase::ModifyOrAppendCriteria( AI_CriteriaSet& criteriaSet )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_ModifyOrAppendCriteria( criteriaSet );
}

//-------------------------------------

void CAI_BehaviorBase::Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity )
{
	Assert( m_pBackBridge != NULL );

	return m_pBackBridge->BackBridge_Teleport( newPosition, newAngles, newVelocity );
}

//-------------------------------------

void CAI_BehaviorBase::HandleAnimEvent( animevent_t *pEvent )
{
	Assert( m_pBackBridge != NULL );

	m_pBackBridge->BackBridge_HandleAnimEvent( pEvent );
}

//-------------------------------------

bool CAI_BehaviorBase::NotifyChangeBehaviorStatus( bool fCanFinishSchedule )
{
	bool fInterrupt = GetOuter()->OnBehaviorChangeStatus( this, fCanFinishSchedule );
	
	if ( !GetOuter()->IsInterruptable())
		return false;
		
	if ( fInterrupt )
	{
		if ( GetOuter()->m_hCine )
		{
			if( GetOuter()->m_hCine->PlayedSequence() )
			{
				DevWarning( "NPC: %s canceled running script %s due to behavior change\n", GetOuter()->GetDebugName(), GetOuter()->m_hCine->GetDebugName() );
			}
			else
			{
				DevWarning( "NPC: %s canceled script %s without playing, due to behavior change\n", GetOuter()->GetDebugName(), GetOuter()->m_hCine->GetDebugName() );
			}

			GetOuter()->m_hCine->CancelScript();
		}

		//!!!HACKHACK
		// this is dirty, but it forces NPC to pick a new schedule next time through.
		GetOuter()->ClearSchedule( "Changed behavior status" );
	}

	return fInterrupt;
}

//-------------------------------------

int	CAI_BehaviorBase::Save( ISave &save )				
{ 
	return save.WriteAll( this, GetDataDescMap() );	
}

//-------------------------------------

int	CAI_BehaviorBase::Restore( IRestore &restore )
{ 
	return restore.ReadAll( this, GetDataDescMap() );	
}

//-------------------------------------

#define BEHAVIOR_SAVE_BLOCKNAME "AI_Behaviors"
#define BEHAVIOR_SAVE_VERSION	2

void CAI_BehaviorBase::SaveBehaviors(ISave &save, CAI_BehaviorBase *pCurrentBehavior, CAI_BehaviorBase **ppBehavior, int nBehaviors )		
{ 
	save.StartBlock( BEHAVIOR_SAVE_BLOCKNAME );
	short temp = BEHAVIOR_SAVE_VERSION;
	save.WriteShort( &temp );
	temp = (short)nBehaviors;
	save.WriteShort( &temp );

	for ( int i = 0; i < nBehaviors; i++ )
	{
		if ( strcmp( ppBehavior[i]->GetDataDescMap()->dataClassName, CAI_BehaviorBase::m_DataMap.dataClassName ) != 0 )
		{
			save.StartBlock();
			save.WriteString( ppBehavior[i]->GetDataDescMap()->dataClassName );
			bool bIsCurrent = ( pCurrentBehavior == ppBehavior[i] );
			save.WriteBool( &bIsCurrent );
			ppBehavior[i]->Save( save );
			save.EndBlock();
		}
	}

	save.EndBlock();
}

//-------------------------------------

int CAI_BehaviorBase::RestoreBehaviors(IRestore &restore, CAI_BehaviorBase **ppBehavior, int nBehaviors )	
{ 
	int iCurrent = -1;
	char szBlockName[SIZE_BLOCK_NAME_BUF];
	restore.StartBlock( szBlockName );
	if ( strcmp( szBlockName, BEHAVIOR_SAVE_BLOCKNAME ) == 0 )
	{
		short version;
		restore.ReadShort( &version );
		if ( version == BEHAVIOR_SAVE_VERSION )
		{
			short nToRestore;
			char szClassNameCurrent[256];
			restore.ReadShort( &nToRestore );
			for ( int i = 0; i < nToRestore; i++ )
			{
				restore.StartBlock();
				restore.ReadString( szClassNameCurrent, sizeof( szClassNameCurrent ), 0 );
				bool bIsCurrent;
				restore.ReadBool( &bIsCurrent );

				for ( int j = 0; j < nBehaviors; j++ )
				{
					if ( strcmp( ppBehavior[j]->GetDataDescMap()->dataClassName, szClassNameCurrent ) == 0 )
					{
						if ( bIsCurrent )
							iCurrent = j;
						ppBehavior[j]->Restore( restore );
					}
				}

				restore.EndBlock();

			}
		}
	}
	restore.EndBlock();
	return iCurrent; 
}


//-----------------------------------------------------------------------------

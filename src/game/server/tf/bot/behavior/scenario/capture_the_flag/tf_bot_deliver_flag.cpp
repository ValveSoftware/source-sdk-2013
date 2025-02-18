//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_deliver_flag.cpp
// Take the flag we are holding to its destination
// Michael Booth, May 2011

#include "cbase.h"

#include "tf_player_shared.h"

#include "bot/tf_bot.h"
#include "bot/behavior/scenario/capture_the_flag/tf_bot_deliver_flag.h"
#include "bot/behavior/tf_bot_taunt.h"
#include "bot/behavior/tf_bot_mvm_deploy_bomb.h"

#include "tf_objective_resource.h"
#include "player_vs_environment/tf_population_manager.h"
#include "econ_item_system.h"
#include "tf_gamestats.h"

#include "bot/behavior/nav_entities/tf_bot_nav_ent_move_to.h"
#include "bot/behavior/nav_entities/tf_bot_nav_ent_wait.h"

#include "particle_parse.h"

ConVar tf_mvm_bot_allow_flag_carrier_to_fight( "tf_mvm_bot_allow_flag_carrier_to_fight", "1", FCVAR_CHEAT );

ConVar tf_mvm_bot_flag_carrier_interval_to_1st_upgrade( "tf_mvm_bot_flag_carrier_interval_to_1st_upgrade", "5", FCVAR_CHEAT );
ConVar tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade( "tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade", "15", FCVAR_CHEAT );
ConVar tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade( "tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade", "15", FCVAR_CHEAT );

ConVar tf_mvm_bot_flag_carrier_health_regen( "tf_mvm_bot_flag_carrier_health_regen", "45.0f", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotDeliverFlag::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_flTotalTravelDistance = -1.0f;

	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	if ( tf_mvm_bot_allow_flag_carrier_to_fight.GetBool() == false )
	{
		me->SetAttribute( CTFBot::SUPPRESS_FIRE );
	}

	// mini-bosses don't upgrade - they are already tough
	if ( me->IsMiniBoss() )
	{
		m_upgradeLevel = DONT_UPGRADE;
		if ( TFObjectiveResource() )
		{
			// Set threat level to max
			TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 4 );
			TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
			TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
		}
	}
	else
	{
		m_upgradeLevel = 0;
		m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_1st_upgrade.GetFloat() );
		if ( TFObjectiveResource() )
		{
			TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
			TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
		}
		
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
// In Mann Vs Machine, the flag carrier gets stronger the longer he carries the flag
bool CTFBotDeliverFlag::UpgradeOverTime( CTFBot *me )
{
	if ( TFGameRules()->IsMannVsMachineMode() && m_upgradeLevel != DONT_UPGRADE )
	{
		CTFNavArea *myArea = me->GetLastKnownArea();
		int spawnRoomFlag = me->GetTeamNumber() == TF_TEAM_RED ? TF_NAV_SPAWN_ROOM_RED : TF_NAV_SPAWN_ROOM_BLUE;

		if ( myArea && myArea->HasAttributeTF( spawnRoomFlag ) )
		{
			// don't start counting down until we leave the spawn
			m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_1st_upgrade.GetFloat() );
			TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
			TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
		}

		// do defensive buff effect ourselves (since we're not a soldier)
		if ( m_upgradeLevel > 0 && m_buffPulseTimer.IsElapsed() )
		{
			m_buffPulseTimer.Start( 1.0f );

			CUtlVector< CTFPlayer * > playerVector;
			CollectPlayers( &playerVector, me->GetTeamNumber(), COLLECT_ONLY_LIVING_PLAYERS );

			const float buffRadius = 450.0f;

			for( int i=0; i<playerVector.Count(); ++i )
			{
				if ( me->IsRangeLessThan( playerVector[i], buffRadius ) )
				{
					playerVector[i]->m_Shared.AddCond( TF_COND_DEFENSEBUFF_NO_CRIT_BLOCK, 1.2f );
				}
			}
		}

		// the flag carrier gets stronger the longer he holds the flag
		if ( m_upgradeTimer.IsElapsed() )
		{
			const int maxLevel = 3;

			if ( m_upgradeLevel < maxLevel )
			{
				++m_upgradeLevel;
				
				TFGameRules()->BroadcastSound( 255, "MVM.Warning" );

				switch( m_upgradeLevel )
				{
				//---------------------------------------
				case 1:
					m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_2nd_upgrade.GetFloat() );

					// permanent buff banner effect (handled above)

					// update the objective resource so clients have the information
					if ( TFObjectiveResource() )
					{
						TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 1 );
						TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
						TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
						TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE1, TF_TEAM_PVE_DEFENDERS );
						DispatchParticleEffect( "mvm_levelup1", PATTACH_POINT_FOLLOW, me, "head" );
					}
					return true;

				//---------------------------------------
				case 2:
				{
					static CSchemaAttributeDefHandle pAttrDef_HealthRegen( "health regen" );

					m_upgradeTimer.Start( tf_mvm_bot_flag_carrier_interval_to_3rd_upgrade.GetFloat() );
					
					if ( !pAttrDef_HealthRegen )
					{
						Warning( "TFBotSpawner: Invalid attribute 'health regen'\n" );
					}
					else
					{
						CAttributeList *pAttrList = me->GetAttributeList();
						if ( pAttrList )
						{
							pAttrList->SetRuntimeAttributeValue( pAttrDef_HealthRegen, tf_mvm_bot_flag_carrier_health_regen.GetFloat() );
						}
					}

					// update the objective resource so clients have the information
					if ( TFObjectiveResource() )
					{
						TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 2 );
						TFObjectiveResource()->SetBaseMvMBombUpgradeTime( gpGlobals->curtime );
						TFObjectiveResource()->SetNextMvMBombUpgradeTime( gpGlobals->curtime + m_upgradeTimer.GetRemainingTime() );
						TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE2, TF_TEAM_PVE_DEFENDERS );
						DispatchParticleEffect( "mvm_levelup2", PATTACH_POINT_FOLLOW, me, "head" );
					}
					return true;
				}

				//---------------------------------------
				case 3:
					// add critz
					me->m_Shared.AddCond( TF_COND_CRITBOOSTED );

					// update the objective resource so clients have the information
					if ( TFObjectiveResource() )
					{
						TFObjectiveResource()->SetFlagCarrierUpgradeLevel( 3 );
						TFObjectiveResource()->SetBaseMvMBombUpgradeTime( -1 );
						TFObjectiveResource()->SetNextMvMBombUpgradeTime( -1 );
						TFGameRules()->HaveAllPlayersSpeakConceptIfAllowed( MP_CONCEPT_MVM_BOMB_CARRIER_UPGRADE3, TF_TEAM_PVE_DEFENDERS );
						DispatchParticleEffect( "mvm_levelup3", PATTACH_POINT_FOLLOW, me, "head" );
					}
					return true;
				}
			}
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotDeliverFlag::Update( CTFBot *me, float interval )
{
	CCaptureFlag *flag = me->GetFlagToFetch();

	if ( !flag )
	{
		return Done( "No flag" );
	}

	CTFPlayer *carrier = ToTFPlayer( flag->GetOwnerEntity() );
	if ( !carrier || !me->IsSelf( carrier ) )
	{
		return Done( "I'm no longer carrying the flag" );
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// let the bomb carrier use it's buff banners/etc
		Action< CTFBot > *result = me->OpportunisticallyUseWeaponAbilities();
		if ( result )
		{
			return SuspendFor( result, "Opportunistically using buff item" );
		}
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	// deliver the flag
	if ( m_repathTimer.IsElapsed() )
	{
		CCaptureZone *zone = me->GetFlagCaptureZone();

		if ( !zone )
		{
			return Done( "No flag capture zone exists!" );
		}

		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_path.Compute( me, zone->WorldSpaceCenter(), cost );

		float flOldTravelDistance = m_flTotalTravelDistance;

		m_flTotalTravelDistance = NavAreaTravelDistance( me->GetLastKnownArea(), TheNavMesh->GetNavArea( zone->WorldSpaceCenter() ), cost );

		if ( flOldTravelDistance != -1.0f && m_flTotalTravelDistance - flOldTravelDistance > 2000.0f )
		{
			TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Bomb_Reset" );

			// Look for players that helped with the reset and send an event
			CUtlVector<CTFPlayer *> playerVector;
			CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
			FOR_EACH_VEC( playerVector, i )
			{
				CTFPlayer *pPlayer = playerVector[i];
				if ( !pPlayer )
					continue;

				if ( me->m_AchievementData.IsPusherInHistory( pPlayer, 3.f ) )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "mvm_bomb_reset_by_player" );
					if ( event )
					{
						event->SetInt( "player", pPlayer->entindex() );
						gameeventmanager->FireEvent( event );
					}

					CTF_GameStats.Event_PlayerAwardBonusPoints( pPlayer, me, 100 );
				}
			}
		}

		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	m_path.Update( me );

	if ( UpgradeOverTime( me ) )
	{
		return SuspendFor( new CTFBotTaunt, "Taunting for our new upgrade" );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotDeliverFlag::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	me->ClearAttribute( CTFBot::SUPPRESS_FIRE );

	if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
	{
		me->m_Shared.ResetRageBuffs();
	}
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotDeliverFlag::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	if ( tf_mvm_bot_allow_flag_carrier_to_fight.GetBool() )
	{
		return ANSWER_UNDEFINED;
	}

	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
// are we in a hurry?
QueryResultType CTFBotDeliverFlag::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
// is it time to retreat?
QueryResultType	CTFBotDeliverFlag::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotDeliverFlag::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	if ( TFGameRules()->IsMannVsMachineMode() && other && FClassnameIs( other, "func_capturezone" ) )
	{
		return TrySuspendFor( new CTFBotMvMDeployBomb, RESULT_CRITICAL, "Delivering the bomb!" );
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CTFBotPushToCapturePoint::CTFBotPushToCapturePoint( Action< CTFBot > *nextAction )
{
	m_nextAction = nextAction;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotPushToCapturePoint::Update( CTFBot *me, float interval )
{
	// flag collection and delivery is handled by our parent behavior, ScenarioMonitor

	CCaptureZone *zone = me->GetFlagCaptureZone();

	if ( !zone )
	{
		if ( m_nextAction )
		{
			return ChangeTo( m_nextAction, "No flag capture zone exists!" );
		}

		return Done( "No flag capture zone exists!" );
	}

	Vector toZone = zone->WorldSpaceCenter() - me->GetAbsOrigin();
	if ( toZone.AsVector2D().IsLengthLessThan( 50.0f ) )
	{
		if ( m_nextAction )
		{
			return ChangeTo( m_nextAction, "At destination" );
		}

		return Done( "At destination" );
	}

	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	if ( threat && threat->IsVisibleRecently() )
	{
		// prepare to fight
		me->EquipBestWeaponForThreat( threat );
	}

	if ( m_repathTimer.IsElapsed() )
	{
		CTFBotPathCost cost( me, FASTEST_ROUTE );
		m_path.Compute( me, zone->WorldSpaceCenter(), cost );

		m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );
	}

	m_path.Update( me );

	return Continue();
}

//-----------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotPushToCapturePoint::OnNavAreaChanged( CTFBot *me, CNavArea *newArea, CNavArea *oldArea )
{
	// does the area we are entering have a prerequisite?
	if ( newArea && newArea->HasPrerequisite( me ) )
	{
		const CUtlVector< CHandle< CFuncNavPrerequisite > > &prereqVector = newArea->GetPrerequisiteVector();

		for( int i=0; i<prereqVector.Count(); ++i )
		{
			const CFuncNavPrerequisite *prereq = prereqVector[i];
			if ( prereq && prereq->IsEnabled() && const_cast< CFuncNavPrerequisite * >( prereq )->PassesTriggerFilters( me ) )
			{
				// this prerequisite applies to me
				if ( prereq->IsTask( CFuncNavPrerequisite::TASK_WAIT ) )
				{
					return TrySuspendFor( new CTFBotNavEntWait( prereq ), RESULT_IMPORTANT, "Prerequisite commands me to wait" );
				}
				else if ( prereq->IsTask( CFuncNavPrerequisite::TASK_MOVE_TO_ENTITY ) )
				{
					return TrySuspendFor( new CTFBotNavEntMoveTo( prereq ), RESULT_IMPORTANT, "Prerequisite commands me to move to an entity" );
				}
			}
		}
	}

	return TryContinue();
}

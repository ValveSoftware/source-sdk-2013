//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_vision.cpp
// Team Fortress NextBot vision interface
// Michael Booth, May 2009

#include "cbase.h"
#include "vprof.h"

#include "tf_bot.h"
#include "tf_bot_vision.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_obj_sentrygun.h"

ConVar tf_bot_choose_target_interval( "tf_bot_choose_target_interval", "0.3f", FCVAR_CHEAT, "How often, in seconds, a TFBot can reselect his target" );
ConVar tf_bot_sniper_choose_target_interval( "tf_bot_sniper_choose_target_interval", "3.0f", FCVAR_CHEAT, "How often, in seconds, a zoomed-in Sniper can reselect his target" );


//------------------------------------------------------------------------------------------
// Update internal state
void CTFBotVision::Update( void )
{
	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		// Throttle vision update rate of robots in MvM for perf at the expense of reaction times
		if ( !m_scanTimer.IsElapsed() )
		{
			return;
		}

		m_scanTimer.Start( RandomFloat( 0.9f, 1.1f ) );
	}

	IVision::Update();

	CTFBot *me = (CTFBot *)GetBot()->GetEntity();
	if ( !me )
		return;

	// forget spies we have lost sight of
	CUtlVector< CTFPlayer * > playerVector;
	CollectPlayers( &playerVector, GetEnemyTeam( me->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

	for( int i=0; i<playerVector.Count(); ++i )
	{
		if ( !playerVector[i]->IsPlayerClass( TF_CLASS_SPY ) )
			continue;

		const CKnownEntity *known = GetKnown( playerVector[i] );

		if ( !known || !known->IsVisibleRecently() )
		{
			// if a hidden spy changes disguises, we no longer recognize him
			if ( playerVector[i]->m_Shared.InCond( TF_COND_DISGUISING ) )
			{
				me->ForgetSpy( playerVector[i] );				
			}
		}
	}
}


//------------------------------------------------------------------------------------------
void CTFBotVision::CollectPotentiallyVisibleEntities( CUtlVector< CBaseEntity * > *potentiallyVisible )
{
	VPROF_BUDGET( "CTFBotVision::CollectPotentiallyVisibleEntities", "NextBot" );

	potentiallyVisible->RemoveAll();

	// include all players
	for( int i=1; i<=gpGlobals->maxClients; ++i )
	{
		CBasePlayer *player = UTIL_PlayerByIndex( i );

		if ( player == NULL )
			continue;

		if ( FNullEnt( player->edict() ) )
			continue;

		if ( !player->IsPlayer() )
			continue;

		if ( !player->IsConnected() )
			continue;

		if ( !player->IsAlive() )
			continue;

		potentiallyVisible->AddToTail( player );
	}

	// include sentry guns
	UpdatePotentiallyVisibleNPCVector();

	FOR_EACH_VEC( m_potentiallyVisibleNPCVector, it )
	{
		potentiallyVisible->AddToTail( m_potentiallyVisibleNPCVector[ it ] );
	}
}


//------------------------------------------------------------------------------------------
void CTFBotVision::UpdatePotentiallyVisibleNPCVector( void )
{
	if ( m_potentiallyVisibleUpdateTimer.IsElapsed() )
	{
		m_potentiallyVisibleUpdateTimer.Start( RandomFloat( 3.0f, 4.0f ) );

		// collect list of active buildings
		m_potentiallyVisibleNPCVector.RemoveAll();

		bool bShouldSeeTeleporter = !TFGameRules()->IsMannVsMachineMode() || GetBot()->GetEntity()->GetTeamNumber() != TF_TEAM_PVE_INVADERS;
		for ( int i=0; i<IBaseObjectAutoList::AutoList().Count(); ++i )
		{
			CBaseObject* pObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[i] );
			if ( pObj->ObjectType() == OBJ_SENTRYGUN )
			{
				m_potentiallyVisibleNPCVector.AddToTail( pObj );
			}
			else if ( pObj->ObjectType() == OBJ_DISPENSER && pObj->ClassMatches( "obj_dispenser" ) )
			{
				m_potentiallyVisibleNPCVector.AddToTail( pObj );
			}
			else if ( bShouldSeeTeleporter && pObj->ObjectType() == OBJ_TELEPORTER )
			{
				m_potentiallyVisibleNPCVector.AddToTail( pObj );
			}
		}

		CUtlVector< INextBot * > botVector;
		TheNextBots().CollectAllBots( &botVector );
		for( int i=0; i<botVector.Count(); ++i )
		{
			CBaseCombatCharacter *botEntity = botVector[i]->GetEntity();
			if ( botEntity && !botEntity->IsPlayer() )
			{
				// NPC
				m_potentiallyVisibleNPCVector.AddToTail( botEntity );
			}
		}
	}
}


//------------------------------------------------------------------------------------------
/**
 * Return true to completely ignore this entity.
 * This is mostly for enemy spies.  If we don't ignore them, we will look at them.
 */
bool CTFBotVision::IsIgnored( CBaseEntity *subject ) const
{
	CTFBot *me = (CTFBot *)GetBot()->GetEntity();

#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		if ( me->IsPlayerClass( TF_CLASS_SCOUT ) )
		{
			// Scouts are wandering defenders, and aggro purely on proximity or damage, not vision
			return true;
		}
	}
#endif // TF_RAID_MODE

	if ( me->IsAttentionFocused() )
	{
		// our attention is restricted to certain subjects
		if ( !me->IsAttentionFocusedOn( subject ) )
		{
			return false;
		}
	}

	if ( !me->IsEnemy( subject ) )
	{
		// don't ignore friends
		return false;
	}

	if ( subject->IsEffectActive( EF_NODRAW ) )
	{
		return true;
	}

	if ( subject->IsPlayer() )
	{
		CTFPlayer *enemy = static_cast< CTFPlayer * >( subject );

		// test for designer-defined ignorance
		switch( enemy->GetPlayerClass()->GetClassIndex() )
		{
		case TF_CLASS_MEDIC:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_MEDICS ) )
			{
				return true;
			}
			break;

		case TF_CLASS_ENGINEER:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_ENGINEERS ) )
			{
				return true;
			}
			break;

		case TF_CLASS_SNIPER:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_SNIPERS ) )
			{
				return true;
			}
			break;

		case TF_CLASS_SCOUT:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_SCOUTS ) )
			{
				return true;
			}
			break;

		case TF_CLASS_SPY:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_SPIES ) )
			{
				return true;
			}
			break;

		case TF_CLASS_DEMOMAN:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_DEMOMEN ) )
			{
				return true;
			}
			break;

		case TF_CLASS_SOLDIER:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_SOLDIERS ) )
			{
				return true;
			}
			break;

		case TF_CLASS_HEAVYWEAPONS:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_HEAVIES ) )
			{
				return true;
			}
			break;

		case TF_CLASS_PYRO:
			if ( me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_PYROS ) )
			{
				return true;
			}
			break;
		}


		if ( me->IsKnownSpy( enemy ) )
		{
			// don't ignore revealed spies
			return false;
		}

		if ( enemy->m_Shared.InCond( TF_COND_BURNING ) ||
			 enemy->m_Shared.InCond( TF_COND_URINE ) ||
			 enemy->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
			 enemy->m_Shared.InCond( TF_COND_BLEEDING ) )
		{
			// always notice players with these conditions
			return false;
		}

		// An upgrade in MvM grants AE stealth where the player can fire
		// while in stealth, and for a short period after it drops
		if ( enemy->m_Shared.InCond( TF_COND_STEALTHED_USER_BUFF_FADING ) )
		{
			return true;
		}

		if ( enemy->m_Shared.IsStealthed() )
		{
			if ( enemy->m_Shared.GetPercentInvisible() < 0.75f )
			{
				// spy is partially cloaked, and therefore attracts our attention
				return false;
			}

			// invisible!
			return true;
		}

		if ( enemy->IsPlacingSapper() )
		{
			return false;
		}

		if ( enemy->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			return false;
		}
		
		if ( enemy->m_Shared.InCond( TF_COND_DISGUISED ) && enemy->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() )
		{
			// spy is disguised as a member of my team
			return true;
		}
	}
	else if ( subject->IsBaseObject() ) // not a player
	{
		CBaseObject *object = assert_cast< CBaseObject * >( subject );
		if ( object )
		{
			// ignore sapped enemy objects
			if ( object->HasSapper() )
			{
				// unless we're in MvM where buildings can have really large health pools,
				// so an engineer can die and run back in time to repair their stuff
				if ( TFGameRules() && TFGameRules()->IsMannVsMachineMode() )
				{
					return false;
				}

				return true;
			}
			
			// ignore carried objects
			if ( object->IsPlacing() || object->IsCarried() )
			{
				return true;
			}
			
			if ( object->GetType() == OBJ_SENTRYGUN && me->IsBehaviorFlagSet( TFBOT_IGNORE_ENEMY_SENTRY_GUNS ) )
			{
				return true;
			}
		}
	}

	return false;
}


//------------------------------------------------------------------------------------------
// Return true if we 'notice' the subject, even though we have LOS to it
bool CTFBotVision::IsVisibleEntityNoticed( CBaseEntity *subject ) const
{
	CTFBot *me = (CTFBot *)GetBot()->GetEntity();

	if ( subject->IsPlayer() && me->IsEnemy( subject ) )
	{
		CTFPlayer *player = static_cast< CTFPlayer * >( subject );

		if ( player->m_Shared.InCond( TF_COND_BURNING ) ||
			 player->m_Shared.InCond( TF_COND_URINE ) ||
			 player->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
			 player->m_Shared.InCond( TF_COND_BLEEDING ) )
		{
			// always notice players with these conditions
			if ( player->m_Shared.InCond( TF_COND_STEALTHED ) )
			{
				me->RealizeSpy( player );
			}
			return true;
		}


		// An upgrade in MvM grants AE stealth where the player can fire
		// while in stealth, and for a short period after it drops
		if ( player->m_Shared.InCond( TF_COND_STEALTHED_USER_BUFF_FADING ) )
		{
			me->ForgetSpy( player );
			return false;
		}

		if ( player->m_Shared.IsStealthed() )
		{
			if ( player->m_Shared.GetPercentInvisible() < 0.75f )
			{
				// spy is partially cloaked, and therefore attracts our attention
				me->RealizeSpy( player );
				return true;
			}

			// invisible!
			me->ForgetSpy( player );
			return false;
		}

		if ( TFGameRules()->IsMannVsMachineMode() )	// in MvM mode, forget spies as soon as they are fully disguised
		{
			CTFBot::SuspectedSpyInfo_t* pSuspectInfo = me->IsSuspectedSpy( player );
			// But only if we aren't suspecting them currently.  This happens when we bump into them.
			if( !pSuspectInfo || !pSuspectInfo->IsCurrentlySuspected() )
			{
				if ( player->m_Shared.InCond( TF_COND_DISGUISED ) && player->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() )
				{
					me->ForgetSpy( player );
					return false;
				}
			}
		}

		if ( me->IsKnownSpy( player ) )
		{
			// always notice non-invisible revealed spies
			return true;
		}

		if ( !TFGameRules()->IsMannVsMachineMode() )	// ignore in MvM mode
		{
			if ( player->IsPlacingSapper() )
			{
				// spotted a spy!
				me->RealizeSpy( player );
				return true;
			}
		}

		if ( player->m_Shared.InCond( TF_COND_DISGUISING ) )
		{
			// spotted a spy!
			me->RealizeSpy( player );
			return true;
		}

		if ( player->m_Shared.InCond( TF_COND_DISGUISED ) && player->m_Shared.GetDisguiseTeam() == me->GetTeamNumber() )
		{
			// spy is disguised as a member of my team, don't notice him
			return false;
		}
	}

	return true;
}


//------------------------------------------------------------------------------------------
// Return VISUAL reaction time
float CTFBotVision::GetMinRecognizeTime( void ) const
{
	CTFBot *me = (CTFBot *)GetBot();

	switch ( me->GetDifficulty() )
	{
	case CTFBot::EASY:		return 1.0f;
	case CTFBot::NORMAL:	return 0.5f;
	case CTFBot::HARD:		return 0.3f;
	case CTFBot::EXPERT:	return 0.2f;
	}

	return 1.0f;
}



//------------------------------------------------------------------------------------------
float CTFBotVision::GetMaxVisionRange( void ) const
{
	CTFBot *me = (CTFBot *)GetBot();

	if ( me->GetMaxVisionRangeOverride() > 0.0f )
	{
		// designer specified vision range
		return me->GetMaxVisionRangeOverride();
	}

	// long range, particularly for snipers
	return 6000.0f;
}

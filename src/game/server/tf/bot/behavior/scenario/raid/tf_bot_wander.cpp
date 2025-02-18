//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_wander.cpp
// Wanderering/idle enemies for Squad Co-op mode
// Michael Booth, October 2009

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "team.h"
#include "raid/tf_raid_logic.h"
#include "bot/tf_bot.h"
#include "bot/behavior/scenario/raid/tf_bot_wander.h"
#include "bot/behavior/scenario/raid/tf_bot_mob_rush.h"


ConVar tf_raid_wanderer_aggro_range( "tf_raid_wanderer_aggro_range", "500", FCVAR_CHEAT, "If wanderers see a threat closer than this, they attack" );
ConVar tf_raid_wanderer_notice_friend_death_range( "tf_raid_wanderer_notice_friend_death_range", "1000", FCVAR_CHEAT, "If a friend dies within this radius of a wanderer, it wakes up and attacks the attacker" );
ConVar tf_raid_wanderer_reaction_factor( "tf_raid_wanderer_reaction_factor", "1", FCVAR_CHEAT );
ConVar tf_raid_wanderer_vocalize_min_interval( "tf_raid_wanderer_vocalize_min_interval", "20", FCVAR_CHEAT );
ConVar tf_raid_wanderer_vocalize_max_interval( "tf_raid_wanderer_vocalize_max_interval", "30", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
CTFBotWander::CTFBotWander( void )
{
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotWander::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_vocalizeTimer.Start( RandomFloat( tf_raid_wanderer_vocalize_min_interval.GetFloat(), tf_raid_wanderer_vocalize_max_interval.GetFloat() ) );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotWander::Update( CTFBot *me, float interval )
{
	// mobs use only their melee weapons
	CBaseCombatWeapon *meleeWeapon = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	if ( meleeWeapon )
	{
		me->Weapon_Switch( meleeWeapon );
	}


	CTeam *raidingTeam = GetGlobalTeam( TF_TEAM_BLUE );

	if ( me->HasAttribute( CTFBot::AGGRESSIVE ) )
	{
		// I'm a mob rusher - pick a random raider and attack them!
		CTFPlayer *victim = TFGameRules()->GetRaidLogic()->SelectRaiderToAttack();
		if ( victim )
		{
			return SuspendFor( new CTFBotMobRush( victim ), "Rushing a raider" );
		}
	}
	else if ( m_visionTimer.IsElapsed() )
	{
		// I'm a wanderer - look for very nearby threats
		m_visionTimer.Start( RandomFloat( 0.5f, 1.0f ) );

		// find closest visible raider within aggro range
		CTFPlayer *threat = NULL;
		float closeThreatRangeSq = tf_raid_wanderer_aggro_range.GetFloat() * tf_raid_wanderer_aggro_range.GetFloat();

		for( int i=0; i<raidingTeam->GetNumPlayers(); ++i )
		{
			CTFPlayer *player = (CTFPlayer *)raidingTeam->GetPlayer(i);

			if ( !player->IsAlive() )
				continue;

			float rangeSq = me->GetRangeSquaredTo( player );
			if ( rangeSq < closeThreatRangeSq )
			{
				if ( me->GetVisionInterface()->IsLineOfSightClearToEntity( player ) )
				{
					threat = player;
					closeThreatRangeSq = rangeSq;
				}
			}
		}

		if ( threat )
		{
			return SuspendFor( new CTFBotMobRush( threat ), "Attacking threat!" );
		}
	}

	if ( m_vocalizeTimer.IsElapsed() )
	{
		m_vocalizeTimer.Start( RandomFloat( tf_raid_wanderer_vocalize_min_interval.GetFloat(), tf_raid_wanderer_vocalize_max_interval.GetFloat() ) );

		// mouth off
		if ( me->IsPlayerClass( TF_CLASS_SCOUT ) )
			me->EmitSound( "Scout.WanderJabber" );
		else
			me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_JEERS );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotWander::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result  )
{
	if ( other && other->IsPlayer() && me->IsEnemy( other ) )
	{
		return TrySuspendFor( new CTFBotMobRush( (CTFPlayer *)other ), RESULT_IMPORTANT, "Attacking threat who touched me!" );
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotWander::OnInjured( CTFBot *me, const CTakeDamageInfo &info )
{
	if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && me->IsEnemy( info.GetAttacker() ) )
	{
		return TrySuspendFor( new CTFBotMobRush( (CTFPlayer *)info.GetAttacker() ), RESULT_IMPORTANT, "Attacking threat who attacked me!" );
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotWander::OnOtherKilled( CTFBot *me, CBaseCombatCharacter *victim, const CTakeDamageInfo &info )
{
	if ( victim && me->IsFriend( victim ) )
	{
		if ( info.GetAttacker() && info.GetAttacker()->IsPlayer() && me->IsEnemy( info.GetAttacker() ) )
		{
			if ( me->IsRangeLessThan( victim, tf_raid_wanderer_notice_friend_death_range.GetFloat() ) )
			{
				if ( me->GetVisionInterface()->IsAbleToSee( victim, IVision::DISREGARD_FOV ) && 
					 me->GetVisionInterface()->IsAbleToSee( info.GetAttacker(), IVision::DISREGARD_FOV ) )
				{
					float rangeToAttacker = me->GetRangeTo( info.GetAttacker() );
					float reactionTime;

					if ( rangeToAttacker < tf_raid_wanderer_aggro_range.GetFloat() )
					{
						reactionTime = 0.0f;
					}
					else
					{
						reactionTime = tf_raid_wanderer_reaction_factor.GetFloat() * ( rangeToAttacker - tf_raid_wanderer_aggro_range.GetFloat() ) / tf_raid_wanderer_aggro_range.GetFloat();
					}

					return TrySuspendFor( new CTFBotMobRush( (CTFPlayer *)info.GetAttacker(), reactionTime ), RESULT_IMPORTANT, "Attacking my friend's attacker!" );
				}
			}
		}
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotWander::OnCommandAttack( CTFBot *me, CBaseEntity *victim )
{
	return TryContinue();
}


#endif // TF_RAID_MODE

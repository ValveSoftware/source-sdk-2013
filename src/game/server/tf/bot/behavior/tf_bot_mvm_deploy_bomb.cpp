//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_mvm_deploy_bomb.cpp
// Set us up the bomb!

#include "cbase.h"
#include "team.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_mvm_deploy_bomb.h"
#include "econ_item_system.h"


extern ConVar tf_deploying_bomb_delay_time;
extern ConVar tf_deploying_bomb_time;



//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMvMDeployBomb::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	me->SetDeployingBombState( TF_BOMB_DEPLOYING_DELAY );
	m_timer.Start( tf_deploying_bomb_delay_time.GetFloat() );

	// remember where we start deploying
	m_anchorPos = me->GetAbsOrigin();
	me->GetLocomotionInterface()->Stop();
	me->SetAbsVelocity( Vector( 0.0f, 0.0f, 0.0f ) );

	if ( me->IsMiniBoss() )
	{
		static CSchemaAttributeDefHandle pAttrDef_AirblastVerticalVulnerability( "airblast vertical vulnerability multiplier" );

		// Minibosses can't be pushed once they start deploying
		if ( !pAttrDef_AirblastVerticalVulnerability )
		{
			Warning( "TFBotSpawner: Invalid attribute 'airblast vertical vulnerability multiplier'\n" );
		}
		else
		{
			me->GetAttributeList()->SetRuntimeAttributeValue( pAttrDef_AirblastVerticalVulnerability, 0.0f );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotMvMDeployBomb::Update( CTFBot *me, float interval )
{
	CCaptureZone *pAreaTrigger = NULL;

	if ( me->GetDeployingBombState() != TF_BOMB_DEPLOYING_COMPLETE )
	{
		pAreaTrigger = me->GetClosestCaptureZone();
		if ( !pAreaTrigger )
		{
			return Done( "No capture zone!" );
		}

		// if we've been moved, give up and go back to normal behavior
		const float movedRange = 20.0f;
		if ( me->IsRangeGreaterThan( m_anchorPos, movedRange ) )
		{
			// Look for players that pushed me away and send an event
			CUtlVector<CTFPlayer *> playerVector;
			CollectPlayers( &playerVector, TF_TEAM_PVE_DEFENDERS );
			FOR_EACH_VEC( playerVector, i )
			{
				CTFPlayer *pPlayer = playerVector[i];
				if ( !pPlayer )
					continue;

				if ( me->m_AchievementData.IsPusherInHistory( pPlayer, 2.f ) )
				{
					IGameEvent *event = gameeventmanager->CreateEvent( "mvm_bomb_deploy_reset_by_player" );
					if ( event )
					{
						event->SetInt( "player", pPlayer->entindex() );
						gameeventmanager->FireEvent( event );
					}
				}
			}

			return Done( "I've been pushed" );
		}

		// face the capture zone
		me->GetBodyInterface()->AimHeadTowards( pAreaTrigger->WorldSpaceCenter(), IBody::CRITICAL, 0.5f, NULL, "Face point for bomb deploy" );

		// slam facing towards bomb hole
		Vector to = pAreaTrigger->WorldSpaceCenter() - me->WorldSpaceCenter();
		to.NormalizeInPlace();

		QAngle desiredAngles;
		VectorAngles( to, desiredAngles );

		me->SnapEyeAngles( desiredAngles );
	}

	switch ( me->GetDeployingBombState() )
	{
	case TF_BOMB_DEPLOYING_DELAY:
		if ( m_timer.IsElapsed() )
		{
			me->PlaySpecificSequence( "primary_deploybomb" );
			m_timer.Start( tf_deploying_bomb_time.GetFloat() );
			me->SetDeployingBombState( TF_BOMB_DEPLOYING_ANIMATING );

			const char *pszSoundName = me->IsMiniBoss() ? "MVM.DeployBombGiant" : "MVM.DeployBombSmall";
			me->EmitSound( pszSoundName );

			TFGameRules()->PlayThrottledAlert( 255, "Announcer.MVM_Bomb_Alert_Deploying", 5.0f );
		}
		break;

	case TF_BOMB_DEPLOYING_ANIMATING:
		if ( m_timer.IsElapsed() )
		{
			if ( pAreaTrigger )
			{
				pAreaTrigger->Capture( me );
			}

			m_timer.Start( 2.0f );
			TFGameRules()->BroadcastSound( 255, "Announcer.MVM_Robots_Planted" );
			me->SetDeployingBombState( TF_BOMB_DEPLOYING_COMPLETE );
			me->m_takedamage = DAMAGE_NO;
			me->AddEffects( EF_NODRAW );
			me->RemoveAllWeapons();
		}
		break;

	case TF_BOMB_DEPLOYING_COMPLETE:
		if ( m_timer.IsElapsed() )
		{
			me->SetDeployingBombState( TF_BOMB_DEPLOYING_NONE );
			me->m_takedamage = DAMAGE_YES;
			me->TakeDamage( CTakeDamageInfo( me, me, 99999.9f, DMG_CRUSH ) );
			return Done( "I've deployed successfully" );
		}
		break;
	}

	return Continue();
}


extern void TE_PlayerAnimEvent( CBasePlayer *pPlayer, PlayerAnimEvent_t event, int nData );


//---------------------------------------------------------------------------------------------
void CTFBotMvMDeployBomb::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	if ( me->GetDeployingBombState() == TF_BOMB_DEPLOYING_ANIMATING )
	{
		// reset the in-progress deploy animation
		me->m_PlayerAnimState->DoAnimationEvent( PLAYERANIMEVENT_SPAWN );
		TE_PlayerAnimEvent( me, PLAYERANIMEVENT_SPAWN, 0 );	// Send to any clients who can see this guy.
	}

	if ( me->IsMiniBoss() )
	{
		static CSchemaAttributeDefHandle pAttrDef_AirblastVerticalVulnerability( "airblast vertical vulnerability multiplier" );

		// Minibosses can be pushed again
		if ( !pAttrDef_AirblastVerticalVulnerability )
		{
			Warning( "TFBotSpawner: Invalid attribute 'airblast vertical vulnerability multiplier'\n" );
		}
		else
		{
			me->GetAttributeList()->RemoveAttribute( pAttrDef_AirblastVerticalVulnerability );
		}
	}

	me->SetDeployingBombState( TF_BOMB_DEPLOYING_NONE );
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotMvMDeployBomb::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	// so event doesn't fall thru to buried action which will then redo transition to this state as we stay in contact with the zone
	return TryToSustain( RESULT_CRITICAL );
}

QueryResultType CTFBotMvMDeployBomb::ShouldAttack( const INextBot *me, const CKnownEntity *them ) const
{
	return ANSWER_NO;
}

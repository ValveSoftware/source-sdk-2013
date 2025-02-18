//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_sniper_lurk.h
// Move into position and wait for victims
// Michael Booth, October 2009

#include "cbase.h"
#include "tf_player.h"

#ifdef TF_RAID_MODE
#include "raid/tf_raid_logic.h"
#endif // TF_RAID_MODE

#include "bot/tf_bot.h"
#include "bot/behavior/sniper/tf_bot_sniper_lurk.h"
#include "bot/behavior/sniper/tf_bot_sniper_attack.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"
#include "bot/behavior/tf_bot_melee_attack.h"
#include "bot/map_entities/tf_bot_hint.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;
extern ConVar tf_bot_sniper_flee_range;
extern ConVar tf_bot_sniper_melee_range;
extern ConVar tf_bot_debug_sniper;

extern float SkewedRandomValue( void );

ConVar tf_bot_sniper_patience_duration( "tf_bot_sniper_patience_duration", "10", FCVAR_CHEAT, "How long a Sniper bot will wait without seeing an enemy before picking a new spot" );
ConVar tf_bot_sniper_target_linger_duration( "tf_bot_sniper_target_linger_duration", "2", FCVAR_CHEAT, "How long a Sniper bot will keep toward at a target it just lost sight of" );
ConVar tf_bot_sniper_allow_opportunistic( "tf_bot_sniper_allow_opportunistic", "1", FCVAR_NONE, "If set, Snipers will stop on their way to their preferred lurking spot to snipe at opportunistic targets" );

ConVar tf_mvm_bot_sniper_target_by_dps( "tf_mvm_bot_sniper_target_by_dps", "1", FCVAR_CHEAT, "If set, Snipers in MvM mode target the victim that has the highest DPS" );


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperLurk::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_boredTimer.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );

	m_homePosition = me->GetAbsOrigin();
	m_isHomePositionValid = false;
	m_isAtHome = false;
	m_failCount = 0;

	m_isOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();

	CTFBotHint *hint = NULL;
	while( ( hint = (CTFBotHint *)( gEntList.FindEntityByClassname( hint, "func_tfbot_hint" ) ) ) != NULL )
	{
		if ( hint->IsA( CTFBotHint::HINT_SNIPER_SPOT ) )
		{
			m_hintVector.AddToTail( hint );

			// make sure we don't yet own any of these hints
			if ( me->IsSelf( hint->GetOwnerEntity() ) )
			{
				hint->SetOwnerEntity( NULL );
			}
		}
	}

	m_priorHint = NULL;

	if ( TFGameRules()->IsMannVsMachineMode() && me->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		// mann vs machine snipers shouldn't stop until they reach their home
		//m_isOpportunistic = false;

		// mann vs machine snipers should ignore the scenario and just snipe
		me->SetMission( CTFBot::MISSION_SNIPER, MISSION_DOESNT_RESET_BEHAVIOR_SYSTEM );
	}


	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperLurk::Update( CTFBot *me, float interval )
{
#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
	}
	else
#endif
	{
		// continuously search for good sniping spots
		me->AccumulateSniperSpots();

		if ( !m_isHomePositionValid )
		{
			// just found our first sniper spot - update our home position
			FindNewHome( me );
		}
	}

	// aim at bad guys
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat && !threat->GetEntity()->IsAlive() )
	{
		// he's dead
		threat = NULL;
	}

	if ( threat && me->GetIntentionInterface()->ShouldAttack( me, threat ) == ANSWER_NO )
	{
		threat = NULL;
	}

	if ( threat && threat->IsVisibleInFOVNow() )
	{
		m_failCount = 0;

		if ( me->IsDistanceBetweenLessThan( threat->GetLastKnownPosition(), tf_bot_sniper_melee_range.GetFloat() ) )
		{
			const float giveUpRange = 1.25f * tf_bot_sniper_melee_range.GetFloat();
			return SuspendFor( new CTFBotMeleeAttack( giveUpRange ), "Melee attacking nearby threat" );
		}
	}

	bool isSightingRifle = false;

	if ( threat && 
		 threat->GetTimeSinceLastSeen() < tf_bot_sniper_target_linger_duration.GetFloat() &&
		 me->IsLineOfFireClear( threat->GetEntity() ) )
	{
		// we see something...
		if ( m_isOpportunistic )
		{
			// switch to our sniper rifle
			CBaseCombatWeapon *myGun = me->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
			if ( myGun )
			{
				me->Weapon_Switch( myGun );
			}

			isSightingRifle = true;
			m_boredTimer.Reset();

			if ( !m_isHomePositionValid )
			{
				// make this our opportunistic home for awhile
				m_homePosition = me->GetAbsOrigin();
				m_boredTimer.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );
			}
		}
		else
		{
			// switch to our SMG and fire while we run
			CBaseCombatWeapon *myGun = me->Weapon_GetSlot( TF_WPN_TYPE_SECONDARY );
			if ( myGun )
			{
				me->Weapon_Switch( myGun );
			}
		}
	}

	const float homeRange = 25.0f; // 100.0f;
	m_isAtHome = ( me->GetAbsOrigin() - m_homePosition ).AsVector2D().IsLengthLessThan( homeRange );

	if ( m_isAtHome )
	{
		isSightingRifle = true;

		// once we've reached a good home spot, opportunistically attack from there
		m_isOpportunistic = tf_bot_sniper_allow_opportunistic.GetBool();

		if ( m_boredTimer.IsElapsed() )
		{
			++m_failCount;

			if ( FindNewHome( me ) )
			{
				me->SpeakConceptIfAllowed( MP_CONCEPT_PLAYER_NEGATIVE );
				m_boredTimer.Start( RandomFloat( 0.9f, 1.1f ) * tf_bot_sniper_patience_duration.GetFloat() );
			}
			else
			{
				// try again soon
				m_boredTimer.Start( 1.0f );
			}
		}
	}
	else
	{
		// not yet at home - can't start to be bored
		m_boredTimer.Reset();
	}

	if ( isSightingRifle )
	{
		// switch to our sniper rifle
		CTFWeaponBase *myGun = (CTFWeaponBase *)me->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
		if ( myGun )
		{
			me->Weapon_Switch( myGun );

			if ( !me->m_Shared.InCond( TF_COND_ZOOMED ) && !myGun->IsWeapon( TF_WEAPON_COMPOUND_BOW ) )
			{
				// zoom in and stand still
				me->PressAltFireButton();
			}
		}
	}
	else 
	{
		// move to our home position
		if ( m_repathTimer.IsElapsed() )
		{
			m_repathTimer.Start( RandomFloat( 1.0f, 2.0f ) );

			CTFBotPathCost cost( me, SAFEST_ROUTE );
			m_path.Compute( me, m_homePosition, cost );
		}

		m_path.Update( me );
		
		if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
		{
			me->PressAltFireButton();
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotSniperLurk::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// we're leaving to do something else - unzoom
		me->PressAltFireButton();
	}

	if ( m_priorHint != NULL )
	{
		// release my hint
		m_priorHint->SetOwnerEntity( NULL );

		if ( tf_bot_debug_sniper.GetBool() )
		{
			DevMsg( "%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, me->GetPlayerName() );
		}
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperLurk::OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// we're leaving to do something else - unzoom
		me->PressAltFireButton();
	}

	if ( m_priorHint != NULL )
	{
		// release my hint
		m_priorHint->SetOwnerEntity( NULL );

		if ( tf_bot_debug_sniper.GetBool() )
		{
			DevMsg( "%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, me->GetPlayerName() );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperLurk::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_repathTimer.Invalidate();
	m_priorHint = NULL;

	// we probably just fetched some health because the enemy shot us - pick a new place to lurk
	FindNewHome( me );

	return Continue();
}


//---------------------------------------------------------------------------------------------
bool CTFBotSniperLurk::FindHint( CTFBot *me )
{
	// if any sniper spot hints exist, pick one of them
	CUtlVector< CTFBotHint * > activeHintVector;
	for( int i=0; i<m_hintVector.Count(); ++i )
	{
		if ( m_hintVector[i] != NULL && m_hintVector[i]->IsFor( me ) )
		{
			activeHintVector.AddToTail( m_hintVector[i] );
		}
	}

	if ( activeHintVector.Count() == 0 )
	{
		return false;
	}

	if ( m_priorHint != NULL )
	{
		// release my hint
		m_priorHint->SetOwnerEntity( NULL );

		if ( tf_bot_debug_sniper.GetBool() )
		{
			DevMsg( "%3.2f: %s: Releasing hint.\n", gpGlobals->curtime, me->GetPlayerName() );
		}
	}

	CTFBotHint *hint = NULL;

	if ( m_priorHint != NULL && m_failCount < 2 )
	{
		// there used to be targets here - pick nearby hint
		float nearRange = 500.0f;
		CUtlVector< CTFBotHint * > nearHintVector;
		for( int i=0; i<activeHintVector.Count(); ++i )
		{
			if ( activeHintVector[i] == m_priorHint )
				continue;

			if ( ( activeHintVector[i]->WorldSpaceCenter() - m_priorHint->WorldSpaceCenter() ).IsLengthGreaterThan( nearRange ) )
				continue;

			if ( activeHintVector[i]->GetOwnerEntity() != NULL )
				continue;

			nearHintVector.AddToTail( activeHintVector[i] );
		}

		if ( nearHintVector.Count() == 0 )
		{
			++m_failCount;
			return false;
		}

		int whichHint = RandomInt( 0, nearHintVector.Count()-1 );
		hint = nearHintVector[ whichHint ];
	}
	else
	{
		// picking either our first hint, or we haven't seen a victim in a long time - pick a hint that can actually see someone
		CUtlVector< CTFPlayer * > victimVector;
		CollectPlayers( &victimVector, GetEnemyTeam( me->GetTeamNumber() ), COLLECT_ONLY_LIVING_PLAYERS );

		CUtlVector< CTFBotHint * > hotHintVector;
		CUtlVector< CTFBotHint * > freeHintVector;

		for( int i=0; i<activeHintVector.Count(); ++i )
		{
			if ( activeHintVector[i]->GetOwnerEntity() != NULL )
				continue;

			freeHintVector.AddToTail( activeHintVector[i] );

			for( int p=0; p<victimVector.Count(); ++p )
			{
				if ( victimVector[p]->IsLineOfSightClear( activeHintVector[i]->WorldSpaceCenter(), CBaseCombatCharacter::IGNORE_ACTORS ) )
				{
					// at least one victim is visible from this hint
					hotHintVector.AddToTail( activeHintVector[i] );
					break;
				}
			}
		}

		if ( hotHintVector.Count() == 0 )
		{
			// no hints can see any victims - pick at random
			if ( freeHintVector.Count() == 0 )
			{
				// all hints are owned by another sniper - double up
				int whichHint = RandomInt( 0, activeHintVector.Count()-1 );
				hint = activeHintVector[ whichHint ];

				if ( tf_bot_debug_sniper.GetBool() )
				{
					DevMsg( "%3.2f: %s: No un-owned hints available! Doubling up.\n", gpGlobals->curtime, me->GetPlayerName() );
				}
			}
			else
			{
				int whichHint = RandomInt( 0, freeHintVector.Count()-1 );
				hint = freeHintVector[ whichHint ];
			}
		}
		else
		{
			int whichHint = RandomInt( 0, hotHintVector.Count()-1 );
			hint = hotHintVector[ whichHint ];
		}
	}

	if ( hint == NULL )
	{
		return false;
	}

	Extent hintExtent;
	hintExtent.Init( hint );

	Vector hintSpot;
	hintSpot.x = RandomFloat( hintExtent.lo.x, hintExtent.hi.x );
	hintSpot.y = RandomFloat( hintExtent.lo.y, hintExtent.hi.y );
	hintSpot.z = ( hintExtent.lo.z + hintExtent.hi.z ) / 2.0f;

	TheNavMesh->GetSimpleGroundHeight( hintSpot, &hintSpot.z );

	m_homePosition = hintSpot;
	m_isHomePositionValid = true;
	m_priorHint = hint;

	// my hint
	hint->SetOwnerEntity( me );

	return true;
}


//---------------------------------------------------------------------------------------------
bool CTFBotSniperLurk::FindNewHome( CTFBot *me )
{
	if ( !m_findHomeTimer.IsElapsed() )
	{
		return false;
	}

	m_findHomeTimer.Start( RandomFloat( 1.0f, 2.0f ) );


#ifdef TF_RAID_MODE
	if ( TFGameRules()->IsRaidMode() )
	{
		// stay put for now
		return true;
	}
	else
#endif // TF_RAID_MODE
	{
		// if any sniper spot hints exist, pick one of them
		if ( FindHint( me ) )
		{
			return true;
		}

		// pick a sniper spot from our ongoing search
		const CUtlVector< CTFBot::SniperSpotInfo > *sniperSpotVector = me->GetSniperSpots();
		if ( sniperSpotVector->Count() > 0 )
		{
			m_homePosition = sniperSpotVector->Element( RandomInt( 0, sniperSpotVector->Count()-1 ) ).m_vantageSpot;
			m_isHomePositionValid = true;
			return true;
		}
	}

	// can't find a real sniper spot - pick another goal that will get us out into the fray
	m_isHomePositionValid = false;

	// head toward the point
	CTeamControlPoint *point = me->GetMyControlPoint();
	if ( point && !point->IsLocked() )
	{
		const CUtlVector< CTFNavArea * > *pointAreaVector = TheTFNavMesh()->GetControlPointAreas( point->GetPointIndex() );

		if ( pointAreaVector && pointAreaVector->Count() > 0 )
		{
			int which = RandomInt( 0, pointAreaVector->Count()-1 );

			m_homePosition = pointAreaVector->Element( which )->GetRandomPoint();

			return false;
		}
	}

	// no available point at the moment - head toward the enemy spawn room and opportunistically snipe
	CUtlVector< CTFNavArea * > enemySpawnThresholdVector;
	TheTFNavMesh()->CollectSpawnRoomThresholdAreas( &enemySpawnThresholdVector, GetEnemyTeam( me->GetTeamNumber() ) );

	if ( enemySpawnThresholdVector.Count() > 0 )
	{
		m_homePosition = enemySpawnThresholdVector[ RandomInt( 0, enemySpawnThresholdVector.Count()-1 ) ]->GetCenter();
	}
	else
	{
		m_homePosition = me->GetAbsOrigin();
	}

	return false;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSniperLurk::ShouldAttack( const INextBot *bot, const CKnownEntity *them ) const
{
	CTFBot *me = (CTFBot *)bot->GetEntity();

	CTFNavArea *area = me->GetLastKnownArea();

	if ( TFGameRules()->IsMannVsMachineMode() && area && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) )
	{
		// don't fire while in the spawn area
		return ANSWER_NO;
	}

	// take the shot if you've got it
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSniperLurk::ShouldRetreat( const INextBot *me ) const
{
	if ( TFGameRules()->IsMannVsMachineMode() && me->GetEntity()->GetTeamNumber() == TF_TEAM_PVE_INVADERS )
	{
		return ANSWER_NO;
	}

	return ANSWER_UNDEFINED;
}

//---------------------------------------------------------------------------------------------
// Return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
const CKnownEntity *CTFBotSniperLurk::SelectMoreDangerousThreat( const INextBot *meBot, 
																 const CBaseCombatCharacter *subject,
																 const CKnownEntity *threat1, 
																 const CKnownEntity *threat2 ) const
{
	if ( TFGameRules()->IsMannVsMachineMode() && tf_mvm_bot_sniper_target_by_dps.GetBool() )
	{
		CTFBot *me = ToTFBot( meBot->GetEntity() );

		// If one threat is visible and the other not, always pick the visible one
		if ( !threat1->IsVisibleRecently() )
		{
			if ( threat2->IsVisibleRecently() )
			{
				return threat2;
			}
		}
		else if ( !threat2->IsVisibleRecently() )
		{
			return threat1;
		}

		// At this point, threat1 and threat2 are either both visible, or both not

		CTFPlayer *playerThreat1 = ToTFPlayer( threat1->GetEntity() );
		CTFPlayer *playerThreat2 = ToTFPlayer( threat2->GetEntity() );

		if ( playerThreat1 && playerThreat2 )
		{
			float rangeSq1 = me->GetRangeSquaredTo( playerThreat1 );
			float rangeSq2 = me->GetRangeSquaredTo( playerThreat2 );

			if ( me->HasWeaponRestriction( CTFBot::MELEE_ONLY ) )
			{
				// Melee-only bots just use closest threat
				if ( rangeSq1 < rangeSq2 )
				{
					return threat1;
				}
				return threat2;
			}

			// Very near threats are always immediately dangerous
			const float nearbyRangeSq = 500.0f * 500.0f;
			if ( rangeSq1 < nearbyRangeSq )
			{
				if ( rangeSq2 > nearbyRangeSq )
				{
					return threat1;
				}
			}
			else if ( rangeSq2 < nearbyRangeSq )
			{
				return threat2;
			}

			// At this point, both threats are either both very near or both "far"

			// Choose the threat that has the highest DPS
			const int equalTolerance = 50;

			if ( playerThreat1->GetDamagePerSecond() > playerThreat2->GetDamagePerSecond() + equalTolerance )
			{
				return threat1;
			}
			else if ( playerThreat2->GetDamagePerSecond() > playerThreat1->GetDamagePerSecond() + equalTolerance )
			{
				return threat2;
			}
			else
			{
				// approximately equal DPS, choose closest
				if ( rangeSq1 < rangeSq2 )
				{
					return threat1;
				}
				return threat2;
			}
		}
	}

	// Use normal threat selection
	return NULL;
}

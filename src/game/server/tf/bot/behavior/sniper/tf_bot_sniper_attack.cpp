//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_sniper_attack.h
// Attack a threat as a Sniper
// Michael Booth, February 2009

#include "cbase.h"
#include "tf_player.h"
#include "tf_obj_sentrygun.h"
#include "tf_gamerules.h"
#include "bot/tf_bot.h"
#include "bot/behavior/sniper/tf_bot_sniper_attack.h"
#include "bot/behavior/tf_bot_melee_attack.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_sniper_flee_range( "tf_bot_sniper_flee_range", "400", FCVAR_CHEAT, "If threat is closer than this, retreat" );
ConVar tf_bot_sniper_melee_range( "tf_bot_sniper_melee_range", "200", FCVAR_CHEAT, "If threat is closer than this, attack with melee weapon" );
ConVar tf_bot_sniper_linger_time( "tf_bot_sniper_linger_time", "5", FCVAR_CHEAT, "How long Sniper will wait around after losing his target before giving up" );


//---------------------------------------------------------------------------------------------
bool CTFBotSniperAttack::IsPossible( CTFBot *me )
{
	return me->IsPlayerClass( TF_CLASS_SNIPER ) && me->GetVisionInterface()->GetPrimaryKnownThreat() && me->GetVisionInterface()->GetPrimaryKnownThreat()->IsVisibleRecently();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperAttack::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperAttack::Update( CTFBot *me, float interval )
{
	// switch to our sniper rifle
	CBaseCombatWeapon *myGun = me->Weapon_GetSlot( TF_WPN_TYPE_PRIMARY );
	if ( myGun )
	{
		me->Weapon_Switch( myGun );
	}

	// shoot at bad guys
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat && !threat->GetEntity()->IsAlive() )
	{
		// he's dead
		threat = NULL;
	}

	if ( threat == NULL || !threat->IsVisibleInFOVNow() )
	{
		if ( m_lingerTimer.IsElapsed() )
		{
			if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
			{
				return Continue();
			}

			return Done( "No threat for awhile" );
		}

		return Continue();
	}

	me->EquipBestWeaponForThreat( threat );

	if ( me->IsDistanceBetweenLessThan( threat->GetLastKnownPosition(), tf_bot_sniper_flee_range.GetFloat() ) )
	{
		return SuspendFor( new CTFBotRetreatToCover, "Retreating from nearby enemy" );
	}

	if ( me->GetTimeSinceLastInjury() < 1.0f )
	{
		return SuspendFor( new CTFBotRetreatToCover, "Retreating due to injury" );
	}

	// we have a target
	m_lingerTimer.Start( RandomFloat( 0.75f, 1.25f ) * tf_bot_sniper_linger_time.GetFloat() );

	if ( !me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		me->PressAltFireButton();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
void CTFBotSniperAttack::OnEnd( CTFBot *me, Action< CTFBot > *nextAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// we're leaving to do something else - unzoom
		me->PressAltFireButton();
	}
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperAttack::OnSuspend( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	if ( me->m_Shared.InCond( TF_COND_ZOOMED ) )
	{
		// we're leaving to do something else - unzoom
		me->PressAltFireButton();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSniperAttack::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	return Continue();
}


//---------------------------------------------------------------------------------------------
// given a subject, return the world space position we should aim at
Vector CTFBotSniperAttack::SelectTargetPoint( const INextBot *me, const CBaseCombatCharacter *subject ) const
{
	VPROF_BUDGET( "CTFBotSniperAttack::SelectTargetPoint", "NextBot" );

	Vector visibleSpot;

	trace_t result;
	NextBotTraceFilterIgnoreActors filter( subject, COLLISION_GROUP_NONE );

	// head, then chest, then feet for the Sniper

	// headshot seems to be a bit higher that EyePosition()
	Vector subjectHeadPos( subject->EyePosition() );
	subjectHeadPos.z += 1.0f;

	UTIL_TraceLine( me->GetBodyInterface()->GetEyePosition(), subjectHeadPos, MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
	if ( result.DidHit() )
	{
		UTIL_TraceLine( me->GetBodyInterface()->GetEyePosition(), subject->WorldSpaceCenter(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );

		if ( result.DidHit() )
		{
			UTIL_TraceLine( me->GetBodyInterface()->GetEyePosition(), subject->GetAbsOrigin(), MASK_BLOCKLOS_AND_NPCS|CONTENTS_IGNORE_NODRAW_OPAQUE, &filter, &result );
		}
	}

	// even if they aren't visible, we have no way to communicate that out, so pick a reasonable spot
	return result.endpos;
}


//---------------------------------------------------------------------------------------------
bool CTFBotSniperAttack::IsImmediateThreat( const CBaseCombatCharacter *subject, const CKnownEntity *threat ) const
{
	if ( subject->InSameTeam( threat->GetEntity() ) )
		return false;

	if ( !threat->GetEntity()->IsAlive() )
		return false;

	const float hiddenAwhile = 3.0f;
	if ( !threat->WasEverVisible() || threat->GetTimeSinceLastSeen() > hiddenAwhile )
		return false;

	CTFPlayer *player = ToTFPlayer( threat->GetEntity() );

	Vector to = subject->GetAbsOrigin() - threat->GetLastKnownPosition();
	float threatRange = to.NormalizeInPlace();

	if ( player == NULL )
	{
		CObjectSentrygun *sentry = dynamic_cast< CObjectSentrygun * >( threat->GetEntity() );
		if ( sentry )
		{
			// are we in range?
			if ( threatRange < SENTRY_MAX_RANGE )
			{
				// is it pointing at us?
				Vector sentryForward;
				AngleVectors( sentry->GetTurretAngles(), &sentryForward );

				if ( DotProduct( to, sentryForward ) > 0.8f )
				{
					return true;
				}
			}
		}
		return false;
	}

	if ( player->IsPlayerClass( TF_CLASS_SNIPER ) )
	{
		// is the sniper pointing at me?
		Vector sniperForward;
		player->EyeVectors( &sniperForward );

		if ( DotProduct( to, sniperForward ) > 0.8f )
		{
			return true;
		}
	}

#ifdef TF_RAID_MODE
	if ( !TFGameRules()->IsRaidMode() )
	{
	}
	else
#endif // TF_RAID_MODE
	{
		if ( player->IsPlayerClass( TF_CLASS_MEDIC ) )
		{
			// always try to kill these guys first
			return true;
		}
	}

	return false;
}


//---------------------------------------------------------------------------------------------
// return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
const CKnownEntity *CTFBotSniperAttack::SelectMoreDangerousThreat( const INextBot *me, 
																   const CBaseCombatCharacter *subject,
																   const CKnownEntity *threat1, 
																   const CKnownEntity *threat2 ) const
{
	if ( threat1 && threat2 )
	{
		bool isImmediateThreat1 = IsImmediateThreat( subject, threat1 );
		bool isImmediateThreat2 = IsImmediateThreat( subject, threat2 );

		if ( isImmediateThreat1 && !isImmediateThreat2 )
		{
			return threat1;
		}
		else if ( !isImmediateThreat1 && isImmediateThreat2 )
		{
			return threat2;
		}
	}

	// both or neither are immediate threats - no preference
	return NULL;
}

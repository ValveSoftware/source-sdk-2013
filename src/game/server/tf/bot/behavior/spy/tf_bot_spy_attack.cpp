//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_spy_attack.cpp
// Backstab or pistol, as appropriate
// Michael Booth, June 2010

#include "cbase.h"
#include "tf_player.h"
#include "bot/tf_bot.h"
#include "bot/behavior/spy/tf_bot_spy_attack.h"
#include "bot/behavior/tf_bot_retreat_to_cover.h"
#include "bot/behavior/spy/tf_bot_spy_sap.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_spy_knife_range( "tf_bot_spy_knife_range", "300", FCVAR_CHEAT, "If threat is closer than this, prefer our knife" );
ConVar tf_bot_spy_change_target_range_threshold( "tf_bot_spy_change_target_range_threshold", "300", FCVAR_CHEAT );


//---------------------------------------------------------------------------------------------
CTFBotSpyAttack::CTFBotSpyAttack( CTFPlayer *victim ) : m_path( ChasePath::LEAD_SUBJECT )
{
	m_victim = victim;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyAttack::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );
	m_isCoverBlown = false;

	if ( m_victim.Get() )
	{
		me->GetVisionInterface()->AddKnownEntity( m_victim );
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotSpyAttack::Update( CTFBot *me, float interval )
{
	const CKnownEntity *threat = me->GetVisionInterface()->GetKnown( m_victim );

	// opportunistically attack closer threat if they are much closer to us than our existing threat
	const CKnownEntity *closestThreat = me->GetVisionInterface()->GetPrimaryKnownThreat();
	
	if ( !threat )
	{
		threat = closestThreat;
		m_isCoverBlown = false;
		if ( closestThreat )
		{
			m_victim = ToTFPlayer( closestThreat->GetEntity() );
		}
	}
	else if ( closestThreat && 
			  closestThreat->GetEntity() && 
			  closestThreat != threat )
	{
		float rangeToCurrentThreat = me->GetRangeTo( threat->GetLastKnownPosition() );
		float rangeToNewThreat = me->GetRangeTo( closestThreat->GetLastKnownPosition() );

		if ( rangeToCurrentThreat - rangeToNewThreat > tf_bot_spy_change_target_range_threshold.GetFloat() )
		{
			if ( closestThreat->GetEntity()->IsPlayer() )
			{
				threat = closestThreat;
				m_victim = ToTFPlayer( closestThreat->GetEntity() );
				m_isCoverBlown = false;
			}
		}
	}

	if ( !threat || threat->IsObsolete() )
	{
		return Done( "No threat" );
	}


	CBaseObject *sapTarget = me->GetNearestKnownSappableTarget();
	if ( sapTarget && me->IsEntityBetweenTargetAndSelf( sapTarget, threat->GetEntity() ) )
	{
		return ChangeTo( new CTFBotSpySap( sapTarget ), "Opportunistically sapping an enemy object between my victim and I" );
	}

	if ( me->IsAnyEnemySentryAbleToAttackMe() )
	{
		m_isCoverBlown = true;

		CBaseCombatWeapon *myGun = me->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY );
		me->Weapon_Switch( myGun );

		return ChangeTo( new CTFBotRetreatToCover, "Escaping sentry fire!" );
	}

	CTFPlayer *playerThreat = ToTFPlayer( threat->GetEntity() );
	if ( !playerThreat )
	{
		return Done( "Current 'threat' is not a player or a building?" );
	}

	// remember who we are attacking (in case we changed our minds)
	m_victim = playerThreat;

	// uncloak so we can attack
	if ( me->m_Shared.IsStealthed() && m_decloakTimer.IsElapsed() )
	{
		me->PressAltFireButton();
		m_decloakTimer.Start( 1.0f );
	}

	bool isKnifeFight = false;

	if ( me->m_Shared.InCond( TF_COND_DISGUISED ) ||
		 me->m_Shared.InCond( TF_COND_DISGUISING ) ||
		 me->m_Shared.IsStealthed() )
	{
		isKnifeFight = true;
	}

	Vector playerThreatForward;
	playerThreat->EyeVectors( &playerThreatForward );

	Vector toPlayerThreat = playerThreat->GetAbsOrigin() - me->GetAbsOrigin();
	float threatRange = toPlayerThreat.NormalizeInPlace();

	float behindTolerance = 0.0f;

	switch( me->GetDifficulty() )
	{
	case CTFBot::EASY:		behindTolerance = 0.9f;		break;
	case CTFBot::NORMAL:	behindTolerance = 0.7071f;	break;
	case CTFBot::HARD:		behindTolerance = 0.2f;		break;
	case CTFBot::EXPERT:	behindTolerance = 0.0f;		break;
	}

	if ( TFGameRules()->IsMannVsMachineMode() )
	{
		behindTolerance = 0.7071f;
	}

	bool isBehindVictim = DotProduct( playerThreatForward, toPlayerThreat ) > behindTolerance;

	// easy Spies always think they're in position to backstab
	if ( me->GetDifficulty() == CTFBot::EASY )
	{
		isBehindVictim = true;
	}

	if ( threatRange < tf_bot_spy_knife_range.GetFloat() )
	{
		isKnifeFight = true;
	}
	else if ( threat->IsVisibleInFOVNow() && isBehindVictim )
	{
		// they are facing away from us - go for the backstab
		isKnifeFight = true;
	}

	// does my threat know I'm a Spy?
	if ( me->IsThreatAimingTowardMe( playerThreat, 0.99f ) && me->GetTimeSinceLastInjury( GetEnemyTeam( me->GetTeamNumber() ) ) < 1.0f )
	{
		m_isCoverBlown |= ( playerThreat->GetTimeSinceWeaponFired() < 0.25f );
	}
	
	if ( m_isCoverBlown ||
		 me->m_Shared.InCond( TF_COND_BURNING ) ||
		 me->m_Shared.InCond( TF_COND_URINE ) ||
		 me->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
		 me->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		isKnifeFight = false;
	}

	CBaseCombatWeapon *myGun = me->Weapon_GetWeaponByType( isKnifeFight ? TF_WPN_TYPE_MELEE : TF_WPN_TYPE_PRIMARY );
	me->Weapon_Switch( myGun );

	CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();

	bool isMovingTowardVictim = true;

	if ( myWeapon && myWeapon->IsMeleeWeapon() )
	{
		if ( threat->IsVisibleInFOVNow() )
		{
			const float circleStrafeRange = 250.0f;

			if ( threatRange < circleStrafeRange )
			{
				// we're close - aim our stab attack
				me->GetBodyInterface()->AimHeadTowards( playerThreat, IBody::MANDATORY, 0.1f, NULL, "Aiming my stab!" );

				if ( !isBehindVictim )
				{
					// circle around our victim to get behind them
					Vector myForward;
					me->EyeVectors( &myForward );

					Vector cross;
					CrossProduct( playerThreatForward, myForward, cross );

					if ( cross.z < 0.0f )
					{
						me->PressRightButton();
					}
					else
					{
						me->PressLeftButton();
					}

					// don't continue to close in if we're already very close so we don't bump them and give ourselves away
					if ( threatRange < 100.0f )
					{
						isMovingTowardVictim = false;
					}
				}
				else if ( TFGameRules()->IsMannVsMachineMode() )
				{
					if ( m_chuckleTimer.IsElapsed() )
					{
						m_chuckleTimer.Start( 1.0f );
						me->EmitSound( "Spy.MVM_Chuckle" );
					}
				}
			}

			if ( threatRange < me->GetDesiredAttackRange() )
			{
				// if we're still disguised, go for the backstab
				if ( me->m_Shared.InCond( TF_COND_DISGUISED ) )
				{
					if ( isBehindVictim || m_isCoverBlown )
					{
						// we're behind them (or they're onto us) - backstab!
						me->PressFireButton();
					}
				}
				else
				{
					// we're exposed - stab! stab! stab!
					me->PressFireButton();
				}
			}
		}
	}
	else
	{
		// aim our pistol
		me->GetBodyInterface()->AimHeadTowards( playerThreat, IBody::MANDATORY, 0.1f, NULL, "Aiming my pistol" );
	}

	if ( isMovingTowardVictim )
	{
		// pursue the threat. if not visible, go to the last known position
		if ( !threat->IsVisibleRecently() || 
			 me->IsRangeGreaterThan( threat->GetEntity()->GetAbsOrigin(), me->GetDesiredAttackRange() ) || 
			 !me->IsLineOfFireClear( threat->GetEntity()->EyePosition() ) )
		{
			// if we're at the threat's last known position and he's still not visible, we lost him
			if ( !threat->IsVisibleRecently() )
			{
				if ( me->IsRangeLessThan( threat->GetLastKnownPosition(), 20.0f ) )
				{
					me->GetVisionInterface()->ForgetEntity( threat->GetEntity() );
					return Done( "I lost my target!" );
				}
			}

			CTFBotPathCost cost( me, FASTEST_ROUTE );
			m_path.Update( me, threat->GetEntity(), cost );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot > CTFBotSpyAttack::OnResume( CTFBot *me, Action< CTFBot > *interruptingAction )
{
	m_victim = NULL;
	m_path.Invalidate();
	m_isCoverBlown = false;

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSpyAttack::OnStuck( CTFBot *me )
{
	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSpyAttack::OnInjured( CTFBot *me, const CTakeDamageInfo &info )
{
	if ( me->IsEnemy( info.GetAttacker() ) )
	{
		if ( !me->m_Shared.InCond( TF_COND_DISGUISED ) )
		{
			// hurt by an enemy and exposed as a spy - flee!
			m_isCoverBlown = true;

			CBaseCombatWeapon *myGun = me->Weapon_GetWeaponByType( TF_WPN_TYPE_PRIMARY );
			me->Weapon_Switch( myGun );

			return TryChangeTo( new CTFBotRetreatToCover, RESULT_IMPORTANT, "Time to get out of here!" );
		}
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CTFBot > CTFBotSpyAttack::OnContact( CTFBot *me, CBaseEntity *other, CGameTrace *result )
{
	if ( me->IsEnemy( other ) && other->MyCombatCharacterPointer() )
	{
		if ( other->MyCombatCharacterPointer()->IsLookingTowards( me ) )
		{
			m_isCoverBlown = true;
		}
	}

	return TryContinue();
}


//---------------------------------------------------------------------------------------------
QueryResultType	CTFBotSpyAttack::ShouldRetreat( const INextBot *me ) const
{
	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSpyAttack::ShouldHurry( const INextBot *me ) const
{
	return ANSWER_YES;
}


//---------------------------------------------------------------------------------------------
QueryResultType CTFBotSpyAttack::ShouldAttack( const INextBot *meBot, const CKnownEntity *them ) const
{
	CTFBot *me = ToTFBot( meBot->GetEntity() );

	if ( m_isCoverBlown ||
		 me->m_Shared.InCond( TF_COND_BURNING ) ||
		 me->m_Shared.InCond( TF_COND_URINE ) ||
		 me->m_Shared.InCond( TF_COND_STEALTHED_BLINK ) ||
		 me->m_Shared.InCond( TF_COND_BLEEDING ) )
	{
		// our cover is blown anyway
		return ANSWER_YES;
	}

	return ANSWER_NO;
}


//---------------------------------------------------------------------------------------------
// Use this to signal the enemy we are focusing on, so we dont avoid them
QueryResultType CTFBotSpyAttack::IsHindrance( const INextBot *me, CBaseEntity *blocker ) const
{
	if ( blocker != IS_ANY_HINDRANCE_POSSIBLE )
	{
		if ( blocker && m_victim.Get() && blocker->entindex() == m_victim->entindex() )
		{
			// don't avoid this guy
			return ANSWER_NO;
		}
	}

	return ANSWER_UNDEFINED;
}


//---------------------------------------------------------------------------------------------
// Return the more dangerous of the two threats to 'subject', or NULL if we have no opinion
const CKnownEntity * CTFBotSpyAttack::SelectMoreDangerousThreat( const INextBot *meBot, 
																 const CBaseCombatCharacter *subject,
																 const CKnownEntity *threat1, 
																 const CKnownEntity *threat2 ) const
{
	CTFBot *me = ToTFBot( meBot->GetEntity() );

	if ( me->IsSelf( subject ) )
	{
		CTFWeaponBase *myWeapon = me->m_Shared.GetActiveTFWeapon();
		if ( myWeapon && myWeapon->IsMeleeWeapon() )
		{
			// attack the closest victim with my knife
			if ( me->GetRangeSquaredTo( threat1->GetEntity() ) < me->GetRangeSquaredTo( threat2->GetEntity() ) )
			{
				return threat1;
			}

			return threat2;
		}
	}

	return NULL;
}


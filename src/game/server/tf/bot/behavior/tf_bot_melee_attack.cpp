//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot_melee_attack.h
// Attack a threat with out melee weapon
// Michael Booth, February 2009

#include "cbase.h"
#include "bot/tf_bot.h"
#include "bot/behavior/tf_bot_melee_attack.h"

#include "nav_mesh.h"

extern ConVar tf_bot_path_lookahead_range;

ConVar tf_bot_melee_attack_abandon_range( "tf_bot_melee_attack_abandon_range", "500", FCVAR_CHEAT, "If threat is farther away than this, bot will switch back to its primary weapon and attack" );


//---------------------------------------------------------------------------------------------
CTFBotMeleeAttack::CTFBotMeleeAttack( float giveUpRange )
{
	m_giveUpRange = giveUpRange < 0.0f ? tf_bot_melee_attack_abandon_range.GetFloat() : giveUpRange;
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMeleeAttack::OnStart( CTFBot *me, Action< CTFBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CTFBot >	CTFBotMeleeAttack::Update( CTFBot *me, float interval )
{
	// bash the bad guys
	const CKnownEntity *threat = me->GetVisionInterface()->GetPrimaryKnownThreat();

	if ( threat == NULL )
	{
		return Done( "No threat" );
	}

	if ( me->IsDistanceBetweenGreaterThan( threat->GetLastKnownPosition(), m_giveUpRange ) )
	{
		// threat is too far away for melee
		return Done( "Threat is too far away for a melee attack" );
	}

	// switch to our melee weapon
	CBaseCombatWeapon *meleeWeapon = me->Weapon_GetSlot( TF_WPN_TYPE_MELEE );
	if ( meleeWeapon )
	{
		me->Weapon_Switch( meleeWeapon );
	}

	// actual head aiming is handled elsewhere

	// just keep swinging
	me->PressFireButton();

	// chase them down
	CTFBotPathCost cost( me, FASTEST_ROUTE );
	m_path.Update( me, threat->GetEntity(), cost );

	return Continue();
}

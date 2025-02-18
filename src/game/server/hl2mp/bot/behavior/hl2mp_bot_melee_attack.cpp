//========= Copyright Valve Corporation, All rights reserved. ============//

#include "cbase.h"
#include "bot/hl2mp_bot.h"
#include "bot/behavior/hl2mp_bot_melee_attack.h"
#include "hl2mp/weapon_hl2mpbasebasebludgeon.h"

#include "nav_mesh.h"

extern ConVar hl2mp_bot_path_lookahead_range;

ConVar hl2mp_bot_melee_attack_abandon_range( "hl2mp_bot_melee_attack_abandon_range", "500", FCVAR_CHEAT, "If threat is farther away than this, bot will switch back to its primary weapon and attack" );


//---------------------------------------------------------------------------------------------
CHL2MPBotMeleeAttack::CHL2MPBotMeleeAttack( float giveUpRange )
{
	m_giveUpRange = giveUpRange < 0.0f ? hl2mp_bot_melee_attack_abandon_range.GetFloat() : giveUpRange;
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotMeleeAttack::OnStart( CHL2MPBot *me, Action< CHL2MPBot > *priorAction )
{
	m_path.SetMinLookAheadDistance( me->GetDesiredPathLookAheadRange() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHL2MPBot >	CHL2MPBotMeleeAttack::Update( CHL2MPBot *me, float interval )
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
	CBaseHL2MPBludgeonWeapon *meleeWeapon = me->GetBludgeonWeapon();

	if ( !meleeWeapon )
	{
		// misyl: TF nextbot is missing this check... Interesting.
		return Done( "Don't have a melee weapon!" );
	}

	me->Weapon_Switch( meleeWeapon );

	// actual head aiming is handled elsewhere

	// just keep swinging
	me->PressFireButton();

	// chase them down
	CHL2MPBotPathCost cost( me, FASTEST_ROUTE );
	m_path.Update( me, threat->GetEntity(), cost );

	return Continue();
}

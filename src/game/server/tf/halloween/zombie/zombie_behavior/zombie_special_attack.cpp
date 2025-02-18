//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_fx.h"

#include "../zombie.h"
#include "zombie_special_attack.h"

ActionResult< CZombie >	CZombieSpecialAttack::OnStart( CZombie *me, Action< CZombie > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_SPECIAL_ATTACK1 );

	m_stompTimer.Start( 1 );

	return Continue();
}


ActionResult< CZombie >	CZombieSpecialAttack::Update( CZombie *me, float interval )
{
	if ( m_stompTimer.HasStarted() && m_stompTimer.IsElapsed() )
	{
		DoSpecialAttack( me );
		m_stompTimer.Invalidate();
	}

	if ( me->IsActivityFinished() )
	{
		return Done();
	}

	return Continue();
}


void CZombieSpecialAttack::DoSpecialAttack( CZombie *me )
{
	CPVSFilter filter( me->GetAbsOrigin() );
	TE_TFParticleEffect( filter, 0.0, "bomibomicon_ring", me->GetAbsOrigin(), vec3_angle );

	int nTargetTeam = TEAM_ANY;
	if ( me->GetTeamNumber() != TF_TEAM_HALLOWEEN )
	{
		nTargetTeam = me->GetTeamNumber() == TF_TEAM_RED ? TF_TEAM_BLUE : TF_TEAM_RED;
	}

	CUtlVector< CTFPlayer* > pushedPlayers;
	TFGameRules()->PushAllPlayersAway( me->GetAbsOrigin(), 200.f, 500.f, nTargetTeam, &pushedPlayers );

	CBaseEntity *pAttacker = me->GetOwnerEntity() ? me->GetOwnerEntity() : me;
	for ( int i=0; i<pushedPlayers.Count(); ++i )
	{
		Vector toVictim = pushedPlayers[i]->WorldSpaceCenter() - me->WorldSpaceCenter();
		toVictim.NormalizeInPlace();

		// hit!
		CTakeDamageInfo info( pAttacker, pAttacker, me->GetAttackDamage(), DMG_SLASH );
		info.SetDamageCustom( TF_DMG_CUSTOM_SPELL_SKELETON );
		CalculateMeleeDamageForce( &info, toVictim, me->WorldSpaceCenter(), 5.0f );
		pushedPlayers[i]->TakeDamage( info );
	}
}

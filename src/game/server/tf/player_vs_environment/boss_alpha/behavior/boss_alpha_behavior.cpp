//========= Copyright Valve Corporation, All rights reserved. ============//
// boss_alpha_behavior.cpp
// Michael Booth, November 2010

#include "cbase.h"

#ifdef TF_RAID_MODE

#include "CRagdollMagnet.h"
#include "tf_gamerules.h"
#include "tf_shareddefs.h"
#include "player_vs_environment/monster_resource.h"
#include "player_vs_environment/boss_alpha/boss_alpha.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_behavior.h"
#include "player_vs_environment/boss_alpha/behavior/boss_alpha_tactical_monitor.h"


//---------------------------------------------------------------------------------------------
Action< CBossAlpha > *CBossAlphaBehavior::InitialContainedAction( CBossAlpha *me )	
{
	return new CBossAlphaTacticalMonitor;
}


//---------------------------------------------------------------------------------------------
ActionResult< CBossAlpha >	CBossAlphaBehavior::Update( CBossAlpha *me, float interval )
{
	if ( m_vocalTimer.IsElapsed() )
	{
		m_vocalTimer.Start( RandomFloat( 3.0f, 5.0f ) );

		if ( !me->IsBusy() )
		{
			me->EmitSound( "RobotBoss.Vocalize" );
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaBehavior::OnKilled( CBossAlpha *me, const CTakeDamageInfo &info )
{
	// relay the event to the map logic
	me->m_outputOnKilled.FireOutput( me, me );

	// Calculate death force
	Vector forceVector = me->CalcDamageForceVector( info );

	// See if there's a ragdoll magnet that should influence our force.
	CRagdollMagnet *magnet = CRagdollMagnet::FindBestMagnet( me );
	if ( magnet )
	{
		forceVector += magnet->GetForceVector( me );
	}

	if ( me->IsMiniBoss() )
	{
		me->EmitSound( "Cart.Explode" );
		me->BecomeRagdoll( info, forceVector );

		if ( g_pMonsterResource )
		{
			g_pMonsterResource->HideBossHealthMeter();
		}
	}
	else
	{
		// full end-of-game boss
		UTIL_Remove( me );

		if ( TFGameRules()->IsBossBattleMode() )
		{
			// check that ALL bosses are dead
			bool isBossBattleWon = true;

			CBossAlpha *boss = NULL;
			while( ( boss = (CBossAlpha *)gEntList.FindEntityByClassname( boss, "boss_alpha" ) ) != NULL )
			{
				if ( !me->IsSelf( boss ) && boss->IsAlive() && !boss->IsMiniBoss() )
				{
					isBossBattleWon = false;
				}
			}

			if ( isBossBattleWon )
			{
				TFGameRules()->SetWinningTeam( TF_TEAM_BLUE, WINREASON_OPPONENTS_DEAD );

				if ( g_pMonsterResource )
				{
					g_pMonsterResource->HideBossHealthMeter();
				}
			}
		}
	}

	return TryDone();
}


//---------------------------------------------------------------------------------------------
EventDesiredResult< CBossAlpha > CBossAlphaBehavior::OnContact( CBossAlpha *me, CBaseEntity *other, CGameTrace *result )
{
	return TryContinue();
}

#endif // TF_RAID_MODE

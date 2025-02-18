//========= Copyright Valve Corporation, All rights reserved. ============//
// eyeball_boss_teleport.cpp
// The 2011 Halloween Boss
// Michael Booth, October 2011

#include "cbase.h"

#include "particle_parse.h"
#include "tf/halloween/eyeball_boss/teleport_vortex.h"
#include "../eyeball_boss.h"
#include "eyeball_boss_teleport.h"
#include "player_vs_environment/monster_resource.h"


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossTeleport::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	m_state = TELEPORTING_OUT;

	int animSequence = me->LookupSequence( "teleport_out" );
	if ( animSequence )
	{
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossTeleport::Update( CEyeballBoss *me, float interval )
{
	if ( me->IsSequenceFinished() )
	{
		switch( m_state )
		{
		case TELEPORTING_OUT:
			{
				CTeleportVortex *vortex = (CTeleportVortex *)CBaseEntity::Create( "teleport_vortex", me->GetAbsOrigin(), vec3_angle );
				if ( vortex )
				{
					vortex->SetupVortex( false );
				}

				DispatchParticleEffect( "eyeboss_tp_normal", me->GetAbsOrigin(), me->GetAbsAngles() );

				me->EmitSound( "Halloween.EyeballBossTeleport" );

				me->AddEffects( EF_NOINTERP | EF_NODRAW );

				me->SetAbsOrigin( me->PickNewSpawnSpot() + Vector( 0, 0, 75.0f ) );

				// wait on the other side for a moment
				m_state = TELEPORTING_IN;
			}
			break;

		case TELEPORTING_IN:
			{
				me->RemoveEffects( EF_NOINTERP | EF_NODRAW );

				DispatchParticleEffect( "eyeboss_tp_normal", me->GetAbsOrigin(), me->GetAbsAngles() );

				int animSequence = me->LookupSequence( "teleport_in" );
				if ( animSequence )
				{
					me->SetSequence( animSequence );
					me->SetPlaybackRate( 1.0f );
					me->SetCycle( 0 );
					me->ResetSequenceInfo();
				}

				m_state = DONE;
			}
			break;

		case DONE:
			return Done();
		}
	}

	return Continue();
}


//---------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossEscape::OnStart( CEyeballBoss *me, Action< CEyeballBoss > *priorAction )
{
	int animSequence = me->LookupSequence( "escape" );
	if ( animSequence )
	{
		me->SetSequence( animSequence );
		me->SetPlaybackRate( 1.0f );
		me->SetCycle( 0 );
		me->ResetSequenceInfo();
	}

	me->EmitSound( "Halloween.EyeballBossLaugh" );

	UTIL_LogPrintf( "HALLOWEEN: eyeball_escaped (max_dps %3.2f) (health %d) (level %d)\n", me->GetMaxInjuryRate(), me->GetHealth(), me->GetLevel() );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CEyeballBoss > CEyeballBossEscape::Update( CEyeballBoss *me, float interval )
{
	if ( me->IsSequenceFinished() )
	{
		if ( me->IsSpell() )
		{
			me->EmitSound( "Halloween.spell_spawn_boss_disappear" );
		}

		DispatchParticleEffect( "eyeboss_tp_escape", me->GetAbsOrigin(), me->GetAbsAngles() );

		if ( g_pMonsterResource )
		{
			g_pMonsterResource->HideBossHealthMeter();
		}

		UTIL_Remove( me );

		me->SetVictim( NULL );

		if ( !me->IsSpell() )
		{
			IGameEvent *event = gameeventmanager->CreateEvent( "eyeball_boss_escaped" );
			if ( event )
			{
				event->SetInt( "level", me->GetLevel() );
				gameeventmanager->FireEvent( event );
			}
		}

		// reset back to normal level
		me->ResetLevel();

		return Done();
	}

	return Continue();
}

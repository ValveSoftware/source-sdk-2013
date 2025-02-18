//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "particle_parse.h"
#include "tf_gamerules.h"

#include "../merasmus.h"
#include "merasmus_dying.h"
#include "tf/halloween/eyeball_boss/teleport_vortex.h"

//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusDying::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_DIESIMPLE );
	me->PlayHighPrioritySound( "Halloween.MerasmusBanish" );
	TFGameRules()->BroadcastSound( 255, "Halloween.Merasmus_Death" );

	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusDying::Update( CMerasmus *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		me->Break();
		DispatchParticleEffect( "merasmus_spawn", me->GetAbsOrigin(), me->GetAbsAngles() );

		IGameEvent *event = gameeventmanager->CreateEvent( "merasmus_killed" );
		if ( event )
		{
			event->SetInt( "level", me->GetLevel() );
			gameeventmanager->FireEvent( event );
		}
		me->TriggerLogicRelay( "boss_dead_relay" );

		// create vortex to loot
		CTeleportVortex *vortex = (CTeleportVortex *)CBaseEntity::Create( "teleport_vortex", me->WorldSpaceCenter(), vec3_angle );
		if ( vortex )
		{
			vortex->SetupVortex( true, true );
		}

		me->GainLevel();

		me->StartRespawnTimer();

		UTIL_Remove( me );

		return Done();
	}

	return Continue();
}


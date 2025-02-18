//========= Copyright Valve Corporation, All rights reserved. ============//
// headless_hatman_dying.cpp
// The HHH in the process of dying
// Michael Booth, October 2010

#include "cbase.h"

#include "particle_parse.h"

#include "../headless_hatman.h"
#include "headless_hatman_dying.h"


//---------------------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CHeadlessHatmanDying::OnStart( CHeadlessHatman *me, Action< CHeadlessHatman > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_DIESIMPLE );
	me->EmitSound( "Halloween.HeadlessBossDying" );
	return Continue();
}


//---------------------------------------------------------------------------------------------
ActionResult< CHeadlessHatman >	CHeadlessHatmanDying::Update( CHeadlessHatman *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		me->Break();
		DispatchParticleEffect( "halloween_boss_death", me->GetAbsOrigin(), me->GetAbsAngles() );

		UTIL_Remove( me );

		IGameEvent *event = gameeventmanager->CreateEvent( "pumpkin_lord_killed" );
		if ( event )
		{
			gameeventmanager->FireEvent( event );
		}

		return Done();
	}

	return Continue();
}


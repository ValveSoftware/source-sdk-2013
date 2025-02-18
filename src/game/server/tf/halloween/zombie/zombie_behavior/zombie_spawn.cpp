//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"

#include "tf_shareddefs.h"
#include "../zombie.h"
#include "zombie_attack.h"
#include "zombie_spawn.h"

ActionResult< CZombie >	CZombieSpawn::OnStart( CZombie *me, Action< CZombie > *priorAction )
{
	me->GetBodyInterface()->StartActivity( ACT_TRANSITION );

	return Continue();
}


ActionResult< CZombie >	CZombieSpawn::Update( CZombie *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CZombieAttack, "Start Attack!" );
	}

	return Continue();
}
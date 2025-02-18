//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//
//=============================================================================
#include "cbase.h"
#include "tf_player.h"
#include "tf_gamerules.h"
#include "tf_team.h"
#include "nav_mesh/tf_nav_area.h"
#include "particle_parse.h"

#include "../merasmus.h"
#include "merasmus_reveal.h"
#include "merasmus_attack.h"


//---------------------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusReveal::OnStart( CMerasmus *me, Action< CMerasmus > *priorAction )
{
	me->OnRevealed(false);

	me->GetBodyInterface()->StartActivity( ACT_SHIELD_UP );

	return Continue();
}


//----------------------------------------------------------------------------------
ActionResult< CMerasmus >	CMerasmusReveal::Update( CMerasmus *me, float interval )
{
	if ( me->IsActivityFinished() )
	{
		return ChangeTo( new CMerasmusAttack, "Here I come!" );
	}

	return Continue();
}

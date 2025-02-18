//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: TF Gas Grenade.
//
//=============================================================================//
#include "cbase.h"
#include "tf_weaponbase.h"
#include "tf_gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_grenade_smoke_bomb.h"

// Server specific.
#ifdef GAME_DLL
#include "tf_player.h"
#include "items.h"
#include "tf_weaponbase_grenadeproj.h"
#include "soundent.h"
#include "KeyValues.h"
#include "particle_parse.h"
#include "te_effect_dispatch.h"
#endif

//=============================================================================
//
// TF Smoke Bomb tables.
//

IMPLEMENT_NETWORKCLASS_ALIASED( TFGrenadeSmokeBomb, DT_TFGrenadeSmokeBomb )

BEGIN_NETWORK_TABLE( CTFGrenadeSmokeBomb, DT_TFGrenadeSmokeBomb )
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CTFGrenadeSmokeBomb )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( tf_weapon_grenade_smoke_bomb, CTFGrenadeSmokeBomb );
PRECACHE_WEAPON_REGISTER( tf_weapon_grenade_smoke_bomb );

//=============================================================================
//
// TF Smoke Bomb functions.
//

// Server specific.
#ifdef GAME_DLL

BEGIN_DATADESC( CTFGrenadeSmokeBomb )
END_DATADESC()

extern ConVar tf_smoke_bomb_time;

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CTFWeaponBaseGrenadeProj *CTFGrenadeSmokeBomb::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, 
													 AngularImpulse angImpulse, CBasePlayer *pPlayer, float flTime, int iflags )
{
#if 0
	CTFPlayer *pTFPlayer = ToTFPlayer( pPlayer );

	if ( pTFPlayer )
	{
		CDisablePredictionFiltering disabler;

		// Explosion effect on client
		CEffectData explosionData;
		explosionData.m_vOrigin = pPlayer->GetAbsOrigin();
		explosionData.m_vAngles = pPlayer->GetAbsAngles();
		explosionData.m_fFlags = GetWeaponID();
//		DispatchEffect( "TF_Explosion", explosionData );

		// give them the smoke bomb condition
		// ( invis for X seconds, able to move full speed )
		// ( attacking removes the condition )

		if ( pTFPlayer->CanGoInvisible() )
		{
			pTFPlayer->m_Shared.AddCond( TF_COND_SMOKE_BOMB, tf_smoke_bomb_time.GetFloat() );
		}
	}
#endif

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Don't explode automatically
//-----------------------------------------------------------------------------
bool CTFGrenadeSmokeBomb::ShouldDetonate( void )
{
	return false;
}

#endif // GAME_DLL

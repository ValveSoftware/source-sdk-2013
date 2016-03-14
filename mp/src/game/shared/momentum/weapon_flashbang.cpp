//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "weapon_csbase.h"
#include "gamerules.h"
#include "npcevent.h"
#include "engine/IEngineSound.h"
#include "weapon_flashbang.h"


#ifndef CLIENT_DLL

	//#include "cs_player.h"
	#include "items.h"
	#include "momentum/flashbang_projectile.h"

#endif


#define GRENADE_TIMER	3.0f //Seconds



IMPLEMENT_NETWORKCLASS_ALIASED( Flashbang, DT_Flashbang )

BEGIN_NETWORK_TABLE(CFlashbang, DT_Flashbang)
END_NETWORK_TABLE()

BEGIN_PREDICTION_DATA( CFlashbang )
END_PREDICTION_DATA()

LINK_ENTITY_TO_CLASS( weapon_flashbang, CFlashbang );
PRECACHE_WEAPON_REGISTER( weapon_flashbang );


#ifndef CLIENT_DLL

	BEGIN_DATADESC( CFlashbang )
	END_DATADESC()

	void CFlashbang::EmitGrenade( Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBasePlayer *pPlayer )
	{
		CFlashbangProjectile::Create( 
			vecSrc,
			vecAngles,
			vecVel,
			angImpulse,
			pPlayer );
	}

#endif



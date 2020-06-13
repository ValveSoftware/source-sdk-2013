//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: VScript functions for Half-Life 2.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"

#include "hl2_gamerules.h"
#ifndef CLIENT_DLL
#include "eventqueue.h"
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


#ifndef CLIENT_DLL
extern CBaseEntity *CreatePlayerLoadSave( Vector vOrigin, float flDuration, float flHoldTime, float flLoadTime );

HSCRIPT ScriptGameOver( const char *pszMessage, float flDelay, float flFadeTime, float flLoadTime, int r, int g, int b )
{
	CBaseEntity *pPlayer = AI_GetSinglePlayer();
	if (pPlayer)
	{
		UTIL_ShowMessage( pszMessage, ToBasePlayer( pPlayer ) );
		ToBasePlayer( pPlayer )->NotifySinglePlayerGameEnding();
	}
	else
	{
		// TODO: How should MP handle this?
		return NULL;
	}

	CBaseEntity *pReload = CreatePlayerLoadSave( vec3_origin, flFadeTime, flLoadTime + 1.0f, flLoadTime );
	if (pReload)
	{
		pReload->SetRenderColor( r, g, b, 255 );
		g_EventQueue.AddEvent( pReload, "Reload", flDelay, pReload, pReload );
	}

	return ToHScript( pReload );
}

bool ScriptMegaPhyscannonActive()
{
	return HL2GameRules()->MegaPhyscannonActive();
}
#endif

//-----------------------------------------------------------------------------
// Purpose: Returns how much damage the given ammo type should do to the victim
//			when fired by the attacker.
// Input  : pAttacker - Dude what shot the gun.
//			pVictim - Dude what done got shot.
//			nAmmoType - What been shot out.
// Output : How much hurt to put on dude what done got shot (pVictim).
//-----------------------------------------------------------------------------
void CHalfLife2::RegisterScriptFunctions( void )
{
	BaseClass::RegisterScriptFunctions();

#ifndef CLIENT_DLL
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptGameOver, "GameOver", "Ends the game and reloads the last save." );
	ScriptRegisterFunctionNamed( g_pScriptVM, ScriptMegaPhyscannonActive, "MegaPhyscannonActive", "Checks if supercharged gravity gun mode is enabled." );
#endif
}

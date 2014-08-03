//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#include "cbase.h"
#include "singleplay_gamerules.h"

#ifdef CLIENT_DLL

#else

	#include "player.h"
	#include "basecombatweapon.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"

#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//=========================================================
//=========================================================
bool CSingleplayRules::IsMultiplayer( void )
{
	return false;
}

// Needed during the conversion, but once DMG_* types have been fixed, this isn't used anymore.
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSingleplayRules::Damage_GetTimeBased( void )
{
	int iDamage = ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSingleplayRules::Damage_GetShouldGibCorpse( void )
{
	int iDamage = ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CSingleplayRules::Damage_GetShowOnHud( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSingleplayRules::Damage_GetNoPhysicsForce( void )
{
	int iTimeBasedDamage = Damage_GetTimeBased();
	int iDamage = ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE );
	return iDamage;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int	CSingleplayRules::Damage_GetShouldNotBleed( void )
{
	int iDamage = ( DMG_POISON | DMG_ACID );
	return iDamage;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSingleplayRules::Damage_IsTimeBased( int iDmgType )
{
	// Damage types that are time-based.
	return ( ( iDmgType & ( DMG_PARALYZE | DMG_NERVEGAS | DMG_POISON | DMG_RADIATION | DMG_DROWNRECOVER | DMG_ACID | DMG_SLOWBURN ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSingleplayRules::Damage_ShouldGibCorpse( int iDmgType )
{
	// Damage types that gib the corpse.
	return ( ( iDmgType & ( DMG_CRUSH | DMG_FALL | DMG_BLAST | DMG_SONIC | DMG_CLUB ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSingleplayRules::Damage_ShowOnHUD( int iDmgType )
{
	// Damage types that have client HUD art.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID | DMG_DROWN | DMG_BURN | DMG_SLOWBURN | DMG_NERVEGAS | DMG_RADIATION | DMG_SHOCK ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSingleplayRules::Damage_NoPhysicsForce( int iDmgType )
{
	// Damage types that don't have to supply a physics force & position.
	int iTimeBasedDamage = Damage_GetTimeBased();
	return ( ( iDmgType & ( DMG_FALL | DMG_BURN | DMG_PLASMA | DMG_DROWN | iTimeBasedDamage | DMG_CRUSH | DMG_PHYSGUN | DMG_PREVENT_PHYSICS_FORCE ) ) != 0 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : iDmgType - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CSingleplayRules::Damage_ShouldNotBleed( int iDmgType )
{
	// Damage types that don't make the player bleed.
	return ( ( iDmgType & ( DMG_POISON | DMG_ACID ) ) != 0 );
}

#ifdef CLIENT_DLL

#else

	extern CGameRules	*g_pGameRules;
	extern bool		g_fGameOver;

	//=========================================================
	//=========================================================
	CSingleplayRules::CSingleplayRules( void )
	{
		RefreshSkillData( true );
	}

	//=========================================================
	//=========================================================
	void CSingleplayRules::Think ( void )
	{
		BaseClass::Think();
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::IsDeathmatch ( void )
	{
		return false;
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::IsCoOp( void )
	{
		return false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Determine whether the player should switch to the weapon passed in
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CSingleplayRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{
		//Must have ammo
		if ( ( pWeapon->HasAnyAmmo() == false ) && ( pPlayer->GetAmmoCount( pWeapon->m_iPrimaryAmmoType ) <= 0 ) )
			return false;

		//Always take a loaded gun if we have nothing else
		if ( pPlayer->GetActiveWeapon() == NULL )
			return true;

		// The given weapon must allow autoswitching to it from another weapon.
		if ( !pWeapon->AllowsAutoSwitchTo() )
			return false;

		// The active weapon must allow autoswitching from it.
		if ( !pPlayer->GetActiveWeapon()->AllowsAutoSwitchFrom() )
			return false;

		//Don't switch if our current gun doesn't want to be holstered
		if ( pPlayer->GetActiveWeapon()->CanHolster() == false )
			return false;

		//Only switch if the weapon is better than what we're using
		if ( ( pWeapon != pPlayer->GetActiveWeapon() ) && ( pWeapon->GetWeight() <= pPlayer->GetActiveWeapon()->GetWeight() ) )
			return false;

		return true;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Find the next best weapon to use and return it.
	//-----------------------------------------------------------------------------
	CBaseCombatWeapon *CSingleplayRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
	{
		if ( pCurrentWeapon && !pCurrentWeapon->AllowsAutoSwitchFrom() )
			return NULL;

		CBaseCombatWeapon	*pBestWeapon = NULL;
		CBaseCombatWeapon	*pWeapon;
		
		int	nBestWeight	= -1;

		//Search for the best weapon to use next based on its weight
		for ( int i = 0; i < pPlayer->WeaponCount(); i++ )
		{
			pWeapon = pPlayer->GetWeapon(i);

			if ( pWeapon == NULL )
				continue;

			// If we have an active weapon and this weapon doesn't allow autoswitching away
			// from another weapon, skip it.
			if ( pCurrentWeapon && !pWeapon->AllowsAutoSwitchTo() )
				continue;

			// Must be eligible for switching to.
			if (!pPlayer->Weapon_CanSwitchTo(pWeapon))
				continue;
			
			// Must be of higher quality.
			if ( pWeapon->GetWeight() <= nBestWeight )
				continue;

			// We must have primary ammo
			if ( pWeapon->UsesClipsForAmmo1() && pWeapon->Clip1() <= 0 && !pPlayer->GetAmmoCount( pWeapon->GetPrimaryAmmoType() ) )
				continue;

			// This is a better candidate than what we had.
			nBestWeight = pWeapon->GetWeight();
			pBestWeapon = pWeapon;
		}

		return pBestWeapon;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CSingleplayRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
	{
		CBaseCombatWeapon *pWeapon = GetNextBestWeapon( pPlayer, pCurrentWeapon );

		if ( pWeapon != NULL )
			return pPlayer->Weapon_Switch( pWeapon );

		return false;
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
	{
		return true;
	}

	void CSingleplayRules::InitHUD( CBasePlayer *pl )
	{
	}

	//=========================================================
	//=========================================================
	void CSingleplayRules::ClientDisconnected( edict_t *pClient )
	{
	}

	//=========================================================
	//=========================================================
	float CSingleplayRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
	{
		// subtract off the speed at which a player is allowed to fall without being hurt,
		// so damage will be based on speed beyond that, not the entire fall
		pPlayer->m_Local.m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
		return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	void CSingleplayRules::PlayerSpawn( CBasePlayer *pPlayer )
	{
		// Player no longer gets all weapons to start.
		// He has to pick them up now.  Use impulse 101
		// to give him all weapons
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::AllowAutoTargetCrosshair( void )
	{
		return ( IsSkillLevel(SKILL_EASY) );
	}

	//=========================================================
	//=========================================================
	int	CSingleplayRules::GetAutoAimMode()
	{
		return sk_autoaim_mode.GetInt();
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	float CSingleplayRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
	{
		return gpGlobals->curtime;//now!
	}

	//=========================================================
	// IPointsForKill - how many points awarded to anyone
	// that kills this player?
	//=========================================================
	int CSingleplayRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
	{
		return 1;
	}

	//=========================================================
	// PlayerKilled - someone/something killed this player
	//=========================================================
	void CSingleplayRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
	{
	}

	//=========================================================
	// Deathnotice
	//=========================================================
	void CSingleplayRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
	{
	}

	//=========================================================
	// FlWeaponRespawnTime - what is the time in the future
	// at which this weapon may spawn?
	//=========================================================
	float CSingleplayRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
	{
		return -1;
	}

	//=========================================================
	// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
	// now,  otherwise it returns the time at which it can try
	// to spawn again.
	//=========================================================
	float CSingleplayRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
	{
		return 0;
	}

	//=========================================================
	// VecWeaponRespawnSpot - where should this weapon spawn?
	// Some game variations may choose to randomize spawn locations
	//=========================================================
	Vector CSingleplayRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
	{
		return pWeapon->GetAbsOrigin();
	}

	//=========================================================
	// WeaponShouldRespawn - any conditions inhibiting the
	// respawning of this weapon?
	//=========================================================
	int CSingleplayRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
	{
		return GR_WEAPON_RESPAWN_NO;
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	void CSingleplayRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
	{
	}

	//=========================================================
	//=========================================================
	int CSingleplayRules::ItemShouldRespawn( CItem *pItem )
	{
		return GR_ITEM_RESPAWN_NO;
	}


	//=========================================================
	// At what time in the future may this Item respawn?
	//=========================================================
	float CSingleplayRules::FlItemRespawnTime( CItem *pItem )
	{
		return -1;
	}

	//=========================================================
	// Where should this item respawn?
	// Some game variations may choose to randomize spawn locations
	//=========================================================
	Vector CSingleplayRules::VecItemRespawnSpot( CItem *pItem )
	{
		return pItem->GetAbsOrigin();
	}

	//=========================================================
	// What angles should this item use to respawn?
	//=========================================================
	QAngle CSingleplayRules::VecItemRespawnAngles( CItem *pItem )
	{
		return pItem->GetAbsAngles();
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::IsAllowedToSpawn( CBaseEntity *pEntity )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	void CSingleplayRules::PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount )
	{
	}

	//=========================================================
	//=========================================================
	float CSingleplayRules::FlHealthChargerRechargeTime( void )
	{
		return 0;// don't recharge
	}

	//=========================================================
	//=========================================================
	int CSingleplayRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
	{
		return GR_PLR_DROP_GUN_NO;
	}

	//=========================================================
	//=========================================================
	int CSingleplayRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
	{
		return GR_PLR_DROP_AMMO_NO;
	}

	//=========================================================
	//=========================================================
	int CSingleplayRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
	{
		// why would a single player in half life need this? 
		return GR_NOTTEAMMATE;
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
	{
		return ( PlayerRelationship( pListener, pSpeaker ) == GR_TEAMMATE );
	}

	//=========================================================
	//=========================================================
	bool CSingleplayRules::FAllowNPCs( void )
	{
		return true;
	}

#endif


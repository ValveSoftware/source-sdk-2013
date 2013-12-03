//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef SINGLEPLAY_GAMERULES_H
#define SINGLEPLAY_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif


#include "gamerules.h"


#ifdef CLIENT_DLL

	#define CSingleplayRules C_SingleplayRules

#endif


//=========================================================
// CSingleplayRules - rules for the single player Half-Life 
// game.
//=========================================================
class CSingleplayRules : public CGameRules
{
public:

	DECLARE_CLASS( CSingleplayRules, CGameRules );


// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void );

	// Damage query implementations.
	virtual bool	Damage_IsTimeBased( int iDmgType );			// Damage types that are time-based.
	virtual bool	Damage_ShouldGibCorpse( int iDmgType );		// Damage types that gib the corpse.
	virtual bool	Damage_ShowOnHUD( int iDmgType );				// Damage types that have client HUD art.
	virtual bool	Damage_NoPhysicsForce( int iDmgType );		// Damage types that don't have to supply a physics force & position.
	virtual bool	Damage_ShouldNotBleed( int iDmgType );			// Damage types that don't make the player bleed.
	// TEMP: These will go away once DamageTypes become enums.
	virtual int		Damage_GetTimeBased( void );		
	virtual int		Damage_GetShouldGibCorpse( void );
	virtual int		Damage_GetShowOnHud( void );
	virtual int		Damage_GetNoPhysicsForce( void );
	virtual int		Damage_GetShouldNotBleed( void );

#ifdef CLIENT_DLL

#else

	CSingleplayRules();
	virtual ~CSingleplayRules() {}

// GR_Think
	virtual void Think( void );
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual bool FAllowFlashlight( void ) { return TRUE; };

	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );
	virtual CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );
	bool SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );
	
// Functions to verify the single/multiplayer status of a game
	virtual bool IsDeathmatch( void );
	virtual bool IsCoOp( void );

// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );
	
// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );

	virtual bool AllowAutoTargetCrosshair( void );
	virtual	int	GetAutoAimMode();

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon );
	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon );
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon );
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon );

// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem );
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem );

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem );
	virtual float FlItemRespawnTime( CItem *pItem );
	virtual Vector VecItemRespawnSpot( CItem *pItem );
	virtual QAngle VecItemRespawnAngles( CItem *pItem );

// Ammo retrieval
	virtual void PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount );

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void );

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

// NPCs
	virtual bool FAllowNPCs( void );

// Teamplay stuff	
	virtual const char *GetTeamID( CBaseEntity *pEntity ) {return "";};
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker );
#endif
};


#endif // SINGLEPLAY_GAMERULES_H

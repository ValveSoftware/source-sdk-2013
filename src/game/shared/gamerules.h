//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef GAMERULES_H
#define GAMERULES_H
#ifdef _WIN32
#pragma once
#endif

// Debug history should be disabled in release builds
//#define DISABLE_DEBUG_HISTORY	

#ifdef CLIENT_DLL

	#include "c_baseentity.h"
	
	#define CGameRules C_GameRules
	#define CGameRulesProxy C_GameRulesProxy

#else
	
	#include "baseentity.h"
	#include "recipientfilter.h"

#endif

#include "igamesystem.h"
#include "gamerules_register.h"


//#include "items.h"
class CBaseCombatWeapon;
class CBaseCombatCharacter;
class CBasePlayer;
class CItem;
class CAmmoDef;
class CTacticalMissionManager;

extern ConVar sk_autoaim_mode;

// Autoaiming modes
enum
{
	AUTOAIM_NONE = 0,		// No autoaim at all.
	AUTOAIM_ON,				// Autoaim is on.
	AUTOAIM_ON_CONSOLE,		// Autoaim is on, including enhanced features for Console gaming (more assistance, etc)
};

// weapon respawning return codes
enum
{	
	GR_NONE = 0,
	
	GR_WEAPON_RESPAWN_YES,
	GR_WEAPON_RESPAWN_NO,
	
	GR_AMMO_RESPAWN_YES,
	GR_AMMO_RESPAWN_NO,
	
	GR_ITEM_RESPAWN_YES,
	GR_ITEM_RESPAWN_NO,

	GR_PLR_DROP_GUN_ALL,
	GR_PLR_DROP_GUN_ACTIVE,
	GR_PLR_DROP_GUN_NO,

	GR_PLR_DROP_AMMO_ALL,
	GR_PLR_DROP_AMMO_ACTIVE,
	GR_PLR_DROP_AMMO_NO,
};

// Player relationship return codes
enum
{
	GR_NOTTEAMMATE = 0,
	GR_TEAMMATE,
	GR_ENEMY,
	GR_ALLY,
	GR_NEUTRAL,
};


// This class has the data tables and gets the CGameRules data to the client.
class CGameRulesProxy : public CBaseEntity
{
public:
	DECLARE_CLASS( CGameRulesProxy, CBaseEntity );
	DECLARE_NETWORKCLASS();

	CGameRulesProxy();
	~CGameRulesProxy();

	// UNDONE: Is this correct, Mike?
	// Don't carry these across a transition, they are recreated.
	virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }

	// ALWAYS transmit to all clients.
	virtual int UpdateTransmitState( void );

	// CGameRules chains its NetworkStateChanged calls to here, since this
	// is the actual entity that will send the data.
	static void NotifyNetworkStateChanged();

private:
	
	static CGameRulesProxy *s_pGameRulesProxy;
};


abstract_class CGameRules : public CAutoGameSystemPerFrame
{
public:
	DECLARE_CLASS_GAMEROOT( CGameRules, CAutoGameSystemPerFrame );

	virtual char const *Name() { return "CGameRules"; }

	// Stuff shared between client and server.

	CGameRules(void);
	virtual ~CGameRules( void );

	virtual void	LevelShutdownPostEntity() OVERRIDE;

	// Damage Queries - these need to be implemented by the various subclasses (single-player, multi-player, etc).
	// The queries represent queries against damage types and properties.
	virtual bool	Damage_IsTimeBased( int iDmgType ) = 0;			// Damage types that are time-based.
	virtual bool	Damage_ShouldGibCorpse( int iDmgType ) = 0;		// Damage types that gib the corpse.
	virtual bool	Damage_ShowOnHUD( int iDmgType ) = 0;			// Damage types that have client HUD art.
	virtual bool	Damage_NoPhysicsForce( int iDmgType ) = 0;		// Damage types that don't have to supply a physics force & position.
	virtual bool	Damage_ShouldNotBleed( int iDmgType ) = 0;		// Damage types that don't make the player bleed.
	//Temp: These will go away once DamageTypes become enums.
	virtual int		Damage_GetTimeBased( void ) = 0;				// Actual bit-fields.
	virtual int		Damage_GetShouldGibCorpse( void ) = 0;
	virtual int		Damage_GetShowOnHud( void ) = 0;					
	virtual int		Damage_GetNoPhysicsForce( void )= 0;
	virtual int		Damage_GetShouldNotBleed( void ) = 0;

// Ammo Definitions
	//CAmmoDef* GetAmmoDef();

	virtual bool SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon ); // Switch to the next best weapon
	virtual CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon ); // I can't use this weapon anymore, get me the next best one.
	virtual bool ShouldCollide( int collisionGroup0, int collisionGroup1 );

	virtual int DefaultFOV( void ) { return 90; }

	// This function is here for our CNetworkVars.
	inline void NetworkStateChanged()
	{
		// Forward the call to the entity that will send the data.
		CGameRulesProxy::NotifyNetworkStateChanged();
	}

	inline void NetworkStateChanged( void *pVar )
	{
		// Forward the call to the entity that will send the data.
		CGameRulesProxy::NotifyNetworkStateChanged();
	}

	// Get the view vectors for this mod.
	virtual const CViewVectors* GetViewVectors() const;

// Damage rules for ammo types
	virtual float GetAmmoDamage( CBaseEntity *pAttacker, CBaseEntity *pVictim, int nAmmoType );
    virtual float GetDamageMultiplier( void ) { return 1.0f; }    

// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void ) = 0;// is this a multiplayer game? (either coop or deathmatch)

	virtual const unsigned char *GetEncryptionKey() { return NULL; }

	virtual bool InRoundRestart( void ) { return false; }

	//Allow thirdperson camera.
	virtual bool AllowThirdPersonCamera( void ) { return false; }

	virtual void ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues ) {} 

	// IsConnectedUserInfoChangeAllowed allows the clients to change
	// cvars with the FCVAR_NOT_CONNECTED rule if it returns true
	virtual bool IsConnectedUserInfoChangeAllowed( CBasePlayer *pPlayer )
	{ 
		Assert( !IsMultiplayer() );
		return true; 
	}

#ifdef CLIENT_DLL

	virtual bool IsBonusChallengeTimeBased( void );

	virtual bool AllowMapParticleEffect( const char *pszParticleEffect ) { return true; }

	virtual bool AllowWeatherParticles( void ) { return true; }

	virtual bool AllowMapVisionFilterShaders( void ) { return false; }
	virtual const char* TranslateEffectForVisionFilter( const char *pchEffectType, const char *pchEffectName ) { return pchEffectName; }

	virtual bool IsLocalPlayer( int nEntIndex );

	virtual void ModifySentChat( char *pBuf, int iBufSize ) { return; }

	virtual bool ShouldConfirmOnDisconnect() { return false; }
	
#else

	virtual void Status( void (*print) (const char *fmt, ...) ) {}

	virtual void GetTaggedConVarList( KeyValues *pCvarTagList ) {}

	// NVNT see if the client of the player entered is using a haptic device.
	virtual void CheckHaptics(CBasePlayer* pPlayer);

// CBaseEntity overrides.
public:

// Setup
	
	// Called when game rules are destroyed by CWorld
	virtual void LevelShutdown( void ) { return; };

	virtual void Precache( void ) { return; };

	virtual void RefreshSkillData( bool forceUpdate );// fill skill data struct with proper values
	
	// Called each frame. This just forwards the call to Think().
	virtual void FrameUpdatePostEntityThink();

	virtual void Think( void ) = 0;// GR_Think - runs every server frame, should handle any timer tasks, periodic events, etc.
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity ) = 0;  // Can this item spawn (eg NPCs don't spawn in deathmatch).

	// Called at the end of GameFrame (i.e. after all game logic has run this frame)
	virtual void EndGameFrame( void );

	virtual bool IsSkillLevel( int iLevel ) { return GetSkillLevel() == iLevel; }
	virtual int	GetSkillLevel() { return g_iSkillLevel; }
	virtual void OnSkillLevelChanged( int iNewLevel ) {};
	virtual void SetSkillLevel( int iLevel )
	{
		int oldLevel = g_iSkillLevel; 

		if ( iLevel < 1 )
		{
			iLevel = 1;
		}
		else if ( iLevel > 3 )
		{
			iLevel = 3; 
		}

		g_iSkillLevel = iLevel;

		if( g_iSkillLevel != oldLevel )
		{
			OnSkillLevelChanged( g_iSkillLevel );
		}
	}

	virtual bool FAllowFlashlight( void ) = 0;// Are players allowed to switch on their flashlight?
	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon ) = 0;// should the player switch to this weapon?

// Functions to verify the single/multiplayer status of a game
	virtual bool IsDeathmatch( void ) = 0;//is this a deathmatch game?
	virtual bool IsTeamplay( void ) { return FALSE; };// is this deathmatch game being played with team rules?
	virtual bool IsCoOp( void ) = 0;// is this a coop game?
	virtual const char *GetGameDescription( void ) { return "Half-Life 2"; }  // this is the game name that gets seen in the server browser
	
// Client connection/disconnection
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen ) = 0;// a client just connected to the server (player hasn't spawned yet)
	virtual void InitHUD( CBasePlayer *pl ) = 0;		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient ) = 0;// a client just disconnected from the server
	
// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer ) = 0;// this client just hit the ground after a fall. How much damage?
	virtual bool  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info ) {return TRUE;};// can this player take damage from this attacker?
	virtual bool ShouldAutoAim( CBasePlayer *pPlayer, edict_t *target ) { return TRUE; }
	virtual float GetAutoAimScale( CBasePlayer *pPlayer ) { return 1.0f; }
	virtual int	GetAutoAimMode()	{ return AUTOAIM_ON; }

	virtual bool ShouldUseRobustRadiusDamage(CBaseEntity *pEntity) { return false; }
	virtual void  RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );
	// Let the game rules specify if fall death should fade screen to black
	virtual bool  FlPlayerFallDeathDoesScreenFade( CBasePlayer *pl ) { return TRUE; }

	virtual bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info ) = 0;


// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer ) = 0;// called by CBasePlayer::Spawn just before releasing player into the game
	virtual void PlayerThink( CBasePlayer *pPlayer ) = 0; // called by CBasePlayer::PreThink every frame, before physics are run and after keys are accepted
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer ) = 0;// is this player allowed to respawn now?
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer ) = 0;// When in the future will this player be able to spawn?
	virtual CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );// Place this player on their spawnspot and face them the proper direction.
	virtual bool IsSpawnPointValid( CBaseEntity *pSpot, CBasePlayer *pPlayer );

	virtual bool AllowAutoTargetCrosshair( void ) { return TRUE; };
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );  // handles the user commands;  returns TRUE if command handled properly
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );		 // the player has changed cvars

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled ) = 0;// how many points do I award whoever kills this player?
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info ) = 0;// Called each time a player dies
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )=  0;// Call this from within a GameRules class to report an obituary.
	virtual const char *GetDamageCustomString( const CTakeDamageInfo &info ) { return NULL; }

// Weapon Damage
	// Determines how much damage Player's attacks inflict, based on skill level.
	virtual float AdjustPlayerDamageInflicted( float damage ) { return damage; }
	virtual void  AdjustPlayerDamageTaken( CTakeDamageInfo *pInfo ) {}; // Base class does nothing.

// Weapon retrieval
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );// The player is touching an CBaseCombatWeapon, do I give it to him?

// Weapon spawn/respawn control
	virtual int WeaponShouldRespawn( CBaseCombatWeapon *pWeapon ) = 0;// should this weapon respawn?
	virtual float FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon ) = 0;// when may this weapon respawn?
	virtual float FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon ) = 0; // can i respawn now,  and if not, when should i try again?
	virtual Vector VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon ) = 0;// where in the world should this weapon respawn?

// Item retrieval
	virtual bool CanHaveItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// is this player allowed to take this item?
	virtual void PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem ) = 0;// call each time a player picks up an item (battery, healthkit)

// Item spawn/respawn control
	virtual int ItemShouldRespawn( CItem *pItem ) = 0;// Should this item respawn?
	virtual float FlItemRespawnTime( CItem *pItem ) = 0;// when may this item respawn?
	virtual Vector VecItemRespawnSpot( CItem *pItem ) = 0;// where in the world should this item respawn?
	virtual QAngle VecItemRespawnAngles( CItem *pItem ) = 0;// what angles should this item use when respawing?

// Ammo retrieval
	virtual bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, int iAmmoIndex ); // can this player take more of this ammo?
	virtual bool CanHaveAmmo( CBaseCombatCharacter *pPlayer, const char *szName );
	virtual void PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount ) = 0;// called each time a player picks up some ammo in the world
	virtual float GetAmmoQuantityScale( int iAmmoIndex ) { return 1.0f; }

// AI Definitions
	virtual void			InitDefaultAIRelationships( void ) { return; }
	virtual const char*		AIClassText(int classType) { return NULL; }

// Healthcharger respawn control
	virtual float FlHealthChargerRechargeTime( void ) = 0;// how long until a depleted HealthCharger recharges itself?
	virtual float FlHEVChargerRechargeTime( void ) { return 0; }// how long until a depleted HealthCharger recharges itself?

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer ) = 0;// what do I do with a player's weapons when he's killed?

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer ) = 0;// Do I drop ammo when the player dies? How much?

// Teamplay stuff
	virtual const char *GetTeamID( CBaseEntity *pEntity ) = 0;// what team is this entity on?
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget ) = 0;// What is the player's relationship with this entity?
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker ) = 0;
	virtual void CheckChatText( CBasePlayer *pPlayer, char *pText ) { return; }

	virtual int GetTeamIndex( const char *pTeamName ) { return -1; }
	virtual const char *GetIndexedTeamName( int teamIndex ) { return ""; }
	virtual bool IsValidTeam( const char *pTeamName ) { return true; }
	virtual void ChangePlayerTeam( CBasePlayer *pPlayer, const char *pTeamName, bool bKill, bool bGib ) {}
	virtual const char *SetDefaultPlayerTeam( CBasePlayer *pPlayer ) { return ""; }
	virtual void UpdateClientData( CBasePlayer *pPlayer ) { };

// Sounds
	virtual bool PlayTextureSounds( void ) { return TRUE; }
	virtual bool PlayFootstepSounds( CBasePlayer *pl ) { return TRUE; }

// NPCs
	virtual bool FAllowNPCs( void ) = 0;//are NPCs allowed

	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) {}
				    
	// trace line rules
	virtual float WeaponTraceEntity( CBaseEntity *pEntity, const Vector &vecStart, const Vector &vecEnd, unsigned int mask, trace_t *ptr );

	// Setup g_pPlayerResource (some mods use a different entity type here).
	virtual void CreateStandardEntities();

	// Team name, etc shown in chat and dedicated server console
	virtual const char *GetChatPrefix( bool bTeamOnly, CBasePlayer *pPlayer );

	// Location name shown in chat
	virtual const char *GetChatLocation( bool bTeamOnly, CBasePlayer *pPlayer ) { return NULL; }

	// VGUI format string for chat, if desired
	virtual const char *GetChatFormat( bool bTeamOnly, CBasePlayer *pPlayer ) { return NULL; }

	// Whether props that are on fire should get a DLIGHT.
	virtual bool ShouldBurningPropsEmitLight() { return false; }

	virtual bool CanEntityBeUsePushed( CBaseEntity *pEnt ) { return true; }

	virtual void CreateCustomNetworkStringTables( void ) { }

	// Game Achievements (server version)
	virtual void MarkAchievement ( IRecipientFilter& filter, char const *pchAchievementName );

	virtual void ResetMapCycleTimeStamp( void ){ return; }

	virtual void OnNavMeshLoad( void ) { return; }

	// game-specific factories
	virtual CTacticalMissionManager *TacticalMissionManagerFactory( void );

	virtual void ProcessVerboseLogOutput( void ){}

#endif

	virtual const char *GetGameTypeName( void ){ return NULL; }
	virtual int GetGameType( void ){ return 0; }

	virtual bool ShouldDrawHeadLabels(){ return true; }

	virtual void ClientSpawned( edict_t * pPlayer ) { return; }

	virtual void OnFileReceived( const char * fileName, unsigned int transferID ) { return; }

	virtual bool IsHolidayActive( /*EHoliday*/ int eHoliday ) const { return false; }

	virtual bool IsManualMapChangeOkay( const char **pszReason ){ return true; }

	virtual void RegisterScriptFunctions() { }

	virtual void SaveConvar( const ConVarRef & cvar );
	virtual void RevertSavedConvars();
	virtual bool HasSavedConvar( const string_t cvarName );

#ifdef GAME_DLL
	virtual bool IsOfficialMap() { return false; }
#endif

protected:
	CUtlVector< string_t > m_SavedConvars;

#ifndef CLIENT_DLL
private:
	float m_flNextVerboseLogOutput;
#endif // CLIENT_DLL
};


#ifndef CLIENT_DLL
	void InstallGameRules();
	
	// Create user messages for game here, calls into static player class creation functions
	void RegisterUserMessages( void );
#endif


extern ConVar g_Language;


//-----------------------------------------------------------------------------
// Gets us at the game rules
//-----------------------------------------------------------------------------

extern CGameRules *g_pGameRules;

inline CGameRules* GameRules()
{
	return g_pGameRules;
}

#endif // GAMERULES_H

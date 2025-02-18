//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef MULTIPLAY_GAMERULES_H
#define MULTIPLAY_GAMERULES_H
#ifdef _WIN32
#pragma once
#endif


#include "gamerules.h"

#ifdef CLIENT_DLL

	#define CMultiplayRules C_MultiplayRules

#else

extern ConVar mp_restartgame;
extern ConVar mp_restartgame_immediate;
extern ConVar mp_waitingforplayers_time;
extern ConVar mp_waitingforplayers_restart;
extern ConVar mp_waitingforplayers_cancel;
extern ConVar mp_clan_readyrestart;
extern ConVar mp_clan_ready_signal;
extern ConVar nextlevel;
extern INetworkStringTable *g_pStringTableServerMapCycle;

#if defined ( TF_DLL ) || defined ( TF_CLIENT_DLL )
extern INetworkStringTable *g_pStringTableServerPopFiles;
extern INetworkStringTable *g_pStringTableServerMapCycleMvM;
#endif

#define VOICE_COMMAND_MAX_SUBTITLE_DIST	1900

class CBaseMultiplayerPlayer;

#endif

extern ConVar mp_show_voice_icons;

#define MAX_SPEAK_CONCEPT_LEN 64
#define MAX_VOICE_COMMAND_SUBTITLE	256

typedef struct
{
#ifndef CLIENT_DLL
	// concept to speak
	int	 m_iConcept;

	// play subtitle?
	bool m_bShowSubtitle;
	bool m_bDistanceBasedSubtitle;

	char m_szGestureActivity[64];

#else
	// localizable subtitle
	char m_szSubtitle[MAX_VOICE_COMMAND_SUBTITLE];

	// localizable string for menu
	char m_szMenuLabel[MAX_VOICE_COMMAND_SUBTITLE];
#endif

} VoiceCommandMenuItem_t;

extern ConVar mp_timelimit;

//=========================================================
// CMultiplayRules - rules for the basic half life multiplayer
// competition
//=========================================================
class CMultiplayRules : public CGameRules
{
public:
	DECLARE_CLASS( CMultiplayRules, CGameRules );

// Functions to verify the single/multiplayer status of a game
	virtual bool IsMultiplayer( void );

	virtual	bool	Init();

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

	CMultiplayRules();
	virtual ~CMultiplayRules() {}

	void LoadVoiceCommandScript( void );

	virtual bool ShouldDrawHeadLabels()
	{ 
		if ( mp_show_voice_icons.GetBool() == false )
			return false;

		return BaseClass::ShouldDrawHeadLabels();
	}

#ifndef CLIENT_DLL
	virtual void FrameUpdatePostEntityThink();

// GR_Think
	virtual void Think( void );
	virtual void RefreshSkillData( bool forceUpdate );
	virtual bool IsAllowedToSpawn( CBaseEntity *pEntity );
	virtual bool FAllowFlashlight( void );

	virtual bool FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );
	virtual CBaseCombatWeapon *GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );
	virtual bool SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon );

// Functions to verify the single/multiplayer status of a game
	virtual bool IsDeathmatch( void );
	virtual bool IsCoOp( void );

// Client connection/disconnection
	// If ClientConnected returns FALSE, the connection is rejected and the user is provided the reason specified in
	//  svRejectReason
	// Only the client's name and remote address are provided to the dll for verification.
	virtual bool ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen );
	virtual void InitHUD( CBasePlayer *pl );		// the client dll is ready for updating
	virtual void ClientDisconnected( edict_t *pClient );

// Client damage rules
	virtual float FlPlayerFallDamage( CBasePlayer *pPlayer );
	virtual bool  FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info );
	virtual bool AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info );

// Client spawn/respawn control
	virtual void PlayerSpawn( CBasePlayer *pPlayer );
	virtual void PlayerThink( CBasePlayer *pPlayer );
	virtual bool FPlayerCanRespawn( CBasePlayer *pPlayer );
	virtual float FlPlayerSpawnTime( CBasePlayer *pPlayer );
	virtual CBaseEntity *GetPlayerSpawnSpot( CBasePlayer *pPlayer );

	virtual bool AllowAutoTargetCrosshair( void );

// Client kills/scoring
	virtual int IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled );
	virtual void PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	virtual void DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info );
	CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor );									// old version of method - kept for backward compat
	virtual CBasePlayer *GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor, CBaseEntity *pVictim );		// new version of method

// Weapon retrieval
	virtual bool CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon );// The player is touching an CBaseCombatWeapon, do I give it to him?

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
	virtual float FlHEVChargerRechargeTime( void );

// What happens to a dead player's weapons
	virtual int DeadPlayerWeapons( CBasePlayer *pPlayer );

// What happens to a dead player's ammo	
	virtual int DeadPlayerAmmo( CBasePlayer *pPlayer );

// Teamplay stuff	
	virtual const char *GetTeamID( CBaseEntity *pEntity ) {return "";}
	virtual int PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget );
	virtual bool PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker );

	virtual bool PlayTextureSounds( void ) { return FALSE; }
	virtual bool PlayFootstepSounds( CBasePlayer *pl );

// NPCs
	virtual bool FAllowNPCs( void );
	
	// Immediately end a multiplayer game
	virtual void EndMultiplayerGame( void ) { GoToIntermission(); }

// Voice commands
	virtual bool ClientCommand( CBaseEntity *pEdict, const CCommand &args );
	virtual VoiceCommandMenuItem_t *VoiceCommand( CBaseMultiplayerPlayer *pPlayer, int iMenu, int iItem );
	
// Bugbait report	
	bool IsLoadingBugBaitReport( void );

	virtual void ResetMapCycleTimeStamp( void ){ m_nMapCycleTimeStamp = 0; }

	virtual void HandleTimeLimitChange( void ){ return; }

	void IncrementMapCycleIndex();

	void HaveAllPlayersSpeakConceptIfAllowed( int iConcept, int iTeam = TEAM_UNASSIGNED, const char *modifiers = NULL );
	void RandomPlayersSpeakConceptIfAllowed( int iConcept, int iNumRandomPlayer = 1, int iTeam = TEAM_UNASSIGNED, const char *modifiers = NULL );

	virtual void GetTaggedConVarList( KeyValues *pCvarTagList );

	void SkipNextMapInCycle();

	virtual void	ClientCommandKeyValues( edict_t *pEntity, KeyValues *pKeyValues );

public:

	struct ResponseRules_t
	{
		CUtlVector<IResponseSystem*> m_ResponseSystems;
	};
	CUtlVector<ResponseRules_t>	m_ResponseRules;

	virtual void InitCustomResponseRulesDicts()	{}
	virtual void ShutdownCustomResponseRulesDicts() {}

	// NVNT virtual to check for haptic device 
	virtual void ClientSettingsChanged( CBasePlayer *pPlayer );
	virtual void GetNextLevelName( char *szNextMap, int bufsize, bool bRandom = false );

	static void DetermineMapCycleFilename( char *pszResult, int nSizeResult, bool bForceSpew );
	virtual void LoadMapCycleFileIntoVector ( const char *pszMapCycleFile, CUtlVector<char *> &mapList );
	static void FreeMapCycleFileVector ( CUtlVector<char *> &mapList );

	// LoadMapCycleFileIntoVector without the fixups inherited versions of gamerules may provide
	static void RawLoadMapCycleFileIntoVector ( const char *pszMapCycleFile, CUtlVector<char *> &mapList );

	bool IsMapInMapCycle( const char *pszName );

	virtual bool IsManualMapChangeOkay( const char **pszReason ) OVERRIDE;

protected:
	virtual bool UseSuicidePenalty() { return true; }		// apply point penalty for suicide?
 	virtual float GetLastMajorEventTime( void ){ return -1.0f; }

public:
	virtual void ChangeLevel( void );

protected:
	virtual void GoToIntermission( void );
	virtual void LoadMapCycleFile( void );
	void ChangeLevelToMap( const char *pszMap );

	float m_flIntermissionEndTime;
	static int m_nMapCycleTimeStamp;
	static int m_nMapCycleindex;
	static CUtlVector<char*> m_MapList;

	float m_flTimeLastMapChangeOrPlayerWasConnected;

#else
	
	public:
		const char *GetVoiceCommandSubtitle( int iMenu, int iItem );
		bool GetVoiceMenuLabels( int iMenu, KeyValues *pKV );

#endif

	private:
		CUtlVector< CUtlVector< VoiceCommandMenuItem_t > > m_VoiceCommandMenus;
};

inline CMultiplayRules* MultiplayRules()
{
	return static_cast<CMultiplayRules*>(g_pGameRules);
}

#endif // MULTIPLAY_GAMERULES_H

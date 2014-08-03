//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef HL2MP_PLAYER_H
#define HL2MP_PLAYER_H
#pragma once

class CHL2MP_Player;

#include "basemultiplayerplayer.h"
#include "hl2_playerlocaldata.h"
#include "hl2_player.h"
#include "simtimer.h"
#include "soundenvelope.h"
#include "hl2mp_player_shared.h"
#include "hl2mp_gamerules.h"
#include "utldict.h"

//=============================================================================
// >> HL2MP_Player
//=============================================================================
class CHL2MPPlayerStateInfo
{
public:
	HL2MPPlayerState m_iPlayerState;
	const char *m_pStateName;

	void (CHL2MP_Player::*pfnEnterState)();	// Init and deinit the state.
	void (CHL2MP_Player::*pfnLeaveState)();

	void (CHL2MP_Player::*pfnPreThink)();	// Do a PreThink() in this state.
};

class CHL2MP_Player : public CHL2_Player
{
#ifdef SecobMod__USE_PLAYERCLASSES
enum
{
	Unassigned = 0,
	Assaulter,
	Supporter,
	Medic,
	Heavy,
	Default,
};
#endif //SecobMod__USE_PLAYERCLASSES

public:

	#ifdef SecobMod__USE_PLAYERCLASSES
	//Can we change player class?
	bool PlayerCanChangeClass;

	// Apply a battery
	bool ApplyBattery( float powerMultiplier = 1.0 );
	#endif //SecobMod__USE_PLAYERCLASSES

	#ifdef SecobMod__SAVERESTORE
	virtual void SaveTransitionFile(void);
	#endif //SecobMod__SAVERESTORE

	#ifdef SecobMod__USE_PLAYERCLASSES
	//SecobMod__Information: Old ChangeClass vs New spam ver.
	// Method to change class.
	//virtual void ChangeClass(int NewClass);
	void ChangeClass();

	//virtual int GetClass();

	// Initalize the class system
	void InitClassSystem();
	
	// Check our classes convars.
	void CheckAllClassConVars();

	// On a class being changed.
	void OnClassChange();
	// Set stuff (health etc).
	void SetClassStuff();

	// Set the class value a player currently has.
	void SetCurrentClassValue();

	// Get the class value
	int GetClassValue()const;
	// Get the default classes value.
	int GetDefaultClassValue()const;

	// Start setting the player onto their selected class
	void SetPlayerClass();

	// int for the classes health.
	int  GetClassHealth()const;
	// int for the classes maximum health.
	int GetClassMaxHealth()const;
	
		// int for the classes as defined by their enum:
	int m_iClass;
	// Int for the current player class of the player.
	int m_iCurrentClass;
	// Default classes int.
	int m_iDefaultClass;

	// Set the classes.
	void SetClassDefault();
	void SetClassGroundUnit();
	void SetClassSupportUnit();
	void SetClassMedic();
	void SetClassHeavy();
// Ints for the movement speeds.
int m_iWalkSpeed; 
int m_iNormSpeed;
int m_iSprintSpeed;
#endif //SecobMod__USE_PLAYERCLASSES

// Armor Ints.
	int m_iArmor;
	int m_iMaxArmor;
	void	IncrementArmorValue( int nCount, int nMaxValue = -1 );
	void	SetArmorValue( int value );
	void	SetMaxArmorValue( int MaxArmorValue );
	
// Armor gets.
int CHL2MP_Player::GetArmorValue()
{
	return m_iArmor;
}


int CHL2MP_Player::GetMaxArmorValue()
{
	return m_iMaxArmor;
}

#ifdef SecobMod__USE_PLAYERCLASSES
private:  
	// Test whether this player is spawning for the first time.
	bool m_bFirstSpawn;
	bool IsFirstSpawn();

void CHL2MP_Player::SetHealthValue( int value )
{
	m_iHealth = value;
}

void CHL2MP_Player::SetMaxHealthValue( int MaxValue )
{
	m_iMaxHealth = MaxValue;
}

int CHL2MP_Player::GetHealthValue()
{
	return m_iHealth;
}

int CHL2MP_Player::GetMaxHealthValue()
{
	return m_iMaxHealth;
}

void CHL2MP_Player::IncrementHealthValue( int nCount )
{ 
	m_iHealth += nCount;
	if (m_iMaxHealth > 0 && m_iHealth > m_iMaxHealth)
		m_iHealth = m_iMaxHealth;
}
#endif //SecobMod__USE_PLAYERCLASSES
public:
	DECLARE_CLASS( CHL2MP_Player, CHL2_Player );

	CHL2MP_Player();
	~CHL2MP_Player( void );
	
	static CHL2MP_Player *CreatePlayer( const char *className, edict_t *ed )
	{
		CHL2MP_Player::s_PlayerEdict = ed;
		return (CHL2MP_Player*)CreateEntityByName( className );
	}

	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	
	#ifdef SecobMod__USE_PLAYERCLASSES
		void SSPlayerClassesBGCheck(CHL2MP_Player *pPlayer);
		void ShowSSPlayerClasses(CHL2MP_Player *pPlayer);
		void ForceHUDReload(CHL2MP_Player *pPlayer);
		bool (m_bDelayedMessage);
		float (m_flDelayedMessageTime); 
		CNetworkVar(int, m_iClientClass); //SecobMod__Information: Lets the client player know its class int.
	#endif //SecobMod__USE_PLAYERCLASSES
	
	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void PostThink( void );
	virtual void PreThink( void );
	virtual void PlayerDeathThink( void );
	virtual void SetAnimation( PLAYER_ANIM playerAnim );
	virtual bool HandleCommand_JoinTeam( int team );
	virtual bool ClientCommand( const CCommand &args );
	virtual void CreateViewModel( int viewmodelindex = 0 );
	virtual bool BecomeRagdollOnClient( const Vector &force );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual int OnTakeDamage( const CTakeDamageInfo &inputInfo );

	#ifdef SecobMod__Enable_Fixed_Multiplayer_AI
		virtual bool WantsLagCompensationOnEntity( const CBaseEntity *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const; 
	#else
		virtual bool WantsLagCompensationOnEntity( const CBasePlayer *pPlayer, const CUserCmd *pCmd, const CBitVec<MAX_EDICTS> *pEntityTransmitBits ) const;
	#endif //SecobMod__Enable_Fixed_Multiplayer_AI

	virtual void FireBullets ( const FireBulletsInfo_t &info );
	virtual bool Weapon_Switch( CBaseCombatWeapon *pWeapon, int viewmodelindex = 0);
	virtual bool BumpWeapon( CBaseCombatWeapon *pWeapon );
	virtual void ChangeTeam( int iTeam );
	virtual void PickupObject ( CBaseEntity *pObject, bool bLimitMassAndSize );
	virtual void PlayStepSound( Vector &vecOrigin, surfacedata_t *psurface, float fvol, bool force );
	virtual void Weapon_Drop( CBaseCombatWeapon *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual void UpdateOnRemove( void );
	virtual void DeathSound( const CTakeDamageInfo &info );
	virtual CBaseEntity* EntSelectSpawnPoint( void );
		
	int FlashlightIsOn( void );
	void FlashlightTurnOn( void );
	void FlashlightTurnOff( void );
	void	PrecacheFootStepSounds( void );
	bool	ValidatePlayerModel( const char *pModel );

	QAngle GetAnimEyeAngles( void ) { return m_angEyeAngles.Get(); }

	Vector GetAttackSpread( CBaseCombatWeapon *pWeapon, CBaseEntity *pTarget = NULL );

	void CheatImpulseCommands( int iImpulse );
	void CreateRagdollEntity( void );
	void GiveAllItems( void );
	void GiveDefaultItems( void );

	void NoteWeaponFired( void );

	void ResetAnimation( void );
	void SetPlayerModel( void );
	void SetPlayerTeamModel( void );
	Activity TranslateTeamActivity( Activity ActToTranslate );
	
	float GetNextModelChangeTime( void ) { return m_flNextModelChangeTime; }
	float GetNextTeamChangeTime( void ) { return m_flNextTeamChangeTime; }
	void  PickDefaultSpawnTeam( void );
	void  SetupPlayerSoundsByModel( const char *pModelName );
	const char *GetPlayerModelSoundPrefix( void );
	int	  GetPlayerModelType( void ) { return m_iPlayerSoundType;	}
	
	void  DetonateTripmines( void );

	void Reset();

	bool IsReady();
	void SetReady( bool bReady );

	void CheckChatText( char *p, int bufsize );

	void State_Transition( HL2MPPlayerState newState );
	void State_Enter( HL2MPPlayerState newState );
	void State_Leave();
	void State_PreThink();
	CHL2MPPlayerStateInfo *State_LookupInfo( HL2MPPlayerState state );

	void State_Enter_ACTIVE();
	void State_PreThink_ACTIVE();
	void State_Enter_OBSERVER_MODE();
	void State_PreThink_OBSERVER_MODE();


	virtual bool StartObserverMode( int mode );
	virtual void StopObserverMode( void );


	Vector m_vecTotalBulletForce;	//Accumulator for bullet force in a single frame

	// Tracks our ragdoll entity.
	CNetworkHandle( CBaseEntity, m_hRagdoll );	// networked entity handle 

	virtual bool	CanHearAndReadChatFrom( CBasePlayer *pPlayer );

		
private:

	CNetworkQAngle( m_angEyeAngles );
	CPlayerAnimState   m_PlayerAnimState;

	int m_iLastWeaponFireUsercmd;
	int m_iModelType;
	CNetworkVar( int, m_iSpawnInterpCounter );
	CNetworkVar( int, m_iPlayerSoundType );

	float m_flNextModelChangeTime;
	float m_flNextTeamChangeTime;

	float m_flSlamProtectTime;	

	HL2MPPlayerState m_iPlayerState;
	CHL2MPPlayerStateInfo *m_pCurStateInfo;

	bool ShouldRunRateLimitedCommand( const CCommand &args );

	// This lets us rate limit the commands the players can execute so they don't overflow things like reliable buffers.
	CUtlDict<float,int>	m_RateLimitLastCommandTimes;

    bool m_bEnterObserver;
	bool m_bReady;
};

inline CHL2MP_Player *ToHL2MPPlayer( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return dynamic_cast<CHL2MP_Player*>( pEntity );
}

#endif //HL2MP_PLAYER_H

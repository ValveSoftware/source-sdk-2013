//========= Copyright Valve Corporation, All rights reserved. ============//

#ifndef HL2MP_BOT_H
#define HL2MP_BOT_H

#include "Player/NextBotPlayer.h"
#include "hl2mp_bot_vision.h"
#include "hl2mp_bot_body.h"
#include "hl2mp_bot_locomotion.h"
#include "hl2mp_player.h"
#include "hl2mp_bot_squad.h"
#include "map_entities/hl2mp_bot_generator.h"
#include "map_entities/hl2mp_bot_proxy.h"
#include "hl2mp_gamerules.h"
#include "hl2mp/weapon_hl2mpbasehlmpcombatweapon.h"
#include "nav_entities.h"
#include "utlstack.h"

#define HL2MP_BOT_TYPE	1337

class CHL2MPBotActionPoint;
class CHL2MPBotGenerator;
class CBaseHL2MPBludgeonWeapon;
class CHL2MPBot;

extern ConVar hl2_sprintspeed;
extern ConVar hl2_normspeed;
extern ConVar hl2_walkspeed;

extern void BotGenerateAndWearItem( CHL2MP_Player *pBot, const char *itemName );

extern int Bot_GetTeamByName( const char *string );

inline int GetEnemyTeam( int team )
{
	if ( team == TEAM_COMBINE )
		return TEAM_REBELS;

	if ( team == TEAM_REBELS )
		return TEAM_COMBINE;

	// no enemy team
	return team;
}

//----------------------------------------------------------------------------

#define HL2MPBOT_ALL_BEHAVIOR_FLAGS		0xFFFF

//----------------------------------------------------------------------------
class CHL2MPBot: public NextBotPlayer< CHL2MP_Player >, public CGameEventListener
{
public:
	DECLARE_CLASS( CHL2MPBot, NextBotPlayer< CHL2MP_Player > );

	DECLARE_ENT_SCRIPTDESC();

	CHL2MPBot();
	virtual ~CHL2MPBot();

	virtual void		Spawn();
	virtual void		FireGameEvent( IGameEvent *event );
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	virtual void		PhysicsSimulate( void );
	virtual void		Touch( CBaseEntity *pOther );
	virtual void		AvoidPlayers( CUserCmd *pCmd );				// some game types allow players to pass through each other, this method pushes them apart
	virtual void		UpdateOnRemove( void );
	virtual int			ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;

	virtual int			DrawDebugTextOverlays(void);

	void				ModifyMaxHealth( int nNewMaxHealth, bool bSetCurrentHealth = true );

	virtual int GetBotType( void ) const;				// return a unique int representing the type of bot instance this is

	virtual CNavArea *GetLastKnownArea( void ) const		{ return static_cast< CNavArea * >( BaseClass::GetLastKnownArea() ); }		// return the last nav area the player occupied - NULL if unknown

	// NextBotPlayer
	static CBasePlayer *AllocatePlayerEntity( edict_t *pEdict, const char *playerName );

	virtual void PressFireButton( float duration = -1.0f ) OVERRIDE;
	virtual void PressAltFireButton( float duration = -1.0f ) OVERRIDE;
	virtual void PressSpecialFireButton( float duration = -1.0f ) OVERRIDE;

	// INextBot
	virtual CHL2MPBotLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CHL2MPBotBody			*GetBodyInterface( void ) const			{ return m_body; }
	virtual CHL2MPBotVision		*GetVisionInterface( void ) const		{ return m_vision; }
	DECLARE_INTENTION_INTERFACE( CHL2MPBot );

	virtual bool IsDormantWhenDead( void ) const;			// should this player-bot continue to update itself when dead (respawn logic, etc)

	virtual void OnWeaponFired( CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon );		// when someone fires their weapon

	// search outwards from startSearchArea and collect all reachable objects from the given list that pass the given filter
	void SelectReachableObjects( const CUtlVector< CHandle< CBaseEntity > > &candidateObjectVector, CUtlVector< CHandle< CBaseEntity > > *selectedObjectVector, const INextBotFilter &filter, CNavArea *startSearchArea, float maxRange = 2000.0f ) const;
	CBaseEntity *FindClosestReachableObject( const char *objectName, CNavArea *from, float maxRange = 2000.0f ) const;

	CNavArea *GetSpawnArea( void ) const;							// get area where we spawned in
	HSCRIPT ScriptGetSpawnArea( void ) const { return ToHScript( this->GetSpawnArea() ); }

	bool IsAmmoLow( void ) const;
	bool IsAmmoFull( void ) const;

	void UpdateLookingAroundForEnemies( void );						// update our view to keep an eye on enemies, and where enemies come from

	#define LOOK_FOR_FRIENDS false
	#define LOOK_FOR_ENEMIES true
	void UpdateLookingAroundForIncomingPlayers( bool lookForEnemies );	// update our view to watch where friends or enemies will be coming from
	void StartLookingAroundForEnemies( void );						// enable updating view for enemy searching
	void StopLookingAroundForEnemies( void );						// disable updating view for enemy searching

	void SetAttentionFocus( CBaseEntity *focusOn );					// restrict bot's attention to only this entity (or radius around this entity) to the exclusion of everything else
	void ClearAttentionFocus( void );								// remove attention focus restrictions
	bool IsAttentionFocused( void ) const;
	bool IsAttentionFocusedOn( CBaseEntity *who ) const;
	void ScriptSetAttentionFocus( HSCRIPT hFocusOn ) { this->SetAttentionFocus( ToEnt( hFocusOn ) ); }
	bool ScriptIsAttentionFocusedOn( HSCRIPT hWho ) const { return this->IsAttentionFocusedOn( ToEnt( hWho ) ); }

	void DelayedThreatNotice( CHandle< CBaseEntity > who, float noticeDelay );	// notice the given threat after the given number of seconds have elapsed
	void UpdateDelayedThreatNotices( void );
	void ScriptDelayedThreatNotice( HSCRIPT hWho, float flNoticeDelay ) { this->DelayedThreatNotice( ToEnt( hWho ), flNoticeDelay ); }

	CNavArea *FindVantagePoint( float maxTravelDistance = 2000.0f ) const;	// return a nearby area where we can see a member of the enemy team
	HSCRIPT ScriptFindVantagePoint( float maxTravelDistance ) { return ToHScript( this->FindVantagePoint( maxTravelDistance ) ); }

	float GetThreatDanger( CBaseCombatCharacter *who ) const;		// return perceived danger of threat (0=none, 1=immediate deadly danger)
	float GetMaxAttackRange( void ) const;							// return the max range at which we can effectively attack
	float GetDesiredAttackRange( void ) const;						// return the ideal range at which we can effectively attack

	bool EquipRequiredWeapon( void );								// if we're required to equip a specific weapon, do it.
	void EquipBestWeaponForThreat( const CKnownEntity *threat );	// equip the best weapon we have to attack the given threat

	void PushRequiredWeapon( CBaseHL2MPCombatWeapon *weapon );				// force us to equip and use this weapon until popped off the required stack
	void PopRequiredWeapon( void );									// pop top required weapon off of stack and discard

	#define MY_CURRENT_GUN NULL										// can be passed as weapon to following queries
	bool IsCombatWeapon( CBaseHL2MPCombatWeapon *weapon ) const;				// return true if given weapon can be used to attack
	bool IsHitScanWeapon( CBaseHL2MPCombatWeapon *weapon ) const;			// return true if given weapon is a "hitscan" weapon (scattered tracelines with instant damage)
	bool IsContinuousFireWeapon( CBaseHL2MPCombatWeapon *weapon ) const;		// return true if given weapon "sprays" bullets/fire/etc continuously (ie: not individual rockets/etc)
	bool IsExplosiveProjectileWeapon( CBaseHL2MPCombatWeapon *weapon ) const;// return true if given weapon launches explosive projectiles with splash damage
	bool IsBarrageAndReloadWeapon( CBaseHL2MPCombatWeapon *weapon ) const;	// return true if given weapon has small clip and long reload cost (ie: rocket launcher, etc)
	bool IsQuietWeapon( CBaseHL2MPCombatWeapon *weapon ) const;				// return true if given weapon doesn't make much sound when used (ie: spy knife, etc)

	bool IsEnvironmentNoisy( void ) const;							// return true if there are/have been loud noises (ie: non-quiet weapons) nearby very recently

	bool IsEnemy( const CBaseEntity* them ) const OVERRIDE;

	CBaseHL2MPBludgeonWeapon *GetBludgeonWeapon( void );

	static bool IsBludgeon( CBaseCombatWeapon *pWeapon );
	static bool IsCloseRange( CBaseCombatWeapon* pWeapon );
	static bool IsRanged( CBaseCombatWeapon* pWeapon );
	static bool PrefersLongRange( CBaseCombatWeapon* pWeapon );

	enum WeaponRestrictionType
	{
		ANY_WEAPON		= 0,
		MELEE_ONLY		= 0x0001,
		GRAVGUN_ONLY	= 0x0002,
	};
	void ClearWeaponRestrictions( void );
	void SetWeaponRestriction( int restrictionFlags );
	void RemoveWeaponRestriction( int restrictionFlags );
	bool HasWeaponRestriction( int restrictionFlags ) const;
	bool IsWeaponRestricted( CBaseHL2MPCombatWeapon *weapon ) const;
	bool ScriptIsWeaponRestricted( HSCRIPT script ) const;

	bool IsLineOfFireClear( const Vector &where ) const;			// return true if a weapon has no obstructions along the line from our eye to the given position
	bool IsLineOfFireClear( CBaseEntity *who ) const;				// return true if a weapon has no obstructions along the line from our eye to the given entity
	bool IsLineOfFireClear( const Vector &from, const Vector &to ) const;			// return true if a weapon has no obstructions along the line between the given points
	bool IsLineOfFireClear( const Vector &from, CBaseEntity *who ) const;			// return true if a weapon has no obstructions along the line between the given point and entity

	bool IsEntityBetweenTargetAndSelf( CBaseEntity *other, CBaseEntity *target );	// return true if "other" is positioned inbetween us and "target"

	CHL2MP_Player *GetClosestHumanLookingAtMe( int team = TEAM_ANY ) const;	// return the nearest human player on the given team who is looking directly at me

	enum AttributeType
	{
		REMOVE_ON_DEATH				= 1<<0,					// kick bot from server when killed
		AGGRESSIVE					= 1<<1,					// in MvM mode, push for the cap point
		IS_NPC						= 1<<2,					// a non-player support character
		SUPPRESS_FIRE				= 1<<3,
		DISABLE_DODGE				= 1<<4,
		BECOME_SPECTATOR_ON_DEATH	= 1<<5,					// move bot to spectator team when killed
		QUOTA_MANANGED				= 1<<6,					// managed by the bot quota in CHL2MPBotManager 
		PROP_FREAK					= 1<<7,
		PROP_HATER					= 1<<8,
		IGNORE_ENEMIES				= 1<<10,
		HOLD_FIRE_UNTIL_FULL_RELOAD	= 1<<11,				// don't fire our barrage weapon until it is full reloaded (rocket launcher, etc)
		PRIORITIZE_DEFENSE			= 1<<12,				// bot prioritizes defending when possible
		ALWAYS_FIRE_WEAPON			= 1<<13,				// constantly fire our weapon
		TELEPORT_TO_HINT			= 1<<14,				// bot will teleport to hint target instead of walking out from the spawn point
		AUTO_JUMP					= 1<<18,				// auto jump
	};
	void SetAttribute( int attributeFlag );
	void ClearAttribute( int attributeFlag );
	void ClearAllAttributes();
	bool HasAttribute( int attributeFlag ) const;

	enum DifficultyType
	{
		UNDEFINED = -1,
		EASY = 0,
		NORMAL = 1,
		HARD = 2,
		EXPERT = 3,

		NUM_DIFFICULTY_LEVELS
	};
	DifficultyType GetDifficulty( void ) const;
	void SetDifficulty( DifficultyType difficulty );
	bool IsDifficulty( DifficultyType skill ) const;
	int ScriptGetDifficulty( void ) const { return this->GetDifficulty(); }
	void ScriptSetDifficulty( int difficulty ) { this->SetDifficulty( (DifficultyType) difficulty ); }
	bool ScriptIsDifficulty( int difficulty ) const { return this->IsDifficulty( (DifficultyType) difficulty ); }

	void SetHomeArea( CNavArea *area );
	CNavArea *GetHomeArea( void ) const;
	void ScriptSetHomeArea( HSCRIPT hScript ) { this->SetHomeArea( ToNavArea( hScript ) ); }
	HSCRIPT ScriptGetHomeArea( void ) { return ToHScript( this->GetHomeArea() ); }

	const Vector &GetSpotWhereEnemySentryLastInjuredMe( void ) const;

	void SetActionPoint( CHL2MPBotActionPoint *point );
	CHL2MPBotActionPoint *GetActionPoint( void ) const;
	void ScriptSetActionPoint( HSCRIPT hPoint ) { SetActionPoint( ScriptToEntClass< CHL2MPBotActionPoint >( hPoint ) ); }
	HSCRIPT ScriptGetActionPoint( void ) const { return ToHScript( GetActionPoint() ); }

	bool HasProxy( void ) const;
	void SetProxy( CHL2MPBotProxy *proxy );					// attach this bot to a bot_proxy entity for map I/O communications
	CHL2MPBotProxy *GetProxy( void ) const;

	bool HasSpawner( void ) const;
	void SetSpawner( CHL2MPBotGenerator *spawner );
	CHL2MPBotGenerator *GetSpawner( void ) const;

	void JoinSquad( CHL2MPBotSquad *squad );					// become a member of the given squad
	void LeaveSquad( void );								// leave our current squad
	void DeleteSquad( void );
	bool IsInASquad( void ) const;
	bool IsSquadmate( CHL2MP_Player *who ) const;				// return true if given bot is in my squad
	CHL2MPBotSquad *GetSquad( void ) const;					// return squad we are in, or NULL
	float GetSquadFormationError( void ) const;				// return normalized error term where 0 = in formation position and 1 = completely out of position
	void SetSquadFormationError( float error );
	bool HasBrokenFormation( void ) const;					// return true if this bot is far out of formation, or has no path back
	void SetBrokenFormation( bool state );

	void ScriptDisbandCurrentSquad( void ) { if ( GetSquad() ) GetSquad()->DisbandAndDeleteSquad(); }

	float TransientlyConsistentRandomValue( float period = 10.0f, int seedValue = 0 ) const;		// compute a pseudo random value (0-1) that stays consistent for the given period of time, but changes unpredictably each period

	void SetBehaviorFlag( unsigned int flags );
	void ClearBehaviorFlag( unsigned int flags );
	bool IsBehaviorFlagSet( unsigned int flags ) const;

	bool FindSplashTarget( CBaseEntity *target, float maxSplashRadius, Vector *splashTarget ) const;

	enum MissionType
	{
		NO_MISSION = 0,
		MISSION_SEEK_AND_DESTROY,		// focus on finding and killing enemy players
		MISSION_DESTROY_SENTRIES,		// focus on finding and destroying enemy sentry guns (and buildings)
		MISSION_SNIPER,					// maintain teams of snipers harassing the enemy
		MISSION_SPY,					// maintain teams of spies harassing the enemy
		MISSION_ENGINEER,				// maintain engineer nests for harassing the enemy
		MISSION_REPROGRAMMED,			// MvM: robot has been hacked and will do bad things to their team
	};
	#define MISSION_DOESNT_RESET_BEHAVIOR_SYSTEM false
	void SetMission( MissionType mission, bool resetBehaviorSystem = true );
	void SetPrevMission( MissionType mission );
	MissionType GetMission( void ) const;
	MissionType GetPrevMission( void ) const;
	bool HasMission( MissionType mission ) const;
	bool IsOnAnyMission( void ) const;
	void SetMissionTarget( CBaseEntity *target );
	CBaseEntity *GetMissionTarget( void ) const;
	void SetMissionString( CUtlString string );
	CUtlString *GetMissionString( void );
	void ScriptSetMission( unsigned int mission, bool resetBehaviorSystem = true ) { this->SetMission( (MissionType)mission, resetBehaviorSystem ); }
	void ScriptSetPrevMission( unsigned int mission ) { this->SetPrevMission( (MissionType)mission ); }
	unsigned int ScriptGetMission( void ) const { return (unsigned int)this->GetMission(); }
	unsigned int ScriptGetPrevMission( void ) const { return (unsigned int)this->GetPrevMission(); }
	bool ScriptHasMission( unsigned int mission ) const { return this->HasMission( (MissionType)mission ); }
	void ScriptSetMissionTarget( HSCRIPT hTarget ) { this->SetMissionTarget( ToEnt( hTarget ) ); }
	HSCRIPT ScriptGetMissionTarget( void ) const { return ToHScript( this->GetMissionTarget() ); }

	void SetTeleportWhere( const CUtlStringList& teleportWhereName );
	const CUtlStringList& GetTeleportWhere();
	void ClearTeleportWhere();

	void SetScaleOverride( float fScale );

	void SetMaxVisionRangeOverride( float range );
	float GetMaxVisionRangeOverride( void ) const;

	void ClearTags( void );
	void AddTag( const char *tag );
	void RemoveTag( const char *tag );
	bool HasTag( const char *tag );
	void ScriptGetAllTags( HSCRIPT hTable );

	Action< CHL2MPBot > *OpportunisticallyUseWeaponAbilities( void );

	CHL2MP_Player *SelectRandomReachableEnemy( void );	// mostly for MvM - pick a random enemy player that is not in their spawn room

	float GetDesiredPathLookAheadRange( void ) const;	// different sized bots used different lookahead distances

	void StartIdleSound( void );
	void StopIdleSound( void );

	void SetAutoJump( float flAutoJumpMin, float flAutoJumpMax ) { m_flAutoJumpMin = flAutoJumpMin; m_flAutoJumpMax = flAutoJumpMax; }
	bool ShouldAutoJump();

	void MarkPhyscannonPickupTime() { m_flPhyscannonPickupTime = gpGlobals->curtime; }
	float GetPhyscannonPickupTime() { return m_flPhyscannonPickupTime;  }
	

	struct EventChangeAttributes_t
	{
		EventChangeAttributes_t()
		{
			Reset();
		}

		EventChangeAttributes_t( const EventChangeAttributes_t& copy )
		{
			Reset();

			m_eventName = copy.m_eventName;

			m_skill = copy.m_skill;
			m_weaponRestriction = copy.m_weaponRestriction;
			m_mission = copy.m_mission;
			m_prevMission = copy.m_prevMission;
			m_attributeFlags = copy.m_attributeFlags;
			m_maxVisionRange = copy.m_maxVisionRange;

			for ( int i=0; i<copy.m_items.Count(); ++i )
			{
				m_items.CopyAndAddToTail( copy.m_items[i] );
			}

			for ( int i=0; i<copy.m_tags.Count(); ++i )
			{
				m_tags.CopyAndAddToTail( copy.m_tags[i] );
			}
		}

		void Reset()
		{
			m_eventName = "default";
			
			m_skill = CHL2MPBot::EASY;
			m_weaponRestriction = CHL2MPBot::ANY_WEAPON;
			m_mission = CHL2MPBot::NO_MISSION;
			m_prevMission = m_mission;
			m_attributeFlags = 0;
			m_maxVisionRange = -1.f;

			m_items.RemoveAll();
			
			m_tags.RemoveAll();
		}

		CUtlString m_eventName;

		DifficultyType m_skill;
		WeaponRestrictionType m_weaponRestriction;
		MissionType m_mission;
		MissionType m_prevMission;
		int m_attributeFlags;
		float m_maxVisionRange;

		CUtlStringList m_items;
		CUtlStringList m_tags;
	};
	void ClearEventChangeAttributes() { m_eventChangeAttributes.RemoveAll(); }
	void AddEventChangeAttributes( const EventChangeAttributes_t* newEvent );
	const EventChangeAttributes_t* GetEventChangeAttributes( const char* pszEventName ) const;
	void OnEventChangeAttributes( const CHL2MPBot::EventChangeAttributes_t* pEvent );

	bool IsPropFreak() const;
	bool IsPropHater() const;
	CBaseEntity *Physcannon_GetHeldProp() const;

private:
	CHL2MPBotLocomotion	*m_locomotor;
	CHL2MPBotBody			*m_body;
	CHL2MPBotVision		*m_vision;

	CountdownTimer m_lookAtEnemyInvasionAreasTimer;

	CNavArea *m_spawnArea;			// where we spawned
	CountdownTimer m_justLostPointTimer;
	
	int m_weaponRestrictionFlags;
	int m_attributeFlags;
	DifficultyType m_difficulty;

	CNavArea *m_homeArea;

	CHandle< CHL2MPBotActionPoint > m_actionPoint;
	CHandle< CHL2MPBotProxy > m_proxy;
	CHandle< CHL2MPBotGenerator > m_spawner;

	CHL2MPBotSquad *m_squad;
	bool m_didReselectClass;

	Vector m_spotWhereEnemySentryLastInjuredMe;			// the last position where I was injured by an enemy sentry

	bool m_isLookingAroundForEnemies;

	unsigned int m_behaviorFlags;						// spawnflags from the bot_generator that spawned us
	CUtlVector< CFmtStr > m_tags;

	CHandle< CBaseEntity > m_attentionFocusEntity;

	float m_fModelScaleOverride;

	MissionType m_mission;
	MissionType m_prevMission;

	CHandle< CBaseEntity > m_missionTarget;
	CUtlString m_missionString;

	CUtlStack< CHandle<CBaseHL2MPCombatWeapon> > m_requiredWeaponStack;	// if non-empty, bot must equip the weapon on top of the stack

	CountdownTimer m_noisyTimer;

	struct DelayedNoticeInfo
	{
		CHandle< CBaseEntity > m_who;
		float m_when;
	};
	CUtlVector< DelayedNoticeInfo > m_delayedNoticeVector;

	float m_maxVisionRangeOverride;

	CountdownTimer m_opportunisticTimer;
	
	CSoundPatch *m_pIdleSound;

	float m_squadFormationError;
	bool m_hasBrokenFormation;

	CUtlStringList m_teleportWhereName;	// spawn name an engineer mission teleporter will override
	bool m_bForceQuickBuild;

	float m_flAutoJumpMin;
	float m_flAutoJumpMax;
	CountdownTimer m_autoJumpTimer;

	float m_flPhyscannonPickupTime = 0.0f;

	CUtlVector< const EventChangeAttributes_t* > m_eventChangeAttributes;
};


inline void CHL2MPBot::SetTeleportWhere( const CUtlStringList& teleportWhereName )
{
	// deep copy strings
	for ( int i=0; i<teleportWhereName.Count(); ++i )
	{
		m_teleportWhereName.CopyAndAddToTail( teleportWhereName[i] );
	}
}

inline const CUtlStringList& CHL2MPBot::GetTeleportWhere()
{
	return m_teleportWhereName;
}

inline void CHL2MPBot::ClearTeleportWhere()
{
	m_teleportWhereName.RemoveAll();
}

inline void CHL2MPBot::SetMissionString( CUtlString string )
{
	m_missionString = string;
}

inline CUtlString *CHL2MPBot::GetMissionString( void )
{
	return &m_missionString;
}

inline void CHL2MPBot::SetMissionTarget( CBaseEntity *target )
{
	m_missionTarget = target;
}

inline CBaseEntity *CHL2MPBot::GetMissionTarget( void ) const
{
	return m_missionTarget;
}

inline float CHL2MPBot::GetSquadFormationError( void ) const
{
	return m_squadFormationError;
}

inline void CHL2MPBot::SetSquadFormationError( float error )
{
	m_squadFormationError = error;
}

inline bool CHL2MPBot::HasBrokenFormation( void ) const
{
	return m_hasBrokenFormation;
}

inline void CHL2MPBot::SetBrokenFormation( bool state )
{
	m_hasBrokenFormation = state;
}

inline void CHL2MPBot::SetMaxVisionRangeOverride( float range )
{
	m_maxVisionRangeOverride = range;
}

inline float CHL2MPBot::GetMaxVisionRangeOverride( void ) const
{
	return m_maxVisionRangeOverride;
}

inline void CHL2MPBot::SetBehaviorFlag( unsigned int flags )
{
	m_behaviorFlags |= flags;
}

inline void CHL2MPBot::ClearBehaviorFlag( unsigned int flags )
{
	m_behaviorFlags &= ~flags;
}

inline bool CHL2MPBot::IsBehaviorFlagSet( unsigned int flags ) const
{
	return ( m_behaviorFlags & flags ) ? true : false;
}

inline void CHL2MPBot::StartLookingAroundForEnemies( void )
{
	m_isLookingAroundForEnemies = true;
}

inline void CHL2MPBot::StopLookingAroundForEnemies( void )
{
	m_isLookingAroundForEnemies = false;
}

inline int CHL2MPBot::GetBotType( void ) const
{
	return HL2MP_BOT_TYPE;
}

inline const Vector &CHL2MPBot::GetSpotWhereEnemySentryLastInjuredMe( void ) const
{
	return m_spotWhereEnemySentryLastInjuredMe;
}

inline CHL2MPBot::DifficultyType CHL2MPBot::GetDifficulty( void ) const
{
	return m_difficulty;
}

inline void CHL2MPBot::SetDifficulty( CHL2MPBot::DifficultyType difficulty )
{
	m_difficulty = difficulty;
}

inline bool CHL2MPBot::IsDifficulty( DifficultyType skill ) const
{
	return skill == m_difficulty;
}

inline bool CHL2MPBot::HasProxy( void ) const
{
	return m_proxy == NULL ? false : true;
}

inline void CHL2MPBot::SetProxy( CHL2MPBotProxy *proxy )
{
	m_proxy = proxy;
}

inline CHL2MPBotProxy *CHL2MPBot::GetProxy( void ) const
{
	return m_proxy;
}

inline bool CHL2MPBot::HasSpawner( void ) const
{
	return m_spawner == NULL ? false : true;
}

inline void CHL2MPBot::SetSpawner( CHL2MPBotGenerator *spawner )
{
	m_spawner = spawner;
}

inline CHL2MPBotGenerator *CHL2MPBot::GetSpawner( void ) const
{
	return m_spawner;
}

inline void CHL2MPBot::SetActionPoint( CHL2MPBotActionPoint *point )
{
	m_actionPoint = point;
}

inline CHL2MPBotActionPoint *CHL2MPBot::GetActionPoint( void ) const
{
	return m_actionPoint;
}

inline bool CHL2MPBot::IsInASquad( void ) const
{
	return m_squad == NULL ? false : true;
}

inline CHL2MPBotSquad *CHL2MPBot::GetSquad( void ) const
{
	return m_squad;
}

inline void CHL2MPBot::SetHomeArea( CNavArea *area )
{
	m_homeArea = area;
}

inline CNavArea *CHL2MPBot::GetHomeArea( void ) const
{
	return m_homeArea;
}

inline void CHL2MPBot::ClearWeaponRestrictions( void )
{
	m_weaponRestrictionFlags = 0;
}

inline void CHL2MPBot::SetWeaponRestriction( int restrictionFlags )
{
	m_weaponRestrictionFlags |= restrictionFlags;
}

inline void CHL2MPBot::RemoveWeaponRestriction( int restrictionFlags )
{
	m_weaponRestrictionFlags &= ~restrictionFlags;
}

inline bool CHL2MPBot::HasWeaponRestriction( int restrictionFlags ) const
{
	return m_weaponRestrictionFlags & restrictionFlags ? true : false;
}

inline void CHL2MPBot::SetAttribute( int attributeFlag )
{
	m_attributeFlags |= attributeFlag;
}

inline void CHL2MPBot::ClearAttribute( int attributeFlag )
{
	m_attributeFlags &= ~attributeFlag;
}

inline void CHL2MPBot::ClearAllAttributes()
{
	m_attributeFlags = 0;
}

inline bool CHL2MPBot::HasAttribute( int attributeFlag ) const
{
	return m_attributeFlags & attributeFlag ? true : false;
}

inline CNavArea *CHL2MPBot::GetSpawnArea( void ) const
{
	return m_spawnArea;
}

inline CHL2MPBot::MissionType CHL2MPBot::GetMission( void ) const
{
	return m_mission;
}

inline void CHL2MPBot::SetPrevMission( MissionType mission )
{
	m_prevMission = mission;
}

inline CHL2MPBot::MissionType CHL2MPBot::GetPrevMission( void ) const
{
	return m_prevMission;
}

inline bool CHL2MPBot::HasMission( MissionType mission ) const
{
	return m_mission == mission ? true : false;
}

inline bool CHL2MPBot::IsOnAnyMission( void ) const
{
	return m_mission == NO_MISSION ? false : true;
}

inline void CHL2MPBot::SetScaleOverride( float fScale )
{
	m_fModelScaleOverride = fScale;

	SetModelScale( m_fModelScaleOverride > 0.0f ? m_fModelScaleOverride : 1.0f );
}

inline bool CHL2MPBot::IsEnvironmentNoisy( void ) const
{
	return !m_noisyTimer.IsElapsed();
}

//---------------------------------------------------------------------------------------------
inline CHL2MPBot *ToHL2MPBot( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() || !ToHL2MPPlayer( pEntity )->IsBotOfType( HL2MP_BOT_TYPE ) )
		return NULL;

	Assert( "***IMPORTANT!!! DONT IGNORE ME!!!***" && dynamic_cast< CHL2MPBot * >( pEntity ) != 0 );

	return static_cast< CHL2MPBot * >( pEntity );
}


//---------------------------------------------------------------------------------------------
inline const CHL2MPBot *ToHL2MPBot( const CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() || !ToHL2MPPlayer( const_cast< CBaseEntity * >( pEntity ) )->IsBotOfType( HL2MP_BOT_TYPE ) )
		return NULL;

	Assert( "***IMPORTANT!!! DONT IGNORE ME!!!***" && dynamic_cast< const CHL2MPBot * >( pEntity ) != 0 );

	return static_cast< const CHL2MPBot * >( pEntity );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class CHL2MPBotPathCost : public IPathCost
{
public:
	CHL2MPBotPathCost( CHL2MPBot *me, RouteType routeType )
	{
		m_me = me;
		m_routeType = routeType;
		m_stepHeight = me->GetLocomotionInterface()->GetStepHeight();
		m_maxJumpHeight = me->GetLocomotionInterface()->GetMaxJumpHeight();
		m_maxDropHeight = me->GetLocomotionInterface()->GetDeathDropHeight();
	}

	virtual float operator()( CNavArea *baseArea, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
	{
		VPROF_BUDGET( "CHL2MPBotPathCost::operator()", "NextBot" );

		CNavArea *area = (CNavArea *)baseArea;

		if ( fromArea == NULL )
		{
			// first area in path, no cost
			return 0.0f;
		}
		else
		{
			if ( !m_me->GetLocomotionInterface()->IsAreaTraversable( area ) )
			{
				return -1.0f;
			}

			// compute distance traveled along path so far
			float dist;

			if ( ladder )
			{
				dist = ladder->m_length;
			}
			else if ( length > 0.0 )
			{
				dist = length;
			}
			else
			{
				dist = ( area->GetCenter() - fromArea->GetCenter() ).Length();
			}


			// check height change
			float deltaZ = fromArea->ComputeAdjacentConnectionHeightChange( area );

			if ( deltaZ >= m_stepHeight )
			{
				if ( deltaZ >= m_maxJumpHeight )
				{
					// too high to reach
					return -1.0f;
				}

				// jumping is slower than flat ground
				const float jumpPenalty = 2.0f;
				dist *= jumpPenalty;
			}
			else if ( deltaZ < -m_maxDropHeight )
			{
				// too far to drop
				return -1.0f;
			}

			// add a random penalty unique to this character so they choose different routes to the same place
			float preference = 1.0f;

			if ( m_routeType == DEFAULT_ROUTE )
			{
				// this term causes the same bot to choose different routes over time,
				// but keep the same route for a period in case of repaths
				int timeMod = (int)( gpGlobals->curtime / 10.0f ) + 1;
				preference = 1.0f + 50.0f * ( 1.0f + FastCos( (float)( m_me->GetEntity()->entindex() * area->GetID() * timeMod ) ) );
			}

			if ( m_routeType == SAFEST_ROUTE )
			{
				// misyl: combat areas.
#if 0
				// avoid combat areas
				if ( area->IsInCombat() )
				{
					const float combatDangerCost = 4.0f;
					dist *= combatDangerCost * area->GetCombatIntensity();
				}
#endif
			}

			float cost = ( dist * preference );

			if ( area->HasAttributes( NAV_MESH_FUNC_COST ) )
			{
				cost *= area->ComputeFuncNavCost( m_me );
				DebuggerBreakOnNaN_StagingOnly( cost );
			}

			return cost + fromArea->GetCostSoFar();
		}
	}

	CHL2MPBot *m_me;
	RouteType m_routeType;
	float m_stepHeight;
	float m_maxJumpHeight;
	float m_maxDropHeight;
};


//---------------------------------------------------------------------------------------------
class CClosestHL2MPPlayer
{
public:
	CClosestHL2MPPlayer( const Vector &where, int team = TEAM_ANY )
	{
		m_where = where;
		m_closeRangeSq = FLT_MAX;
		m_closePlayer = NULL;
		m_team = team;
	}

	CClosestHL2MPPlayer( CBaseEntity *entity, int team = TEAM_ANY )
	{
		m_where = entity->WorldSpaceCenter();
		m_closeRangeSq = FLT_MAX;
		m_closePlayer = NULL;
		m_team = team;
	}

	bool operator() ( CBasePlayer *player )
	{
		if ( !player->IsAlive() )
			return true;

		if ( player->GetTeamNumber() != TEAM_REBELS && player->GetTeamNumber() != TEAM_COMBINE && player->GetTeamNumber() != TEAM_UNASSIGNED )
			return true;

		if ( m_team != TEAM_ANY && player->GetTeamNumber() != m_team )
			return true;

		CHL2MPBot *bot = ToHL2MPBot( player );
		if ( bot && bot->HasAttribute( CHL2MPBot::IS_NPC ) )
			return true;

		float rangeSq = ( m_where - player->GetAbsOrigin() ).LengthSqr();
		if ( rangeSq < m_closeRangeSq )
		{
			m_closeRangeSq = rangeSq;
			m_closePlayer = player;
		}
		return true;
	}

	Vector m_where;
	float m_closeRangeSq;
	CBasePlayer *m_closePlayer;
	int m_team;
};

static const char* g_ppszRandomCitizenModels[] =
{
	"models/humans/group03/male_01.mdl",
	"models/humans/group03/male_02.mdl",
	"models/humans/group03/female_01.mdl",
	"models/humans/group03/male_03.mdl",
	"models/humans/group03/female_02.mdl",
	"models/humans/group03/male_04.mdl",
	"models/humans/group03/female_03.mdl",
	"models/humans/group03/male_05.mdl",
	"models/humans/group03/female_04.mdl",
	"models/humans/group03/male_06.mdl",
	"models/humans/group03/female_06.mdl",
	"models/humans/group03/male_07.mdl",
	"models/humans/group03/female_07.mdl",
	"models/humans/group03/male_08.mdl",
	"models/humans/group03/male_09.mdl",
};

static const char* g_ppszRandomCombineModels[] =
{
	"models/combine_soldier.mdl",
	"models/combine_soldier_prisonguard.mdl",
	"models/combine_super_soldier.mdl",
	"models/police.mdl",
};

static const char* g_ppszRandomModels[] =
{
	"models/humans/group03/male_01.mdl",
	"models/humans/group03/male_02.mdl",
	"models/humans/group03/female_01.mdl",
	"models/humans/group03/male_03.mdl",
	"models/humans/group03/female_02.mdl",
	"models/humans/group03/male_04.mdl",
	"models/humans/group03/female_03.mdl",
	"models/humans/group03/male_05.mdl",
	"models/humans/group03/female_04.mdl",
	"models/humans/group03/male_06.mdl",
	"models/humans/group03/female_06.mdl",
	"models/humans/group03/male_07.mdl",
	"models/humans/group03/female_07.mdl",
	"models/humans/group03/male_08.mdl",
	"models/humans/group03/male_09.mdl",

	"models/combine_soldier.mdl",
	"models/combine_soldier_prisonguard.mdl",
	"models/combine_super_soldier.mdl",
	"models/police.mdl",
};


class CTraceFilterIgnoreFriendlyCombatItems : public CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterIgnoreFriendlyCombatItems, CTraceFilterSimple );

	CTraceFilterIgnoreFriendlyCombatItems( const CHL2MPBot *player, int collisionGroup, int iIgnoreTeam, bool bIsProjectile = false )
		: CTraceFilterSimple( player, collisionGroup ), m_iIgnoreTeam( iIgnoreTeam ), m_pPlayer( player )
	{
		m_bCallerIsProjectile = bIsProjectile;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask )
	{
		CBaseEntity *pEntity = EntityFromEntityHandle( pServerEntity );

		if ( pEntity->IsCombatItem() )
		{
			if ( pEntity->GetTeamNumber() == m_iIgnoreTeam )
				return false;

			// If source is a enemy projectile, be explicit, otherwise we fail a "IsTransparent" test downstream
			if ( m_bCallerIsProjectile )
				return true;
		}

		// misyl: Ignore any held props, as if we can see through them...
		if ( !pEntity->BlocksLOS() )
			return false;

		IPhysicsObject* pObject = pEntity->VPhysicsGetObject();
		if ( pObject && ( pObject->GetGameFlags() & FVPHYSICS_PLAYER_HELD ) )
			return false;

		return BaseClass::ShouldHitEntity( pServerEntity, contentsMask );
	}

	const CHL2MPBot *m_pPlayer;
	int m_iIgnoreTeam;
	bool m_bCallerIsProjectile;
};

#endif // HL2MP_BOT_H

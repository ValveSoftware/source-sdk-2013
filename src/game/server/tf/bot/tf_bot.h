//========= Copyright Valve Corporation, All rights reserved. ============//
// tf_bot.h
// Team Fortress NextBot
// Michael Booth, February 2009

#ifndef TF_BOT_H
#define TF_BOT_H

#include "Player/NextBotPlayer.h"
#include "../nav_mesh/tf_nav_mesh.h"
#include "tf_bot_vision.h"
#include "tf_bot_body.h"
#include "tf_bot_locomotion.h"
#include "tf_player.h"
#include "tf_bot_squad.h"
#include "bot/map_entities/tf_bot_proxy.h"
#include "tf_gamerules.h"
#include "entity_capture_flag.h"
#include "func_capture_zone.h"
#include "nav_entities.h"
#include "utlstack.h"
#include "bot/map_entities/tf_bot_generator.h"		// action point

#define TF_BOT_TYPE	1337

class CTriggerAreaCapture;
class CTFBotActionPoint;
class CObjectSentrygun;
class CTFBotGenerator;

extern void BotGenerateAndWearItem( CTFPlayer *pBot, const char *itemName );

//----------------------------------------------------------------------------
// These must remain in sync with the bot_generator's spawnflags in tf.fgd:
#define TFBOT_IGNORE_ENEMY_SCOUTS		0x0001
#define TFBOT_IGNORE_ENEMY_SOLDIERS		0x0002
#define TFBOT_IGNORE_ENEMY_PYROS		0x0004
#define TFBOT_IGNORE_ENEMY_DEMOMEN		0x0008
#define TFBOT_IGNORE_ENEMY_HEAVIES		0x0010
#define TFBOT_IGNORE_ENEMY_MEDICS		0x0020
#define TFBOT_IGNORE_ENEMY_ENGINEERS	0x0040
#define TFBOT_IGNORE_ENEMY_SNIPERS		0x0080
#define TFBOT_IGNORE_ENEMY_SPIES		0x0100
#define TFBOT_IGNORE_ENEMY_SENTRY_GUNS	0x0200
#define TFBOT_IGNORE_SCENARIO_GOALS		0x0400

#define TFBOT_ALL_BEHAVIOR_FLAGS		0xFFFF

#define TFBOT_MVM_MAX_PATH_LENGTH		0.0f // 7000.0f			// in MvM, all pathfinds are limited to this (0 == no limit)


//----------------------------------------------------------------------------
class CTFBot: public NextBotPlayer< CTFPlayer >, public CGameEventListener
{
public:
	DECLARE_CLASS( CTFBot, NextBotPlayer< CTFPlayer > );

	DECLARE_ENT_SCRIPTDESC();

	CTFBot();
	virtual ~CTFBot();

	virtual void		Spawn();
	virtual void		FireGameEvent( IGameEvent *event );
	virtual void		Event_Killed( const CTakeDamageInfo &info );
	virtual void		PhysicsSimulate( void );
	virtual void		Touch( CBaseEntity *pOther );
	virtual void		AvoidPlayers( CUserCmd *pCmd );				// some game types allow players to pass through each other, this method pushes them apart
	virtual void		UpdateOnRemove( void );
	virtual int			ShouldTransmit( const CCheckTransmitInfo *pInfo ) OVERRIDE;
	virtual void		ChangeTeam( int iTeamNum, bool bAutoTeam, bool bSilent, bool bAutoBalance = false ) OVERRIDE;
	virtual bool		ShouldGib( const CTakeDamageInfo &info ) OVERRIDE;

	virtual int			DrawDebugTextOverlays(void);

	virtual bool IsAllowedToPickUpFlag( void ) const;

	virtual void		InitClass( void );				// set health/etc 
	void				ModifyMaxHealth( int nNewMaxHealth, bool bSetCurrentHealth = true, bool bAllowModelScaling = true );

	virtual int GetBotType( void ) const;				// return a unique int representing the type of bot instance this is

	virtual CTFNavArea *GetLastKnownArea( void ) const		{ return static_cast< CTFNavArea * >( BaseClass::GetLastKnownArea() ); }		// return the last nav area the player occupied - NULL if unknown

	// NextBotPlayer
	static CBasePlayer *AllocatePlayerEntity( edict_t *pEdict, const char *playerName );

	virtual void PressFireButton( float duration = -1.0f ) OVERRIDE;
	virtual void PressAltFireButton( float duration = -1.0f ) OVERRIDE;
	virtual void PressSpecialFireButton( float duration = -1.0f ) OVERRIDE;

	// INextBot
	virtual CTFBotLocomotion	*GetLocomotionInterface( void ) const	{ return m_locomotor; }
	virtual CTFBotBody			*GetBodyInterface( void ) const			{ return m_body; }
	virtual CTFBotVision		*GetVisionInterface( void ) const		{ return m_vision; }
	DECLARE_INTENTION_INTERFACE( CTFBot );

	virtual bool IsDormantWhenDead( void ) const;			// should this player-bot continue to update itself when dead (respawn logic, etc)

	virtual void OnWeaponFired( CBaseCombatCharacter *whoFired, CBaseCombatWeapon *weapon );		// when someone fires their weapon

	virtual bool IsDebugFilterMatch( const char *name ) const;	// return true if we match the given debug symbol

	virtual int	GetAllowedTauntPartnerTeam() const OVERRIDE { return GetTeamNumber(); }

	// CTFBot specific
	CTeamControlPoint *GetMyControlPoint( void ) const;				// return point we want to capture, or need to defend
	void ClearMyControlPoint( void );
	bool WasPointJustLost( void ) const;							// return true if we just lost territory recently
	bool AreAllPointsUncontestedSoFar( void ) const;				// return true if no enemy has contested any point yet
	bool IsPointBeingCaptured( CTeamControlPoint *point ) const;	// return true if the given point is being captured
	bool IsAnyPointBeingCaptured( void ) const;						// return true if any point is being captured
	bool IsNearPoint( CTeamControlPoint *point ) const;				// return true if we are within a short travel distance of the current point
	float GetTimeLeftToCapture( void ) const;						// return time left to capture the point before we lose the game

	CCaptureFlag *GetFlagToFetch( void ) const;						// return flag we want to fetch
	CCaptureZone *GetFlagCaptureZone( void ) const;					// return capture zone for our flag(s)

	struct SniperSpotInfo
	{
		CTFNavArea *m_vantageArea;
		Vector m_vantageSpot;

		CTFNavArea *m_theaterArea;
		Vector m_theaterSpot;

		float m_range;
		float m_advantage;				// the difference in how long it takes us to reach our vantage spot vs them to reach the theater spot
	};

	void AccumulateSniperSpots( void );									// find good sniping spots and store them
	const CUtlVector< SniperSpotInfo > *GetSniperSpots( void ) const;	// return vector of good sniping positions
	bool HasSniperSpots( void ) const;
	void ClearSniperSpots( void );

	// search outwards from startSearchArea and collect all reachable objects from the given list that pass the given filter
	void SelectReachableObjects( const CUtlVector< CHandle< CBaseEntity > > &candidateObjectVector, CUtlVector< CHandle< CBaseEntity > > *selectedObjectVector, const INextBotFilter &filter, CNavArea *startSearchArea, float maxRange = 2000.0f ) const;
	CBaseEntity *FindClosestReachableObject( const char *objectName, CNavArea *from, float maxRange = 2000.0f ) const;

	CTFNavArea *GetSpawnArea( void ) const;							// get area where we spawned in
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

	CTFNavArea *FindVantagePoint( float maxTravelDistance = 2000.0f ) const;	// return a nearby area where we can see a member of the enemy team
	HSCRIPT ScriptFindVantagePoint( float maxTravelDistance ) { return ToHScript( this->FindVantagePoint( maxTravelDistance ) ); }

	bool GetWeightDesiredClassToSpawn( CUtlVector< ETFClass > &vecClassToSpawn ) const;	// return true if class in the output vector is required
	ETFClass GetPresetClassToSpawn() const;	// return next class from preset table to spawn
	bool CanChangeClass() const;
	
	const char *GetNextSpawnClassname( void ) const;

	float GetThreatDanger( CBaseCombatCharacter *who ) const;		// return perceived danger of threat (0=none, 1=immediate deadly danger)
	float GetMaxAttackRange( void ) const;							// return the max range at which we can effectively attack
	float GetDesiredAttackRange( void ) const;						// return the ideal range at which we can effectively attack

	bool EquipRequiredWeapon( void );								// if we're required to equip a specific weapon, do it.
	void EquipBestWeaponForThreat( const CKnownEntity *threat );	// equip the best weapon we have to attack the given threat
	bool EquipLongRangeWeapon( void );								// equip a weapon that can damage far-away targets

	void PushRequiredWeapon( CTFWeaponBase *weapon );				// force us to equip and use this weapon until popped off the required stack
	void PopRequiredWeapon( void );									// pop top required weapon off of stack and discard

	#define MY_CURRENT_GUN NULL										// can be passed as weapon to following queries
	bool IsCombatWeapon( CTFWeaponBase *weapon ) const;				// return true if given weapon can be used to attack
	bool IsHitScanWeapon( CTFWeaponBase *weapon ) const;			// return true if given weapon is a "hitscan" weapon (scattered tracelines with instant damage)
	bool IsContinuousFireWeapon( CTFWeaponBase *weapon ) const;		// return true if given weapon "sprays" bullets/fire/etc continuously (ie: not individual rockets/etc)
	bool IsExplosiveProjectileWeapon( CTFWeaponBase *weapon ) const;// return true if given weapon launches explosive projectiles with splash damage
	bool IsBarrageAndReloadWeapon( CTFWeaponBase *weapon ) const;	// return true if given weapon has small clip and long reload cost (ie: rocket launcher, etc)
	bool IsQuietWeapon( CTFWeaponBase *weapon ) const;				// return true if given weapon doesn't make much sound when used (ie: spy knife, etc)

	bool IsEnvironmentNoisy( void ) const;							// return true if there are/have been loud noises (ie: non-quiet weapons) nearby very recently

	enum WeaponRestrictionType
	{
		ANY_WEAPON		= 0,
		MELEE_ONLY		= 0x0001,
		PRIMARY_ONLY	= 0x0002,
		SECONDARY_ONLY	= 0x0004,
	};
	void ClearWeaponRestrictions( void );
	void SetWeaponRestriction( int restrictionFlags );
	void RemoveWeaponRestriction( int restrictionFlags );
	bool HasWeaponRestriction( int restrictionFlags ) const;
	bool IsWeaponRestricted( CTFWeaponBase *weapon ) const;
	bool ScriptIsWeaponRestricted( HSCRIPT script ) const;

	bool ShouldFireCompressionBlast( void );

	bool IsLineOfFireClear( const Vector &where ) const;			// return true if a weapon has no obstructions along the line from our eye to the given position
	bool IsLineOfFireClear( CBaseEntity *who ) const;				// return true if a weapon has no obstructions along the line from our eye to the given entity
	bool IsLineOfFireClear( const Vector &from, const Vector &to ) const;			// return true if a weapon has no obstructions along the line between the given points
	bool IsLineOfFireClear( const Vector &from, CBaseEntity *who ) const;			// return true if a weapon has no obstructions along the line between the given point and entity

	bool IsEntityBetweenTargetAndSelf( CBaseEntity *other, CBaseEntity *target );	// return true if "other" is positioned inbetween us and "target"

	class SuspectedSpyInfo_t
	{
	public:
		bool IsCurrentlySuspected();
		void Suspect(); // The verb form of the word, not the noun.
		bool TestForRealizing();
		CHandle< CTFPlayer > m_suspectedSpy;

	private:

		
		CUtlVector< int > m_touchTimes;
	};

	bool IsKnownSpy( CTFPlayer *player ) const;				// return true if we are sure this player actually is an enemy spy
	SuspectedSpyInfo_t* IsSuspectedSpy( CTFPlayer *player );			// return true if we suspect this player might be an enemy spy
	void SuspectSpy( CTFPlayer *player );					// note that this player might be a spy
	void RealizeSpy( CTFPlayer *player );					// note that this player *IS* a spy
	void ForgetSpy( CTFPlayer *player );					// remove player from spy suspect system
	void StopSuspectingSpy( CTFPlayer *pPlayer );

	CTFPlayer *GetClosestHumanLookingAtMe( int team = TEAM_ANY ) const;	// return the nearest human player on the given team who is looking directly at me

	enum AttributeType
	{
		REMOVE_ON_DEATH				= 1<<0,					// kick bot from server when killed
		AGGRESSIVE					= 1<<1,					// in MvM mode, push for the cap point
		IS_NPC						= 1<<2,					// a non-player support character
		SUPPRESS_FIRE				= 1<<3,
		DISABLE_DODGE				= 1<<4,
		BECOME_SPECTATOR_ON_DEATH	= 1<<5,					// move bot to spectator team when killed
		QUOTA_MANANGED				= 1<<6,					// managed by the bot quota in CTFBotManager 
		RETAIN_BUILDINGS			= 1<<7,					// don't destroy this bot's buildings when it disconnects
		SPAWN_WITH_FULL_CHARGE		= 1<<8,					// all weapons start with full charge (ie: uber)
		ALWAYS_CRIT					= 1<<9,					// always fire critical hits
		IGNORE_ENEMIES				= 1<<10,
		HOLD_FIRE_UNTIL_FULL_RELOAD	= 1<<11,				// don't fire our barrage weapon until it is full reloaded (rocket launcher, etc)
		PRIORITIZE_DEFENSE			= 1<<12,				// bot prioritizes defending when possible
		ALWAYS_FIRE_WEAPON			= 1<<13,				// constantly fire our weapon
		TELEPORT_TO_HINT			= 1<<14,				// bot will teleport to hint target instead of walking out from the spawn point
		MINIBOSS					= 1<<15,				// is miniboss?
		USE_BOSS_HEALTH_BAR			= 1<<16,				// should I use boss health bar?
		IGNORE_FLAG					= 1<<17,				// don't pick up flag/bomb
		AUTO_JUMP					= 1<<18,				// auto jump
		AIR_CHARGE_ONLY				= 1<<19,				// demo knight only charge in the air
		PREFER_VACCINATOR_BULLETS	= 1<<20,				// When using the vaccinator, prefer to use the bullets shield
		PREFER_VACCINATOR_BLAST		= 1<<21,				// When using the vaccinator, prefer to use the blast shield
		PREFER_VACCINATOR_FIRE		= 1<<22,				// When using the vaccinator, prefer to use the fire shield
		BULLET_IMMUNE				= 1<<23,				// Has a shield that makes the bot immune to bullets
		BLAST_IMMUNE				= 1<<24,				// "" blast
		FIRE_IMMUNE					= 1<<25,				// "" fire
		PARACHUTE					= 1<<26,				// demo/soldier parachute when falling
		PROJECTILE_SHIELD			= 1<<27,				// medic projectile shield
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

	void SetHomeArea( CTFNavArea *area );
	CTFNavArea *GetHomeArea( void ) const;
	void ScriptSetHomeArea( HSCRIPT hScript ) { this->SetHomeArea( ToNavArea( hScript ) ); }
	HSCRIPT ScriptGetHomeArea( void ) { return ToHScript( this->GetHomeArea() ); }

	CObjectSentrygun *GetEnemySentry( void ) const;			// if we've been attacked/killed by an enemy sentry, this will return it, otherwise NULL
	void RememberEnemySentry( CObjectSentrygun *sentry, const Vector &injurySpot );
	const Vector &GetSpotWhereEnemySentryLastInjuredMe( void ) const;

	void SetActionPoint( CTFBotActionPoint *point );
	CTFBotActionPoint *GetActionPoint( void ) const;
	void ScriptSetActionPoint( HSCRIPT hPoint ) { SetActionPoint( ScriptToEntClass< CTFBotActionPoint >( hPoint ) ); }
	HSCRIPT ScriptGetActionPoint( void ) const { return ToHScript( GetActionPoint() ); }

	bool HasProxy( void ) const;
	void SetProxy( CTFBotProxy *proxy );					// attach this bot to a bot_proxy entity for map I/O communications
	CTFBotProxy *GetProxy( void ) const;

	bool HasSpawner( void ) const;
	void SetSpawner( CTFBotGenerator *spawner );
	CTFBotGenerator *GetSpawner( void ) const;

	void JoinSquad( CTFBotSquad *squad );					// become a member of the given squad
	void LeaveSquad( void );								// leave our current squad
	void DeleteSquad( void );
	bool IsInASquad( void ) const;
	bool IsSquadmate( CTFPlayer *who ) const;				// return true if given bot is in my squad
	CTFBotSquad *GetSquad( void ) const;					// return squad we are in, or NULL
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

	void GiveRandomItem( loadout_positions_t loadoutPosition );
	void ScriptGenerateAndWearItem( const char *pszItemName ) { if ( pszItemName ) BotGenerateAndWearItem( this, pszItemName ); }

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

	void DisguiseAsMemberOfEnemyTeam( void );		// set Spy disguise to be a class that someone on the enemy team is actually using
	CBaseObject *GetNearestKnownSappableTarget( void );
	HSCRIPT ScriptGetNearestKnownSappableTarget( void ) { return ToHScript( this->GetNearestKnownSappableTarget() ); }

	void ClearTags( void );
	void AddTag( const char *tag );
	void RemoveTag( const char *tag );
	bool HasTag( const char *tag );
	void ScriptGetAllTags( HSCRIPT hTable );

	Action< CTFBot > *OpportunisticallyUseWeaponAbilities( void );

	CTFPlayer *SelectRandomReachableEnemy( void );	// mostly for MvM - pick a random enemy player that is not in their spawn room

	float GetDesiredPathLookAheadRange( void ) const;	// different sized bots used different lookahead distances

	void StartIdleSound( void );
	void StopIdleSound( void );

	bool ShouldQuickBuild() const { return m_bForceQuickBuild; }
	void SetShouldQuickBuild( bool bShouldQuickBuild ) { m_bForceQuickBuild = bShouldQuickBuild; }

	void SetAutoJump( float flAutoJumpMin, float flAutoJumpMax ) { m_flAutoJumpMin = flAutoJumpMin; m_flAutoJumpMax = flAutoJumpMax; }
	bool ShouldAutoJump();

	void SetFlagTarget( CCaptureFlag* pFlag );
	CCaptureFlag* GetFlagTarget() const { return m_hFollowingFlagTarget; }
	bool HasFlagTaget() const { return m_hFollowingFlagTarget != NULL; }

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

			m_itemsAttributes = copy.m_itemsAttributes;
			m_characterAttributes = copy.m_characterAttributes;

			for ( int i=0; i<copy.m_tags.Count(); ++i )
			{
				m_tags.CopyAndAddToTail( copy.m_tags[i] );
			}
		}

		void Reset()
		{
			m_eventName = "default";
			
			m_skill = CTFBot::EASY;
			m_weaponRestriction = CTFBot::ANY_WEAPON;
			m_mission = CTFBot::NO_MISSION;
			m_prevMission = m_mission;
			m_attributeFlags = 0;
			m_maxVisionRange = -1.f;

			m_items.RemoveAll();
			
			m_itemsAttributes.RemoveAll();
			m_characterAttributes.RemoveAll();
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

		struct item_attributes_t
		{
			CUtlString m_itemName;
			CCopyableUtlVector< static_attrib_t > m_attributes;
		};
		CUtlVector< item_attributes_t > m_itemsAttributes;
		CUtlVector< static_attrib_t >	m_characterAttributes;
		CUtlStringList m_tags;
	};
	void ClearEventChangeAttributes() { m_eventChangeAttributes.RemoveAll(); }
	void AddEventChangeAttributes( const EventChangeAttributes_t* newEvent );
	const EventChangeAttributes_t* GetEventChangeAttributes( const char* pszEventName ) const;
	void OnEventChangeAttributes( const CTFBot::EventChangeAttributes_t* pEvent );

	void AddItem( const char* pszItemName );

	int GetUberHealthThreshold();
	float GetUberDeployDelayDuration();

	bool ShouldReEvaluateCurrentClass( void ) const;
	void ReEvaluateCurrentClass( void );

private:
	CTFBotLocomotion	*m_locomotor;
	CTFBotBody			*m_body;
	CTFBotVision		*m_vision;

	CountdownTimer m_lookAtEnemyInvasionAreasTimer;

	CTFNavArea *m_spawnArea;			// where we spawned
	CountdownTimer m_justLostPointTimer;
	
	int m_weaponRestrictionFlags;
	int m_attributeFlags;
	DifficultyType m_difficulty;

	CTFNavArea *m_homeArea;

	CHandle< CTFBotActionPoint > m_actionPoint;
	CHandle< CTFBotProxy > m_proxy;
	CHandle< CTFBotGenerator > m_spawner;

	CTFBotSquad *m_squad;
	bool m_didReselectClass;

	CHandle< CObjectSentrygun > m_enemySentry;
	Vector m_spotWhereEnemySentryLastInjuredMe;			// the last position where I was injured by an enemy sentry

	CUtlVector< SuspectedSpyInfo_t* > m_suspectedSpyVector;
	CUtlVector< CHandle< CTFPlayer > > m_knownSpyVector;

	CUtlVector< SniperSpotInfo > m_sniperSpotVector;	// collection of good sniping spots for the current objective

	CUtlVector< CTFNavArea * > m_sniperVantageAreaVector;
	CUtlVector< CTFNavArea * > m_sniperTheaterAreaVector;

	CBaseEntity *m_snipingGoalEntity;					// the entity we are guarding (control point, payload cart)
	Vector m_lastSnipingGoalEntityPosition;

	void SetupSniperSpotAccumulation( void );			// do internal setup when control point changes
	CountdownTimer m_retrySniperSpotSetupTimer;

	bool m_isLookingAroundForEnemies;

	unsigned int m_behaviorFlags;						// spawnflags from the bot_generator that spawned us
	CUtlVector< CFmtStr > m_tags;

	CHandle< CBaseEntity > m_attentionFocusEntity;

	CTeamControlPoint *SelectPointToCapture( CUtlVector< CTeamControlPoint * > *captureVector ) const;
	CTeamControlPoint *SelectPointToDefend( CUtlVector< CTeamControlPoint * > *defendVector ) const;
	mutable CHandle< CTeamControlPoint > m_myControlPoint;
	mutable CountdownTimer m_evaluateControlPointTimer;

	float m_fModelScaleOverride;

	MissionType m_mission;
	MissionType m_prevMission;

	CHandle< CBaseEntity > m_missionTarget;
	CUtlString m_missionString;

	CUtlStack< CHandle<CTFWeaponBase> > m_requiredWeaponStack;	// if non-empty, bot must equip the weapon on top of the stack

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

	CHandle< CCaptureFlag > m_hFollowingFlagTarget;

	CUtlVector< const EventChangeAttributes_t* > m_eventChangeAttributes;
};


inline void CTFBot::SetTeleportWhere( const CUtlStringList& teleportWhereName )
{
	// deep copy strings
	for ( int i=0; i<teleportWhereName.Count(); ++i )
	{
		m_teleportWhereName.CopyAndAddToTail( teleportWhereName[i] );
	}
}

inline const CUtlStringList& CTFBot::GetTeleportWhere()
{
	return m_teleportWhereName;
}

inline void CTFBot::ClearTeleportWhere()
{
	m_teleportWhereName.RemoveAll();
}

inline void CTFBot::SetMissionString( CUtlString string )
{
	m_missionString = string;
}

inline CUtlString *CTFBot::GetMissionString( void )
{
	return &m_missionString;
}

inline void CTFBot::SetMissionTarget( CBaseEntity *target )
{
	m_missionTarget = target;
}

inline CBaseEntity *CTFBot::GetMissionTarget( void ) const
{
	return m_missionTarget;
}

inline float CTFBot::GetSquadFormationError( void ) const
{
	return m_squadFormationError;
}

inline void CTFBot::SetSquadFormationError( float error )
{
	m_squadFormationError = error;
}

inline bool CTFBot::HasBrokenFormation( void ) const
{
	return m_hasBrokenFormation;
}

inline void CTFBot::SetBrokenFormation( bool state )
{
	m_hasBrokenFormation = state;
}

inline void CTFBot::SetMaxVisionRangeOverride( float range )
{
	m_maxVisionRangeOverride = range;
}

inline float CTFBot::GetMaxVisionRangeOverride( void ) const
{
	return m_maxVisionRangeOverride;
}

inline void CTFBot::SetBehaviorFlag( unsigned int flags )
{
	m_behaviorFlags |= flags;
}

inline void CTFBot::ClearBehaviorFlag( unsigned int flags )
{
	m_behaviorFlags &= ~flags;
}

inline bool CTFBot::IsBehaviorFlagSet( unsigned int flags ) const
{
	return ( m_behaviorFlags & flags ) ? true : false;
}

inline void CTFBot::StartLookingAroundForEnemies( void )
{
	m_isLookingAroundForEnemies = true;
}

inline void CTFBot::StopLookingAroundForEnemies( void )
{
	m_isLookingAroundForEnemies = false;
}

inline int CTFBot::GetBotType( void ) const
{
	return TF_BOT_TYPE;
}

inline void CTFBot::RememberEnemySentry( CObjectSentrygun *sentry, const Vector &injurySpot )
{
	m_enemySentry = sentry;
	m_spotWhereEnemySentryLastInjuredMe = injurySpot;
}

inline CObjectSentrygun *CTFBot::GetEnemySentry( void ) const
{
	return m_enemySentry;
}

inline const Vector &CTFBot::GetSpotWhereEnemySentryLastInjuredMe( void ) const
{
	return m_spotWhereEnemySentryLastInjuredMe;
}

inline CTFBot::DifficultyType CTFBot::GetDifficulty( void ) const
{
	return m_difficulty;
}

inline void CTFBot::SetDifficulty( CTFBot::DifficultyType difficulty )
{
	m_difficulty = difficulty;

	m_nBotSkill = m_difficulty;
}

inline bool CTFBot::IsDifficulty( DifficultyType skill ) const
{
	return skill == m_difficulty;
}

inline bool CTFBot::HasProxy( void ) const
{
	return m_proxy == NULL ? false : true;
}

inline void CTFBot::SetProxy( CTFBotProxy *proxy )
{
	m_proxy = proxy;
}

inline CTFBotProxy *CTFBot::GetProxy( void ) const
{
	return m_proxy;
}

inline bool CTFBot::HasSpawner( void ) const
{
	return m_spawner == NULL ? false : true;
}

inline void CTFBot::SetSpawner( CTFBotGenerator *spawner )
{
	m_spawner = spawner;
}

inline CTFBotGenerator *CTFBot::GetSpawner( void ) const
{
	return m_spawner;
}

inline void CTFBot::SetActionPoint( CTFBotActionPoint *point )
{
	m_actionPoint = point;
}

inline CTFBotActionPoint *CTFBot::GetActionPoint( void ) const
{
	return m_actionPoint;
}

inline bool CTFBot::IsInASquad( void ) const
{
	return m_squad == NULL ? false : true;
}

inline CTFBotSquad *CTFBot::GetSquad( void ) const
{
	return m_squad;
}

inline void CTFBot::SetHomeArea( CTFNavArea *area )
{
	m_homeArea = area;
}

inline CTFNavArea *CTFBot::GetHomeArea( void ) const
{
	return m_homeArea;
}

inline void CTFBot::ClearWeaponRestrictions( void )
{
	m_weaponRestrictionFlags = 0;
}

inline void CTFBot::SetWeaponRestriction( int restrictionFlags )
{
	m_weaponRestrictionFlags |= restrictionFlags;
}

inline void CTFBot::RemoveWeaponRestriction( int restrictionFlags )
{
	m_weaponRestrictionFlags &= ~restrictionFlags;
}

inline bool CTFBot::HasWeaponRestriction( int restrictionFlags ) const
{
	return m_weaponRestrictionFlags & restrictionFlags ? true : false;
}

inline void CTFBot::SetAttribute( int attributeFlag )
{
	m_attributeFlags |= attributeFlag;
}

inline void CTFBot::ClearAttribute( int attributeFlag )
{
	m_attributeFlags &= ~attributeFlag;
}

inline void CTFBot::ClearAllAttributes()
{
	m_attributeFlags = 0;
}

inline bool CTFBot::HasAttribute( int attributeFlag ) const
{
	return m_attributeFlags & attributeFlag ? true : false;
}

inline CTFNavArea *CTFBot::GetSpawnArea( void ) const
{
	return m_spawnArea;
}

inline bool CTFBot::WasPointJustLost( void ) const
{
	return m_justLostPointTimer.HasStarted() && !m_justLostPointTimer.IsElapsed();
}

inline const CUtlVector< CTFBot::SniperSpotInfo > *CTFBot::GetSniperSpots( void ) const
{
	return &m_sniperSpotVector;
}

inline bool CTFBot::HasSniperSpots( void ) const
{
	return m_sniperSpotVector.Count() > 0 ? true : false;
}

inline CTFBot::MissionType CTFBot::GetMission( void ) const
{
	return m_mission;
}

inline void CTFBot::SetPrevMission( MissionType mission )
{
	m_prevMission = mission;
}

inline CTFBot::MissionType CTFBot::GetPrevMission( void ) const
{
	return m_prevMission;
}

inline bool CTFBot::HasMission( MissionType mission ) const
{
	return m_mission == mission ? true : false;
}

inline bool CTFBot::IsOnAnyMission( void ) const
{
	return m_mission == NO_MISSION ? false : true;
}

inline void CTFBot::SetScaleOverride( float fScale )
{
	m_fModelScaleOverride = fScale;

	SetModelScale( m_fModelScaleOverride > 0.0f ? m_fModelScaleOverride : 1.0f );
}

inline bool CTFBot::IsEnvironmentNoisy( void ) const
{
	return !m_noisyTimer.IsElapsed();
}

//---------------------------------------------------------------------------------------------
inline CTFBot *ToTFBot( CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() || !ToTFPlayer( pEntity )->IsBotOfType( TF_BOT_TYPE ) )
		return NULL;

	Assert( "***IMPORTANT!!! DONT IGNORE ME!!!***" && dynamic_cast< CTFBot * >( pEntity ) != 0 );

	return static_cast< CTFBot * >( pEntity );
}


//---------------------------------------------------------------------------------------------
inline const CTFBot *ToTFBot( const CBaseEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() || !ToTFPlayer( const_cast< CBaseEntity * >( pEntity ) )->IsBotOfType( TF_BOT_TYPE ) )
		return NULL;

	Assert( "***IMPORTANT!!! DONT IGNORE ME!!!***" && dynamic_cast< const CTFBot * >( pEntity ) != 0 );

	return static_cast< const CTFBot * >( pEntity );
}


//--------------------------------------------------------------------------------------------------------------
/**
 * Functor used with NavAreaBuildPath()
 */
class CTFBotPathCost : public IPathCost
{
public:
	CTFBotPathCost( CTFBot *me, RouteType routeType )
	{
		m_me = me;
		m_routeType = routeType;
		m_stepHeight = me->GetLocomotionInterface()->GetStepHeight();
		m_maxJumpHeight = me->GetLocomotionInterface()->GetMaxJumpHeight();
		m_maxDropHeight = me->GetLocomotionInterface()->GetDeathDropHeight();
	}

	virtual float operator()( CNavArea *baseArea, CNavArea *fromArea, const CNavLadder *ladder, const CFuncElevator *elevator, float length ) const
	{
		VPROF_BUDGET( "CTFBotPathCost::operator()", "NextBot" );

		CTFNavArea *area = (CTFNavArea *)baseArea;

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

			// in training, avoid capturing the point until the human trainee does so
			if ( TFGameRules()->IsInTraining() && 
				 area->HasAttributeTF( TF_NAV_CONTROL_POINT ) && 
				 !m_me->IsAnyPointBeingCaptured() &&
				 !m_me->IsPlayerClass( TF_CLASS_ENGINEER ) )	// allow engineers to path so they can test travel distance for sentry placement
			{
				return -1.0f;
			}

			// don't path through enemy spawn rooms
			if ( ( m_me->GetTeamNumber() == TF_TEAM_RED && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_BLUE ) ) ||
				 ( m_me->GetTeamNumber() == TF_TEAM_BLUE && area->HasAttributeTF( TF_NAV_SPAWN_ROOM_RED ) ) )
			{
				if ( !TFGameRules()->RoundHasBeenWon() )
				{
					return -1.0f;
				}
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

			if ( m_routeType == DEFAULT_ROUTE && !m_me->IsMiniBoss() ) 
			{
				// this term causes the same bot to choose different routes over time,
				// but keep the same route for a period in case of repaths
				int timeMod = (int)( gpGlobals->curtime / 10.0f ) + 1;
				preference = 1.0f + 50.0f * ( 1.0f + FastCos( (float)( m_me->GetEntity()->entindex() * area->GetID() * timeMod ) ) );
			}

			if ( m_routeType == SAFEST_ROUTE )
			{
				// avoid combat areas
				if ( area->IsInCombat() )
				{
					const float combatDangerCost = 4.0f;
					dist *= combatDangerCost * area->GetCombatIntensity();
				}

				// if this area exposes us to enemy sentry fire, avoid it
				const float sentryDangerCost = 5.0f;
				if ( ( m_me->GetTeamNumber() == TF_TEAM_RED && area->HasAttributeTF( TF_NAV_BLUE_SENTRY_DANGER ) ) ||
					 ( m_me->GetTeamNumber() == TF_TEAM_BLUE && area->HasAttributeTF( TF_NAV_RED_SENTRY_DANGER ) ) )
				{
					dist *= sentryDangerCost;
				}
			}

			if ( m_me->IsPlayerClass( TF_CLASS_SPY ) )
			{
				int enemyTeam = GetEnemyTeam( m_me->GetTeamNumber() );

				// Since spies can get right up to enemy buildings, avoid them.
				for ( int oit = 0; oit < IBaseObjectAutoList::AutoList().Count(); ++oit )
				{
					CBaseObject *enemyObj = static_cast< CBaseObject* >( IBaseObjectAutoList::AutoList()[ oit ] );

					if ( ( enemyObj->ObjectType() == OBJ_SENTRYGUN ) &&
						( enemyObj->GetTeamNumber() == enemyTeam ) )
					{
						enemyObj->UpdateLastKnownArea();

						if ( enemyObj->GetLastKnownArea() == area )
						{
							// There is an enemy building in this area - avoid it as a spy.
							const float enemyBuildingCost = 10.0f;
							dist *= enemyBuildingCost;
						}
					}
				}

				// Spies avoid teammates, since they draw attention and gunfire.
				const float teammateCost = 10.0f;
				dist += dist * teammateCost * area->GetPlayerCount( m_me->GetTeamNumber() );

				// We shouldn't be getting NaNs here. It will be handled when we return, but ideally
				//  it should be fixed here and not just worked around in NavAreaBuildPath.
				DebuggerBreakOnNaN_StagingOnly( dist );
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

	CTFBot *m_me;
	RouteType m_routeType;
	float m_stepHeight;
	float m_maxJumpHeight;
	float m_maxDropHeight;
};


//---------------------------------------------------------------------------------------------
class CClosestTFPlayer
{
public:
	CClosestTFPlayer( const Vector &where, int team = TEAM_ANY )
	{
		m_where = where;
		m_closeRangeSq = FLT_MAX;
		m_closePlayer = NULL;
		m_team = team;
	}

	CClosestTFPlayer( CBaseEntity *entity, int team = TEAM_ANY )
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

		if ( player->GetTeamNumber() != TF_TEAM_RED && player->GetTeamNumber() != TF_TEAM_BLUE )
			return true;

		if ( m_team != TEAM_ANY && player->GetTeamNumber() != m_team )
			return true;

		CTFBot *bot = ToTFBot( player );
		if ( bot && bot->HasAttribute( CTFBot::IS_NPC ) )
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


#endif // TF_BOT_H

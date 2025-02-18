//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef FUNC_TANK_H
#define FUNC_TANK_H
#ifdef _WIN32
#pragma once
#endif

#include "triggers.h"

#define SF_TANK_ACTIVE						0x0001
#define SF_TANK_PLAYER						0x0002
#define SF_TANK_HUMANS						0x0004
#define SF_TANK_ALIENS						0x0008
#define SF_TANK_LINEOFSIGHT					0x0010
#define SF_TANK_CANCONTROL					0x0020
#define	SF_TANK_DAMAGE_KICK					0x0040	// Kick when take damage
#define	SF_TANK_AIM_AT_POS					0x0080	// Aim at a particular position
#define SF_TANK_AIM_ASSISTANCE				0x0100
#define SF_TANK_NPC							0x0200		
#define SF_TANK_NPC_CONTROLLABLE			0x0400		// 1024
#define SF_TANK_NPC_SET_CONTROLLER			0x0800		// 2048
#define SF_TANK_ALLOW_PLAYER_HITS			0x1000		// 4096		Allow friendly NPCs to fire upon enemies near the player
#define SF_TANK_IGNORE_RANGE_IN_VIEWCONE	0x2000		// 8192		Don't use range as a factor in determining if something is in view cone
#define SF_TANK_NOTSOLID					0x8000		// 32768
#define SF_TANK_SOUNDON						0x10000		// FIXME: This is not really a spawnflag! It holds transient state!!!
#define SF_TANK_HACKPLAYERHIT				0x20000		// 131072	Make this func_tank cheat and hit the player regularly

#define FUNCTANK_DISTANCE_MAX				1200			// 100 ft.
#define FUNCTANK_DISTANCE_MIN_TO_ENEMY		180
#define FUNCTANK_FIREVOLUME					1000
#define FUNCTANK_NPC_ROUTE_TIME				5.0f

// Effect handling
// If the func_tank has a chosen method of handling effects, use that 
// instead of the individual effect settings. (muzzleflash, sound, tracer, etc)
enum FUNCTANK_EFFECT_HANDLING
{
	EH_NONE,			// Use the effect settings
	EH_AR2,				// Use AR2 effects
	EH_COMBINE_CANNON	// Large Combine cannon
};

enum TANKBULLET
{
	TANK_BULLET_NONE	= 0,
	TANK_BULLET_SMALL	= 1,
	TANK_BULLET_MEDIUM	= 2,
	TANK_BULLET_LARGE	= 3,
};

#define MORTAR_BLAST_RADIUS	350


//			Custom damage
//			env_laser (duration is 0.5 rate of fire)
//			rockets
//			explosion?

class CFuncTank : public CBaseEntity
{

	DECLARE_CLASS( CFuncTank, CBaseEntity );

public:

			CFuncTank();
			~CFuncTank();
	void	Spawn( void );
	void	Activate( void );
	void	Precache( void );
	bool	CreateVPhysics( void );
	bool	KeyValue( const char *szKeyName, const char *szValue );
	void	UpdateOnRemove();

	void	SetYawRate( float flYawRate ) { m_yawRate = flYawRate; }
	void	SetPitchRate( float flPitchRate ) { m_pitchRate = flPitchRate; }

	int	ObjectCaps( void ) 
	{ 
		return ( BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS ); 
	}

	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	
	virtual	void FuncTankPreThink() { return; }
	void	Think( void );
	virtual	void FuncTankPostThink() { return; }

	int		GetAmmoCount( void )						{ return m_iAmmoCount; }

	// NPC
	bool	NPC_FindManPoint( Vector &vecPos );
	bool	NPC_HasEnemy( void );
	void	NPC_Fire( void );
	void	NPC_InterruptRoute( void );
	void	NPC_JustSawPlayer( CBaseEntity *pTarget );
	void	NPC_SetInRoute( bool bInRoute )				{ m_bNPCInRoute = bInRoute; }
	void	NPC_SetIdleAngle( Vector vecIdle )			{ m_vecNPCIdleTarget = vecIdle; }

	// LOS
	bool	IsEntityInViewCone( CBaseEntity *pEntity );
	bool	HasLOSTo( CBaseEntity *pEntity );

	// Controller
	CBaseCombatCharacter *GetController( void );
	bool StartControl( CBaseCombatCharacter *pController );
	void StopControl( void );
	Vector GetTargetPosition()						{ return m_vTargetPosition; }
	void SetTargetPosition( const Vector &vecPos )	{ m_vTargetPosition = vecPos; }

	const float YawCenter() const { return m_yawCenter; }
	const float YawCenterWorld() const { return m_yawCenterWorld; }
	const float YawRange() const { return m_yawRange; }
	const float PitchCenter() const { return m_pitchCenter; }
	const float PitchCenterWorld() const { return m_pitchCenterWorld; }
	const float PitchRange() const { return m_pitchRange; }

	virtual void PhysicsSimulate( void );

	virtual void OnStartControlled() {}
	virtual void OnStopControlled() {}

	// SF Tests.
	inline bool IsControllable( void )		{ return ( m_spawnflags & SF_TANK_CANCONTROL ) ? true : false; }
	inline bool IsActive( void )			{ return ( m_spawnflags & SF_TANK_ACTIVE ) ? true : false; }
	inline bool IsNPCControllable( void )	{ return ( m_spawnflags & SF_TANK_NPC_CONTROLLABLE ) ? true : false; }
	inline bool IsNPCSetController( void )	{ return ( m_spawnflags & SF_TANK_NPC_SET_CONTROLLER ) ? true : false; }

	virtual void DoMuzzleFlash( void );
	virtual const char *GetTracerType( void );

protected:
	virtual float GetShotSpeed() { return 0; }

	virtual Vector WorldBarrelPosition( void );
	void		UpdateMatrix( void );

	float GetNextAttack() const { return m_flNextAttack; }
	virtual void SetNextAttack( float flWait ) { m_flNextAttack = flWait; }

	virtual void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker, bool bIgnoreSpread );
	void		TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, trace_t &tr );
	int			GetRandomBurst( void );
	float		GetRandomFireTime( void );

	void	CalcPlayerCrosshairTarget( Vector *pVecTarget );
	void	CalcNPCEnemyTarget( Vector *pVecTarget );

	inline bool IsPlayerManned( void )	{ return m_hController && m_hController->IsPlayer() && ( m_spawnflags & SF_TANK_PLAYER ); }
	inline bool IsNPCManned( void )		{ return m_hController && m_hController->MyNPCPointer() && ( m_spawnflags & SF_TANK_NPC ); }

private:
	void	TrackTarget( void );
	int		DrawDebugTextOverlays(void);
	void	DrawDebugGeometryOverlays(void);

	virtual void FiringSequence( const Vector &barrelEnd, const Vector &forward, CBaseEntity *pAttacker );

	void	StartRotSound( void );
	void	StopRotSound( void );

	// Input handlers.
	void InputActivate( inputdata_t &inputdata );
	void InputDeactivate( inputdata_t &inputdata );
	void InputSetFireRate( inputdata_t &inputdata );
	void InputSetDamage( inputdata_t &inputdata );
	void InputSetTargetDir( inputdata_t &inputdata );
	void InputSetTargetPosition( inputdata_t &inputdata );
	void InputSetTargetEntityName( inputdata_t &inputdata );

protected:
	virtual void InputSetTargetEntity( inputdata_t &inputdata );
	virtual void InputClearTargetEntity( inputdata_t &inputdata );

private:
	void InputFindNPCToManTank( inputdata_t &inputdata );
	void InputStopFindingNPCs( inputdata_t &inputdata );
	void InputStartFindingNPCs( inputdata_t &inputdata );
	void InputForceNPCOff( inputdata_t &inputdata );
	void InputSetMaxRange( inputdata_t &inputdata );

	inline bool CanFire( void );
	bool		InRange( float range );
	bool		InRange2( float flRange2 );

	void		TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType);

	QAngle		AimBarrelAt( const Vector &parentTarget );

	DECLARE_DATADESC();

	bool OnControls( CBaseEntity *pTest );
	bool HasController( void );

	CBaseEntity *FindTarget( string_t targetName, CBaseEntity *pActivator );

	// NPC
	void		NPC_FindController( void );
	bool		NPC_InRoute( void )							{ return m_bNPCInRoute; }
	bool		NPC_InterruptController( void );

	// Aim the tank at the player crosshair 
	void AimBarrelAtPlayerCrosshair( QAngle *pAngles );

	// Aim the tank at the NPC's enemy
	void AimBarrelAtNPCEnemy( QAngle *pAngles );

	// Aim the tank at the func_tank's enemy
	void AimFuncTankAtTarget( void );

	// Returns true if the desired angles are out of range 
	bool RotateTankToAngles( const QAngle &angles, float *pDistX = NULL, float *pDistY = NULL );

	// We lost our target! 
	void LostTarget( void );

	// Purpose: 
	void ComputeLeadingPosition( const Vector &vecShootPosition, CBaseEntity *pTarget, Vector *pLeadPosition );

protected:
	virtual void ControllerPostFrame( void );

	virtual void TankActivate(void);
	virtual void TankDeactivate(void);

	float					m_fireLast;		// Last time I fired
	float					m_fireRate;		// How many rounds/second

	EHANDLE					m_hTarget;

	TANKBULLET				m_bulletType;	// Bullet type
	int						m_iBulletDamage; // 0 means use Bullet type's default damage
	int						m_iBulletDamageVsPlayer; // Damage vs player. 0 means use m_iBulletDamage

#ifdef HL2_EPISODIC
	string_t				m_iszAmmoType;		// The name of the ammodef that we use when we fire. Bullet damage still comes from keyvalues.
	int						m_iAmmoType;		// The cached index of the ammodef that we use when we fire.
#else
	int						m_iSmallAmmoType;
	int						m_iMediumAmmoType;
	int						m_iLargeAmmoType;
#endif // HL2_EPISODIC

	int						m_spread;		// firing spread

	EntityMatrix			m_parentMatrix;

	Vector					m_sightOrigin;	// Last sight of target
	EHANDLE					m_hFuncTankTarget;

	int						m_nBulletCount;

private:

	// This is either the player manning the func_tank, or an NPC. The NPC is either manning the tank, or running
	// to the man point. If he's en-route, m_bNPCInRoute will be true. 
	CHandle<CBaseCombatCharacter> m_hController;

	float					m_flNextAttack;
	Vector					m_vecControllerUsePos;
	
	float					m_yawCenter;	// "Center" yaw
	float					m_yawCenterWorld;	// "Center" yaw in world space
	float					m_yawRate;		// Max turn rate to track targets
	float					m_yawRange;		// Range of turning motion (one-sided: 30 is +/- 30 degress from center)
											// Zero is full rotation
	float					m_yawTolerance;	// Tolerance angle

	float					m_pitchCenter;	// "Center" pitch
	float					m_pitchCenterWorld;	// "Center" pitch in world space
	float					m_pitchRate;	// Max turn rate on pitch
	float					m_pitchRange;	// Range of pitch motion as above
	float					m_pitchTolerance;	// Tolerance angle

	float					m_fireTime;		// How much time has been used to fire the weapon so far.
	float					m_lastSightTime;// Last time I saw target
	float					m_persist;		// Persistence of firing (how long do I shoot when I can't see)
	float					m_persist2;		// Secondary persistence of firing (randomly shooting when I can't see)
	float					m_persist2burst;// How long secondary persistence burst lasts
	float					m_minRange;		// Minimum range to aim/track
	float					m_maxRange;		// Max range to aim/track
	float					m_flMinRange2;
	float					m_flMaxRange2;
	int						m_iAmmoCount;	// ammo 

	Vector					m_barrelPos;	// Length of the freakin barrel
	float					m_spriteScale;	// Scale of any sprites we shoot
	string_t				m_iszSpriteSmoke;
	string_t				m_iszSpriteFlash;

	string_t				m_iszMaster;	// Master entity (game_team_master or multisource)

	string_t				m_soundStartRotate;
	string_t				m_soundStopRotate;
	string_t				m_soundLoopRotate;

	float					m_flPlayerGracePeriod;
	float					m_flIgnoreGraceUpto;
	float					m_flPlayerLockTimeBeforeFire;
	float					m_flLastSawNonPlayer;

	string_t				m_targetEntityName;
	Vector					m_vTargetPosition;
	Vector					m_vecNPCIdleTarget;

	// Used for when the gun is attached to another entity
	string_t				m_iszBarrelAttachment;
	int						m_nBarrelAttachment;
	string_t				m_iszBaseAttachment;

	// Used when the gun is actually a part of the parent entity, and pose params aim it
	string_t				m_iszYawPoseParam;
	string_t				m_iszPitchPoseParam;
	float					m_flYawPoseCenter;
	float					m_flPitchPoseCenter;
	bool					m_bUsePoseParameters;

	// Lead the target?
	bool					m_bPerformLeading;
	float					m_flStartLeadFactor;
	float					m_flStartLeadFactorTime;
	float					m_flNextLeadFactor;
	float					m_flNextLeadFactorTime;

	COutputEvent			m_OnFire;
	COutputEvent			m_OnLoseTarget;
	COutputEvent			m_OnAquireTarget;
	COutputEvent			m_OnAmmoDepleted;
	COutputEvent			m_OnGotController;
	COutputEvent			m_OnLostController;
	COutputEvent			m_OnGotPlayerController;
	COutputEvent			m_OnLostPlayerController;
	COutputEvent			m_OnReadyToFire;

	CHandle<CBaseTrigger>	m_hControlVolume;
	string_t				m_iszControlVolume;

	float					m_flNextControllerSearch;
	bool					m_bShouldFindNPCs;
	bool					m_bNPCInRoute;
	string_t				m_iszNPCManPoint; 

	bool					m_bReadyToFire;

	int						m_iEffectHandling;
};

#endif // FUNC_TANK_H

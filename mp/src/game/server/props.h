//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#ifndef PROPS_H
#define PROPS_H
#ifdef _WIN32
#pragma once
#endif

#include "props_shared.h"
#include "baseanimating.h"
#include "physics_bone_follower.h"
#include "player_pickup.h"
#include "positionwatcher.h"

//=============================================================================================================
// PROP TYPES
//=============================================================================================================
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBaseProp : public CBaseAnimating
{
	DECLARE_CLASS( CBaseProp, CBaseAnimating );
public:

	void Spawn( void );
	void Precache( void );
	void Activate( void );
	bool KeyValue( const char *szKeyName, const char *szValue );
	void CalculateBlockLOS( void );
	int  ParsePropData( void );
	
	void DrawDebugGeometryOverlays( void );

	// Don't treat as a live target
	virtual bool IsAlive( void ) { return false; }
	virtual bool OverridePropdata() { return true; }
};


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBreakableProp : public CBaseProp, public IBreakableWithPropData, public CDefaultPlayerPickupVPhysics
{
public:
	CBreakableProp();

	DECLARE_CLASS( CBreakableProp, CBaseProp );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	virtual void Spawn();
	virtual void Precache();
	virtual float GetAutoAimRadius() { return 24.0f; }

	void BreakablePropTouch( CBaseEntity *pOther );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	void Event_Killed( const CTakeDamageInfo &info );
	void Break( CBaseEntity *pBreaker, const CTakeDamageInfo &info );
	void BreakThink( void );
	void AnimateThink( void );

	virtual void PlayPuntSound();

	void InputBreak( inputdata_t &inputdata );
	void InputAddHealth( inputdata_t &inputdata );
	void InputRemoveHealth( inputdata_t &inputdata );
	void InputSetHealth( inputdata_t &inputdata );

	int	 GetNumBreakableChunks( void ) { return m_iNumBreakableChunks; }

	virtual bool OverridePropdata() { return false; }
	virtual IPhysicsObject *GetRootPhysicsObjectForBreak();

	bool PropDataOverrodeBlockLOS( void ) { return m_bBlockLOSSetByPropData; }
	bool PropDataOverrodeAIWalkable( void ) { return m_bIsWalkableSetByPropData; }

	virtual bool   HasPreferredCarryAnglesForPlayer( CBasePlayer *pPlayer )
	{
		if ( HasInteraction( PROPINTER_PHYSGUN_LAUNCH_SPIN_Z ) )
			return true;

		return false; 
	}

	virtual QAngle PreferredCarryAngles( void ) { return m_preferredCarryAngles; }

	virtual void Ignite( float flFlameLifetime, bool bNPCOnly, float flSize = 0.0f, bool bCalledByLevelDesigner = false );

	// Specific interactions
	void	HandleFirstCollisionInteractions( int index, gamevcollisionevent_t *pEvent );
	void	HandleInteractionStick( int index, gamevcollisionevent_t *pEvent );
	void	StickAtPosition( const Vector &stickPosition, const Vector &savePosition, const QAngle &saveAngles );
	
	// Disable auto fading under dx7 or when level fades are specified
	void	DisableAutoFade();

public:
	COutputEvent	m_OnBreak;
	COutputFloat	m_OnHealthChanged;
	COutputEvent	m_OnTakeDamage;

	float			m_impactEnergyScale;

	int				m_iMinHealthDmg;

	QAngle			m_preferredCarryAngles;

public:
// IBreakableWithPropData
	void			SetDmgModBullet( float flDmgMod ) { m_flDmgModBullet = flDmgMod; }
	void			SetDmgModClub( float flDmgMod ) { m_flDmgModClub = flDmgMod; }
	void			SetDmgModExplosive( float flDmgMod ) { m_flDmgModExplosive = flDmgMod; }
	float			GetDmgModBullet( void ) { return m_flDmgModBullet; }
	float			GetDmgModClub( void ) { return m_flDmgModClub; }
	float			GetDmgModExplosive( void ) { return m_flDmgModExplosive; }
	void			SetExplosiveRadius( float flRadius ) { m_explodeRadius = flRadius; }
	void			SetExplosiveDamage( float flDamage ) { m_explodeDamage = flDamage; }
	float			GetExplosiveRadius( void ) { return m_explodeRadius; }
	float			GetExplosiveDamage( void ) { return m_explodeDamage; }
	void			SetPhysicsDamageTable( string_t iszTableName ) { m_iszPhysicsDamageTableName = iszTableName; }
	string_t		GetPhysicsDamageTable( void ) { return m_iszPhysicsDamageTableName; }
	void			SetBreakableModel( string_t iszModel ) { m_iszBreakableModel = iszModel; }
	string_t 		GetBreakableModel( void ) { return m_iszBreakableModel; }
	void			SetBreakableSkin( int iSkin ) { m_iBreakableSkin = iSkin; }
	int				GetBreakableSkin( void ) { return m_iBreakableSkin; }
	void			SetBreakableCount( int iCount ) { m_iBreakableCount = iCount; }
	int				GetBreakableCount( void ) { return m_iBreakableCount; }
	void			SetMaxBreakableSize( int iSize ) { m_iMaxBreakableSize = iSize; }
	int				GetMaxBreakableSize( void ) { return m_iMaxBreakableSize; }
	void			SetPropDataBlocksLOS( bool bBlocksLOS ) { m_bBlockLOSSetByPropData = true; SetBlocksLOS( bBlocksLOS ); }
	void			SetPropDataIsAIWalkable( bool b ) { m_bIsWalkableSetByPropData = true; SetAIWalkable( b ); }
	void			SetBasePropData( string_t iszBase ) { m_iszBasePropData = iszBase; }
	string_t		GetBasePropData( void ) { return m_iszBasePropData; }
	void			SetInteraction( propdata_interactions_t Interaction ) { m_iInteractions |= (1 << Interaction); }
	void			RemoveInteraction( propdata_interactions_t Interaction ) { m_iInteractions &= ~(1 << Interaction); }
	bool			HasInteraction( propdata_interactions_t Interaction ) { return ( m_iInteractions & (1 << Interaction) ) != 0; }
	void			SetMultiplayerBreakMode( mp_break_t mode ) { m_mpBreakMode = mode; }
	mp_break_t		GetMultiplayerBreakMode( void ) const { return m_mpBreakMode; }

// derived by multiplayer phys props:
	virtual void	SetPhysicsMode(int iMode) {}
	virtual int		GetPhysicsMode() { return PHYSICS_MULTIPLAYER_SOLID; }

	// Copy fade from another breakable prop
	void CopyFadeFrom( CBreakableProp *pSource );

protected:

	bool			UpdateHealth( int iNewHealth, CBaseEntity *pActivator );
	virtual void	OnBreak( const Vector &vecVelocity, const AngularImpulse &angVel, CBaseEntity *pBreaker ) {}

protected:

	unsigned int	m_createTick;
	float			m_flPressureDelay;
	EHANDLE			m_hBreaker;

	PerformanceMode_t m_PerformanceMode;

	// Prop data storage
	float			m_flDmgModBullet;
	float			m_flDmgModClub;
	float			m_flDmgModExplosive;
	string_t		m_iszPhysicsDamageTableName;
	string_t		m_iszBreakableModel;
	int				m_iBreakableSkin;
	int				m_iBreakableCount;
	int				m_iMaxBreakableSize;
	string_t		m_iszBasePropData;	
	int				m_iInteractions;
	float			m_explodeDamage;
	float			m_explodeRadius;
	string_t		m_iszBreakModelMessage;

	// Count of how many pieces we'll break into, custom or generic
	int				m_iNumBreakableChunks;

	void			SetEnableMotionPosition( const Vector &position, const QAngle &angles );
	bool			GetEnableMotionPosition( Vector *pPosition, QAngle *pAngles );
	void			ClearEnableMotionPosition();
private:
	CBaseEntity		*FindEnableMotionFixup();

public:
	virtual bool OnAttemptPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason );
	virtual AngularImpulse	PhysGunLaunchAngularImpulse();
	virtual	CBasePlayer *HasPhysicsAttacker( float dt );

#ifdef HL2_EPISODIC
	void CreateFlare( float flLifetime );
#endif //HL2_EPISODIC

protected:
	void SetPhysicsAttacker( CBasePlayer *pEntity, float flTime );
	void CheckRemoveRagdolls();
	
private:
	void InputEnablePhyscannonPickup( inputdata_t &inputdata );
	void InputDisablePhyscannonPickup( inputdata_t &inputdata );

	void InputEnablePuntSound( inputdata_t &inputdata ) { m_bUsePuntSound = true; }
	void InputDisablePuntSound( inputdata_t &inputdata ) { m_bUsePuntSound = false; }

	// Prevents fade scale from happening
	void ForceFadeScaleToAlwaysVisible();
	void RampToDefaultFadeScale();

private:
	enum PhysgunState_t
	{
		PHYSGUN_MUST_BE_DETACHED = 0,
		PHYSGUN_IS_DETACHING,
		PHYSGUN_CAN_BE_GRABBED,					
		PHYSGUN_ANIMATE_ON_PULL,
		PHYSGUN_ANIMATE_IS_ANIMATING,
		PHYSGUN_ANIMATE_FINISHED,
		PHYSGUN_ANIMATE_IS_PRE_ANIMATING,
		PHYSGUN_ANIMATE_IS_POST_ANIMATING,
	};

	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;
	bool					m_bBlockLOSSetByPropData;
	bool					m_bIsWalkableSetByPropData;
	bool					m_bOriginalBlockLOS;	// BlockLOS state before physgun pickup
	char					m_nPhysgunState;		// Ripped-off state
	COutputEvent			m_OnPhysCannonDetach;	// We've ripped it off!
	COutputEvent			m_OnPhysCannonAnimatePreStarted;	// Started playing the pre-pull animation
	COutputEvent			m_OnPhysCannonAnimatePullStarted;	// Player started the pull anim
	COutputEvent			m_OnPhysCannonAnimatePostStarted;	// Started playing the post-pull animation
	COutputEvent			m_OnPhysCannonPullAnimFinished; // We've had our pull anim finished, or the post-pull has finished if there is one
	float					m_flDefaultFadeScale;	// Things may temporarily change the fade scale, but this is its steady-state condition

	mp_break_t m_mpBreakMode;

	EHANDLE					m_hLastAttacker;		// Last attacker that harmed me.
	EHANDLE					m_hFlareEnt;
	string_t				m_iszPuntSound;
	bool					m_bUsePuntSound;
};

// Spawnflags
#define SF_DYNAMICPROP_USEHITBOX_FOR_RENDERBOX		64
#define SF_DYNAMICPROP_NO_VPHYSICS					128
#define SF_DYNAMICPROP_DISABLE_COLLISION			256

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CDynamicProp : public CBreakableProp, public IPositionWatcher
{
	DECLARE_CLASS( CDynamicProp, CBreakableProp );

public:
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CDynamicProp();

	void	Spawn( void );
	bool	CreateVPhysics( void );
	void	CreateBoneFollowers();
	void	UpdateOnRemove( void );
	void	AnimThink( void );
	void	PropSetSequence( int nSequence );
	void	OnRestore( void );
	bool	OverridePropdata( void );
	void	HandleAnimEvent( animevent_t *pEvent );

	// baseentity - watch dynamic hierarchy updates
	virtual void	SetParent( CBaseEntity* pNewParent, int iAttachment = -1 );
	bool			TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );

	// breakable prop
	virtual IPhysicsObject *GetRootPhysicsObjectForBreak();

	// IPositionWatcher
	virtual void NotifyPositionChanged( CBaseEntity *pEntity );

	// Input handlers
	void InputSetAnimation( inputdata_t &inputdata );
	void InputSetDefaultAnimation( inputdata_t &inputdata );
	void InputTurnOn( inputdata_t &inputdata );
	void InputTurnOff( inputdata_t &inputdata );
	void InputDisableCollision( inputdata_t &inputdata );
	void InputEnableCollision( inputdata_t &inputdata );
	void InputSetPlaybackRate( inputdata_t &inputdata );

	COutputEvent		m_pOutputAnimBegun;
	COutputEvent		m_pOutputAnimOver;

	string_t			m_iszDefaultAnim;

	int					m_iGoalSequence;
	int					m_iTransitionDirection;

	// Random animations
	bool				m_bRandomAnimator;
	float				m_flNextRandAnim;
	float				m_flMinRandAnimTime;
	float				m_flMaxRandAnimTime;
	short				m_nPendingSequence;

	bool				m_bStartDisabled;
	bool				m_bDisableBoneFollowers;

	CNetworkVar( bool, m_bUseHitboxesForRenderBox );

protected:
	void FinishSetSequence( int nSequence );
	void PropSetAnim( const char *szAnim );
	void BoneFollowerHierarchyChanged();

	// Contained Bone Follower manager
	CBoneFollowerManager	m_BoneFollowerManager;
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
DECLARE_AUTO_LIST( IPhysicsPropAutoList );
class CPhysicsProp : public CBreakableProp, public IPhysicsPropAutoList
{
	DECLARE_CLASS( CPhysicsProp, CBreakableProp );
	DECLARE_SERVERCLASS();

public:
	~CPhysicsProp();
	CPhysicsProp( void ) 
	{
	}

	void Spawn( void );
	void Precache();
	bool CreateVPhysics( void );
	bool OverridePropdata( void );

	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	void InputWake( inputdata_t &inputdata );
	void InputSleep( inputdata_t &inputdata );
	void InputEnableMotion( inputdata_t &inputdata );
	void InputDisableMotion( inputdata_t &inputdata );
	void InputDisableFloating( inputdata_t &inputdata );

	void EnableMotion( void );
	bool CanBePickedUpByPhyscannon( void );
	void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t reason );

	bool GetPropDataAngles( const char *pKeyName, QAngle &vecAngles );
	float GetCarryDistanceOffset( void );

	int ObjectCaps();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	void GetMassCenter( Vector *pMassCenter );
	float GetMass() const;

	void ClearFlagsThink( void );

	virtual int OnTakeDamage( const CTakeDamageInfo &info );
	int DrawDebugTextOverlays(void);
	bool IsGib();
	DECLARE_DATADESC();

	// Specific interactions
	void	HandleAnyCollisionInteractions( int index, gamevcollisionevent_t *pEvent );

	string_t GetPhysOverrideScript( void ) { return m_iszOverrideScript; }
	float	GetMassScale( void ) { return m_massScale; }

private:
	// Compute impulse to apply to the enabled entity.
	void ComputeEnablingImpulse( int index, gamevcollisionevent_t *pEvent );

	COutputEvent m_MotionEnabled;
	COutputEvent m_OnAwakened;
	COutputEvent m_OnPhysGunPickup;
	COutputEvent m_OnPhysGunPunt;
	COutputEvent m_OnPhysGunOnlyPickup;
	COutputEvent m_OnPhysGunDrop;
	COutputEvent m_OnPlayerUse;
	COutputEvent m_OnPlayerPickup;
	COutputEvent m_OnOutOfWorld;

	float		m_massScale;
	float		m_inertiaScale;
	int			m_damageType;
	string_t	m_iszOverrideScript;
	int			m_damageToEnableMotion;
	float		m_flForceToEnableMotion;

	bool		m_bThrownByPlayer;
	bool		m_bFirstCollisionAfterLaunch;

protected:
	CNetworkVar( bool, m_bAwake );
};


// An interface so that objects parented to props can receive collision interaction events.
enum parentCollisionInteraction_t
{
	COLLISIONINTER_PARENT_FIRST_IMPACT = 1,
};


abstract_class IParentPropInteraction
{
public:
	virtual void OnParentCollisionInteraction( parentCollisionInteraction_t eType, int index, gamevcollisionevent_t *pEvent ) = 0;
	virtual void OnParentPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason ) = 0;
};


// Used by prop_physics_create and the server benchmark.
// pModelName should not include the "models/" prefix.
CPhysicsProp* CreatePhysicsProp( const char *pModelName, const Vector &vTraceStart, const Vector &vTraceEnd, const IHandleEntity *pTraceIgnore, bool bRequireVCollide, const char *pClassName="physics_prop" );

bool UTIL_CreateScaledPhysObject( CBaseAnimating *pInstance, float flScale );

float GetBreakableDamage( const CTakeDamageInfo &inputInfo, IBreakableWithPropData *pProp = NULL );
int PropBreakablePrecacheAll( string_t modelName );

extern ConVar func_breakdmg_bullet;
extern ConVar func_breakdmg_club;
extern ConVar func_breakdmg_explosive;

#endif // PROPS_H

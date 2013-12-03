//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYSICS_PROP_RAGDOLL_H
#define PHYSICS_PROP_RAGDOLL_H
#ifdef _WIN32
#pragma once
#endif

#include "ragdoll_shared.h"
#include "player_pickup.h"


//-----------------------------------------------------------------------------
// Purpose: entity class for simple ragdoll physics
//-----------------------------------------------------------------------------

// UNDONE: Move this to a private header
class CRagdollProp : public CBaseAnimating, public CDefaultPlayerPickupVPhysics
{
	DECLARE_CLASS( CRagdollProp, CBaseAnimating );

public:
	CRagdollProp( void );
	~CRagdollProp( void );

	virtual void UpdateOnRemove( void );

	void DrawDebugGeometryOverlays();

	void Spawn( void );
	void Precache( void );

	// Disable auto fading under dx7 or when level fades are specified
	void DisableAutoFade();

	int ObjectCaps();

	DECLARE_SERVERCLASS();
	// Don't treat as a live target
	virtual bool IsAlive( void ) { return false; }
	
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &dir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual bool TestCollision( const Ray_t &ray, unsigned int mask, trace_t& trace );
	virtual void Teleport( const Vector *newPosition, const QAngle *newAngles, const Vector *newVelocity );
	virtual void SetupBones( matrix3x4_t *pBoneToWorld, int boneMask );
	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );
	virtual int VPhysicsGetObjectList( IPhysicsObject **pList, int listMax );

	virtual int DrawDebugTextOverlays(void);

	// Response system stuff
	virtual IResponseSystem *GetResponseSystem();
	virtual void ModifyOrAppendCriteria( AI_CriteriaSet& set );
	void SetSourceClassName( const char *pClassname );

	// Physics attacker
	virtual CBasePlayer *HasPhysicsAttacker( float dt );

	// locals
	void InitRagdollAnimation( void );
	void InitRagdoll( const Vector &forceVector, int forceBone, const Vector &forcePos, matrix3x4_t *pPrevBones, matrix3x4_t *pBoneToWorld, float dt, int collisionGroup, bool activateRagdoll, bool bWakeRagdoll = true );
	
	void RecheckCollisionFilter( void );
	void SetDebrisThink();
	void ClearFlagsThink( void );
	inline ragdoll_t *GetRagdoll( void ) { return &m_ragdoll; }

	virtual bool	IsRagdoll() { return true; }

	// Damage passing
	virtual void	SetDamageEntity( CBaseEntity *pEntity );
	virtual int		OnTakeDamage( const CTakeDamageInfo &info );
	virtual void OnSave( IEntitySaveUtils *pUtils );
	virtual void OnRestore();

	// Purpose: CDefaultPlayerPickupVPhysics
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
 	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason );
	virtual AngularImpulse	PhysGunLaunchAngularImpulse();
	bool HasPhysgunInteraction( const char *pszKeyName, const char *pszValue );
	void HandleFirstCollisionInteractions( int index, gamevcollisionevent_t *pEvent );

	void			SetUnragdoll( CBaseAnimating *pOther );

	void			SetBlendWeight( float weight ) { m_flBlendWeight = weight; }
	void			SetOverlaySequence( Activity activity );
	void			FadeOut( float flDelay = 0, float fadeTime = -1 );
	bool			IsFading();
	CBaseEntity*	GetKiller() { return m_hKiller; }
	void			SetKiller( CBaseEntity *pKiller ) { m_hKiller = pKiller; }
	void			GetAngleOverrideFromCurrentState( char *pOut, int size );

	void			DisableMotion( void );

	// Input/Output
	void			InputStartRadgollBoogie( inputdata_t &inputdata );
	void			InputEnableMotion( inputdata_t &inputdata );
	void			InputDisableMotion( inputdata_t &inputdata );
	void			InputTurnOn( inputdata_t &inputdata );
	void			InputTurnOff( inputdata_t &inputdata );
	void			InputFadeAndRemove( inputdata_t &inputdata );

	DECLARE_DATADESC();

protected:
	void CalcRagdollSize( void );
	ragdoll_t			m_ragdoll;

private:
	void UpdateNetworkDataFromVPhysics( IPhysicsObject *pPhysics, int index );
	void FadeOutThink();

	bool				m_bStartDisabled;

	CNetworkArray( Vector, m_ragPos, RAGDOLL_MAX_ELEMENTS );
	CNetworkArray( QAngle, m_ragAngles, RAGDOLL_MAX_ELEMENTS );

	string_t			m_anglesOverrideString;

	typedef CHandle<CBaseAnimating> CBaseAnimatingHandle;
	CNetworkVar( CBaseAnimatingHandle, m_hUnragdoll );


	unsigned int		m_lastUpdateTickCount;
	bool				m_allAsleep;
	bool				m_bFirstCollisionAfterLaunch;
	EHANDLE				m_hDamageEntity;
	EHANDLE				m_hKiller;	// Who killed me?
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;
	float				m_flFadeOutStartTime;
	float				m_flFadeTime;


	string_t			m_strSourceClassName;
	bool				m_bHasBeenPhysgunned;

	// If not 1, then allow underlying sequence to blend in with simulated bone positions
	CNetworkVar( float, m_flBlendWeight );
	CNetworkVar( int, m_nOverlaySequence );
	float	m_flDefaultFadeScale;
	
	Vector				m_ragdollMins[RAGDOLL_MAX_ELEMENTS];
	Vector				m_ragdollMaxs[RAGDOLL_MAX_ELEMENTS];
};

CBaseEntity *CreateServerRagdoll( CBaseAnimating *pAnimating, int forceBone, const CTakeDamageInfo &info, int collisionGroup, bool bUseLRURetirement = false );
CRagdollProp *CreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce, int forceBone, int collisionGroup, IPhysicsObject *pAttached, CBaseAnimating *pParentEntity, int boneAttach, const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin );
void DetachAttachedRagdoll( CBaseEntity *pRagdollIn );
void DetachAttachedRagdollsForEntity( CBaseEntity *pRagdollParent );
CBaseAnimating *CreateServerRagdollSubmodel( CBaseAnimating *pOwner, const char *pModelName, const Vector &position, const QAngle &angles, int collisionGroup );

bool Ragdoll_IsPropRagdoll( CBaseEntity *pEntity );
void Ragdoll_GetAngleOverrideString( char *pOut, int size, CBaseEntity *pEntity );
ragdoll_t *Ragdoll_GetRagdoll( CBaseEntity *pEntity );

#endif // PHYSICS_PROP_RAGDOLL_H

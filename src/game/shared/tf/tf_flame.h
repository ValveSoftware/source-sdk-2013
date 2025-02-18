//========= Copyright Valve Corporation, All rights reserved. ============//
//
//
//=============================================================================
#ifndef TF_FLAME_H
#define TF_FLAME_H
#ifdef _WIN32
#pragma once
#endif


#ifdef CLIENT_DLL
#define CTFFlameManager C_TFFlameManager
#endif // CLIENT_DLL

#include "tf_point_manager.h"
#include "tf_weaponbase.h"

struct flame_point_t : tf_point_t
{
	Vector m_vecAttackerVelocity = vec3_origin;
	Vector m_vecInitialPos = vec3_origin;
};

#define WATERFALL_FLAMETHROWER_STREAMS 5

#ifdef GAME_DLL
//-----------------------------------------------------------------------------
class CFlameEntityEnum : public IEntityEnumerator
{
public:
	CFlameEntityEnum( CBaseEntity *pShooter )
	{
		m_pShooter = pShooter;
	}

	virtual bool EnumEntity( IHandleEntity *pHandleEntity ) OVERRIDE;

	const CUtlVector< CBaseEntity* >& GetTargets() { return m_Targets; }

public:
	CBaseEntity	*m_pShooter;
	CUtlVector< CBaseEntity* > m_Targets;
};

struct burned_entity_t
{
	float m_flLastBurnTime;
	float m_flHeatIndex;
};
#endif // GAME_DLL

DECLARE_AUTO_LIST( ITFFlameManager );
class CTFFlameManager : public CTFPointManager, public ITFFlameManager
{
	DECLARE_CLASS( CTFFlameManager, CTFPointManager );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();
	DECLARE_PREDICTABLE();

	CTFFlameManager();

	virtual void UpdateOnRemove( void ) OVERRIDE;

#ifdef GAME_DLL
	void PostEntityThink( void );

	static CTFFlameManager* Create( CBaseEntity *pOwner, bool bIsAdditionalManager = false );
	virtual bool AddPoint( int iCurrentTick ) OVERRIDE;
	void HookAttributes();
	void UpdateDamage( int iDmgType, float flDamage, float flBurnFrequency, bool bCritFromBehind );
	bool BCanBurnEntityThisFrame( CBaseEntity *pEnt ) const;
#endif // GAME_DLL

	void StartFiring();
	void StopFiring();

#ifdef CLIENT_DLL
	virtual void OnPreDataChanged( DataUpdateType_t updateType ) OVERRIDE;
	virtual void OnDataChanged( DataUpdateType_t updateType ) OVERRIDE;
#endif // CLIENT_DLL
	
	CBaseEntity *GetAttacker( void ) { return m_hAttacker.Get(); }

protected:
	virtual void InitializePoint( tf_point_t *pPoint, int nPointIndex ) OVERRIDE;
	virtual Vector GetInitialPosition() const OVERRIDE;
	virtual Vector GetInitialVelocity() const OVERRIDE;

#ifdef GAME_DLL
	virtual bool ShouldCollide( CBaseEntity *pEnt ) const OVERRIDE;
	virtual void OnCollide( CBaseEntity *pEnt, int iPointIndex ) OVERRIDE;
#endif // GAME_DLL

	virtual tf_point_t *AllocatePoint() OVERRIDE { return new flame_point_t; }

#ifdef CLIENT_DLL
	virtual void OnClientPointAdded( const tf_point_t *pNewestPoint ) OVERRIDE;
#endif // CLIENT_DLL

	virtual void Update() OVERRIDE;

	virtual bool OnPointHitWall( tf_point_t *pPoint, Vector &vecNewPos, Vector &vecNewVelocity, const trace_t& tr, float flDT ) OVERRIDE;
	virtual void ModifyAdditionalMovementInfo( tf_point_t *pPoint, float flDT ) OVERRIDE;

	virtual float	GetInitialSpeed() const OVERRIDE;
	virtual float	GetLifeTime() const OVERRIDE;
	virtual float	GetGravity() const OVERRIDE;
	virtual float	GetDrag() const OVERRIDE;
	virtual Vector	GetAdditionalVelocity( const tf_point_t *pPoint ) const OVERRIDE;

	virtual float	GetRadius( const tf_point_t *pPoint ) const OVERRIDE;

	virtual int	GetMaxPoints() const OVERRIDE { return 30; } // this should match with the number of CP defined in particle system

private:

	float GetFlameSizeMult( const tf_point_t *pPoint ) const;
	float GetStartSizeMult() const;
	float GetEndSizeMult() const;

	bool ShouldIgnorePlayerVelocity() const;
	float ReflectionAdditionalLifeTime() const;
	float ReflectionDamageReduction() const;
	int GetMaxFlameReflectionCount() const;

#ifdef GAME_DLL
	bool IsValidBurnTarget( CBaseEntity *pEntity ) const;
	float GetFlameDamageScale( const tf_point_t* pPoint, CTFPlayer *pTFTarget = NULL ) const;

	void SetHitTarget( void );

	// map of entities burnt with the last burn time
	CUtlMap< EHANDLE, burned_entity_t > m_mapEntitiesBurnt;

	int				m_iDmgType;
	float			m_flDamage;
	float			m_flBurnFrequency;	// how often this flame can burn an entity
	bool			m_bCritFromBehind;

#else
	void UpdateWeaponParticleControlPoint( const flame_point_t *pNewestFlame );
	void UpdateFlameParticleControlPoint( const flame_point_t *pFlame );

	void OnFiringStateChange();
	void RemoveAllParticles();

	CUtlVector< HPARTICLEFFECT > m_hOldParticleEffects;
	HPARTICLEFFECT	m_hParticleEffect = NULL;
	int				m_nMuzzleAttachment;
	bool			m_bOldFiring;
#endif // GAME_DLL

	CNetworkHandle( CTFWeaponBase, m_hWeapon );
	CNetworkHandle( CBaseEntity, m_hAttacker );

	// cache attrs
	CNetworkVar( float,	m_flSpreadDegree );
	CNetworkVar( float,	m_flRedirectedFlameSizeMult );
	CNetworkVar( float,	m_flFlameStartSizeMult );
	CNetworkVar( float,	m_flFlameEndSizeMult );
	CNetworkVar( float,	m_flFlameIgnorePlayerVelocity );
	CNetworkVar( float,	m_flFlameReflectionAdditionalLifeTime );
	CNetworkVar( float,	m_flFlameReflectionDamageReduction );
	CNetworkVar( int,	m_iMaxFlameReflectionCount );
	CNetworkVar( int,	m_nShouldReflect );
	CNetworkVar( float,	m_flFlameSpeed );
	CNetworkVar( float,	m_flFlameLifeTime );
	CNetworkVar( float,	m_flRandomLifeTimeOffset );

	CNetworkVar( float,	m_flFlameGravity );
	CNetworkVar( float,	m_flFlameDrag );
	CNetworkVar( float,	m_flFlameUp );

	CNetworkVar( bool,	m_bIsFiring );

#ifdef WATERFALL_FLAMETHROWER_TEST
	CNetworkVar( int,	m_iWaterfallMode );

	// Don't ship this, just for prototyping
	typedef CHandle<CTFFlameManager> CTFFlameManagerHandle;
	CNetworkArray( CTFFlameManagerHandle, m_hAdditionalFlameManagers, WATERFALL_FLAMETHROWER_STREAMS - 1 );
	CNetworkVar( int,	m_iStreamIndex );
#endif // WATERFALL_FLAMETHROWER_TEST
};

#endif // TF_FLAME_H

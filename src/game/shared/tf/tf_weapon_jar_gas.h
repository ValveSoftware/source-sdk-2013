//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#ifndef TF_WEAPON_JAR_GAS_H
#define TF_WEAPON_JAR_GAS_H
#ifdef _WIN32
#pragma once
#endif

#include "tf_weaponbase_gun.h"
#include "tf_weapon_grenade_pipebomb.h"
#include "tf_weapon_jar.h"
#include "tf_point_manager.h"

#ifdef GAME_DLL
#include "triggers.h"
#endif

#ifdef CLIENT_DLL
#define CTFJarGas C_TFJarGas
#define CTFProjectile_JarGas C_TFProjectile_JarGas
#define CTFGasManager C_TFGasManager
#endif

#define TF_GAS_LIFETIME	5.f
#define TF_GAS_POINT_RADIUS	25.f
#define TF_GAS_AFTERBURN_RATE 10.f
#define TF_GAS_PROJ_SPEED 2000.f
#define TF_GAS_FALLRATE 5.f
#define TF_GAS_MOVE_DISTANCE 10.f

#define TF_WEAPON_JAR_GAS_JAR_MODEL	"models/weapons/c_models/c_gascan/c_gascan.mdl"
#define TF_WEAPON_JARGAS_EXPLODE_SOUND	"Weapon_GasCan.Explode"

// *************************************************************************************************************************
class CTFJarGas : public CTFJar
{
public:
	DECLARE_CLASS( CTFJarGas, CTFJar );
	DECLARE_NETWORKCLASS();

	virtual int GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_JAR_GAS; }
	virtual float GetProjectileSpeed( void ) OVERRIDE;
	virtual void Equip( CBaseCombatCharacter* pOwner ) OVERRIDE;
	virtual bool CanAttack() OVERRIDE;

#ifdef GAME_DLL
	virtual CTFProjectile_Jar* CreateJarProjectile( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo ) OVERRIDE;
	virtual float GetAfterburnRateOnHit() const OVERRIDE;
	virtual void OnResourceMeterFilled() OVERRIDE;
	virtual float GetDefaultItemChargeMeterValue( void ) const OVERRIDE { return 0.f; }
#else
	virtual const char* ModifyEventParticles( const char* token ) OVERRIDE;
	virtual bool Holster( CBaseCombatWeapon *pSwitchingTo = NULL ) OVERRIDE;
#endif // GAME_DLL

	virtual const char* GetEffectLabelText( void ) OVERRIDE { return "#TF_Gas"; }
	virtual bool ShouldUpdateMeter() const OVERRIDE;
	virtual float InternalGetEffectBarRechargeTime( void ) OVERRIDE { return 0.f; }

private:
	void RemoveJarGas( CBaseCombatCharacter *pOwner );
};

// *************************************************************************************************************************
class CTFProjectile_JarGas : public CTFProjectile_Jar
{
public:
	DECLARE_CLASS( CTFProjectile_JarGas, CTFProjectile_Jar );
	DECLARE_NETWORKCLASS();

#ifdef GAME_DLL
	static CTFProjectile_JarGas *Create( const Vector &position, const QAngle &angles, const Vector &velocity,
		const AngularImpulse &angVelocity, CBaseCombatCharacter *pOwner, const CTFWeaponInfo &weaponInfo );

	virtual void		SetCustomPipebombModel() OVERRIDE;

	virtual const char* GetImpactEffect() OVERRIDE { return ( GetTeamNumber() == TF_TEAM_BLUE ) ? "gas_can_impact_blue" : "gas_can_impact_red"; }
	virtual ETFCond		GetEffectCondition( void ) OVERRIDE { return TF_COND_GAS; }
	virtual const char* GetExplodeSound() OVERRIDE { return TF_WEAPON_JARGAS_EXPLODE_SOUND; }

	virtual void Explode( trace_t *pTrace, int bitsDamageType ) OVERRIDE;
#endif

	virtual int GetWeaponID( void ) const OVERRIDE { return TF_WEAPON_GRENADE_JAR_GAS; }
	virtual void Precache() OVERRIDE;
};

#ifdef CLIENT_DLL
struct gas_particle_t
{
	gas_particle_t();
	~gas_particle_t();

	int								m_nUniqueID;
	CSmartPtr<CNewParticleEffect>	m_hParticleEffect;
	EHANDLE							m_hParticleOwner;
};

typedef CUtlVector< gas_particle_t > GasParticle_t;
#endif // CLIENT_DLL

class CTFGasManager : public CTFPointManager
{
	DECLARE_CLASS( CTFGasManager, CTFPointManager );
public:
	DECLARE_NETWORKCLASS();
	DECLARE_DATADESC();

	CTFGasManager();
	virtual void UpdateOnRemove( void ) OVERRIDE;

#ifdef GAME_DLL
	static CTFGasManager* Create( CBaseEntity *pOwner, const Vector& vPos );
	void AddGas();
	virtual void OnCollide( CBaseEntity *pEnt, int iPointIndex ) OVERRIDE;
#else
	virtual void PostDataUpdate( DataUpdateType_t updateType ) OVERRIDE;
#endif // GAME_DLL

	virtual void Update() OVERRIDE;

protected:
	virtual Vector	GetInitialPosition() const OVERRIDE;

	virtual float	GetLifeTime() const OVERRIDE { return TF_GAS_LIFETIME; }
	virtual Vector	GetAdditionalVelocity( const tf_point_t *pPoint ) const OVERRIDE { return vec3_origin; }
	virtual float	GetRadius( const tf_point_t *pPoint ) const OVERRIDE { return TF_GAS_POINT_RADIUS; }
	virtual int		GetMaxPoints() const OVERRIDE { return 20; }
	virtual bool	ShouldIgnoreStartSolid( void ) OVERRIDE{ return true; }
	virtual bool	OnPointHitWall( tf_point_t *pPoint, Vector &vecNewPos, Vector &vecNewVelocity, const trace_t& tr, float flDT ) OVERRIDE;

#ifdef GAME_DLL
	virtual bool	ShouldCollide( CBaseEntity *pEnt ) const OVERRIDE;
#endif // GAME_DLL

private:
#ifdef GAME_DLL
	CUtlVector< EHANDLE > m_Touched;
#else
	GasParticle_t m_hGasParticleEffects;
#endif // GAME_DLL

	float m_flLastMoveUpdate;
	bool m_bKeepMovingPoints;
};

#endif // TF_WEAPON_JAR_GAS_H

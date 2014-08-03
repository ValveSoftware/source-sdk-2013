//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Defines a class for objects that break after taking a certain amount
//			of damage.
//
// $NoKeywords: $
//=============================================================================//

#ifndef FUNC_BREAK_H
#define FUNC_BREAK_H
#pragma once

#include "entityoutput.h"
#include "props.h"

typedef enum { expRandom = 0, expDirected, expUsePrecise} Explosions;
typedef enum { matGlass = 0, matWood, matMetal, matFlesh, matCinderBlock, matCeilingTile, matComputer, matUnbreakableGlass, matRocks, matWeb, matNone, matLastMaterial } Materials;


#define	NUM_SHARDS 6 // this many shards spawned when breakable objects break;

// Spawnflags for func breakable
#define SF_BREAK_TRIGGER_ONLY				0x0001	// may only be broken by trigger
#define	SF_BREAK_TOUCH						0x0002	// can be 'crashed through' by running player (plate glass)
#define SF_BREAK_PRESSURE					0x0004	// can be broken by a player standing on it
#define SF_BREAK_PHYSICS_BREAK_IMMEDIATELY	0x0200	// the first physics collision this breakable has will immediately break it
#define SF_BREAK_DONT_TAKE_PHYSICS_DAMAGE	0x0400	// this breakable doesn't take damage from physics collisions
#define SF_BREAK_NO_BULLET_PENETRATION		0x0800  // don't allow bullets to penetrate

// Spawnflags for func_pushable (it's also func_breakable, so don't collide with those flags)
#define SF_PUSH_BREAKABLE					0x0080
#define SF_PUSH_NO_USE						0x0100	// player cannot +use pickup this ent

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CBreakable : public CBaseEntity, public IBreakableWithPropData, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS( CBreakable, CBaseEntity );

	// basic functions
	virtual void Spawn( void );
	void ParsePropData( void );
	bool CreateVPhysics( void );
	virtual void Precache( void );
	virtual bool KeyValue( const char *szKeyName, const char *szValue );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	void BreakTouch( CBaseEntity *pOther );
	void DamageSound( void );
	void Break( CBaseEntity *pBreaker );

	// Input handlers
	void InputAddHealth( inputdata_t &inputdata );
	void InputBreak( inputdata_t &inputdata );
	void InputRemoveHealth( inputdata_t &inputdata );
	void InputSetHealth( inputdata_t &inputdata );
	void InputSetMass( inputdata_t &inputdata );


	// breakables use an overridden takedamage
	virtual int OnTakeDamage( const CTakeDamageInfo &info );

	// To spark when hit
	void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	bool IsBreakable( void );
	bool SparkWhenHit( void );

	char const		*DamageDecal( int bitsDamageType, int gameMaterial );

	virtual void	Die( void );
	void			ResetOnGroundFlags(void);

	inline bool		Explodable( void ) { return GetExplosiveRadius() > 0; }

	Materials		GetMaterialType( void ) { return m_Material; }
	static void MaterialSoundRandom( int entindex, Materials soundMaterial, float volume );
	static const char *MaterialSound( Materials precacheMaterial );

	static const char *pSpawnObjects[];

	int DrawDebugTextOverlays(void);

	DECLARE_DATADESC();

public:
// IBreakableWithPropData
	void			SetDmgModBullet( float flDmgMod ) { m_flDmgModBullet = flDmgMod; }
	void			SetDmgModClub( float flDmgMod ) { m_flDmgModClub = flDmgMod; }
	void			SetDmgModExplosive( float flDmgMod ) { m_flDmgModExplosive = flDmgMod; }
	float			GetDmgModBullet( void ) { return m_flDmgModBullet; }
	float			GetDmgModClub( void ) { return m_flDmgModClub; }
	float			GetDmgModExplosive( void ) { return m_flDmgModExplosive; }
	void			SetExplosiveRadius( float flRadius ) { m_explodeRadius = flRadius; }
	void			SetExplosiveDamage( float flDamage ) { m_ExplosionMagnitude = flDamage; }
	float			GetExplosiveRadius( void ) { return m_explodeRadius; }
	float			GetExplosiveDamage( void ) { return m_ExplosionMagnitude; }
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
	void			SetPropDataBlocksLOS( bool bBlocksLOS ) { SetBlocksLOS( bBlocksLOS ); }
	void			SetPropDataIsAIWalkable( bool bBlocksLOS ) { SetAIWalkable( bBlocksLOS ); }
	void			SetBasePropData( string_t iszBase ) { m_iszBasePropData = iszBase; }
	string_t		GetBasePropData( void ) { return m_iszBasePropData; }
	void			SetInteraction( propdata_interactions_t Interaction ) { m_iInteractions |= (1 << Interaction); }
	bool			HasInteraction( propdata_interactions_t Interaction ) { return ( m_iInteractions & (1 << Interaction) ) != 0; }
	void			SetPhysicsMode(int iMode){}
	int				GetPhysicsMode() { return PHYSICS_MULTIPLAYER_SOLID; }
	void			SetMultiplayerBreakMode( mp_break_t mode ) {}
	mp_break_t		GetMultiplayerBreakMode( void ) const { return MULTIPLAYER_BREAK_DEFAULT; }

protected:
	float		m_angle;
	Materials	m_Material;
	EHANDLE m_hBreaker;			// The entity that broke us. Held as a data member because sometimes breaking is delayed.

private:

	Explosions	m_Explosion;
	QAngle		m_GibDir;
	string_t 	m_iszGibModel;
	string_t 	m_iszSpawnObject;
	int			m_ExplosionMagnitude;
	float		m_flPressureDelay;		// Delay before breaking when destoyed by pressure
	int			m_iMinHealthDmg;		// minimum damage attacker must have to cause damage
	bool		m_bTookPhysicsDamage;

	string_t	m_iszPropData;
	string_t	m_iszModelName;

protected:

	bool		UpdateHealth( int iNewHealth, CBaseEntity *pActivator );

	float		m_impactEnergyScale;

	COutputEvent m_OnBreak;
	COutputFloat m_OnHealthChanged;

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
	PerformanceMode_t m_PerformanceMode;

	float			m_explodeRadius;

public:
	// IPlayerPickupVPhysics
	virtual void OnPhysGunPickup( CBasePlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CBasePlayer *pPhysGunUser, PhysGunDrop_t Reason );
	virtual	CBasePlayer *HasPhysicsAttacker( float dt );
private:
	CHandle<CBasePlayer>	m_hPhysicsAttacker;
	float					m_flLastPhysicsInfluenceTime;
};

#endif	// FUNC_BREAK_H

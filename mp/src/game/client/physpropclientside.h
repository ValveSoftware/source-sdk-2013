//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PHYSPROPCLIENTSIDE_H
#define PHYSPROPCLIENTSIDE_H
#ifdef _WIN32
#pragma once
#endif

#include "c_breakableprop.h"
#include "props_shared.h"

class C_FuncPhysicsRespawnZone;

class C_PhysPropClientside : public C_BreakableProp, public IBreakableWithPropData, public IMultiplayerPhysics
{
	
public:
	DECLARE_CLASS( C_PhysPropClientside, C_BreakableProp );
	
	C_PhysPropClientside();
	virtual ~C_PhysPropClientside();

			bool			Initialize();
			void			Spawn();
			int				ParsePropData( void );
	virtual bool			IsDormant( void ) { return false; } // we could add a PVS check here
	virtual void			ClientThink( void );
	virtual CollideType_t	GetCollideType( void ) { return ENTITY_SHOULD_RESPOND; }
	virtual void			StartTouch( C_BaseEntity *pOther );
	virtual	void			HitSurface( C_BaseEntity *pOther );
	virtual	void			ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	virtual	bool			IsClientCreated( void ) const { return true; }
	virtual int				GetMultiplayerPhysicsMode() { return m_iPhysicsMode; }
	virtual float			GetMass();
	virtual bool			IsAsleep();
	
	virtual bool			KeyValue( const char *szKeyName, const char *szValue );

	virtual	void			OnTakeDamage( int iDamage ); // very simple version
	virtual void			Break();
	virtual	void			Clone( Vector &velocity );
	virtual void			StartFadeOut( float fTime );
	virtual void			SetHealth(int iHealth) { m_iHealth = iHealth; }
	virtual int				GetHealth() const { return m_iHealth; }
			int				GetNumBreakableChunks( void ) { return m_iNumBreakableChunks; }
	
			void			SetRespawnZone( C_FuncPhysicsRespawnZone *pZone );

// IBreakableWithPropData interface:
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
	bool			HasInteraction( propdata_interactions_t Interaction ) { return ( m_iInteractions & (1 << Interaction) ) != 0; }
	void			SetPhysicsMode(int iMode);
	int				GetPhysicsMode() { return m_iPhysicsMode; }
	void			SetMultiplayerBreakMode( mp_break_t mode ) {}
	mp_break_t		GetMultiplayerBreakMode( void ) const { return MULTIPLAYER_BREAK_DEFAULT; }


// static management fucntions:
	static void RecreateAll(); // recreate all clientside props in map
	static void DestroyAll();  // clear all clientside created phys props
	static C_PhysPropClientside *CreateNew(bool bForce = false);

protected:
	
	static void ParseAllEntities(const char *pMapData);
	static const char *ParseEntity( const char *pEntData );
	static void InitializePropRespawnZones(void);
		
public:
	
	int		m_iPhysicsMode;		// should always be PHYSICS_MULTIPLAYER_CLIENTSIDE
	float	m_flTouchDelta;		// Amount of time that must pass before another touch function can be called
	float 	m_fDeathTime;		// Point at which this object self destructs.  
								// The default of -1 indicates the object shouldn't destruct.

// properties from serverclass CPhysicsProp
	float		m_impactEnergyScale;
	int			m_spawnflags;
	float		m_inertiaScale;

protected:
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
	bool			m_bBlockLOSSetByPropData;
	bool			m_bIsWalkableSetByPropData;

	// Count of how many pieces we'll break into, custom or generic
	int				m_iNumBreakableChunks;

	C_FuncPhysicsRespawnZone	*m_pRespawnZone;
};

//-----------------------------------------------------------------------------
// Purpose: A clientside zone that respawns physics props in it when the player leaves the PVS
//-----------------------------------------------------------------------------
class C_FuncPhysicsRespawnZone : public C_BaseEntity
{
	DECLARE_CLASS( C_FuncPhysicsRespawnZone, C_BaseEntity );
public:

	C_FuncPhysicsRespawnZone( void );
	~C_FuncPhysicsRespawnZone( void );

	bool KeyValue( const char *szKeyName, const char *szValue );
	bool Initialize( void );
	void InitializePropsWithin( void );
	void PropDestroyed( C_PhysPropClientside *pProp );
	bool CanMovePropAt( Vector vecOrigin, const Vector &vecMins, const Vector &vecMaxs );
	void RespawnProps( void );
	void ClientThink( void );

private:
	struct clientsideproprespawn_t
	{
		string_t				iszModelName;
		Vector					vecOrigin;
		QAngle					vecAngles;
		int						iSkin;
		int						iHealth;
		int						iSpawnFlags;
		ClientEntityHandle_t	hClientEntity;
	};
	CUtlVector<clientsideproprespawn_t> m_PropList;
};

#endif // PHYSPROPCLIENTSIDE_H

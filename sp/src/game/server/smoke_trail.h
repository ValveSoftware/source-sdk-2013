//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef SMOKE_TRAIL_H
#define SMOKE_TRAIL_H

#include "baseparticleentity.h"

//==================================================
// SmokeTrail
//==================================================

class SmokeTrail : public CBaseParticleEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( SmokeTrail, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	SmokeTrail();
	virtual bool KeyValue( const char *szKeyName, const char *szValue ); 
	void					SetEmit(bool bVal);
	void					FollowEntity( CBaseEntity *pEntity, const char *pAttachmentName = NULL);
	static	SmokeTrail*		CreateSmokeTrail();

public:
	// Effect parameters. These will assume default values but you can change them.
	CNetworkVector( m_StartColor );			// Fade between these colors.
	CNetworkVector( m_EndColor );
	CNetworkVar( float, m_Opacity );

	CNetworkVar( float, m_SpawnRate );			// How many particles per second.
	CNetworkVar( float, m_ParticleLifetime );		// How long do the particles live?
	CNetworkVar( float, m_StopEmitTime );			// When do I stop emitting particles?
	CNetworkVar( float, m_MinSpeed );				// Speed range.
	CNetworkVar( float, m_MaxSpeed );
	CNetworkVar( float, m_StartSize );			// Size ramp.
	CNetworkVar( float, m_EndSize );	
	CNetworkVar( float, m_SpawnRadius );
	CNetworkVar( float, m_MinDirectedSpeed );				// Speed range.
	CNetworkVar( float, m_MaxDirectedSpeed );
	CNetworkVar( bool, m_bEmit );

	CNetworkVar( int, m_nAttachment );
};

//==================================================
// RocketTrail
//==================================================

class RocketTrail : public CBaseParticleEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( RocketTrail, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	RocketTrail();
	void					SetEmit(bool bVal);
	void					FollowEntity( CBaseEntity *pEntity, const char *pAttachmentName = NULL);
	static RocketTrail		*CreateRocketTrail();

public:
	// Effect parameters. These will assume default values but you can change them.
	CNetworkVector( m_StartColor );			// Fade between these colors.
	CNetworkVector( m_EndColor );
	CNetworkVar( float, m_Opacity );

	CNetworkVar( float, m_SpawnRate );			// How many particles per second.
	CNetworkVar( float, m_ParticleLifetime );		// How long do the particles live?
	CNetworkVar( float, m_StopEmitTime );			// When do I stop emitting particles?
	CNetworkVar( float, m_MinSpeed );				// Speed range.
	CNetworkVar( float, m_MaxSpeed );
	CNetworkVar( float, m_StartSize );			// Size ramp.
	CNetworkVar( float, m_EndSize );	
	CNetworkVar( float, m_SpawnRadius );
	
	CNetworkVar( bool, m_bEmit );

	CNetworkVar( int, m_nAttachment );
	
	CNetworkVar( bool, m_bDamaged );

	CNetworkVar( float, m_flFlareScale );			// Size of the flare
};

//==================================================
// SporeTrail
//==================================================

class SporeTrail : public CBaseParticleEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( SporeTrail, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	SporeTrail( void );

	static SporeTrail*		CreateSporeTrail();

//Data members
public:

	CNetworkVector( m_vecEndColor );

	CNetworkVar( float, m_flSpawnRate );
	CNetworkVar( float, m_flParticleLifetime );
	CNetworkVar( float, m_flStartSize );
	CNetworkVar( float, m_flEndSize );
	CNetworkVar( float, m_flSpawnRadius );

	CNetworkVar( bool, m_bEmit );
};

//==================================================
// SporeExplosion
//==================================================

class SporeExplosion : public CBaseParticleEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( SporeExplosion, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	SporeExplosion( void );
	void Spawn( void );

	static SporeExplosion*		CreateSporeExplosion();

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

//Data members
public:

	bool m_bDisabled;

	CNetworkVar( float, m_flSpawnRate );
	CNetworkVar( float, m_flParticleLifetime );
	CNetworkVar( float, m_flStartSize );
	CNetworkVar( float, m_flEndSize );
	CNetworkVar( float, m_flSpawnRadius );

	CNetworkVar( bool, m_bEmit );
	CNetworkVar( bool, m_bDontRemove );
};

//==================================================
// CFireTrail
//==================================================

class CFireTrail : public CBaseParticleEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CFireTrail, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	static CFireTrail	*CreateFireTrail( void );
	void				FollowEntity( CBaseEntity *pEntity, const char *pAttachmentName );
	void				Precache( void );

	CNetworkVar( int, m_nAttachment );
	CNetworkVar( float, m_flLifetime );
};

//==================================================
// DustTrail
//==================================================

class DustTrail : public CBaseParticleEntity
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( DustTrail, CBaseParticleEntity );
	DECLARE_SERVERCLASS();

	DustTrail();
	virtual bool KeyValue( const char *szKeyName, const char *szValue ); 
	void					SetEmit(bool bVal);
	static	DustTrail*		CreateDustTrail();

public:
	// Effect parameters. These will assume default values but you can change them.
	CNetworkVector( m_Color );
	CNetworkVar( float, m_Opacity );

	CNetworkVar( float, m_SpawnRate );			// How many particles per second.
	CNetworkVar( float, m_ParticleLifetime );		// How long do the particles live?
	CNetworkVar( float, m_StopEmitTime );			// When do I stop emitting particles?
	CNetworkVar( float, m_MinSpeed );				// Speed range.
	CNetworkVar( float, m_MaxSpeed );
	CNetworkVar( float, m_StartSize );			// Size ramp.
	CNetworkVar( float, m_EndSize );	
	CNetworkVar( float, m_SpawnRadius );
	CNetworkVar( float, m_MinDirectedSpeed );				// Speed range.
	CNetworkVar( float, m_MaxDirectedSpeed );
	CNetworkVar( bool, m_bEmit );

	CNetworkVar( int, m_nAttachment );
};


#endif

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYFLAME_H
#define ENTITYFLAME_H
#ifdef _WIN32
#pragma once
#endif

#define FLAME_DAMAGE_INTERVAL			0.2f // How often to deal damage.
#define FLAME_DIRECT_DAMAGE_PER_SEC		5.0f
#define FLAME_RADIUS_DAMAGE_PER_SEC		4.0f

#define FLAME_DIRECT_DAMAGE ( FLAME_DIRECT_DAMAGE_PER_SEC * FLAME_DAMAGE_INTERVAL )
#define FLAME_RADIUS_DAMAGE ( FLAME_RADIUS_DAMAGE_PER_SEC * FLAME_DAMAGE_INTERVAL )

#define FLAME_MAX_LIFETIME_ON_DEAD_NPCS	10.0f

class CEntityFlame : public CBaseEntity 
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_CLASS( CEntityFlame, CBaseEntity );

	CEntityFlame( void );

	static CEntityFlame	*Create( CBaseEntity *pTarget, bool useHitboxes = true );

	void	AttachToEntity( CBaseEntity *pTarget );
	void	SetLifetime( float lifetime );
	void	SetUseHitboxes( bool use );
	void	SetNumHitboxFires( int iNumHitBoxFires );
	void	SetHitboxFireScale( float flHitboxFireScale );

	float	GetRemainingLife( void );
	int		GetNumHitboxFires( void );
	float	GetHitboxFireScale( void );

	virtual void Precache();
	virtual void UpdateOnRemove();

	void	SetSize( float size ) { m_flSize = size; }

	DECLARE_DATADESC();

protected:

	void InputIgnite( inputdata_t &inputdata );

	void	FlameThink( void );

	CNetworkHandle( CBaseEntity, m_hEntAttached );		// The entity that we are burning (attached to).

	CNetworkVar( float, m_flSize );
	CNetworkVar( bool, m_bUseHitboxes );
	CNetworkVar( int, m_iNumHitboxFires );
	CNetworkVar( float, m_flHitboxFireScale );

	CNetworkVar( float, m_flLifetime );
	bool	m_bPlayingSound;
};

#endif // ENTITYFLAME_H

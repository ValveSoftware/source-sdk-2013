//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef RAGDOLLBOOGIE_H
#define RAGDOLLBOOGIE_H

#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Set this spawnflag before calling Spawn to get electrical effects
//-----------------------------------------------------------------------------
#define SF_RAGDOLL_BOOGIE_ELECTRICAL	0x10000
#define SF_RAGDOLL_BOOGIE_ELECTRICAL_NARROW_BEAM	0x20000


//-----------------------------------------------------------------------------
// Makes ragdolls DANCE!
//-----------------------------------------------------------------------------
class CRagdollBoogie : public CBaseEntity 
{
	DECLARE_DATADESC();
	DECLARE_CLASS( CRagdollBoogie, CBaseEntity );

public:
#ifdef MAPBASE
	static CRagdollBoogie	*Create( CBaseEntity *pTarget, float flMagnitude, float flStartTime, float flLengthTime = 0.0f, int nSpawnFlags = 0, const Vector *vecColor = NULL );
#else
	static CRagdollBoogie	*Create( CBaseEntity *pTarget, float flMagnitude, float flStartTime, float flLengthTime = 0.0f, int nSpawnFlags = 0 );
#endif
	static void IncrementSuppressionCount( CBaseEntity *pTarget );
	static void DecrementSuppressionCount( CBaseEntity *pTarget );

#ifdef MAPBASE
	void SetColor( const Vector &vecColor ) { m_vecColor = vecColor; }
#endif

	void Spawn();

private:
	void	AttachToEntity( CBaseEntity *pTarget );
	void	SetBoogieTime( float flStartTime, float flLengthTime );
	void	SetMagnitude( float flMagnitude );
	void	BoogieThink( void );
	void	ZapThink();
	
	float m_flStartTime;
	float m_flBoogieLength;
	float m_flMagnitude;
	int	m_nSuppressionCount;

#ifdef MAPBASE
	Vector m_vecColor = Vector(1, 1, 1);
#endif
};

#endif // RAGDOLLBOOGIE_H

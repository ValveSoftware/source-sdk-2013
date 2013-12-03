//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENTITYDISSOLVE_H
#define ENTITYDISSOLVE_H

#ifdef _WIN32
#pragma once
#endif

class CEntityDissolve : public CBaseEntity 
{
public:
	DECLARE_SERVERCLASS();
	DECLARE_CLASS( CEntityDissolve, CBaseEntity );

	CEntityDissolve( void );
	~CEntityDissolve( void );

	static CEntityDissolve	*Create( CBaseEntity *pTarget, const char *pMaterialName, 
		float flStartTime, int nDissolveType = 0, bool *pRagdollCreated = NULL );
	static CEntityDissolve	*Create( CBaseEntity *pTarget, CBaseEntity *pSource );
	
	void	Precache();
	void	Spawn();
	void	AttachToEntity( CBaseEntity *pTarget );
	void	SetStartTime( float flStartTime );
	void	SetDissolverOrigin( Vector vOrigin ) { m_vDissolverOrigin = vOrigin; }
	void	SetMagnitude( int iMagnitude ){ m_nMagnitude = iMagnitude; }
	void	SetDissolveType( int iType ) { m_nDissolveType = iType;	}

	Vector	GetDissolverOrigin( void ) 
	{ 
		Vector vReturn = m_vDissolverOrigin; 
		return vReturn;	
	}
	int		GetMagnitude( void ) { return m_nMagnitude;	}
	int		GetDissolveType( void ) { return m_nDissolveType;	}

	DECLARE_DATADESC();

	CNetworkVar( float, m_flStartTime );
	CNetworkVar( float, m_flFadeInStart );
	CNetworkVar( float, m_flFadeInLength );
	CNetworkVar( float, m_flFadeOutModelStart );
	CNetworkVar( float, m_flFadeOutModelLength );
	CNetworkVar( float, m_flFadeOutStart );
	CNetworkVar( float, m_flFadeOutLength );

protected:
	void	InputDissolve( inputdata_t &inputdata );
	void	DissolveThink( void );
	void	ElectrocuteThink( void );

	CNetworkVar( int, m_nDissolveType );
	CNetworkVector( m_vDissolverOrigin );
	CNetworkVar( int, m_nMagnitude );
};

#endif // ENTITYDISSOLVE_H

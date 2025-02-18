//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ENV_ALYXEMP_SHARED_H
#define ENV_ALYXEMP_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#include "beam_shared.h"

enum
{
	ALYXEMP_STATE_OFF,
	ALYXEMP_STATE_CHARGING,
	ALYXEMP_STATE_DISCHARGING,
};

class CAlyxEmpEffect : public CBaseEntity
{
	DECLARE_CLASS( CAlyxEmpEffect, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:

	void	InputStartCharge( inputdata_t &inputdata );
	void	InputStartDischarge( inputdata_t &inputdata );
	void	InputStop( inputdata_t &inputdata );
	void	InputSetTargetEnt( inputdata_t &inputdata );

	void	StartCharge( float flDuration );
	void	StartDischarge();
	void	Stop( float flDuration );
	void	SetTargetEntity( CBaseEntity *pTarget );

	void	ActivateAutomatic( CBaseEntity *pAlyx, CBaseEntity *pTarget );
	void	AutomaticThink();

	void	Spawn( void );
	void	Precache( void );
	void	Activate( void );

private:

	void	SetTargetEntity( const char *szEntityName );
	CHandle<CBeam>			m_hBeam;
	CHandle<CBaseEntity>	m_hTargetEnt;
	string_t				m_strTargetName;
	int						m_nType;			// What type of effect this is (small, large)
	int						m_iState;
	bool					m_bAutomated;

	CNetworkVar( int, m_nState );
	CNetworkVar( float, m_flDuration );
	CNetworkVar( float, m_flStartTime );
};


#endif // ENV_ALYXEMP_SHARED_H

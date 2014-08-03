//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef CITADEL_EFFECTS_SHARED_H
#define CITADEL_EFFECTS_SHARED_H
#ifdef _WIN32
#pragma once
#endif

#define	SF_ENERGYCORE_NO_PARTICLES	(1<<0)
#define	SF_ENERGYCORE_START_ON		(1<<1)

enum
{
	ENERGYCORE_STATE_OFF,
	ENERGYCORE_STATE_CHARGING,
	ENERGYCORE_STATE_DISCHARGING,
};

#ifndef CLIENT_DLL

// ============================================================================
//
//  Energy core - charges up and then releases energy from its position
//
// ============================================================================

class CCitadelEnergyCore : public CBaseEntity
{
	DECLARE_CLASS( CCitadelEnergyCore, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

public:
	void	InputStartCharge( inputdata_t &inputdata );
	void	InputStartDischarge( inputdata_t &inputdata );
	void	InputStop( inputdata_t &inputdata );
	void	SetScale( float flScale ) { m_flScale = flScale; }

	void	StartCharge( float flWarmUpTime );
	void	StartDischarge();
	void	StopDischarge( float flCoolDownTime );

	virtual int	ShouldTransmit( const CCheckTransmitInfo *pInfo );
	virtual int UpdateTransmitState( void );

	virtual void Precache();
	void	Spawn( void );

private:
	CNetworkVar( float, m_flScale );
	CNetworkVar( int, m_nState );
	CNetworkVar( float, m_flDuration );
	CNetworkVar( float, m_flStartTime );
};

#endif

#endif // CITADEL_EFFECTS_SHARED_H

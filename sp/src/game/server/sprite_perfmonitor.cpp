//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: an entity which turns on and off counting and display of the particle
// performance metric
//
//=============================================================================

#include "cbase.h"
#include "baseentity.h"
#include "entityoutput.h"
#include "convar.h"
// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Purpose: Entity that particle performance measuring
//-----------------------------------------------------------------------------
class CParticlePerformanceMonitor : public CPointEntity
{
	DECLARE_CLASS( CParticlePerformanceMonitor, CPointEntity );
public:
	DECLARE_DATADESC();
	DECLARE_SERVERCLASS();

	void	Spawn( void );
	int		UpdateTransmitState( void );

	// Inputs
	void	InputTurnOnDisplay( inputdata_t &inputdata );
	void	InputTurnOffDisplay( inputdata_t &inputdata );
	void	InputStartMeasuring( inputdata_t &inputdata );
	void	InputStopMeasuring( inputdata_t &inputdata );

private:
	CNetworkVar( bool, m_bDisplayPerf );
	CNetworkVar( bool, m_bMeasurePerf );
};

LINK_ENTITY_TO_CLASS( env_particle_performance_monitor, CParticlePerformanceMonitor );

BEGIN_DATADESC( CParticlePerformanceMonitor )
	DEFINE_FIELD( m_bDisplayPerf, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bMeasurePerf, FIELD_BOOLEAN ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOnDisplay", InputTurnOnDisplay ),
	DEFINE_INPUTFUNC( FIELD_VOID, "TurnOffDisplay", InputTurnOffDisplay ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartMeasuring", InputStartMeasuring ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopMeasuring", InputStopMeasuring ),
END_DATADESC()

IMPLEMENT_SERVERCLASS_ST( CParticlePerformanceMonitor, DT_ParticlePerformanceMonitor )
	SendPropInt( SENDINFO(m_bDisplayPerf), 1, SPROP_UNSIGNED ),
	SendPropInt( SENDINFO(m_bMeasurePerf), 1, SPROP_UNSIGNED ),
END_SEND_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CParticlePerformanceMonitor::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	m_bDisplayPerf = false;
	m_bMeasurePerf = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
int CParticlePerformanceMonitor::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

void CParticlePerformanceMonitor::InputTurnOnDisplay( inputdata_t &inputdata )
{
	m_bDisplayPerf = true;
}

void CParticlePerformanceMonitor::InputTurnOffDisplay( inputdata_t &inputdata )
{
	m_bDisplayPerf = false;
}

void CParticlePerformanceMonitor::InputStartMeasuring( inputdata_t &inputdata )
{
	m_bMeasurePerf = true;
}

void CParticlePerformanceMonitor::InputStopMeasuring( inputdata_t &inputdata )
{
	m_bMeasurePerf = false;
}


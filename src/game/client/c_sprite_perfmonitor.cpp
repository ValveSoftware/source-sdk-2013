//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"

extern bool g_bMeasureParticlePerformance;
extern bool g_bDisplayParticlePerformance;

void ResetParticlePerformanceCounters( void );

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_ParticlePerformanceMonitor : public C_BaseEntity
{
	DECLARE_CLASS( C_ParticlePerformanceMonitor, C_BaseEntity );
public:
	DECLARE_CLIENTCLASS();

	C_ParticlePerformanceMonitor();
	~C_ParticlePerformanceMonitor();
	virtual void	OnDataChanged( DataUpdateType_t updateType );

private:
	bool m_bDisplayPerf;
	bool m_bMeasurePerf;
private:
	C_ParticlePerformanceMonitor( const C_ParticlePerformanceMonitor & );
};

IMPLEMENT_CLIENTCLASS_DT( C_ParticlePerformanceMonitor, DT_ParticlePerformanceMonitor, CParticlePerformanceMonitor )
	RecvPropInt( RECVINFO(m_bMeasurePerf) ),
	RecvPropInt( RECVINFO(m_bDisplayPerf) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ParticlePerformanceMonitor::C_ParticlePerformanceMonitor( void )
{
	m_bDisplayPerf = false;
	m_bMeasurePerf = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_ParticlePerformanceMonitor::~C_ParticlePerformanceMonitor( void )
{
	g_bMeasureParticlePerformance = false;
	g_bDisplayParticlePerformance = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_ParticlePerformanceMonitor::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged(updateType);

	if ( m_bMeasurePerf && ( ! g_bMeasureParticlePerformance ) )
		ResetParticlePerformanceCounters();
	g_bMeasureParticlePerformance = m_bMeasurePerf;
	g_bDisplayParticlePerformance = m_bDisplayPerf;
}


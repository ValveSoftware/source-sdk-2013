//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Real-Time Hierarchical Telemetry Profiling
//
// $NoKeywords: $
//=============================================================================//

#ifndef VPROF_TELEMETRY_H
#define VPROF_TELEMETRY_H

#if !defined( MAKE_VPC )


#endif // !MAKE_VPC


#if !defined( RAD_TELEMETRY_ENABLED )

//
// Kill all tmZone() macros, etc.
//
#include "tmapi_dummy.h"

inline void TelemetryTick() {}
inline void TelemetrySetLevel( unsigned int Level ) {}
#define TelemetrySetLockName( _ctx, _location, _description ) 

class CTelemetryLock
{
public:
	CTelemetryLock(void *plocation, const char *description) {}
	~CTelemetryLock() {}
	void Locked() {}
	void Unlocked() {}
};

class CTelemetrySpikeDetector
{
public:
	CTelemetrySpikeDetector( const char *msg, uint32 threshold = 50 ) {}
	~CTelemetrySpikeDetector() { }
};

#define TM_ZONE_DEFAULT( context ) 
#define TM_ZONE_DEFAULT_PARAM( context, string_param ) 

#else

//
// Telemetry is enabled. Include the telemetry header.
//
#include "../../thirdparty/telemetry/include/telemetry.h"
// Different versions of radbase.h define RADCOPYRIGHT to different values. So undef that here.
#undef RADCOPYRIGHT

struct TelemetryData
{
	HTELEMETRY tmContext[32];
	float flRDTSCToMilliSeconds;	// Conversion from tmFastTime() (rdtsc) to milliseconds.
	uint32 FrameCount;				// Count of frames to capture before turning off.
	char ServerAddress[128];		// Server name to connect to.
	int playbacktick;				// GetPlaybackTick() value from demo file (or 0 if not playing a demo).
	uint32 DemoTickStart;			// Start telemetry on demo tick #
	uint32 DemoTickEnd;				// End telemetry on demo tick #
	uint32 Level;					// Current Telemetry level (Use TelemetrySetLevel to modify)
};
PLATFORM_INTERFACE TelemetryData g_Telemetry;

PLATFORM_INTERFACE void TelemetryTick();
PLATFORM_INTERFACE void TelemetrySetLevel( unsigned int Level );

#define TELEMETRY_LEVEL0	g_Telemetry.tmContext[0]	// high level tmZone()
#define TELEMETRY_LEVEL1	g_Telemetry.tmContext[1]	// lower level tmZone(), tmZoneFiltered()
#define TELEMETRY_LEVEL2	g_Telemetry.tmContext[2]	// VPROF_0
#define TELEMETRY_LEVEL3	g_Telemetry.tmContext[3]	// VPROF_1
#define TELEMETRY_LEVEL4	g_Telemetry.tmContext[4]	// VPROF_2
#define TELEMETRY_LEVEL5	g_Telemetry.tmContext[5]	// VPROF_3
#define TELEMETRY_LEVEL6	g_Telemetry.tmContext[6]	// VPROF_4

#define TM_ZONE_DEFAULT( context ) tmZone(context, TMZF_NONE, __FUNCTION__ )
#define TM_ZONE_DEFAULT_PARAM( context, string_param ) tmZone(context, TMZF_NONE, "%s( %s )", __FUNCTION__ , tmDynamicString( context, (string_param) ) )

#define TelemetrySetLockName( _ctx, _location, _description ) \
	do  													  \
	{   													  \
		static bool s_bNameSet = false; 					  \
		if( _ctx && !s_bNameSet )							  \
		{   												  \
			tmLockName( _ctx, _location, _description ); 	  \
			s_bNameSet = true;  							  \
		}   												  \
	} while( 0 )

class CTelemetryLock
{
public:
	CTelemetryLock(void *plocation, const char *description)
	{
		m_plocation = (const char *)plocation;
		m_description = description;
		TelemetrySetLockName( TELEMETRY_LEVEL1, m_plocation, m_description );
		tmTryLock( TELEMETRY_LEVEL1, m_plocation, "%s", m_description );
	}
	~CTelemetryLock()
	{
		Unlocked();
	}
	void Locked()
	{
		tmEndTryLock( TELEMETRY_LEVEL1, m_plocation, TMLR_SUCCESS );
		tmSetLockState( TELEMETRY_LEVEL1, m_plocation, TMLS_LOCKED, "%s Locked", m_description );
	}
	void Unlocked()
	{
		if( m_plocation )
		{
			tmSetLockState( TELEMETRY_LEVEL1, m_plocation, TMLS_RELEASED, "%s Released", m_description );
			m_plocation = NULL;
		}
	}

public:
	const char *m_plocation;
	const char *m_description;
};

class CTelemetrySpikeDetector
{
public:
	// Spews Telemetry message when threshold hit (in milliseconds.)
	CTelemetrySpikeDetector( const char *msg, float threshold = 5 ) :
		m_message( msg ), m_threshold( threshold ), time0( tmFastTime() ) {}
	~CTelemetrySpikeDetector()
	{
		float time = ( tmFastTime() - time0 ) * g_Telemetry.flRDTSCToMilliSeconds;
		if( time >= m_threshold )
		{
			tmMessage( TELEMETRY_LEVEL0, TMMF_ICON_NOTE | TMMF_SEVERITY_WARNING, "(source/spike)%s %.2fms %t", m_message, time, tmSendCallStack( TELEMETRY_LEVEL0, 0 ) );
		}
	}

private:
	TmU64 time0;
	float m_threshold;
	const char *m_message;
};

#endif // RAD_TELEMETRY_ENABLED

#endif // VPROF_TELEMETRY_H

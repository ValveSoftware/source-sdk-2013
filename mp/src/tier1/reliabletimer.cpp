//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================

#include "tier1/reliabletimer.h"

int64 CReliableTimer::sm_nPerformanceFrequency = 0;
bool CReliableTimer::sm_bUseQPC = false;

#ifdef _WIN32
#include "winlite.h"
#endif

//-----------------------------------------------------------------------------
// Purpose: Constructor
//-----------------------------------------------------------------------------
CReliableTimer::CReliableTimer()
{
	m_nPerformanceCounterStart = 0;
	m_nPerformanceCounterEnd = 0;
	m_nPerformanceCounterLimit = 0;

#ifdef _WIN32
	// calculate performance frequency the first time we use a timer
	if ( 0 == sm_nPerformanceFrequency )
	{
		// Are we on a bad CPU?
		sm_bUseQPC = false; // todo
		const CPUInformation &cpu = *GetCPUInformation();
		sm_bUseQPC = ( ( 0 == Q_stricmp( cpu.m_szProcessorID, "AuthenticAMD" ) )
			&& ( cpu.m_nPhysicalProcessors > 1 )
			&& !cpu.m_bSSE41 );

		if ( sm_bUseQPC )
		{
			LARGE_INTEGER li;
			QueryPerformanceFrequency( &li );
			sm_nPerformanceFrequency = li.QuadPart;
		}
		else
		{
			sm_nPerformanceFrequency = g_ClockSpeed;
		}
	}
#elif defined(_PS3)
	// On PowerPC, the time base register increment frequency is implementation dependent, and doesn't have to be constant.
	// On PS3, measured it to be just shy of 80Mhz on the PPU and doesn't seem to change
	if ( sm_nPerformanceFrequency == 0 )
		sm_nPerformanceFrequency = sys_time_get_timebase_frequency(); 
#else
	// calculate performance frequency the first time we use a timer
	if ( 0 == sm_nPerformanceFrequency )
	{
		sm_nPerformanceFrequency = g_ClockSpeed;
	}
#endif
}


//-----------------------------------------------------------------------------
// Purpose: Returns current QueryPerformanceCounter value
//-----------------------------------------------------------------------------
int64 CReliableTimer::GetPerformanceCountNow()
{
	//VPROF_BUDGET( "CReliableTimer::GetPerformanceCountNow", VPROF_BUDGETGROUP_OTHER_UNACCOUNTED );
#ifdef _WIN32
	if ( sm_bUseQPC )
	{
		LARGE_INTEGER li = {0};
		QueryPerformanceCounter( &li );
		return li.QuadPart;
	}
	else
	{
		CCycleCount CycleCount;
		CycleCount.Sample();
		return CycleCount.GetLongCycles();
	}
#elif defined( _PS3 )
	// use handy macro to grab tb
	uint64 ulNow;
	SYS_TIMEBASE_GET( ulNow );
	return ulNow;
#else
	uint64 un64;
	 __asm__ __volatile__ (
                        "rdtsc\n\t"
                        : "=A" (un64) );
	return (int64)un64;
#endif
}

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef PMELIB_H
#define PMELIB_H

#include "Windows.h"
#include "tier0/platform.h"

// Get rid of a bunch of STL warnings!
#pragma warning( push, 3 )
#pragma warning( disable : 4018 )

#define VERSION "1.0.2"

// uncomment this list to add some runtime checks
//#define PME_DEBUG   

#include "tier0/valve_off.h"
#include <string>
#include "tier0/valve_on.h"

using namespace std;

// RDTSC Instruction macro
#define RDTSC(var) var = __rdtsc()

// RDPMC Instruction macro
#define RDPMC(counter, var) \
_asm mov ecx, counter \
_asm RDPMC \
_asm mov DWORD PTR var,eax \
_asm mov DWORD PTR var+4,edx

// RDPMC Instruction macro, for performance counter 1 (ecx = 1)
#define RDPMC0(var) \
_asm mov ecx, 0 \
_asm RDPMC \
_asm mov DWORD PTR var,eax \
_asm mov DWORD PTR var+4,edx

#define RDPMC1(var) \
_asm mov ecx, 1 \
_asm RDPMC \
_asm mov DWORD PTR var,eax \
_asm mov DWORD PTR var+4,edx

#define EVENT_TYPE(mode) EventType##mode
#define EVENT_MASK(mode) EventMask##mode

#include "ia32detect.h"    

enum ProcessPriority
{
    ProcessPriorityNormal,
    ProcessPriorityHigh,
};

enum PrivilegeCapture
{
    OS_Only,  // ring 0, kernel level
    USR_Only, // app level
    OS_and_USR, // all levels
};

enum CompareMethod
{
    CompareGreater,  // 
    CompareLessEqual, // 
};

enum EdgeState
{
    RisingEdgeDisabled, // 
    RisingEdgeEnabled,  // 
};

enum CompareState
{
    CompareDisable, // 
    CompareEnable,  // 
};

// Singletion Class
class PME : public ia32detect
{
public:
//private:

    static PME*		_singleton;

    HANDLE			hFile;
    bool			bDriverOpen;
    double			m_CPUClockSpeed;

    //ia32detect detect;
    HRESULT Init();
    HRESULT Close();

protected:

    PME()
    {
        hFile = NULL;
        bDriverOpen = FALSE;
        m_CPUClockSpeed = 0;
        Init(); 
    }

public:

    static PME* Instance();  // gives back a real object

    ~PME()
    {
        Close();
    }

    double GetCPUClockSpeedSlow( void );
    double GetCPUClockSpeedFast( void );

    HRESULT SelectP5P6PerformanceEvent( uint32 dw_event, uint32 dw_counter, bool b_user, bool b_kernel );

    HRESULT ReadMSR( uint32 dw_reg, int64 * pi64_value );
    HRESULT ReadMSR( uint32 dw_reg, uint64 * pi64_value );

    HRESULT WriteMSR( uint32 dw_reg, const int64 & i64_value );
    HRESULT WriteMSR( uint32 dw_reg, const uint64 & i64_value );

    void SetProcessPriority( ProcessPriority priority )
    {
        switch( priority )
        {
        case ProcessPriorityNormal:
			{
				SetPriorityClass (GetCurrentProcess(),NORMAL_PRIORITY_CLASS);
				SetThreadPriority (GetCurrentThread(),THREAD_PRIORITY_NORMAL);
				break;
			}
        case ProcessPriorityHigh:
			{
				SetPriorityClass (GetCurrentProcess(),REALTIME_PRIORITY_CLASS);
				SetThreadPriority (GetCurrentThread(),THREAD_PRIORITY_HIGHEST);
				break;
			}
        }
    }

    //---------------------------------------------------------------------------
    // Return the family of the processor
    //---------------------------------------------------------------------------
    CPUVendor GetVendor(void)
    {
        return vendor;
    }

    int GetProcessorFamily(void)
    {
        return version.Family;
    }

#ifdef DBGFLAG_VALIDATE
	void Validate( CValidator &validator, tchar *pchName );		// Validate our internal structures
#endif // DBGFLAG_VALIDATE

};

#include "P5P6PerformanceCounters.h"    
#include "P4PerformanceCounters.h"    
#include "K8PerformanceCounters.h"    

enum PerfErrors
{
    E_UNKNOWN_CPU_VENDOR	= -1,
    E_BAD_COUNTER			= -2,
    E_UNKNOWN_CPU			= -3,
    E_CANT_OPEN_DRIVER		= -4,
    E_DRIVER_ALREADY_OPEN	= -5,
    E_DRIVER_NOT_OPEN		= -6,
    E_DISABLED				= -7,
    E_BAD_DATA				= -8,
    E_CANT_CLOSE			= -9,
    E_ILLEGAL_OPERATION		= -10,
};

#pragma warning( pop )

#endif // PMELIB_H
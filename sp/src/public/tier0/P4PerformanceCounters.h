//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef P4PERFORMANCECOUNTERS_H
#define P4PERFORMANCECOUNTERS_H

#pragma once
// Pentium 4 support

/*
    http://developer.intel.com/design/Pentium4/documentation.htm

    IA-32 Intel Architecture Software Developer's Manual Volume 1: Basic Architecture 

    IA-32 Intel Architecture Software Developer's Manual Volume 2A: Instruction Set Reference, A-M 

    IA-32 Intel Architecture Software Developer's Manual Volume 2B: Instruction Set Reference, N-Z 

    IA-32 Intel Architecture Software Developer's Manual Volume 3: System Programming Guide 


    From Mikael Pettersson's perfctr:
    
        http://user.it.uu.se/~mikpe/linux/perfctr/

     * Known quirks:
 - OVF_PMI+FORCE_OVF counters must have an ireset value of -1.
   This allows the regular overflow check to also handle FORCE_OVF
   counters. Not having this restriction would lead to MAJOR
   complications in the driver's "detect overflow counters" code.
   There is no loss of functionality since the ireset value doesn't
   affect the counter's PMI rate for FORCE_OVF counters.

 - In experiments with FORCE_OVF counters, and regular OVF_PMI
   counters with small ireset values between -8 and -1, it appears
   that the faulting instruction is subjected to a new PMI before
   it can complete, ad infinitum. This occurs even though the driver
   clears the CCCR (and in testing also the ESCR) and invokes a
   user-space signal handler before restoring the CCCR and resuming
   the instruction.
*/

#define NCOUNTERS 18

// The 18 counters
enum Counters
{
    MSR_BPU_COUNTER0,
    MSR_BPU_COUNTER1,
    MSR_BPU_COUNTER2,
    MSR_BPU_COUNTER3,
    MSR_MS_COUNTER0,
    MSR_MS_COUNTER1,
    MSR_MS_COUNTER2,
    MSR_MS_COUNTER3,
    MSR_FLAME_COUNTER0,
    MSR_FLAME_COUNTER1,
    MSR_FLAME_COUNTER2,
    MSR_FLAME_COUNTER3,
    MSR_IQ_COUNTER0,
    MSR_IQ_COUNTER1,
    MSR_IQ_COUNTER2,
    MSR_IQ_COUNTER3,
    MSR_IQ_COUNTER4,
    MSR_IQ_COUNTER5
};

// register base for counters
#define MSR_COUNTER_BASE    0x300

// register base for CCCR register
#define MSR_CCCR_BASE       0x360

#pragma pack(push, 1)
// access to these bits is through the methods
typedef union ESCR
{
    struct
    {
        uint64	Reserved0_1     : 2; // 
        uint64	USR			    : 1; // 
        uint64	OS			    : 1; // 
        uint64	TagEnable       : 1; //  
        uint64	TagValue        : 4; //  
        uint64	EventMask	    : 16; // from event select
        uint64	ESCREventSelect	: 6; // 31:25 class of event
        uint64	Reserved31	    : 1; // 

        uint64	Reserved32_63	: 32; // 
    };
    uint64 flat;

} ESCR;

typedef union CCCR
{
    struct
    {
        uint64	Reserved0_11    : 12;// 0 -11 
        uint64	Enable		    : 1; // 12
        uint64	CCCRSelect	    : 3; // 13-15
        uint64	Reserved16_17   : 2; // 16 17

        uint64	Compare         : 1; // 18
        uint64	Complement	    : 1; // 19
        uint64	Threshold	    : 4; // 20-23
        uint64	Edge	        : 1; // 24
        uint64	FORCE_OVF	    : 1; // 25
        uint64	OVF_PMI	        : 1; // 26
        uint64	Reserved27_29	: 3; // 27-29
        uint64	Cascade	        : 1; // 30
        uint64	OVF	            : 1; // 31

        uint64	Reserved32_63	: 32; // 
    };
    uint64 flat;

} CCCR;

#pragma pack(pop)

extern const unsigned short cccr_escr_map[NCOUNTERS][8];

enum P4TagState
{
    TagDisable, // 
    TagEnable,  // 
};

enum P4ForceOverflow
{
    ForceOverflowDisable,
    ForceOverflowEnable,
};

enum P4OverflowInterrupt
{
    OverflowInterruptDisable,
    OverflowInterruptEnable,
};

// Turn off the no return value warning in ReadCounter.
#pragma warning( disable : 4035 )
class P4BaseEvent
{
    int m_counter;

protected:

    void SetCounter(int counter)
    {
        m_counter  = counter;
        cccrPort = MSR_CCCR_BASE + m_counter;
        counterPort = MSR_COUNTER_BASE + m_counter;
        escrPort = cccr_escr_map[m_counter][cccr.CCCRSelect];
    }

public:

    unsigned short	m_eventMask;
    const tchar		*description;
    PME				*pme;
    ESCR			escr;
    CCCR			cccr;
    int				counterPort;
    int				cccrPort;
    int				escrPort;

    P4BaseEvent()
    {
        pme = PME::Instance();
        m_eventMask = 0;
        description = _T("");
        escr.flat = 0;
        cccr.flat = 0;
        cccr.Reserved16_17 = 3;    // must be set
        escrPort = 0;
        m_counter = -1;
    }

    void StartCounter()
    {
        cccr.Enable = 1;
        pme->WriteMSR( cccrPort, cccr.flat );
    }

    void StopCounter()
    {
        cccr.Enable = 0;
        pme->WriteMSR( cccrPort, cccr.flat );
    }

    void ClearCounter()
    {
        pme->WriteMSR( counterPort, 0ui64 ); // clear
    }

    void WriteCounter( int64 value )
    {
        pme->WriteMSR( counterPort, value ); // clear
    }

    int64 ReadCounter()
    {
#if PME_DEBUG
        if ( escr.USR == 0 && escr.OS == 0 )
            return -1; // no area to collect, use SetCaptureMode

        if ( escr.EventMask == 0 )
            return -2;  // no event mask set

        if ( m_counter == -1 )
            return -3; // counter not legal
#endif

        // ReadMSR should work here too, but RDPMC should be faster
		int64 value = 0;
        pme->ReadMSR( counterPort, &value );
		return value;
#if 0
        // we need to copy this into a temp for some reason
        int temp = m_counter;
        _asm 
        {
            mov ecx, temp
            RDPMC 
        }
#endif
    }

    void SetCaptureMode( PrivilegeCapture priv )
    {
        switch ( priv )
        {
        case OS_Only:
			{
				escr.USR = 0;
				escr.OS = 1;
				break;
			}
        case USR_Only:
			{
	            escr.USR = 1;
		        escr.OS = 0;
			    break;
			}
        case OS_and_USR:
			{
	            escr.USR = 1;
		        escr.OS = 1;
			    break;
			}
        }

        escr.EventMask = m_eventMask;
        pme->WriteMSR( escrPort, escr.flat );
    }

    void SetTagging( P4TagState tagEnable, uint8 tagValue )
    {
        escr.TagEnable = tagEnable; 
        escr.TagValue = tagValue;
        pme->WriteMSR( escrPort, escr.flat );
    }

    void SetFiltering( CompareState compareEnable, CompareMethod compareMethod, uint8 threshold, EdgeState edgeEnable )
    {
        cccr.Compare = compareEnable;
        cccr.Complement = compareMethod;
        cccr.Threshold = threshold;
        cccr.Edge = edgeEnable; 
        pme->WriteMSR( cccrPort, cccr.flat );
    }

    void SetOverflowEnables( P4ForceOverflow overflowEnable, P4OverflowInterrupt overflowInterruptEnable )
    {
        cccr.FORCE_OVF = overflowEnable;
        cccr.OVF_PMI = overflowInterruptEnable;
        pme->WriteMSR( cccrPort, cccr.flat );
    }

    void SetOverflow()
    {
        cccr.OVF = 1;
        pme->WriteMSR( cccrPort, cccr.flat );
    }

    void ClearOverflow()
    {
        cccr.OVF = 0;
        pme->WriteMSR( cccrPort, cccr.flat );
    }

    bool isOverflow()
    {
        CCCR cccr_temp;
        pme->ReadMSR( cccrPort, &cccr_temp.flat );
        return cccr_temp.OVF;
    }

    void SetCascade()
    {
        cccr.Cascade = 1;
        pme->WriteMSR( cccrPort, cccr.flat );
    }

    void ClearCascade()
    {
        cccr.Cascade = 0;
        pme->WriteMSR( cccrPort, cccr.flat );
    }
};
#pragma warning( default : 4035 )

#include "EventMasks.h" 
#include "EventModes.h" 

#endif // P4PERFORMANCECOUNTERS_H

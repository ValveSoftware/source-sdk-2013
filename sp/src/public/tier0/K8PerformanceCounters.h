//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef K8PERFORMANCECOUNTERS_H
#define K8PERFORMANCECOUNTERS_H

/*
 * AMD K8 events.
 *
 */



typedef union EVENT_MASK(NULL_MASK)
{
    // no tests defined
    uint16 flat;
} EVENT_MASK(NULL_MASK);


#define MSR_K8_EVNTSEL0		0xC0010000	/* .. 0xC0010003 */
#define MSR_K8_PERFCTR0		0xC0010004	/* .. 0xC0010007 */

#	pragma pack(push, 1)



// access to these bits is through the methods
typedef union PerfEvtSel
{
    struct
    {
        uint64	EventMask   : 8; 

        uint64	UnitMask    : 8; 
        uint64	USR         : 1; 
        uint64	OS          : 1; 
        uint64	Edge        : 1; 
        uint64	PC          : 1; 
        uint64	INTAPIC     : 1; 
        uint64	Reserved21  : 1; 
        uint64	Enable      : 1; 
        uint64	Complement  : 1; // aka INV
        uint64	Threshold   : 8; // aka CounterMask
        uint64  Reserver32  : 32;
    };
    uint64 flat;

} PerfEvtSel;


enum UnitEncode
{
    FP,
    LS,
    DC,
    BU,
    IC,
    UE_Unknown,
    FR,
    NB
};

#	pragma pack(pop)

// Turn off the no return value warning in ReadCounter.
#pragma warning( disable : 4035 )		
#define k8NUM_COUNTERS 4
class k8BaseEvent
{
public:

    PME * pme;

    PerfEvtSel eventSelect[k8NUM_COUNTERS];

    unsigned short m_eventMask;
    int event_id;
    tchar * name;
    tchar revRequired;
    int eventSelectNum;
    UnitEncode unitEncode;


    void SetCounter(int n)
    {
        if (n < 0)
            n = 0;
        else if (n > 3)
            n = 3;
        eventSelectNum = n;

    }
    k8BaseEvent()
    {
        pme = PME::Instance();

        for(int i = 0; i< k8NUM_COUNTERS; i++)
        {
            eventSelect[i].flat = 0;

        }
        eventSelectNum = 0;

        m_eventMask = 0;
        event_id = 0;
        name = 0;
        revRequired = 'A'; 


    }

    void SetCaptureMode(PrivilegeCapture priv)
    {
        PerfEvtSel & select = eventSelect[eventSelectNum]; 
        StopCounter();

        switch (priv)
        {
        case OS_Only:
            select.USR = 0;
            select.OS = 1;
            break;

        case USR_Only:
            select.USR = 1;
            select.OS = 0;
            break;

        case OS_and_USR:
            select.USR = 1;
            select.OS = 1;
            break;
        }

       
        select.UnitMask = m_eventMask;
        select.EventMask = event_id;
        

        int selectPort = MSR_K8_EVNTSEL0 + eventSelectNum;
        pme->WriteMSR(selectPort, select.flat);
    }


    void SetFiltering(CompareState compareEnable,
        CompareMethod compareMethod,
        uint8 threshold,
        EdgeState edgeEnable)
    {

        PerfEvtSel & select = eventSelect[eventSelectNum]; 
        
        StopCounter();

        if (compareEnable == CompareDisable)
            select.Threshold = 0;
        else
            select.Threshold = threshold;

        select.Complement = compareMethod;

        select.Edge = edgeEnable;

        int selectPort = MSR_K8_EVNTSEL0 + eventSelectNum;
        pme->WriteMSR(selectPort, select.flat);


    }

    
    void StartCounter()
    {
        PerfEvtSel & select = eventSelect[eventSelectNum]; 

        select.Enable = 1;
        int selectPort = MSR_K8_EVNTSEL0 + eventSelectNum;

        pme->WriteMSR(selectPort, select.flat);

    }
    void StopCounter()
    {
        PerfEvtSel & select = eventSelect[eventSelectNum]; 
        select.Enable = 0;
        int selectPort = MSR_K8_EVNTSEL0 + eventSelectNum;

        pme->WriteMSR(selectPort, select.flat);
    }



    void ClearCounter()
    {
        PerfEvtSel & select = eventSelect[eventSelectNum]; 
        
        int counterPort = MSR_K8_PERFCTR0 + eventSelectNum;

        pme->WriteMSR(counterPort, 0ui64 ); // clear
    }

    void WriteCounter(int64 value)
    {

        PerfEvtSel & select = eventSelect[eventSelectNum]; 

        int counterPort = MSR_K8_PERFCTR0 + eventSelectNum;
        pme->WriteMSR(counterPort, value); // clear
    }

    int64 ReadCounter()
    {

#if PME_DEBUG
        PerfEvtSel & select = eventSelect[eventSelectNum]; 

        if (select.USR == 0 && select.OS == 0)
            return -1; // no area to collect, use SetCaptureMode

        if (select.EventMask == 0)
            return -2;  // no event mask set

        if (eventSelectNum < 0 || eventSelectNum > 3)
            return -3; // counter not legal

        // check revision

#endif

        // ReadMSR should work here too, but RDPMC should be faster
        //ReadMSR(counterPort, int64); 

        // we need to copy this into a temp for some reason
#ifdef COMPILER_MSVC64
	return __readpmc((unsigned long) eventSelectNum);
#else
        int temp = eventSelectNum;
        _asm 
        {
            mov ecx, temp
            RDPMC 
        }
#endif

	}


};
#pragma warning( default : 4035 )




typedef union EVENT_MASK(k8_dispatched_fpu_ops)
{
    // event 0
    struct 
    {
        uint16 AddPipeOps:1; // Add pipe ops excluding junk ops" },
        uint16 MulPipeOps:1; // Multiply pipe ops excluding junk ops" },,
        uint16 StoreOps:1; // Store pipe ops excluding junk ops" },
        uint16 AndPipeOpsJunk:1; // Add pipe junk ops" },,
        uint16 MulPipeOpsJunk:1; // Multiply pipe junk ops" },
        uint16 StoreOpsJunk:1; // Store pipe junk ops" } }
    };
    uint16 flat;
} EVENT_MASK(k8_dispatched_fpu_ops);

class k8Event_DISPATCHED_FPU_OPS : public k8BaseEvent
{ 
public:

    k8Event_DISPATCHED_FPU_OPS()
    {
        eventMask = (EVENT_MASK(k8_dispatched_fpu_ops) *)&m_eventMask;

        event_id = 0x00;
        unitEncode = FP;
        name = _T("Dispatched FPU ops");
        revRequired = 'B';
    } 
    EVENT_MASK(k8_dispatched_fpu_ops) * eventMask;

};

//////////////////////////////////////////////////////////



class k8Event_NO_FPU_OPS : public k8BaseEvent
{ 
public:

    k8Event_NO_FPU_OPS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        event_id = 0x01;
        unitEncode = FP;

        name = _T("Cycles with no FPU ops retired");
        revRequired = 'B';
    } 
    EVENT_MASK(NULL_MASK) * eventMask;

};

//////////////////////////////////////////////////////////

class k8Event_FAST_FPU_OPS : public k8BaseEvent
{ 
public:

    k8Event_FAST_FPU_OPS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        event_id = 0x02;
        unitEncode = FP;

        name = _T("Dispatched FPU ops that use the fast flag interface");
        revRequired = 'B';

    } 
    EVENT_MASK(NULL_MASK) * eventMask;

};


//////////////////////////////////////////////////////////


typedef union EVENT_MASK(k8_segment_register_load)
{

    struct 
    {
        uint16 ES:1; 
        uint16 CS:1; 
        uint16 SS:1; 
        uint16 DS:1; 
        uint16 FS:1; 
        uint16 GS:1; 
        uint16 HS:1; 
    };
    uint16 flat;
} EVENT_MASK(k8_segment_register_load);


class k8Event_SEG_REG_LOAD : public k8BaseEvent
{ 
public:

    k8Event_SEG_REG_LOAD()
    {
        eventMask = (EVENT_MASK(k8_segment_register_load) *)&m_eventMask;
        name = _T("Segment register load");
        event_id = 0x20;
        unitEncode = LS;

    } 
    EVENT_MASK(k8_segment_register_load) * eventMask;

};


class k8Event_SELF_MODIFY_RESYNC : public k8BaseEvent
{ 
public:

    k8Event_SELF_MODIFY_RESYNC()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Microarchitectural resync caused by self modifying code");
        event_id = 0x21;
        unitEncode = LS;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;


};
class k8Event_LS_RESYNC_BY_SNOOP : public k8BaseEvent
{ 
public:

    k8Event_LS_RESYNC_BY_SNOOP()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        event_id = 0x22;
        unitEncode = LS;

        name = _T("Microarchitectural resync caused by snoop");
    } 
    EVENT_MASK(NULL_MASK) * eventMask;


};
class k8Event_LS_BUFFER_FULL : public k8BaseEvent
{ 
public:

    k8Event_LS_BUFFER_FULL()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("LS Buffer 2 Full");
        event_id = 0x23;
        unitEncode = LS;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;

};


typedef union EVENT_MASK(k8_locked_op)
{


    struct 
    {
        uint16 NumLockInstr : 1;  //Number of lock instructions executed
        uint16 NumCyclesInRequestGrant : 1;  //Number of cycles spent in the lock request/grant stage

        uint16 NumCyclesForLock:1;  
        /*Number of cycles a lock takes to complete once it is 
        non-speculative and is the oldest load/store operation 
        (non-speculative cycles in Ls2 entry 0)*/


    };
    uint16 flat;


} EVENT_MASK(k8_locked_op);



class k8Event_LOCKED_OP : public k8BaseEvent
{ 
public:

    EVENT_MASK(k8_locked_op) * eventMask;

    k8Event_LOCKED_OP()
    {
        eventMask = (EVENT_MASK(k8_locked_op) *)&m_eventMask;
        name = _T("Locked operation");
        event_id = 0x24; 
        unitEncode = LS;

        revRequired = 'C';
    }     


};

class k8Event_OP_LATE_CANCEL : public k8BaseEvent
{ 
public:

    k8Event_OP_LATE_CANCEL()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Microarchitectural late cancel of an operation");
        event_id = 0x25; 
        unitEncode = LS;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("OP_LATE_CANCEL");


};
class k8Event_CFLUSH_RETIRED : public k8BaseEvent
{ 
public:

    k8Event_CFLUSH_RETIRED()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Retired CFLUSH instructions");
        event_id = 0x26; 
        unitEncode = LS;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("CFLUSH_RETIRED");


};
class k8Event_CPUID_RETIRED : public k8BaseEvent
{ 
public:

    k8Event_CPUID_RETIRED()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Retired CPUID instructions");
        event_id = 0x27; 
        unitEncode = LS;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("CPUID_RETIRED");


};

typedef union EVENT_MASK( k8_cache)
{

    struct 
    {
        uint16 Invalid:1;  
        uint16 Exclusive:1;   
        uint16 Shared:1;   
        uint16 Owner:1;    
        uint16 Modified:1; 
    };
    uint16 flat;

}EVENT_MASK(  k8_cache);
    /* 0x40-0x47: from K7 official event set */


class k8Event_DATA_CACHE_ACCESSES  : public k8BaseEvent 
{ 
    k8Event_DATA_CACHE_ACCESSES()
    {

        event_id = 0x40; 
        unitEncode = DC;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        //_T("DATA_CACHE_ACCESSES"),
        name = _T("Data cache accesses");
    }
    EVENT_MASK(NULL_MASK) * eventMask;

};

class k8Event_DATA_CACHE_MISSES     : public k8BaseEvent
{ 
    k8Event_DATA_CACHE_MISSES()
    {

        event_id = 0x41;
        unitEncode = DC;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        //_T("DATA_CACHE_MISSES"),
        name = _T("Data cache misses");
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};

class k8Event_DATA_CACHE_REFILLS_FROM_L2     : public k8BaseEvent
{ 
    k8Event_DATA_CACHE_REFILLS_FROM_L2()
    {

        event_id = 0x42;
        unitEncode = DC;

        eventMask = (EVENT_MASK(k8_cache) *)&m_eventMask;


        name = _T("Data cache refills from L2");
    }
    EVENT_MASK(k8_cache) * eventMask;

};

class k8Event_DATA_CACHE_REFILLS_FROM_SYSTEM     : public k8BaseEvent
{ 
    k8Event_DATA_CACHE_REFILLS_FROM_SYSTEM()
    {

        event_id = 0x43;
        unitEncode = DC;


        eventMask = (EVENT_MASK(k8_cache) *)&m_eventMask;

        //UM(k7_um_moesi), 
        //_T("DATA_CACHE_REFILLS_FROM_SYSTEM"),
        name = _T("Data cache refills from system");
    }
    EVENT_MASK(k8_cache) * eventMask;

};

class k8Event_DATA_CACHE_WRITEBACKS     : public k8BaseEvent
{ 
    k8Event_DATA_CACHE_WRITEBACKS()
    {

        event_id = 0x44;
        unitEncode = DC;

        eventMask = (EVENT_MASK(k8_cache) *)&m_eventMask;

        //UM(k7_um_moesi), 
        //_T("DATA_CACHE_WRITEBACKS"),
        name = _T("Data cache writebacks");
    }
    EVENT_MASK(k8_cache) * eventMask;


};

class k8Event_L1_DTLB_MISSES_AND_L2_DTLB_HITS     : public k8BaseEvent
{ 
    k8Event_L1_DTLB_MISSES_AND_L2_DTLB_HITS()
    {

        event_id = 0x45;
        unitEncode = DC;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        name = _T("L1 DTLB misses and L2 DTLB hits");
    }
    EVENT_MASK(NULL_MASK) * eventMask;


};

class k8Event_L1_AND_L2_DTLB_MISSES     : public k8BaseEvent
{ 
    k8Event_L1_AND_L2_DTLB_MISSES()
    {

        event_id = 0x46;
        unitEncode = DC;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        name = _T("L1 and L2 DTLB misses") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;

};

class k8Event_MISALIGNED_DATA_REFERENCES     : public k8BaseEvent
{ 
    k8Event_MISALIGNED_DATA_REFERENCES()
    {

        event_id = 0x47;
        unitEncode = DC;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        //NULL, _T("MISALIGNED_DATA_REFERENCES"),
        name = _T("Misaligned data references");
    }
    EVENT_MASK(NULL_MASK) * eventMask;

};



class k8Event_ACCESS_CANCEL_LATE : public k8BaseEvent
{ 
public:

    k8Event_ACCESS_CANCEL_LATE()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Microarchitectural late cancel of an access");
        event_id = 0x48;
        unitEncode = DC;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("ACCESS_CANCEL_LATE");


};
class k8Event_ACCESS_CANCEL_EARLY : public k8BaseEvent
{ 
public:

    k8Event_ACCESS_CANCEL_EARLY()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Microarchitectural early cancel of an access");
        event_id = 0x49; 
        unitEncode = DC;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("ACCESS_CANCEL_EARLY");


};
typedef union EVENT_MASK(  k8_ecc)
{
    struct 
    {
        uint16 ScrubberError : 1; // Scrubber error" },
        uint16 PiggybackScrubberErrors : 1; // Piggyback scrubber errors" } }
    };
    uint16 flat;

}EVENT_MASK(  k8_ecc);


class k8Event_ECC_BIT_ERR : public k8BaseEvent
{ 
public:

    k8Event_ECC_BIT_ERR()
    {
        eventMask = (EVENT_MASK(k8_ecc) *)&m_eventMask;
        name = _T("One bit ECC error recorded found by scrubber");
        event_id = 0x4A; 
        unitEncode = DC;

    }     
    EVENT_MASK(k8_ecc) * eventMask;
    // name = _T("ECC_BIT_ERR");


};

// 4B
typedef union EVENT_MASK(  k8_distpatch_prefetch_instructions)
{
    struct 
    {
        uint16 Load : 1; 
        uint16 Store : 1; 
        uint16 NTA : 1; 
    };
    uint16 flat;


}EVENT_MASK(  k8_distpatch_prefetch_instructions);

class k8Event_DISPATCHED_PRE_INSTRS : public k8BaseEvent
{ 
public:

    k8Event_DISPATCHED_PRE_INSTRS()
    {
        eventMask = (EVENT_MASK(k8_distpatch_prefetch_instructions) *)&m_eventMask;
        name = _T("Dispatched prefetch instructions");
        event_id = 0x4B; 
        unitEncode = DC;

    }     
    EVENT_MASK(k8_distpatch_prefetch_instructions) * eventMask;
    // name = _T("DISPATCHED_PRE_INSTRS");

    /* 0x4C: added in Revision C */

};



typedef union EVENT_MASK(  k8_lock_accesses)
{
    struct 
    {
        uint16 DcacheAccesses:1;       // Number of dcache accesses by lock instructions" },
        uint16 DcacheMisses:1;       // Number of dcache misses by lock instructions" } }
    };
    uint16 flat;

}EVENT_MASK(  k8_lock_accesses);



class k8Event_LOCK_ACCESSES : public k8BaseEvent
{ 
public:

    k8Event_LOCK_ACCESSES()
    {
        eventMask = (EVENT_MASK(k8_lock_accesses) *)&m_eventMask;
        name = _T("DCACHE accesses by locks") ;
        event_id = 0x4C; 
        unitEncode = DC;

        revRequired = 'C';
    }     
    EVENT_MASK(k8_lock_accesses) * eventMask;


};


class k8Event_CYCLES_PROCESSOR_IS_RUNNING : public k8BaseEvent
{ 
public:

    k8Event_CYCLES_PROCESSOR_IS_RUNNING()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Cycles processor is running (not in HLT or STPCLK)");
        event_id = 0x76;
        unitEncode = BU;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("CYCLES_PROCESSOR_IS_RUNNING"); /* undocumented *;


};


typedef union EVENT_MASK(  k8_internal_L2_request)
{
    struct 
    {
        uint16 ICFill:1;     // IC fill" },
        uint16 DCFill:1;     // DC fill" },
        uint16 TLBReload:1;       // TLB reload" },
        uint16 TagSnoopRequest:1; // Tag snoop request" },
        uint16 CancelledRequest:1; // Cancelled request" } }
    };
    uint16 flat;


}EVENT_MASK(  k8_internal_L2_request);

class k8Event_BU_INT_L2_REQ : public k8BaseEvent
{ 
public:

    k8Event_BU_INT_L2_REQ()
    {
        eventMask = (EVENT_MASK(k8_internal_L2_request) *)&m_eventMask;
        name = _T("Internal L2 request");
        unitEncode = BU;
        event_id = 0x7D; 
    }     

    EVENT_MASK(k8_internal_L2_request) * eventMask;
} ;

    // name = _T("BU_INT_L2_REQ");



// 7E
typedef union EVENT_MASK(  k8_fill_request_missed_L2)
{

    struct 
    {
        uint16 ICFill:1;     // IC fill" },
        uint16 DCFill:1;     // DC fill" },
        uint16 TLBReload:1;       // TLB reload" },
    };
    uint16 flat;

} EVENT_MASK(  k8_fill_request_missed_L2);


class k8Event_BU_FILL_REQ : public k8BaseEvent
{ 
public:

    k8Event_BU_FILL_REQ()
    {
        eventMask = (EVENT_MASK(k8_fill_request_missed_L2) *)&m_eventMask;
        name = _T("Fill request that missed in L2");
        event_id = 0x7E; 
        unitEncode = BU;

    }     
    EVENT_MASK(k8_fill_request_missed_L2) * eventMask;
    // name = _T("BU_FILL_REQ");



};




// 7F
typedef union EVENT_MASK(  k8_fill_into_L2)
{

    struct 
    {
        uint16 DirtyL2Victim:1;     // Dirty L2 victim
        uint16 VictimFromL2:1;     // Victim from L2
    };
    uint16 flat;

}EVENT_MASK(  k8_fill_into_L2);

class k8Event_BU_FILL_L2 : public k8BaseEvent
{ 
public:

    k8Event_BU_FILL_L2()
    {
        eventMask = (EVENT_MASK(k8_fill_into_L2) *)&m_eventMask;
        name = _T("Fill into L2");
        event_id = 0x7F; 
        unitEncode = BU;

    }     
    EVENT_MASK(k8_fill_into_L2) * eventMask;
    // name = _T("BU_FILL_L2");


};

class k8Event_INSTRUCTION_CACHE_FETCHES : public k8BaseEvent
{
public:
    k8Event_INSTRUCTION_CACHE_FETCHES()
    {
        event_id = 0x80;
        unitEncode = IC;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        name = _T("Instruction cache fetches");
    }
    EVENT_MASK(NULL_MASK) * eventMask;

};


class k8Event_INSTRUCTION_CACHE_MISSES : public k8BaseEvent
{
public:
   k8Event_INSTRUCTION_CACHE_MISSES()
   {
        event_id = 0x81;
        unitEncode = IC;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        //0xF, NULL, _T("INSTRUCTION_CACHE_MISSES"),
        name = _T("Instruction cache misses");
   }
    EVENT_MASK(NULL_MASK) * eventMask;

};


class k8Event_IC_REFILL_FROM_L2 : public k8BaseEvent
{ 
public:

    k8Event_IC_REFILL_FROM_L2()
    {
        eventMask = (EVENT_MASK(k8_cache) *)&m_eventMask;
        name = _T("Refill from L2");
        event_id = 0x82;
        unitEncode = IC;

    } 
    EVENT_MASK(k8_cache) * eventMask;
    // name = _T("IC_REFILL_FROM_L2");



};
class k8Event_IC_REFILL_FROM_SYS : public k8BaseEvent
{ 
public:

    k8Event_IC_REFILL_FROM_SYS()
    {
        eventMask = (EVENT_MASK(k8_cache) *)&m_eventMask;
        name = _T("Refill from system");
        event_id = 0x83; 
        unitEncode = IC;

    }     
    EVENT_MASK(k8_cache) * eventMask;
    // name = _T("IC_REFILL_FROM_SYS");


 
};
class k8Event_L1_ITLB_MISSES_AND_L2_ITLB_HITS : public k8BaseEvent
{
public:
    k8Event_L1_ITLB_MISSES_AND_L2_ITLB_HITS()
    {

        event_id = 0x84;
        unitEncode = IC;

        name = _T("L1 ITLB misses (and L2 ITLB hits)");
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;


    }
    EVENT_MASK(NULL_MASK) * eventMask;


};

class k8Event_L1_AND_L2_ITLB_MISSES : public k8BaseEvent
{
public:
    k8Event_L1_AND_L2_ITLB_MISSES()
    {
        event_id = 0x85;
        unitEncode = IC;

        name = _T("(L1 and) L2 ITLB misses");
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

    }
    EVENT_MASK(NULL_MASK) * eventMask;

};




class k8Event_IC_RESYNC_BY_SNOOP : public k8BaseEvent
{ 
public:

    k8Event_IC_RESYNC_BY_SNOOP()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        event_id = 0x86;
        unitEncode = IC;

        name = _T("Microarchitectural resync caused by snoop");
    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("IC_RESYNC_BY_SNOOP");
    /* similar to 0x22; but IC unit instead of LS unit */



};
class k8Event_IC_FETCH_STALL : public k8BaseEvent
{ 
public:

    k8Event_IC_FETCH_STALL()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Instruction fetch stall");
        event_id = 0x87; 
        unitEncode = IC;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("IC_FETCH_STALL");



};
class k8Event_IC_STACK_HIT : public k8BaseEvent
{ 
public:

    k8Event_IC_STACK_HIT()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Return stack hit");
        event_id = 0x88;
        unitEncode = IC;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("IC_STACK_HIT");



};
class k8Event_IC_STACK_OVERFLOW : public k8BaseEvent
{ 
public:

    k8Event_IC_STACK_OVERFLOW()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Return stack overflow");
        event_id = 0x89; 
        unitEncode = IC;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("IC_STACK_OVERFLOW");

 


};

   /* 0xC0-0xC7: from K7 official event set */
class k8Event_RETIRED_INSTRUCTIONS   : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_INSTRUCTIONS()
    {
        event_id = 0xC0;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

        //0xF, NULL, _T("RETIRED_INSTRUCTIONS"),
      name = _T("Retired instructions (includes exceptions, interrupts, resyncs)");
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};

class k8Event_RETIRED_OPS     : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_OPS()
    {
        event_id = 0xC1;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        //0xF, NULL, _T("RETIRED_OPS"),
      name = _T("Retired Ops") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};
class k8Event_RETIRED_BRANCHES     : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_BRANCHES()
    {
        event_id = 0xC2;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        //0xF, NULL, _T("RETIRED_BRANCHES"),
      name = _T("Retired branches (conditional, unconditional, exceptions, interrupts)") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};
class k8Event_RETIRED_BRANCHES_MISPREDICTED     : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_BRANCHES_MISPREDICTED()
    {
        event_id = 0xC3;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        //0xF, NULL, _T("RETIRED_BRANCHES_MISPREDICTED"),
      name = _T("Retired branches mispredicted") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};
class k8Event_RETIRED_TAKEN_BRANCHES     : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_TAKEN_BRANCHES()
    {
        event_id = 0xC4;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        //0xF, NULL, _T("RETIRED_TAKEN_BRANCHES"),
      name = _T("Retired taken branches") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};
class k8Event_RETIRED_TAKEN_BRANCHES_MISPREDICTED     : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_TAKEN_BRANCHES_MISPREDICTED()
    {
        event_id = 0xC5;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        //0xF, NULL, _T("RETIRED_TAKEN_BRANCHES_MISPREDICTED"),
      name = _T("Retired taken branches mispredicted") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};
class k8Event_RETIRED_FAR_CONTROL_TRANSFERS     : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_FAR_CONTROL_TRANSFERS()
    {
        event_id = 0xC6;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        //0xF, NULL, _T("RETIRED_FAR_CONTROL_TRANSFERS"),
      name = _T("Retired far control transfers") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};
class k8Event_RETIRED_RESYNC_BRANCHES     : public k8BaseEvent
{ 
public:
    k8Event_RETIRED_RESYNC_BRANCHES()
    {
        event_id = 0xC7;
        unitEncode = FR;

        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        //0xF, NULL, _T("RETIRED_RESYNC_BRANCHES"),
      name = _T("Retired resync branches (only non-control transfer branches counted)") ;
    }
    EVENT_MASK(NULL_MASK) * eventMask;
};

class k8Event_RETIRED_NEAR_RETURNS : public k8BaseEvent
{ 
public:

    k8Event_RETIRED_NEAR_RETURNS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Retired near returns");
        event_id = 0xC8; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;



};
class k8Event_RETIRED_RETURNS_MISPREDICT : public k8BaseEvent
{ 
public:

    k8Event_RETIRED_RETURNS_MISPREDICT()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Retired near returns mispredicted");
        event_id = 0xC9; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("RETIRED_RETURNS_MISPREDICT");


};
class k8Event_RETIRED_BRANCH_MISCOMPARE : public k8BaseEvent
{ 
public:

    k8Event_RETIRED_BRANCH_MISCOMPARE()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Retired taken branches mispredicted due to address miscompare");
        event_id = 0xCA;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("RETIRED_BRANCH_MISCOMPARE");


};


    /* Revision B and later */

typedef union EVENT_MASK(  k8_retired_fpu_instr)
{
    struct 
    {
        uint16 DirtyL2Victim:1;     // x87 instructions
        uint16 CombinedMMX_3DNow:1;     // Combined MMX & 3DNow! instructions" },
        uint16 CombinedPackedSSE_SSE2:1;     // Combined packed SSE and SSE2 instructions" },
        uint16 CombinedScalarSSE_SSE2:1;      // Combined scalar SSE and SSE2 instructions" } }
    };
    uint16 flat;


}EVENT_MASK(  k8_retired_fpu_instr);


class k8Event_RETIRED_FPU_INSTRS : public k8BaseEvent
{ 
public:

    k8Event_RETIRED_FPU_INSTRS()
    {
        eventMask = (EVENT_MASK(k8_retired_fpu_instr) *)&m_eventMask;
        event_id = 0xCB; 
        unitEncode = FR;

        name = _T("Retired FPU instructions");
        revRequired = 'B';
    }     
    EVENT_MASK(k8_retired_fpu_instr) * eventMask;
    /* Revision B and later */



};

// CC
typedef union EVENT_MASK(  k8_retired_fastpath_double_op_instr )
{

    struct 
    {
        uint16 LowOpPosition0:1;             // With low op in position 0" },
        uint16 LowOpPosition1:1;         // With low op in position 1" },
        uint16 LowOpPosition2:1;    // With low op in position 2" } }
    };
    uint16 flat;


}EVENT_MASK(  k8_retired_fastpath_double_op_instr);

class k8Event_RETIRED_FASTPATH_INSTRS : public k8BaseEvent
{ 
public:

    k8Event_RETIRED_FASTPATH_INSTRS()
    {
        eventMask = (EVENT_MASK(k8_retired_fastpath_double_op_instr) *)&m_eventMask;
        event_id = 0xCC; 
        unitEncode = FR;

        name = _T("Retired fastpath double op instructions");
        revRequired = 'B';

    }     
    EVENT_MASK(k8_retired_fastpath_double_op_instr) * eventMask;


};

class k8Event_INTERRUPTS_MASKED_CYCLES     : public k8BaseEvent
{ 
public:
    k8Event_INTERRUPTS_MASKED_CYCLES()
    {
        event_id = 0xCD;
        unitEncode = FR;

        //0xF, NULL, _T("INTERRUPTS_MASKED_CYCLES"),
        name = _T("Interrupts masked cycles (IF=0)") ;
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;

    }
    EVENT_MASK(NULL_MASK) * eventMask;

};
class k8Event_INTERRUPTS_MASKED_WHILE_PENDING_CYCLES     : public k8BaseEvent
{ 
public:
    k8Event_INTERRUPTS_MASKED_WHILE_PENDING_CYCLES()
    {
        event_id = 0xCE;
        unitEncode = FR;

        //0xF, NULL, _T("INTERRUPTS_MASKED_WHILE_PENDING_CYCLES"),
        name = _T("Interrupts masked while pending cycles (INTR while IF=0)") ;
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
    }
    EVENT_MASK(NULL_MASK) * eventMask;

};
class k8Event_NUMBER_OF_TAKEN_HARDWARE_INTERRUPTS     : public k8BaseEvent
{ 
public:
    k8Event_NUMBER_OF_TAKEN_HARDWARE_INTERRUPTS()
    {
        event_id = 0xCF;
        unitEncode = FR;

        //0xF, NULL, _T("NUMBER_OF_TAKEN_HARDWARE_INTERRUPTS"),
        name = _T("Number of taken hardware interrupts") ;
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
    }
    EVENT_MASK(NULL_MASK) * eventMask;

};


class k8Event_DECODER_EMPTY : public k8BaseEvent
{ 
public:

    k8Event_DECODER_EMPTY()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Nothing to dispatch (decoder empty)");
        event_id = 0xD0;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DECODER_EMPTY");


};
class k8Event_DISPATCH_STALLS : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALLS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stalls (events 0xD2-0xDA combined)");
        event_id = 0xD1; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALLS");



};
class k8Event_DISPATCH_STALL_FROM_BRANCH_ABORT : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_FROM_BRANCH_ABORT()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall from branch abort to retire");
        event_id = 0xD2; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_FROM_BRANCH_ABORT");



};
class k8Event_DISPATCH_STALL_SERIALIZATION : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_SERIALIZATION()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall for serialization");
        event_id = 0xD3; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_SERIALIZATION");


};
class k8Event_DISPATCH_STALL_SEG_LOAD : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_SEG_LOAD()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall for segment load");
        event_id = 0xD4; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_SEG_LOAD");



};
class k8Event_DISPATCH_STALL_REORDER_BUFFER : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_REORDER_BUFFER()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall when reorder buffer is full");
        event_id = 0xD5;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_REORDER_BUFFER");


};
class k8Event_DISPATCH_STALL_RESERVE_STATIONS : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_RESERVE_STATIONS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall when reservation stations are full");
        event_id = 0xD6; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_RESERVE_STATIONS");


};
class k8Event_DISPATCH_STALL_FPU : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_FPU()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall when FPU is full");
        event_id = 0xD7; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_FPU");


};
class k8Event_DISPATCH_STALL_LS : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_LS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall when LS is full");
        event_id = 0xD8; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_LS");


};
class k8Event_DISPATCH_STALL_QUIET_WAIT : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_QUIET_WAIT()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall when waiting for all to be quiet");
        event_id = 0xD9; 
        unitEncode = FR;

    }     
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_QUIET_WAIT");



};
class k8Event_DISPATCH_STALL_PENDING : public k8BaseEvent
{ 
public:

    k8Event_DISPATCH_STALL_PENDING()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Dispatch stall when far control transfer or resync branch is pending");
        event_id = 0xDA;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DISPATCH_STALL_PENDING");



};


typedef union EVENT_MASK(  k8_fpu_exceptions)
{



    struct 
    {
        uint16 x87ReclassMicrofaults:1;     // x87 reclass microfaults" },
        uint16 SSERetypeMicrofaults:1;      // SSE retype microfaults" },
        uint16 SSEReclassMicrofaults:1;     // SSE reclass microfaults" },
        uint16 SSE_x87Microtraps:1;         // SSE and x87 microtraps" } }
    };
    uint16 flat;



}EVENT_MASK(  k8_fpu_exceptions);

class k8Event_FPU_EXCEPTIONS : public k8BaseEvent
{ 
public:

    k8Event_FPU_EXCEPTIONS()
    {
        eventMask = (EVENT_MASK(k8_fpu_exceptions) *)&m_eventMask;
        event_id = 0xDB; 
        unitEncode = FR;

        name = _T("FPU exceptions");
        revRequired = 'B';

    }     
    EVENT_MASK(k8_fpu_exceptions) * eventMask;
    // name = _T("FPU_EXCEPTIONS");
    /* Revision B and later */



};
class k8Event_DR0_BREAKPOINTS : public k8BaseEvent
{ 
public:

    k8Event_DR0_BREAKPOINTS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Number of breakpoints for DR0");
        event_id = 0xDC;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DR0_BREAKPOINTS");



};
class k8Event_DR1_BREAKPOINTS : public k8BaseEvent
{ 
public:

    k8Event_DR1_BREAKPOINTS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Number of breakpoints for DR1");
        event_id = 0xDD;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DR1_BREAKPOINTS");



};
class k8Event_DR2_BREAKPOINTS : public k8BaseEvent
{ 
public:

    k8Event_DR2_BREAKPOINTS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Number of breakpoints for DR2");
        event_id = 0xDE;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DR2_BREAKPOINTS");


};
class k8Event_DR3_BREAKPOINTS : public k8BaseEvent
{ 
public:

    k8Event_DR3_BREAKPOINTS()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Number of breakpoints for DR3");
        event_id = 0xDF;
        unitEncode = FR;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DR3_BREAKPOINTS");


};



// E0
typedef union EVENT_MASK(  k8_page_access_event)
{
    struct 
    {
        uint16 PageHit:1;       // Page hit" },
        uint16 PageMiss:1;      // Page miss" },
        uint16 PageConflict:1;  // Page conflict" } }
    };
    uint16 flat;

}EVENT_MASK(  k8_page_access_event);

class k8Event_MEM_PAGE_ACCESS : public k8BaseEvent
{ 
public:

    k8Event_MEM_PAGE_ACCESS()
    {
        eventMask = (EVENT_MASK(k8_page_access_event) *)&m_eventMask;
        name = _T("Memory controller page access");
        event_id = 0xE0;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_page_access_event) * eventMask;
    // name = _T("MEM_PAGE_ACCESS");


};
class k8Event_MEM_PAGE_TBL_OVERFLOW : public k8BaseEvent
{ 
public:

    k8Event_MEM_PAGE_TBL_OVERFLOW()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Memory controller page table overflow");
        event_id = 0xE1;
        unitEncode = NB;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("MEM_PAGE_TBL_OVERFLOW");


};
class k8Event_DRAM_SLOTS_MISSED : public k8BaseEvent
{ 
public:

    k8Event_DRAM_SLOTS_MISSED()
    {
        eventMask = (EVENT_MASK(NULL_MASK) *)&m_eventMask;
        name = _T("Memory controller DRAM command slots missed (in MemClks)");
        event_id = 0xE2;
        unitEncode = NB;

    } 
    EVENT_MASK(NULL_MASK) * eventMask;
    // name = _T("DRAM_SLOTS_MISSED");


};


// e3
typedef union EVENT_MASK(  k8_turnaround)
{

    struct 
    {
        uint16 DIMMTurnaround:1;       //DIMM turnaround" },
        uint16 ReadToWriteTurnaround:1;      //Read to write turnaround" },
        uint16 WriteToReadTurnaround:1;  //Write to read turnaround" } }
    };
    uint16 flat;


}EVENT_MASK(  k8_turnaround);

class k8Event_MEM_TURNAROUND : public k8BaseEvent
{ 
public:

    k8Event_MEM_TURNAROUND()
    {
        eventMask = (EVENT_MASK(k8_turnaround) *)&m_eventMask;
        name = _T("Memory controller turnaround");
        event_id = 0xE3;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_turnaround) * eventMask;
    // name = _T("MEM_TURNAROUND");


};




// E4
typedef union EVENT_MASK(  k8_bypass_counter_saturation)
{
    struct 
    {
        uint16 MEM_HighPriorityBypass:1;       // Memory controller high priority bypass" },
        uint16 MEM_LowPriorityBypass:1;      // Memory controller low priority bypass" },
        uint16 DRAM_InterfaceBypass:1;  // DRAM controller interface bypass" },
        uint16 DRAM_QueueBypass:1;  // DRAM controller queue bypass" } }
    };
    uint16 flat;

}EVENT_MASK(  k8_bypass_counter_saturation);

class k8Event_MEM_BYPASS_SAT : public k8BaseEvent
{ 
public:

    k8Event_MEM_BYPASS_SAT()
    {
        eventMask = (EVENT_MASK(k8_bypass_counter_saturation) *)&m_eventMask;
        name = _T("Memory controller bypass counter saturation");
        event_id = 0xE4;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_bypass_counter_saturation) * eventMask;
    // name = _T("MEM_BYPASS_SAT");


};



//EB
typedef union EVENT_MASK(  k8_sized_commands)
{

    struct 
    {
        uint16 NonPostWrSzByte:1;       // NonPostWrSzByte" },
        uint16 NonPostWrSzDword:1;       // NonPostWrSzDword" },
        uint16 PostWrSzByte:1;       // PostWrSzByte" },
        uint16 PostWrSzDword:1;       // PostWrSzDword" },
        uint16 RdSzByte:1;       // RdSzByte" },
        uint16 RdSzDword:1;       // RdSzDword" },
        uint16 RdModWr:1;       // RdModWr" } }
    };
    uint16 flat;


}EVENT_MASK(  k8_sized_commands);


class k8Event_SIZED_COMMANDS : public k8BaseEvent
{ 
public:

    k8Event_SIZED_COMMANDS()
    {
        eventMask = (EVENT_MASK(k8_sized_commands) *)&m_eventMask;
        name = _T("Sized commands");
        event_id = 0xEB;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_sized_commands) * eventMask;
    // name = _T("SIZED_COMMANDS");


};

typedef union EVENT_MASK(  k8_probe_result)
{
    struct 
    {
        uint16 ProbeMiss:1;       // Probe miss" },
        uint16 ProbeHit:1;       // Probe hit" },
        uint16 ProbeHitDirtyWithoutMemoryCancel:1;       // Probe hit dirty without memory cancel" },
        uint16 ProbeHitDirtyWithMemoryCancel:1;       // Probe hit dirty with memory cancel" } }
        uint16 UpstreamDisplayRefreshReads:1;       // Rev D and later
        uint16 UpstreamNonDisplayRefreshReads:1;       // Rev D and later
        uint16 UpstreamWrites:1;       // Rev D and later
    };
    uint16 flat;


}EVENT_MASK(  k8_probe_result);


class k8Event_PROBE_RESULT : public k8BaseEvent
{ 
public:

    k8Event_PROBE_RESULT()
    {
        eventMask = (EVENT_MASK(k8_probe_result) *)&m_eventMask;
        name = _T("Probe result");
        event_id = 0xEC;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_probe_result) * eventMask;
    // name = _T("PROBE_RESULT");


};

typedef union EVENT_MASK(  k8_ht)
{

    struct 
    {
        uint16 CommandSent:1;       //Command sent" },
        uint16 DataSent:1;       //Data sent" },
        uint16 BufferReleaseSent:1;       //Buffer release sent" 
        uint16 NopSent:1;       //Nop sent" } }
    };
    uint16 flat;


}EVENT_MASK(  k8_ht);


class k8Event_HYPERTRANSPORT_BUS0_WIDTH : public k8BaseEvent
{ 
public:

    k8Event_HYPERTRANSPORT_BUS0_WIDTH()
    {
        eventMask = (EVENT_MASK(k8_ht) *)&m_eventMask;
        name = _T("Hypertransport (tm) bus 0 bandwidth");
        event_id = 0xF6;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_ht) * eventMask;
    // name = _T("HYPERTRANSPORT_BUS0_WIDTH");


};
class k8Event_HYPERTRANSPORT_BUS1_WIDTH : public k8BaseEvent
{ 
public:

    k8Event_HYPERTRANSPORT_BUS1_WIDTH()
    {
        eventMask = (EVENT_MASK(k8_ht) *)&m_eventMask;
        name = _T("Hypertransport (tm) bus 1 bandwidth");
        event_id = 0xF7;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_ht) * eventMask;
    // name = _T("HYPERTRANSPORT_BUS1_WIDTH");


};
class k8Event_HYPERTRANSPORT_BUS2_WIDTH : public k8BaseEvent
{ 
public:

    k8Event_HYPERTRANSPORT_BUS2_WIDTH()
    {
        eventMask = (EVENT_MASK(k8_ht) *)&m_eventMask;
        name = _T("Hypertransport (tm) bus 2 bandwidth");
        event_id = 0xF8;
        unitEncode = NB;

    } 
    EVENT_MASK(k8_ht) * eventMask;
    // name = _T("HYPERTRANSPORT_BUS2_WIDTH");

};

//
//typedef union EVENT_MASK( perfctr_event_set k8_common_event_set)
//{
//
//    .cpu_type = PERFCTR_X86_AMD_K8,
//    .event_prefix = _T("K8_"),
//    .include = &k7_official_event_set,
//    .nevents = ARRAY_SIZE(k8_common_events),
//    .events = k8_common_events,
//}EVENT_MASK( perfctr_event_set k8_common_event_set);
//
//typedef union EVENT_MASK( perfctr_event k8_events[])
//{
//
//     { 0x24, 0xF, UM(NULL), _T("LOCKED_OP"), /* unit mask changed in Rev. C */
//       _T("Locked operation") },
//}EVENT_MASK( perfctr_event k8_events[]);




//const struct perfctr_event_set perfctr_k8_event_set)
//{
//
//    .cpu_type = PERFCTR_X86_AMD_K8,
//    .event_prefix = _T("K8_"),
//    .include = &k8_common_event_set,
//    .nevents = ARRAY_SIZE(k8_events),
//    .events = k8_events,
//};
//
/*
 * K8 Revision C. Starts at CPUID 0xF58 for Opteron/Athlon64FX and
 * CPUID 0xF48 for Athlon64. (CPUID 0xF51 is Opteron Revision B3.)
 */








//
//typedef union EVENT_MASK(  k8_lock_accesses)
//{
//    struct 
//    {
//        uint16 DcacheAccesses:1;       // Number of dcache accesses by lock instructions" },
//        uint16 DcacheMisses:1;       // Number of dcache misses by lock instructions" } }
//    };
//    uint16 flat;
//
//}EVENT_MASK(  k8_lock_accesses);
//

#endif // K8PERFORMANCECOUNTERS_H

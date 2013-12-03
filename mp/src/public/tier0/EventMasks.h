//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#pragma once

typedef union EVENT_MASK(TC_deliver_mode)
{
    struct 
    {
        uint16 DD:1; // both logical processors in deliver mode },
        uint16 DB:1; // logical processor 0 in deliver mode, 1 in build mode },
        uint16 DI:1; // logical processor 0 in deliver mode, 1 is inactive },
        uint16 BD:1; // logical processor 0 in build mode, 1 in deliver mode },
        uint16 BB:1; // both logical processors in build mode },
        uint16 BI:1; // logical processor 0 in build mode, 1 is inactive },
        uint16 ID:1; // logical processor 0 is inactive, 1 in deliver mode },
        uint16 IB:1; // logical processor 0 is inactive, 1 in build mode } 
    };
    uint16 flat;
} EVENT_MASK(TC_deliver_mode);

typedef union EVENT_MASK(BPU_fetch_request)
{

    struct 
    {
        uint16 TCMISS:1; // Trace cache lookup miss },
    };
    uint16 flat;
} EVENT_MASK(BPU_fetch_request);


typedef union EVENT_MASK(ITLB_reference)
{
    struct 
    {
        uint16 HIT : 1; //ITLB hit },
        uint16 MISS : 1;//ITLB miss },
        uint16 HIT_UC :1; // Uncacheable ITLB hit } 
    };
    uint16 flat;
} EVENT_MASK(ITLB_reference);

typedef union EVENT_MASK(memory_cancel)
{
    struct 
    {
        uint16  dummy : 2; 

        uint16  ST_RB_FULL:1; //Replayed because no store request buffer is available },
        uint16  _64K_CONF:1; //Conflicts due to 64K aliasing }         
    };
    uint16 flat;
}EVENT_MASK(memory_cancel);

typedef union EVENT_MASK(memory_complete)
{
    struct 
    {
        uint16 LSC:1; // Load split completed, excluding UC/WC loads },
        uint16  SSC:1; //Any split stores completed } }
    };
    uint16 flat;
} EVENT_MASK(memory_complete);

typedef union EVENT_MASK(load_port_replay)
{
    struct 
    {
        uint16 dummy:1; 
        uint16 SPLIT_LD:1; //Split load } }
    };
    uint16 flat;
} EVENT_MASK(load_port_replay);

typedef union EVENT_MASK(store_port_replay)
{
    struct 
    {
        uint16 dummy0:1; 
        uint16  SPLIT_ST:1; //Split store } }

    };
    uint16 flat;
} EVENT_MASK(store_port_replay);

typedef union EVENT_MASK(MOB_load_replay) 
{
    struct 
    {
        uint16 dummy0:1; 

        uint16 NO_STA:1; //Replayed because of unknown store address },

        uint16 dummy2:1; 

        uint16 NO_STD:1; //Replayed because of unknown store data },
        uint16 PARTIAL_DATA:1; //Replayed because of partially overlapped data access between the load and store operations },
        uint16 UNALGN_ADDR:1; //Replayed because the lower 4 bits of the linear address do not match between the load and store operations } }
    };
    uint16 flat;
}EVENT_MASK(MOB_load_replay);

typedef union EVENT_MASK(page_walk_type)
{
    struct 
    {
        uint16 DTMISS:1; // Page walk for a data TLB miss },
        uint16 ITMISS:1; // Page walk for an instruction TLB miss } }
    };
    uint16 flat;
}EVENT_MASK(page_walk_type);


typedef union EVENT_MASK(BSQ_cache_reference) 
{
    struct 
    {
        uint16 RD_2ndL_HITS:1; // Read 2nd level cache hit Shared },
        uint16 RD_2ndL_HITE:1; // Read 2nd level cache hit Exclusive },
        uint16 RD_2ndL_HITM:1; // Read 2nd level cache hit Modified },
        uint16 RD_3rdL_HITS:1; // Read 3rd level cache hit Shared },
        uint16 RD_3rdL_HITE:1; // Read 3rd level cache hit Exclusive },
        uint16 RD_3rdL_HITM:1; // Read 3rd level cache hit Modified },
        uint16 dummy6:1; 
        uint16 dummy7:1; 
        uint16 RD_2ndL_MISS:1; // Read 2nd level cache miss },
        uint16 RD_3rdL_MISS:1; // Read 3rd level cache miss },
        uint16 WR_2ndL_MISS:1; // Writeback lookup from DAC misses the 2nd level cache } }
    };
    uint16 flat;
} EVENT_MASK(BSQ_cache_reference) ;

typedef union EVENT_MASK(IOQ) 
{
    struct 
    {
        uint16 bit0:1; // bus request type (use 00001 for invalid or default)
        uint16 bit1:1; // 
        uint16 bit2:1; // 
        uint16 bit3:1; // 
        uint16 bit4:1; // 
        uint16 ALL_READ:1; // Count read entries },
        uint16 ALL_WRITE:1; // Count write entries },
        uint16 MEM_UC:1; // Count UC memory access entries },
        uint16 MEM_WC:1; // Count WC memory access entries },
        uint16 MEM_WT:1; // Count WT memory access entries },
        uint16 MEM_WP:1; // Count WP memory access entries },
        uint16 MEM_WB:1; // Count WB memory access entries },
        uint16 dummy12:1; 

        uint16 OWN:1; // Count own store requests },
        uint16 OTHER:1; // Count other and DMA store requests },
        uint16 PREFETCH:1; // Include HW and SW prefetch requests } }
    };
    uint16 flat;
} EVENT_MASK(IOQ) ;

typedef union EVENT_MASK(FSB_data_activity) 
{
    struct 
    {
        /* DRDY_OWN is mutually exclusive with DRDY_OTHER */
        /* DBSY_OWN is mutually exclusive with DBSY_OTHER */
        uint16 DRDY_DRV:1; // Count when this processor drives data onto the bus },
        uint16 DRDY_OWN:1; // Count when this processor reads data from the bus },
        uint16 DRDY_OTHER:1; // Count when data is on the bus but not being sampled by the processor },
        uint16 DBSY_DRV:1; // Count when this processor reserves the bus for driving data },
        uint16 DBSY_OWN:1; // Count when this processor reserves the bus for sampling data },
        uint16 DBSY_OTHER:1; // Count when the bus is reserved for driving data this processor will not sample } }
    };
    uint16 flat;
}EVENT_MASK(FSB_data_activity);

typedef union EVENT_MASK(BSQ) 
{
    struct 
    {
        uint16 REQ_TYPE0:1; // Request type encoding bit 0 },
        uint16 REQ_TYPE1:1; // Request type encoding bit 1 },
        uint16 REQ_LEN0:1; // Request length encoding bit 0 },
        uint16 REQ_LEN1:1; // Request length encoding bit 1 },
        uint16 dummy4: 1;
        uint16 REQ_IO_TYPE:1; // Request type is input or output },
        uint16 REQ_LOCK_TYPE:1; // Request type is bus lock },
        uint16 REQ_CACHE_TYPE:1; // Request type is cacheable },
        uint16 REQ_SPLIT_TYPE:1; // Request type is a bus 8-byte chunk split across 8-byte boundary },
        uint16 REQ_DEM_TYPE:1; // Request type is a demand (1) or prefetch (0) },
        uint16 REQ_ORD_TYPE:1; // Request is an ordered type },
        uint16 MEM_TYPE0:1; // Memory type encoding bit 0 },
        uint16 MEM_TYPE1:1; // Memory type encoding bit 1 },
        uint16 MEM_TYPE2:1; // Memory type encoding bit 2 } }
    };
    uint16 flat;
} EVENT_MASK(BSQ);

typedef union EVENT_MASK(firm_uop)
{
    struct 
    {
        uint16 dummy15 : 15;
        uint16 ALL:1; // count all uops of this type } }
    };
    uint16 flat;
} EVENT_MASK(firm_uop);



typedef union EVENT_MASK(TC_misc)
{
    struct 
    {
        uint16 dymmy4 : 4;
        uint16 FLUSH:1; // Number of flushes } }
    };
    uint16 flat;
} EVENT_MASK(TC_misc);

typedef union EVENT_MASK(global_power_events)
{
    struct 
    {
        uint16 Running:1; // The processor is active } }
    };
    uint16 flat;
} EVENT_MASK(global_power_events);

typedef union EVENT_MASK(tc_ms_xfer)
{
    struct 
    {
        uint16 CISC:1; // A TC to MS transfer ocurred } }
    };
    uint16 flat;
}EVENT_MASK(tc_ms_xfer);



typedef union EVENT_MASK(uop_queue_writes) 
{
    struct 
    {
        uint16 FROM_TC_BUILD:1; // uops written from TC build mode 
        uint16 FROM_TC_DELIVER:1; // uops written from TC deliver mode 
        uint16 FROM_ROM:1; // uops written from microcode ROM } }
    };
    uint16 flat;
} EVENT_MASK(uop_queue_writes);

typedef union EVENT_MASK(branch_type) 
{
    struct 
    {
        uint16 dummy : 1;
        uint16 CONDITIONAL:1; // Conditional jumps 
        uint16 CALL:1; // Direct or indirect call 
        uint16 RETURN:1; // Return branches 
        uint16 INDIRECT:1; // Returns, indirect calls, or indirect jumps 
    };
    uint16 flat;
} EVENT_MASK(branch_type);



typedef union EVENT_MASK(resource_stall) 
{
    struct 
    {
        uint16 dummy1 : 5;
        uint16 SBFULL:1; // A Stall due to lack of store buffers } }
    };
    uint16 flat;
} EVENT_MASK(resource_stall);




typedef union EVENT_MASK(WC_Buffer) 
{
    struct 
    {
        uint16 WCB_EVICTS : 1; // all causes },
        uint16 WCB_FULL_EVICT : 1; // no WC buffer is available },
        /* XXX: 245472-011 no longer lists bit 2, but that looks like
        a table formatting error. Keeping it for now. */
        uint16 WCB_HITM_EVICT : 1; // store encountered a Hit Modified condition } }
    };
    uint16 flat;
} EVENT_MASK(WC_Buffer);


typedef union EVENT_MASK(b2b_cycles) 
{
    struct 
    {
        uint16 dummy0 : 1;
        uint16 bit1 : 1; // 
        uint16 bit2 : 1; // 
        uint16 bit3 : 1; // 
        uint16 bit4 : 1; // 
        uint16 bit5 : 1; // 
        uint16 bit6 : 1; // 

    };
    uint16 flat;
} EVENT_MASK(b2b_cycles);

typedef union EVENT_MASK(bnr) 
{
    struct 
    {
        uint16 bit0:1; // 
        uint16 bit1:1; // 
        uint16 bit2:1; // 
    };
    uint16 flat;
} EVENT_MASK(bnr);


typedef union EVENT_MASK(snoop)
{
    struct 
    {
        uint16 dummy0 : 1;
        uint16 dummy1 : 1;

        uint16 bit2:1; //  
        uint16 dummy3:1; // 
        uint16 dummy4:1; // 
        uint16 dummy5:1; // 
        uint16 bit6:1; // 
        uint16 bit7:1; // 
    };
    uint16 flat;
} EVENT_MASK(snoop);


typedef union EVENT_MASK(response) 
{
    struct 
    {
        uint16 dummy0:1; // 
        uint16 bit1:1; // 
        uint16 bit2:1; // 
        uint16 dummy3:1; // 
        uint16 dummy4:1; // 
        uint16 dummy5:1; // 
        uint16 dummy6:1; // 
        uint16 dummy7:1; // 
        uint16 bit8:1; // 
        uint16 bit9:1; // 
    };
    uint16 flat;
} EVENT_MASK(response);


typedef union EVENT_MASK(nbogus_bogus) 
{
    struct 
    {
        uint16 NBOGUS:1; // The marked uops are not bogus 
        uint16 BOGUS:1; // The marked uops are bogus 
    };
    uint16 flat;
} EVENT_MASK(nbogus_bogus);


typedef union EVENT_MASK(execution_event) 
{
    struct 
    {
        uint16 NBOGUS0:1; // non-bogus uops with tag bit 0 set },
        uint16 NBOGUS1:1; // non-bogus uops with tag bit 1 set },
        uint16 NBOGUS2:1; // non-bogus uops with tag bit 2 set },
        uint16 NBOGUS3:1; // non-bogus uops with tag bit 3 set },
        uint16 BOGUS0:1; // bogus uops with tag bit 0 set },
        uint16 BOGUS1:1; // bogus uops with tag bit 1 set },
        uint16 BOGUS2:1; // bogus uops with tag bit 2 set },
        uint16 BOGUS3:1; // bogus uops with tag bit 3 set } }
    };
    uint16 flat;
}EVENT_MASK(execution_event);

typedef union EVENT_MASK(instr_retired)
{
    struct 
    {
        uint16 NBOGUSNTAG:1; // Non-bogus instructions that are not tagged },
        uint16 NBOGUSTAG:1; // Non-bogus instructions that are tagged },
        uint16 BOGUSNTAG:1; // Bogus instructions that are not tagged },
        uint16 BOGUSTAG:1; // Bogus instructions that are tagged } }
    };
    uint16 flat;
} EVENT_MASK(instr_retired);


typedef union EVENT_MASK(uop_type) 
{
    struct 
    {
        uint16 dummy0 : 1;
        uint16 TAGLOADS:1; // The uop is a load operation },
        uint16 TAGSTORES:1; // The uop is a store operation } }
    };
    uint16 flat;
} EVENT_MASK(uop_type);

typedef union EVENT_MASK(branch_retired) 
{
    struct 
    {
        uint16 MMNP:1; // Branch Not-taken Predicted 
        uint16 MMNM:1; // Branch Not-taken Mispredicted 
        uint16 MMTP:1; // Branch Taken Predicted 
        uint16 MMTM:1; // Branch Taken Mispredicted 
    };
    uint16 flat;
} EVENT_MASK(branch_retired);

typedef union EVENT_MASK(mispred_branch_retired) 
{
    struct 
    {
        uint16 NBOGUS:1; // The retired branch is not bogus } }
    };
    uint16 flat;
} EVENT_MASK(mispred_branch_retired);


typedef union EVENT_MASK(x87_assist) 
{
    struct 
    {
        uint16 FPSU:1; // FP stack underflow },
        uint16 FPSO:1; // FP stack overflow },
        uint16 POAO:1; // x87 output overflow },
        uint16 POAU:1; // x87 output underflow },
        uint16 PREA:1; // x87 input assist } }
    };
    uint16 flat;
}EVENT_MASK(x87_assist);

typedef union EVENT_MASK(machine_clear) 
{
    struct 
    {
        uint16 CLEAR:1; // Count a portion of the cycles when the machine is cleared },
        uint16 dummy1: 1;
        uint16 MOCLEAR:1; // Count clears due to memory ordering issues },
        uint16 dummy3: 1;
        uint16 dummy4: 1;
        uint16 dummy5: 1;

        uint16 SMCLEAR:1;// Count clears due to self-modifying code issues } }
    };
    uint16 flat;
} EVENT_MASK(machine_clear);


typedef union EVENT_MASK(x87_SIMD_moves_uop) 
{
    struct 
    {
        uint16 dummy3:3;
        uint16 ALLP0:1; // Count all x87/SIMD store/move uops },
        uint16 ALLP2:1; // count all x87/SIMD load uops } }
    };
    uint16 flat;
} EVENT_MASK(x87_SIMD_moves_uop);








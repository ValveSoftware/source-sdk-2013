//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef EVENTMODES_H
#define EVENTMODES_H


#pragma once

/*
 


        Event Modes to choose from:

            P4Event_TC_deliver_mode

            P4Event_BPU_fetch_request

            P4Event_ITLB_reference

            P4Event_memory_cancel

            P4Event_memory_complete

            P4Event_load_port_replay

            P4Event_store_port_replay

            P4Event_MOB_load_replay

            P4Event_page_walk_type

            P4Event_BSQ_cache_reference

            P4Event_IOQ_allocation

            P4Event_IOQ_active_entries

            P4Event_FSB_data_activity

            P4Event_BSQ_allocation

            P4Event_BSQ_active_entries

            P4Event_SSE_input_assist

            P4Event_packed_SP_uop

            P4Event_packed_DP_uop

            P4Event_scalar_SP_uop

            P4Event_scalar_DP_uop

            P4Event_64bit_MMX_uop

            P4Event_128bit_MMX_uop

            P4Event_x87_FP_uop

            P4Event_x87_SIMD_moves_uop

            P4Event_TC_misc

            P4Event_global_power_events

            P4Event_tc_ms_xfer

            P4Event_uop_queue_writes

            P4Event_retired_mispred_branch_type

            P4Event_retired_branch_type

            P4Event_resource_stall

            P4Event_WC_Buffer

            P4Event_b2b_cycles

            P4Event_bnr

            P4Event_snoop

            P4Event_response

            P4Event_front_end_event

            P4Event_execution_event

            P4Event_replay_event

            P4Event_instr_retired

            P4Event_uops_retired

            P4Event_uop_type

            P4Event_branch_retired

            P4Event_mispred_branch_retired

            P4Event_x87_assist

            P4Event_machine_clear


*/



class P4P4Event_TC_deliver_mode: public P4BaseEvent
{
public:
    EVENT_MASK(TC_deliver_mode) * eventMask;

    P4P4Event_TC_deliver_mode()
    {
        eventMask = (EVENT_MASK(TC_deliver_mode) *)&m_eventMask;

        escr.ESCREventSelect = 0x01;
        cccr.CCCRSelect =    0x01;
        //// eventType = EVENT_TYPE(TC_deliver_mode);
        description = _T("TC_deliver_mode");
        UseCounter4();
    }


    void UseCounter4()
    {
        SetCounter(4);;
    }
    void UseCounter5()
    {
        SetCounter(5);
    }
    void UseCounter6()
    {
        SetCounter(6);
    }
    void UseCounter7()
    {
        SetCounter(7);
    }
};

class P4P4Event_BPU_fetch_request: public P4BaseEvent

{
public:
    EVENT_MASK(BPU_fetch_request)* eventMask;

    P4P4Event_BPU_fetch_request()
    {
        eventMask = (EVENT_MASK(BPU_fetch_request) *)&m_eventMask;

        escr.ESCREventSelect=     0x03;
        cccr.CCCRSelect=    0x00;
        // eventType = EVENT_TYPE(BPU_fetch_request);
        description=_T("BPU_fetch_request");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }

};
class P4P4Event_ITLB_reference: public P4BaseEvent

{
public:
    EVENT_MASK(ITLB_reference) * eventMask;

    P4P4Event_ITLB_reference()
    {
        eventMask = (EVENT_MASK(ITLB_reference) *)&m_eventMask;

        escr.ESCREventSelect=     0x18;
        cccr.CCCRSelect=    0x03;
        // eventType=EVENT_TYPE(ITLB_reference);
        description=_T("ITLB_reference");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_memory_cancel: public P4BaseEvent

{
public:
    EVENT_MASK(memory_cancel) * eventMask;

    P4Event_memory_cancel()
    {
        eventMask = (EVENT_MASK(memory_cancel) *)&m_eventMask;

        escr.ESCREventSelect=     0x02;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(memory_cancel);
        description=_T("memory_cancel");
        UseCounter8();
    }

    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_memory_complete: public P4BaseEvent

{
public:
    EVENT_MASK(memory_complete) * eventMask;

    P4Event_memory_complete()
    {
        eventMask = (EVENT_MASK(memory_complete) *)&m_eventMask;

        escr.ESCREventSelect=     0x08;
        cccr.CCCRSelect=    0x02;
        // eventType=EVENT_TYPE(memory_complete);
        description=_T("memory_complete");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_load_port_replay: public P4BaseEvent

{
public:
    EVENT_MASK(load_port_replay) * eventMask;

    P4Event_load_port_replay()
    {
        eventMask = (EVENT_MASK(load_port_replay) *)&m_eventMask;

        escr.ESCREventSelect=     0x04;
        cccr.CCCRSelect=    0x02;
        // eventType=EVENT_TYPE(load_port_replay);
        description=_T("load_port_replay");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_store_port_replay: public P4BaseEvent

{
public:
    EVENT_MASK(store_port_replay) * eventMask;

    P4Event_store_port_replay()
    {
        eventMask = (EVENT_MASK(store_port_replay) *)&m_eventMask;

        escr.ESCREventSelect=     0x05;
        cccr.CCCRSelect=    0x02;
        // eventType=EVENT_TYPE(store_port_replay);
        description=_T("store_port_replay");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_MOB_load_replay: public P4BaseEvent

{
public:
    EVENT_MASK(MOB_load_replay) * eventMask;

    P4Event_MOB_load_replay()
    {
        eventMask = (EVENT_MASK(MOB_load_replay) *)&m_eventMask;

        escr.ESCREventSelect=     0x03;
        cccr.CCCRSelect=    0x02;
        // eventType=EVENT_TYPE(MOB_load_replay);
        description=_T("MOB_load_replay");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_page_walk_type: public P4BaseEvent

{
public:
    EVENT_MASK(page_walk_type) * eventMask;

    P4Event_page_walk_type()
    {
        eventMask = (EVENT_MASK(page_walk_type) *)&m_eventMask;

        escr.ESCREventSelect=     0x01;
        cccr.CCCRSelect=    0x04;
        // eventType=EVENT_TYPE(page_walk_type);
        description=_T("page_walk_type");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_BSQ_cache_reference: public P4BaseEvent

{
public:
    EVENT_MASK(BSQ_cache_reference) * eventMask;

    P4Event_BSQ_cache_reference()
    {
        eventMask = (EVENT_MASK(BSQ_cache_reference) *)&m_eventMask;

        escr.ESCREventSelect=     0x0C;
        cccr.CCCRSelect=    0x07;
        // eventType=EVENT_TYPE(BSQ_cache_reference);
        description=_T("BSQ_cache_reference");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_IOQ_allocation: public P4BaseEvent

{
public:
    EVENT_MASK(IOQ) * eventMask;

    P4Event_IOQ_allocation()
    {
        eventMask = (EVENT_MASK(IOQ) *)&m_eventMask;

        escr.ESCREventSelect=     0x03;
        cccr.CCCRSelect=    0x06;
        // eventType=EVENT_TYPE(IOQ);
        description=_T("IOQ_allocation");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_IOQ_active_entries: public P4BaseEvent

{
public:
    EVENT_MASK(IOQ) * eventMask;

    P4Event_IOQ_active_entries()
    {
        eventMask = (EVENT_MASK(IOQ) *)&m_eventMask;

        escr.ESCREventSelect=     0x1A;
        cccr.CCCRSelect=    0x06;
        // eventType=EVENT_TYPE(IOQ);
        description=_T("IOQ_active_entries");
        UseCounter2();
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_FSB_data_activity: public P4BaseEvent

{
public:
    EVENT_MASK(FSB_data_activity) * eventMask;

    P4Event_FSB_data_activity()
    {
        eventMask = (EVENT_MASK(FSB_data_activity) *)&m_eventMask;

        escr.ESCREventSelect=     0x17;
        cccr.CCCRSelect=    0x06;
        // eventType=EVENT_TYPE(FSB_data_activity);
        description=_T("FSB_data_activity");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_BSQ_allocation: public P4BaseEvent

{
public:
    EVENT_MASK(BSQ) * eventMask;

    P4Event_BSQ_allocation()
    {
        eventMask = (EVENT_MASK(BSQ) *)&m_eventMask;

        escr.ESCREventSelect=     0x05;
        cccr.CCCRSelect=    0x07;
        // eventType=EVENT_TYPE(BSQ);
        description=_T("BSQ_allocation");
        UseCounter0();
    }

    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);

    }
};
class P4Event_BSQ_active_entries: public P4BaseEvent

{
public:
    EVENT_MASK(BSQ) * eventMask;

    P4Event_BSQ_active_entries()
    {
        eventMask = (EVENT_MASK(BSQ) *)&m_eventMask;

        escr.ESCREventSelect=     0x06;
        cccr.CCCRSelect=    0x07;
        // eventType=EVENT_TYPE(BSQ);
        description=_T("bsq_active_entries");
        UseCounter2();
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_SSE_input_assist: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_SSE_input_assist()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x34;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("SSE_input_assist");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_packed_SP_uop: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_packed_SP_uop()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x08;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("packed_SP_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_packed_DP_uop: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_packed_DP_uop()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x0C;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("packed_DP_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_scalar_SP_uop: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_scalar_SP_uop()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x0A;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("scalar_SP_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_scalar_DP_uop: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_scalar_DP_uop()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x0E;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("scalar_DP_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_64bit_MMX_uop: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_64bit_MMX_uop()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x02;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("64bit_MMX_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_128bit_MMX_uop: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_128bit_MMX_uop()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x1A;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("128bit_MMX_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_x87_FP_uop: public P4BaseEvent

{
public:
    EVENT_MASK(firm_uop) * eventMask;

    P4Event_x87_FP_uop()
    {
        eventMask = (EVENT_MASK(firm_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x04;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(firm_uop);
        description=_T("x87_FP_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_x87_SIMD_moves_uop: public P4BaseEvent

{
public:
    EVENT_MASK(x87_SIMD_moves_uop) * eventMask;

    P4Event_x87_SIMD_moves_uop()
    {
        eventMask = (EVENT_MASK(x87_SIMD_moves_uop) *)&m_eventMask;

        escr.ESCREventSelect=     0x2E;
        cccr.CCCRSelect=    0;
        // eventType=EVENT_TYPE(x87_SIMD_moves_uop); 
        description=_T("x87_SIMD_moves_uop");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_TC_misc: public P4BaseEvent

{
public:
    EVENT_MASK(TC_misc) * eventMask;

    P4Event_TC_misc()
    {
        eventMask = (EVENT_MASK(TC_misc) *)&m_eventMask;

        escr.ESCREventSelect=     0x06;
        cccr.CCCRSelect=    0x01;
        // eventType=EVENT_TYPE(TC_misc);
        description=_T("TC_misc");
        UseCounter4();
    }

    void UseCounter4()
    {
        SetCounter(4);;
    }
    void UseCounter5()
    {
        SetCounter(5);
    }
    void UseCounter6()
    {
        SetCounter(6);
    }
    void UseCounter7()
    {
        SetCounter(7);
    }
};
class P4Event_global_power_events: public P4BaseEvent

{
public:
    EVENT_MASK(global_power_events) * eventMask;

    P4Event_global_power_events()
    {
        eventMask = (EVENT_MASK(global_power_events) *)&m_eventMask;

        escr.ESCREventSelect=     0x13;
        cccr.CCCRSelect=    0x06;
        // eventType=EVENT_TYPE(global_power_events);
        description=_T("global_power_events");
        UseCounter0();
    }

    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_tc_ms_xfer: public P4BaseEvent

{
public:
    EVENT_MASK(tc_ms_xfer) * eventMask;

    P4Event_tc_ms_xfer()
    {
        eventMask = (EVENT_MASK(tc_ms_xfer) *)&m_eventMask;

        escr.ESCREventSelect=     0x05;
        cccr.CCCRSelect=    0x00;
        // eventType=EVENT_TYPE(tc_ms_xfer);
        description=_T("tc_ms_xfer");
        UseCounter4();
    }

    void UseCounter4()
    {
        SetCounter(4);;
    }
    void UseCounter5()
    {
        SetCounter(5);
    }
    void UseCounter6()
    {
        SetCounter(6);
    }
    void UseCounter7()
    {
        SetCounter(7);
    }
};
class P4Event_uop_queue_writes: public P4BaseEvent

{
public:
    EVENT_MASK(uop_queue_writes) * eventMask;

    P4Event_uop_queue_writes()
    {
        eventMask = (EVENT_MASK(uop_queue_writes) *)&m_eventMask;

        escr.ESCREventSelect=     0x09;
        cccr.CCCRSelect=    0x00;
        // eventType=EVENT_TYPE(uop_queue_writes);
        description=_T("uop_queue_writes");
        UseCounter4();
    }

    void UseCounter4()
    {
        SetCounter(4);;
    }
    void UseCounter5()
    {
        SetCounter(5);
    }
    void UseCounter6()
    {
        SetCounter(6);
    }
    void UseCounter7()
    {
        SetCounter(7);
    }
};
class P4Event_retired_mispred_branch_type: public P4BaseEvent

{
public:
    EVENT_MASK(branch_type) * eventMask;

    P4Event_retired_mispred_branch_type()
    {
        eventMask = (EVENT_MASK(branch_type) *)&m_eventMask;

        escr.ESCREventSelect=     0x05;
        cccr.CCCRSelect=    0x02;
        // eventType=EVENT_TYPE(branch_type);
        description=_T("retired_mispred_branch_type");
        UseCounter4();
    }

    void UseCounter4()
    {
        SetCounter(4);;
    }
    void UseCounter5()
    {
        SetCounter(5);
    }
    void UseCounter6()
    {
        SetCounter(6);
    }
    void UseCounter7()
    {
        SetCounter(7);
    }
};
class P4Event_retired_branch_type: public P4BaseEvent

{
public:
    EVENT_MASK(branch_type) * eventMask;

    P4Event_retired_branch_type()
    {
        eventMask = (EVENT_MASK(branch_type) *)&m_eventMask;

        escr.ESCREventSelect=     0x04;
        cccr.CCCRSelect=    0x04;
        // eventType=EVENT_TYPE(branch_type);
        description=_T("retired_branch_type");
        UseCounter4();
    }

    void UseCounter4()
    {
        SetCounter(4);;
    }
    void UseCounter5()
    {
        SetCounter(5);
    }
    void UseCounter6()
    {
        SetCounter(6);
    }
    void UseCounter7()
    {
        SetCounter(7);
    }
};
class P4Event_resource_stall: public P4BaseEvent

{
public:
    EVENT_MASK(resource_stall) * eventMask;

    P4Event_resource_stall()
    {
        eventMask = (EVENT_MASK(resource_stall) *)&m_eventMask;

        escr.ESCREventSelect=     0x01;
        cccr.CCCRSelect=    0x02;
        // eventType=EVENT_TYPE(resource_stall);
        description=_T("resource_stall");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }

};
class P4Event_WC_Buffer: public P4BaseEvent

{
public:
    EVENT_MASK(WC_Buffer) * eventMask;

    P4Event_WC_Buffer()
    {
        eventMask = (EVENT_MASK(WC_Buffer) *)&m_eventMask;

        escr.ESCREventSelect=     0x05;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(WC_Buffer);
        description=_T("WC_Buffer");
        UseCounter8();
    }
    void UseCounter8()
    {
        SetCounter(8);
    }
    void UseCounter9()
    {
        SetCounter(9);
    }
    void UseCounter10()
    {
        SetCounter(10);
    }
    void UseCounter11()
    {
        SetCounter(11);
    }

};
class P4Event_b2b_cycles: public P4BaseEvent

{
public:
    EVENT_MASK(b2b_cycles) * eventMask;

    P4Event_b2b_cycles()
    {
        eventMask = (EVENT_MASK(b2b_cycles) *)&m_eventMask;

        escr.ESCREventSelect=     0x16;
        cccr.CCCRSelect=    0x03;
        // eventType=EVENT_TYPE(b2b_cycles);
        description=_T("b2b_cycles");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_bnr: public P4BaseEvent

{
public:
    EVENT_MASK(bnr) * eventMask;

    P4Event_bnr()
    {
        eventMask = (EVENT_MASK(bnr) *)&m_eventMask;

        escr.ESCREventSelect=     0x08;
        cccr.CCCRSelect=    0x03;
        // eventType=EVENT_TYPE(bnr);
        description=_T("bnr");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_snoop: public P4BaseEvent

{
public:
    EVENT_MASK(snoop) * eventMask;

    P4Event_snoop()
    {
        eventMask = (EVENT_MASK(snoop) *)&m_eventMask;

        escr.ESCREventSelect=     0x06;
        cccr.CCCRSelect=    0x03;
        // eventType=EVENT_TYPE(snoop);
        description=_T("snoop");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_response: public P4BaseEvent

{
public:
    EVENT_MASK(response) * eventMask;

    P4Event_response()
    {
        eventMask = (EVENT_MASK(response) *)&m_eventMask;

        escr.ESCREventSelect=     0x04;
        cccr.CCCRSelect=    0x03;
        // eventType=EVENT_TYPE(response);
        description=_T("response");
        UseCounter0();
    }
    void UseCounter0()
    {
        SetCounter(0);
    }
    void UseCounter1()
    {
        SetCounter(1);
    }
    void UseCounter2()
    {
        SetCounter(2);
    }
    void UseCounter3()
    {
        SetCounter(3);
    }
};
class P4Event_front_end_event: public P4BaseEvent

{
public:
    EVENT_MASK(nbogus_bogus) * eventMask;

    P4Event_front_end_event()
    {
        eventMask = (EVENT_MASK(nbogus_bogus) *)&m_eventMask;

        escr.ESCREventSelect=     0x08;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(nbogus_bogus);
        description=_T("front_end_event");
        UseCounter12();
    }

    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_execution_event: public P4BaseEvent

{
public:
    EVENT_MASK(execution_event) * eventMask;

    P4Event_execution_event()
    {
        eventMask = (EVENT_MASK(execution_event) *)&m_eventMask;

        escr.ESCREventSelect=     0x0C;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(execution_event);
        description=_T("execution_event");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_replay_event: public P4BaseEvent

{
public:
    EVENT_MASK(nbogus_bogus) * eventMask;

    P4Event_replay_event()
    {
        eventMask = (EVENT_MASK(nbogus_bogus) *)&m_eventMask;

        escr.ESCREventSelect=     0x09;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(nbogus_bogus);
        description=_T("replay_event");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_instr_retired: public P4BaseEvent

{
public:
    EVENT_MASK(instr_retired) * eventMask;

    P4Event_instr_retired()
    {
        eventMask = (EVENT_MASK(instr_retired) *)&m_eventMask;

        escr.ESCREventSelect=     0x02;
        cccr.CCCRSelect=    0x04;
        // eventType=EVENT_TYPE(instr_retired);
        description=_T("instr_retired");
        UseCounter12();
    }

    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_uops_retired: public P4BaseEvent

{
public:
    EVENT_MASK(nbogus_bogus) * eventMask;

    P4Event_uops_retired()
    {
        eventMask = (EVENT_MASK(nbogus_bogus) *)&m_eventMask;

        escr.ESCREventSelect=     0x01;
        cccr.CCCRSelect=    0x04;
        // eventType=EVENT_TYPE(nbogus_bogus);
        description=_T("uops_retired");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_uop_type: public P4BaseEvent

{
public:
    EVENT_MASK(uop_type) * eventMask;

    P4Event_uop_type()
    {
        eventMask = (EVENT_MASK(uop_type) *)&m_eventMask;

        escr.ESCREventSelect=     0x02;
        cccr.CCCRSelect=    0x02;
        // eventType=EVENT_TYPE(uop_type);
        description=_T("uop_type");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_branch_retired: public P4BaseEvent

{
public:
    EVENT_MASK(branch_retired) * eventMask;

    P4Event_branch_retired()
    {
        eventMask = (EVENT_MASK(branch_retired) *)&m_eventMask;

        escr.ESCREventSelect=     0x06;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(branch_retired);
        description=_T("branch_retired");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_mispred_branch_retired: public P4BaseEvent

{
public:
    EVENT_MASK(mispred_branch_retired) * eventMask;

    P4Event_mispred_branch_retired()
    {
        eventMask = (EVENT_MASK(mispred_branch_retired) *)&m_eventMask;

        escr.ESCREventSelect=     0x03;
        cccr.CCCRSelect=    0x04;
        // eventType=EVENT_TYPE(mispred_branch_retired);
        description=_T("mispred_branch_retired");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_x87_assist: public P4BaseEvent

{
public:
    EVENT_MASK(x87_assist) * eventMask;

    P4Event_x87_assist()
    {
        eventMask = (EVENT_MASK(x87_assist) *)&m_eventMask;

        escr.ESCREventSelect=     0x03;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(x87_assist);
        description=_T("x87_assist");
        UseCounter12();
    }
    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }
};
class P4Event_machine_clear: public P4BaseEvent

{
public:
    EVENT_MASK(machine_clear) * eventMask;

    P4Event_machine_clear()
    {
        eventMask = (EVENT_MASK(machine_clear) *)&m_eventMask;
        escr.ESCREventSelect=     0x02;
        cccr.CCCRSelect=    0x05;
        // eventType=EVENT_TYPE(machine_clear);
        description=_T("machine_clear");
        UseCounter12();
    }



    void UseCounter12()
    {
        SetCounter(12);
    }
    void UseCounter13()
    {
        SetCounter(13);

    }

    void UseCounter14()
    {
        SetCounter(14);
    }
    void UseCounter15()
    {
        SetCounter(15);

    }

    void UseCounter16()
    {
        SetCounter(16);
    }
    void UseCounter17()
    {
        SetCounter(17);

    }

};

#endif // EVENTMODES_H

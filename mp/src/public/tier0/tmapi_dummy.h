//========= Copyright Valve Corporation, All rights reserved. ============//
// This is the null header file used to remove Telemetry calls.

#define TMERR_DISABLED 1
#define TMPRINTF_TOKEN_NONE 0

#define tmGetSessionName(...)
#define tmEndTryLock(...)
#define tmEndTryLockEx(...)
#define tmSetLockState(...)
#define tmSetLockStateEx(...)
#define tmSetLockStateMinTime(...) 0
#define tmSetLockStateMinTimeEx(...) 0
#define tmSignalLockCount(...)

#define tmCheckVersion(...) 0
#define tmGetCallStack(...) 0
#define tmSendCallStack( ... ) TMPRINTF_TOKEN_NONE
#define tmGetCallStackR(...) 0
#define tmSendCallStackR(...) TMPRINTF_TOKEN_NONE
#define tmSendCallStackWithSkipR(...) TMPRINTF_TOKEN_NONE

#define tmGetVersion(...) 0
#define tmStartup(...)  TMERR_DISABLED
#define tmGetPlatformInformation(...) TMERR_DISABLED
#define tmInitializeContext(...) TMERR_DISABLED
#define tmShutdown(...) TMERR_DISABLED

#define tmEnter(...)
#define tmEnterEx(...)
#define tmZone(...) 
#define tmZoneFiltered(...)
#define tmLeave(...)
#define tmLeaveEx(...)

#define tmBeginTimeSpan(...)
#define tmEndTimeSpan(...)

#define tmBeginTimeSpanAt(...)
#define tmEndTimeSpanAt(...)

#define tmDynamicString(...) ""

#define tmEmitAccumulationZone(...)

#define tmGetStati(...) 0

#define tmSetVariable(...)

#define tmBlob(...)
#define tmDisjointBlob(...)
#define tmSetTimelineSectionName(...)
#define tmThreadName(...)
#define tmLockName(...)
#define tmMessage(...)
#define tmAlloc(...)
#define tmAllocEx(...)

#define tmTryLock(...)
#define tmTryLockEx(...)
    
#define tmPlot(...)
#define tmPlotF32(...)
#define tmPlotF64(...)
#define tmPlotI32(...)
#define tmPlotU32(...)
#define tmPlotS32(...)
#define tmPlotI64(...)
#define tmPlotU64(...)
#define tmPlotS64(...)

#define tmPPUGetListener(...) TMERR_DISABLED
#define tmPPURegisterSPUProgram(...) TMERR_DISABLED
#define tmSPUBindContextToListener(...) 
#define tmSPUUpdateTime(...)
#define tmSPUFlushImage(...)

#define NTELEMETRY 1

#define TM_CONTEXT_LITE(val) ((char*)(val))
#define TM_CONTEXT_FULL(val) ((char*)(val))

typedef char *HTELEMETRY;


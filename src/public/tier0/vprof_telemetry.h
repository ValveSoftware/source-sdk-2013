//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Real-Time Hierarchical Telemetry Profiling
//
// $NoKeywords: $
//=============================================================================//

#ifndef VPROF_TELEMETRY_H
#define VPROF_TELEMETRY_H

#if !defined( MAKE_VPC )

#if !defined( RAD_TELEMETRY_DISABLED ) && ( defined( IS_WINDOWS_PC ) || defined( _LINUX ) )
// Rad Telemetry profiling is enabled on Win32 and Win64.
#define RAD_TELEMETRY_ENABLED
#endif

#endif // !defined( MAKE_VPC )


#if !defined( RAD_TELEMETRY_ENABLED )

//
// If Telemetry isn't enabled, then kill all the tmZone() macros, etc.
//
#define NTELEMETRY		1
// Different versions of radbase.h define RADCOPYRIGHT to different values. So undef that here.
#undef RADCOPYRIGHT

#include "tmapi_dummy.h"

inline void TelemetryTick() {}
inline void TelemetrySetLevel( unsigned int Level ) {}

#define TELEMETRY_REQUIRED( tmRequiredCode ) //a basic wrapper to only enable code if telemetry is present
#define TELEMETRY_REQUIRED_REPLACE( tmRequiredCode, replacementCode ) replacementCode //in case you need to replace the code with something specific if telemetry isn't present

//#define TMZF_NONE

#else

//
// Telemetry is enabled. Include the telemetry header.
//
#include "../../thirdparty/telemetry/include/telemetry.h"
// Different versions of radbase.h define RADCOPYRIGHT to different values. So undef that here.
#undef RADCOPYRIGHT

PLATFORM_INTERFACE void TelemetryTick();
PLATFORM_INTERFACE void TelemetrySetLevel( unsigned int Level );

struct TelemetryData
{
	HTELEMETRY tmContext[32];
	float flRDTSCToMilliSeconds;	// Conversion from tmFastTime() (rdtsc) to milliseconds.
	uint32 FrameCount;				// Count of frames to capture before turning off.
	char ServerAddress[128];		// Server name to connect to.
	uint32 ZoneFilterVal;			// tmZoneFiltered default filtered value (in MicroSeconds)
	int playbacktick;				// GetPlaybackTick() value from demo file (or 0 if not playing a demo).
	float dotatime;					// CDOTAGamerules::GetDOTATime()
	uint32 DemoTickStart;			// Start telemetry on demo tick #
	uint32 DemoTickEnd;				// End telemetry on demo tick #
	uint32 Level;					// Current Telemetry level (Use TelemetrySetLevel to modify)
};
PLATFORM_INTERFACE TelemetryData g_Telemetry;

#define TELEMETRY_REQUIRED( tmRequiredCode ) tmRequiredCode //a basic wrapper to only enable code if telemetry is present
#define TELEMETRY_REQUIRED_REPLACE( tmRequiredCode, replacementCode ) tmRequiredCode //in case you need to replace the code with something specific if telemetry isn't present

#endif // RAD_TELEMETRY_ENABLED










#define TELEMETRY_ERROR_BUILD_DISABLED		TELEMETRY_REQUIRED_REPLACE( TMERR_DISABLED, 0x0001 )
#define TELEMETRY_ERROR_DISCONNECTED		TELEMETRY_REQUIRED_REPLACE( TMCS_DISCONNECTED, 0 )


#define TELEMETRY_LEVEL0	TELEMETRY_REQUIRED_REPLACE( g_Telemetry.tmContext[0], 0 )	// high level tmZone()
#define TELEMETRY_LEVEL1	TELEMETRY_REQUIRED_REPLACE( g_Telemetry.tmContext[1], 0 )	// lower level tmZone(), tmZoneFiltered()
#define TELEMETRY_LEVEL2	TELEMETRY_REQUIRED_REPLACE( g_Telemetry.tmContext[2], 0 )	// VPROF_0
#define TELEMETRY_LEVEL3	TELEMETRY_REQUIRED_REPLACE( g_Telemetry.tmContext[3], 0 )	// VPROF_1
#define TELEMETRY_LEVEL4	TELEMETRY_REQUIRED_REPLACE( g_Telemetry.tmContext[4], 0 )	// VPROF_2
#define TELEMETRY_LEVEL5	TELEMETRY_REQUIRED_REPLACE( g_Telemetry.tmContext[5], 0 )	// VPROF_3
#define TELEMETRY_LEVEL6	TELEMETRY_REQUIRED_REPLACE( g_Telemetry.tmContext[6], 0 )	// VPROF_4

//__rdtsc()
#define TM_FAST_TIME() TELEMETRY_REQUIRED_REPLACE( tmFastTime(), 0 )


//int RADEXPLINK tmLoadTelemetry( int const kUseCheckedDLL );
//
//Description
//	On dynamic library platforms, this function loads the Telemetry DLL (or shared library) and then hooks up all of the dynamic function pointers.
//Parameters
//	kUseCheckedDLL [in] set to 0 if you want to use the release mode DLL or 1 if you want to use the checked DLL. The checked DLL is compiled with optimizations but does extra run time checks and reporting.
#define TM_LOAD_TELEMETRY(kUseCheckedDll) TELEMETRY_REQUIRED_REPLACE( tmLoadTelemetry(kUseCheckedDll), 0 )


//TmErrorCode tmStartup(  ); 
//
//Description
//	Starts up all of Telemetry.
#define TM_STARTUP() TELEMETRY_REQUIRED_REPLACE( tmStartup(), TELEMETRY_ERROR_BUILD_DISABLED )


//void tmShutdown(  );
//
//Description
//	Shuts down all of Telemetry.
#define TM_SHUTDOWN() TELEMETRY_REQUIRED( tmShutdown() )


//TmErrorCode tmInitializeContext( HTELEMETRY * pcx,void * pArena,TmU32 const kArenaSize ); 
//
//Description
//	Initializes a context. 
//
//Parameters
//	pcx [out] pointer to a TmHandle in which to store a handle to the initialized context 
//	pArena [in] pointer to a memory buffer for use by Telemetry to create the context. Applications should never free or modify this memory unless tmShutdownContext has been called! 
//	kArenaSize [in] size of the memory buffer pointed to by pArena. You want this large enough so that Telemetry never stalls trying to send over the network, but not so large that it takes up too much memory from your app. A good initial value is 1MB. 
#define TM_INITIALIZE_CONTEXT( pContext, pArena, kArenaSize ) TELEMETRY_REQUIRED_REPLACE( tmInitializeContext( pContext, pArena, kArenaSize ), TELEMETRY_ERROR_BUILD_DISABLED )


//void tmZoneFiltered( HTELEMETRY cx,TmU64 const kThreshold,TmU32 const kFlags,char const * kpFormat,...  );
//
//Description
//	Identical to tmZone however also allows specification of threshold duration.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kThreshold [in] microseconds that the zone must span before being sent to server.
//	kFlags [in] flags for the zone (same as those passed to tmEnter.
//	kpFormat [in] name of the zone (same as those passed to tmEnter. This may contain printf-style format specifiers.
#define TM_ZONE_FILTERED( context, kThreshold, kFlags, kpFormat, ... ) TELEMETRY_REQUIRED( tmZoneFiltered( context, kThreshold, kFlags, kpFormat, ##__VA_ARGS__ ) )


//void tmZone( HTELEMETRY cx,TmU32 const kFlags,char const * kpFormat,...  );
//
//Description
//	Helper macro in C++ that creates an anonymous local object that automatically calls tmEnter and then tmLeave when exiting scope.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kFlags [in] flags for the zone (same as those passed to tmEnter
//	kpFormat [in] name of the zone (same as those passed to tmEnter. This may contain printf-style format specifiers.
#define TM_ZONE( context, kFlags, kpFormat, ... ) TELEMETRY_REQUIRED( tmZone( context, kFlags, kpFormat, ##__VA_ARGS__ ) )

//Standardized zones
#define TM_ZONE_DEFAULT( context ) TM_ZONE( context, TMZF_NONE, __FUNCTION__ )
#define TM_ZONE_IDLE( context ) TM_ZONE( context, TMZF_IDLE, __FUNCTION__ )
#define TM_ZONE_STALL( context ) TM_ZONE( context, TMZF_STALL, __FUNCTION__ )


//TmErrorCode tmCheckVersion( HTELEMETRY cx, TmU32 const major, TmU32 const minor, TmU32 const build, TmU32 const cust );
//
//could not find documentation
#define TM_CHECK_VERSION( context, major, minor, build, cust ) TELEMETRY_REQUIRED_REPLACE( tmCheckVersion( context, major, minor, build, cust ), TELEMETRY_ERROR_BUILD_DISABLED )


//TmErrorCode tmListenIPC( HTELEMETRY cx, char const *name );
//
//could not find documentation
#define TM_LISTEN_IPC( context, name ) TELEMETRY_REQUIRED_REPLACE( tmListenIPC( context, name ), TELEMETRY_ERROR_BUILD_DISABLED )


//void tmUpdateSymbolData( HTELEMETRY cx );
//
//Description
//	Tells Telemetry to rescan loaded modules for symbol database information.
//
//Parameters
//	cx [in] a valid context
#define TM_UPDATE_SYMBOL_DATA( context ) TELEMETRY_REQUIRED( tmUpdateSymbolData( context ) )


//TmErrorCode tmGetSessionName( HTELEMETRY cx,char * dst,int const kDstSize );
//
//Description
//	Gets the name of the current session as determined by the server.
//
//Parameters
//	cx [in] a valid context
//	dst [in] pointer to buffer in which to copy the name
//	kDstSize [in] size of dst
#define TM_GET_SESSION_NAME( context, dst, kDstSize ) TELEMETRY_REQUIRED_REPLACE( tmGetSessionName( context, dst, kDstSize ), TELEMETRY_ERROR_BUILD_DISABLED )


//void tmUnwindToDebugZoneLevel( HTELEMETRY cx,int const kLevel );
//
//Description
//	Unwinds the current thread's zone stack back to the given level.
//
//Parameters
//	cx [in] a valid context
//	kLevel [in] an arbitrary constant that was previously set using tmSetDebugZoneLevel
#define TM_UNWIND_TO_DEBUG_ZONE_LEVEL( context, kLevel ) TELEMETRY_REQUIRED( tmUnwindToDebugZoneLevel( context, kLevel ) )


//void tmSetDebugZoneLevel( HTELEMETRY cx,int const kLevel );
//
//Description
//	Set the Telemetry debug zone level.
//
//Parameters
//	cx [in] a valid context
//	kLevel [in] an arbitrary constant used to differentiate this zone level from another
#define TM_SET_DEBUG_ZONE_LEVEL( context, kLevel ) TELEMETRY_REQUIRED( tmSetDebugZoneLevel( context, kLevel ) )


//void tmCheckDebugZoneLevel( HTELEMETRY cx,int const kLevel ); 
//
//Description
//	Check the current Telemetry debug zone level.
//
//Parameters
//	cx [in] a valid context
//	kLevel [in] an arbitrary constant that was previously set using tmSetDebugZoneLevel
#define TM_CHECK_DEBUG_ZONE_LEVEL( context, kLevel ) TELEMETRY_REQUIRED( tmCheckDebugZoneLevel( context, kLevel ) )


//int tmGetCallStack( HTELEMETRY cx,TmCallStack * dst );
//
//Description
//	Retrieves the callstack for the current thread.
//
//Parameters
//	cx [in] a valid context
//	dst [in] TmCallStack structure in which to store the callstack information
#define TM_GET_CALL_STACK( context, TmCallStack_Ptr ) TELEMETRY_REQUIRED_REPLACE( tmGetCallStack( context, TmCallStack_Ptr ), 0 )


//int tmSendCallStack( HTELEMETRY cx,TmCallStack const * kpCallStack );
//
//Description
//	Sends the current callstack to the server so that later references with string format specifiers are up to date.
//
//Parameters
//	cx [in] a valid context
//	kpCallStack [in] pointer to a pre-existing callstack or 0 if it should use the current callstack
#define TM_SEND_CALL_STACK( context, TmCallStack_Ptr ) TELEMETRY_REQUIRED_REPLACE( tmSendCallStack( context, TmCallStack_Ptr ), 0 )


//TmErrorCode tmGetLastError( HTELEMETRY cx );
//
//Description
//	Returns and clears the current error condition for the given context.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
#define TM_GET_LAST_ERROR( context ) TELEMETRY_REQUIRED_REPLACE( tmGetLastError( context ), TELEMETRY_ERROR_BUILD_DISABLED )


//void tmShutdownContext( HTELEMETRY cx );
//
//Description
//	Shuts down the given context.
//
//Parameters
//	cx [in] handle to a valid context
#define TM_SHUTDOWN_CONTEXT( context ) TELEMETRY_REQUIRED( tmShutdownContext( context ) )


//TmU64 tmGetAccumulationStart( HTELEMETRY cx );
//
//Description
//	Gets the start time for a set of accumulation zones.
//
//Parameters
//	cx [in] handle to a telemetry context
//
//Return value
//	return [out] start time for use with tmEmitAccumulationZone.
#define TM_GET_ACCUMULATION_START( context ) TELEMETRY_REQUIRED_REPLACE( tmGetAccumulationStart( context ), 0 )


//TmU64 tmGetLastContextSwitchTime( HTELEMETRY cx );
//
//Description
//	Returns the time of the last received context switch event.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//
//Return value
//	return [out] the time of the last received context switch event, or 0 on failure (e.g. invalid context, context switches disabled, etc.) 
#define TM_GET_LAST_CONTEXT_SWITCH_TIME( context ) TELEMETRY_REQUIRED_REPLACE( tmGetLastContextSwitchTime( context ), 0 )


//void tmEnterAccumulationZone( HTELEMETRY cx,TmI64 * zone_variable );
//
//Description
//	Updates a zone variable passed to tmEmitAccumulationZone later.
//
//Parameters
//	cx [in] handle to a telemetry context
//	zone_variable [in] zone_variable to update
#define TM_ENTER_ACCUMULATION_ZONE( context, zone_variable ) TELEMETRY_REQUIRED( tmEnterAccumulationZone( context, zone_variable ) )


//void tmLeaveAccumulationZone( HTELEMETRY cx,TmI64 * zone_variable );
//
//Description
//	Updates a zone variable passed to tmEmitAccumulationZone later.
//
//Parameters
//	cx [in] handle to a telemetry context
//	zone_variable [in] zone_variable to update
#define TM_LEAVE_ACCUMULATION_ZONE( context, zone_variable ) TELEMETRY_REQUIRED( tmLeaveAccumulationZone( context, zone_variable ) )


//void tmGetFormatCode( TmU32* pCode, char const * kpFmt );
//
//could not find documentation
#define TM_GET_FORMAT_CODE( context, pCode, kpFmt ) TELEMETRY_REQUIRED( tmGetFormatCode( context, pCode, kpFmt ) )


//char const * tmDynamicString( HTELEMETRY cx,char const * kpString );
//
//Description
//	Returns an opaque string identifier usable by Telemetry that is a copy of the kpString parameter.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpString [in] string to copy
#define TM_DYNAMIC_STRING( context, kpString ) TELEMETRY_REQUIRED_REPLACE( tmDynamicString( context, kpString ), NULL )


//void tmClearStaticString( HTELEMETRY cx,char const * kpString );
//
//Description
//	Marks the given string pointer as 'changed', forcing Telemetry to resend its contents.
//
//Parameters
//	cx [in] handle to a valid Telemetry context 
//	kpString [in] string to reset 
#define TM_CLEAR_STATIC_STRING( context, kpString ) TELEMETRY_REQUIRED( tmClearStaticString( context, kpString ) )


//void tmEnable( HTELEMETRY cx,TmOption const kOption,int const kValue );
//
//Description
//	Enables or disables a specific Telemetry option.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kOption [in] the option to modify. One of the TmOption constants.
//	kValue [in] the new value for the option. Must be 0 (disable) or 1 (enable).
#define TM_ENABLE( context, kOption, kValue ) TELEMETRY_REQUIRED( tmEnable( context, kOption, kValue ) )


//int tmIsEnabled( HTELEMETRY cx,TmOption const kOption );
//
//Description
//	Inspects the state of a Telemetry option.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kOption [in] the option to inspect. Must be one of the TmOption constants.
//
//Return value
//	return [out] 1 if the option is enabled, 0 if not enabled. In case of error, 0 is enabled and last error is set.
#define TM_IS_ENABLED( context, kOption ) TELEMETRY_REQUIRED_REPLACE( tmIsEnabled( context, kOption ), 0 )

//void tmSetParameter( HTELEMETRY cx,TmParameter const kParameter,void const * kpValue );
//
//Description
//	Sets a Telemetry parameter.
//
//Parameters
//	cx [in] a valid context
//	kParameter [in] TmParameter constant
//	kpValue [in] pointer to value of the parameter. This varies depending on the value.
#define TM_SET_PARAMETER( context, kParameter, kpValue ) TELEMETRY_REQUIRED( tmSetParameter( context, kParameter, kpValue ) )


//TmErrorCode tmOpen( HTELEMETRY cx,char const * kpAppName,char const * kpBuildInfo,char const * kpServerAddress,TmConnectionType const kConnection,TmU16 const kServerPort,TmU32 const kFlags,int const kTimeoutMS );
//
//Description
//	Starts a Telemetry context and attempts to connect to a Telemetry server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpAppName [in] name of the application. NOTE: This MUST be alphanumeric only, e.g. "Game234", no spaces, special characters, etc. are allowed.
//	kpBuildInfo [in] information about the application. Used to annotate this Telemetry session. This information will show up when connecting to the server later to examine this particular run. Common uses include build numbers, compiled options, etc.
//	kpServerAddress [in] name or IP address of the Telemetry server to connect to
//	kConnection [in] type of connection to establish. Currently only TMCT_TCP is supported.
//	kServerPort [in] port to connect to. Specify 0 or TELEMETRY_DEFAULT_PORT for the default port.
//	kFlags [in] bitwise OR of TmOpenFlag flags
//	kTimeoutMS [in] duration, in milliseconds, to wait for a connection to the Telemetry server. Specify -1 to wait indefinitely.
//
//Return value
//	return [out] TM_OK on success, or on error one of the following: TMERR_INVALID_PARAM, TMERR_INVALID_CONTEXT, TMERR_UNKNOWN_NETWORK.
#define TM_OPEN( context, kpAppName, kpBuildInfo, kpServerAddress, kConnection, kServerPort, kFlags, kTimeoutMS ) TELEMETRY_REQUIRED_REPLACE( tmOpen( context, kpAppName, kpBuildInfo, kpServerAddress, kConnection, kServerPort, kFlags, kTimeoutMS ), TELEMETRY_ERROR_BUILD_DISABLED )


//void tmClose( HTELEMETRY cx );
//
//Description
//	Closes a context previously opened with tmOpen.
//
//Parameters
//	cx [in] handle to a valid and open Telemetry context
#define TM_CLOSE( context ) TELEMETRY_REQUIRED( tmClose( context ) )


//void tmTick( HTELEMETRY cx );
//
//Description
//	"Ticks" the Telemetry context, i.e. signifies the end of a frame of execution so that Telemetry can perform internal processing and send data to the Telemetry server.
//
//Parameters
//	cx [in] handle to a valid and open Telemetry context
#define TM_TICK( context ) TELEMETRY_REQUIRED( tmTick( context ) )


//void tmFlush( HTELEMETRY cx );
//
//cannot find documentation
#define TM_FLUSH( context ) TELEMETRY_REQUIRED( tmFlush( context ) )


//void tmPause( HTELEMETRY cx,int const kPause );
//
//Description
//	Pauses/unpauses Telemetry capture.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kPause [in] 1 to pause, 0 to unpause
#define TM_PAUSE( context, kPause ) TELEMETRY_REQUIRED( tmPause( context, kPause ) )


//int tmIsPaused( HTELEMETRY cx );
//
//cannot find documentation. Presumably returns 1 if paused, 0 if not
#define TM_IS_PAUSED( context ) TELEMETRY_REQUIRED_REPLACE( tmIsPaused( context ), 0 )


//TmConnectionStatus tmGetConnectionStatus( HTELEMETRY cx );
//
//Description
//	Get the current connection status.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//
//Return value
//	return [out] a TmConnectionStatus constant. On error returns TMCS_DISCONNECTED and sets the context's last error.
#define TM_GET_CONNECTION_STATUS( context ) TELEMETRY_REQUIRED_REPLACE( tmGetConnectionStatus( context ), TELEMETRY_ERROR_DISCONNECTED )


//void tmFree( HTELEMETRY cx,void const * kpPtr );
//
//Description
//	Notify Telemetry that memory has been freed.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpPtr [in] address of the old memory block previously passed to a tmAlloc call
#define TM_FREE( context, kpPtr ) TELEMETRY_REQUIRED( tmFree( context, kpPtr ) )


//TmI32 tmGetStati( HTELEMETRY cx,TmStat const kStat );
//
//Description
//	Retrieves internal Telemetry statistics.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kStat [in] the TmStat to retrieve
//
//Return value
//	return [out] the value of the indicated internal statistic. On error returns 0 and sets last error.
#define TM_GET_STAT_I( context, kStat ) TELEMETRY_REQUIRED_REPLACE( tmGetStati( context, kStat ), 0 )


//void tmLeave( HTELEMETRY cx );
//
//Description
//	Notify Telemetry that a zone is being left.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
#define TM_LEAVE( context ) TELEMETRY_REQUIRED( tmLeave( context ) )


//void tmLeaveEx( HTELEMETRY cx,TmU64 const kMatchId,TmU32 const kThreadId,char const * kpFilename,int const kLine );
//
//Description
//	Notify Telemetry that a zone is being left.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kMatchId [in] match id returned by tmEnterEx, used for Zones: Runtime Zone Filtering by Duration . Pass 0 if you're not filtering.
//	kThreadId [in] thread id for this zone. Pass 0 for 'current thread'.
//	kpFilename [in] name of the file
//	kLine [in] line number for the leave
#define TM_LEAVE_EX( context, kMatchId, kThreadId, kpFilename, kLine ) TELEMETRY_REQUIRED( tmLeaveEx( context, kMatchId, kThreadId, kpFilename, kLine ) )


//void tmTryLock( HTELEMETRY cx,void const * kPtr,char const * kpLockName,...  );
//
//Description
//	Notify Telemetry that an attempt to grab a lock is beginning.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kPtr [in] pointer to the item being locked
//	kpLockName [in] description of this lock attempt. This may contain printf-style format specifiers.
#define TM_TRY_LOCK( context, kPtr, kpLockName, ... ) TELEMETRY_REQUIRED( tmTryLock( context, kPtr, kpLockName, ##__VA_ARGS__ ) )


//void tmTryLockEx( HTELEMETRY cx,TmU64 * matcher,TmU64 const kThreshold,char const * kpFileName,int const kLine,void const * kPtr,char const * kpLockName,...  );
//
//Description
//	Same as tmTryLock, but also specifying file and line information.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	matcher [in] reference in which to store a match ID used for later filtering by duration
//	kpFileName [in] pointer to filename
//	kLine [in] line number
//	kPtr [in] pointer to the item being locked
//	kpLockName [in] description of this lock attempt. This may contain printf-style format specifiers.
#define TM_TRY_LOCK_EX( context, matcher, kThreshold, kpFileName, kLine, kPtr, kpLockName, ... ) TELEMETRY_REQUIRED( tmTryLockEx( context, matcher, kThreshold, kpFileName, kLine, kPtr, kpLockName, ##__VA_ARGS__ ) )


//void tmEndTryLock( HTELEMETRY cx,void const * kPtr,TmLockResult const kResult );
//
//Description
//	Specify the result of a lock attempt previously started with tmTryLock.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kPtr [in] pointer for the object (mutex, etc.) being locked
//	kResult [in] the result of the lock attempt, one of TmLockResult.
#define TM_END_TRY_LOCK( context, kPtr, kResult ) TELEMETRY_REQUIRED( tmEndTryLock( context, kPtr, kResult ) )


//void tmEndTryLockEx( HTELEMETRY cx,TmU64 const kMatchId,char const * kpFileName,int const kLine,void const * kPtr,TmLockResult const kResult );
//
//Description
//	Specify the result of a lock attempt previously started with tmTryLock, but also with file and line information.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kMatchId [in] match id returned from tmTryLockEx
//	kpFileName [in] pointer to filename
//	kLine [in] line number
//	kPtr [in] pointer for the object (mutex, etc.) being locked
//	kResult [in] the result of the lock attempt, one of TmLockResult.
#define TM_END_TRY_LOCK_EX( context, kMatchId, kpFileName, kLine, kPtr, kResult ) TELEMETRY_REQUIRED( tmEndTryLockEx( context, kMatchId, kpFileName, kLine, kPtr, kResult ) )


//void tmBeginTimeSpan( HTELEMETRY cx,TmU64 const kId,TmU32 const kFlags,char const * kpNameFormat,...  );
//
//Description
//	Start the beginning of a timespan.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kId [in] a user defined identifier for the timespan. Should be > 0.
//	kFlags [in] flags for the timespan. Reserved, must be 0.
//	kpNameFormat [in] information about this timespan. This may contain printf-style format specifiers.
#define TM_BEGIN_TIME_SPAN( context, kId, kFlags, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmBeginTimeSpan( context, kId, kFlags, kpNameFormat, ##__VA_ARGS__ ) )


//void tmEndTimeSpan( HTELEMETRY cx,TmU64 const kId,TmU32 const kFlags,char const * kpNameFormat,...  );
//
//Description
//	Mark the end a timespan.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kId [in] a user defined identifier for the timespan. Should be the same as the identifier passed to tmBeginTimeSpan.
//	kFlags [in] flags for the timespan. Reserved, must be 0.
//	kpNameFormat [in] information about this timespan. This may contain printf-style format specifiers.
#define TM_END_TIME_SPAN( context, kId, kFlags, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmEndTimeSpan( context, kId, kFlags, kpNameFormat, ##__VA_ARGS__ ) )


//void tmBeginTimeSpanAt( HTELEMETRY cx,TmU64 const kId,TmU32 const kFlags,TmU64 const kTimeStamp,char const * kpNameFormat,...  );
//
//Description
//	Start the beginning of a timespan but at an explicit time.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kId [in] a user defined identifier for the timespan. Should be > 0.
//	kFlags [in] flags for the timespan. Reserved, must be 0.
//	kTimeStamp [in] explicit timestamp (e.g. from tmFastTime)
//	kpNameFormat [in] information about this timespan. This may contain printf-style format specifiers.
#define TM_BEGIN_TIME_SPAN_AT( context, kId, kFlags, kTimeStamp, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmBeginTimeSpanAt( context, kId, kFlags, kTimeStamp, kpNameFormat, ##__VA_ARGS__ ) )


//void tmEndTimeSpanAt( HTELEMETRY cx,TmU64 const kId,TmU32 const kFlags,TmU64 const kTimeStamp,char const * kpNameFormat,...  );
//
//Description
//	Mark the end a timespan, but with an explicit timestamp.
//
//Parameters
//	cx [in] handle to a valid Telemetry context 
//	kId [in] a user defined identifier for the timespan. Should be the same as the identifier passed to tmBeginTimeSpan.
//	kFlags [in] flags for the timespan. Reserved, must be 0.
//	kTimeStamp [in] a user specified timestamp (e.g. from tmFastTime.
//	kpNameFormat [in] information about this timespan. This may contain printf-style format specifiers.
#define TM_END_TIME_SPAN_AT( context, kId, kFlags, kTimeStamp, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmEndTimeSpanAt( context, kId, kFlags, kTimeStamp, kpNameFormat, ##__VA_ARGS__ ) )


//void tmSignalLockCount( HTELEMETRY cx,void const * kPtr,TmU32 const kCount,char const * kpDescription,...  );
//
//Description
//	Signal that a semaphore's lock count has been incremented.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kPtr [in] pointer to the being whose count is being incremented (e.g. HANDLE)
//	kCount [in] count value
//	kpDescription [in] description of the event. This may contain printf-style format specifiers.
#define TM_SIGNAL_LOCK_COUNT( context, kPtr, kCount, kpDescription, ... ) TELEMETRY_REQUIRED( tmSignalLockCount( context, kPtr, kCount, kpDescription, ##__VA_ARGS__ ) )


//void tmSetLockState( HTELEMETRY cx,void const * kPtr,TmLockState const kState,char const * kpDescription,... );
//
//Description
//	Sets the state of a lock (mutex, critical section, semaphore, etc.)
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kPtr [in] identifier for the lock, typically its address or HANDLE
//	kState [in] new state of the lock
//	kpDescription [in] description of the event. This may contain printf-style format specifiers.
#define TM_SET_LOCK_STATE( context, kPtr, kState, kpDescription, ... ) TELEMETRY_REQUIRED( tmSetLockState( context, kPtr, kState, kpDescription, ##__VA_ARGS__ ) )


//void tmSetLockStateEx( HTELEMETRY cx,char const * kpFilename,int const kLine,void const * kPtr,TmLockState const kState );
//
//Description
//	Set the state of a lock but with explicit file and line information.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpFilename [in] filename where the state change occurs
//	kLine [in] line number of the state change
//	kPtr [in] identifier for the lock, typically its address or HANDLE
//	kState [in] new state of the lock
//	kpDescription [in] description of the event. This may contain printf-style format specifiers.
#define TM_SET_LOCK_STATE_EX( context, kpFileName, kLine, kPtr, kState, kpDescription, ... ) TELEMETRY_REQUIRED( tmSetLockStateEx( context, kpFileName, kLine, kPtr, kState, kpDescription, ##__VA_ARGS__ ) )


//void tmSetLockStateMinTime( HTELEMETRY cx,void * buf,void const * kPtr,TmLockState const kState,char const * fmt,...  );
//
//Description
//	Set the state of a lock but with a minimum time threshold.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	buf [in] a user buffer used to store the lock event. Must be at least TM_LOCK_MIN_TIME_BUFSIZE in size.
//	kPtr [in] identifier for the lock, typically its address or HANDLE
//	kState [in] new state of the lock
//	kpDescription [in] description of the event. This may contain printf-style format specifiers.
#define TM_SET_LOCK_STATE_MIN_TIME( context, buf, kPtr, kState, kpDescription, ... ) TELEMETRY_REQUIRED( tmSetLockStateMinTime( context, buf, kPtr, kState, kpDescription, ##__VA_ARGS__ ) )


//void tmSetLockStateMinTimeEx( HTELEMETRY cx,void * buf,char const * kpFilename,int const kLine,void const * kPtr,TmLockState const kState );
//
//Description
//	This is the 'Ex' version of tmSetLockStateMinTimeEx. Please refer to tmSetLockStateEx and tmSetLockStateMinTime for more information.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	buf [in] a user buffer used to store the lock event. Must be at least TM_LOCK_MIN_TIME_BUFSIZE in size.
//	kpFilename [in] filename where the state change occurs
//	kLine [in] line number of the state change
//	kPtr [in] identifier for the lock, typically its address or HANDLE
//	kState [in] new state of the lock
//	kpDescription [in] description of the event. This may contain printf-style format specifiers.
#define TM_SET_LOCK_STATE_MIN_TIME_EX( context, buf, kpFilename, kLine, kPtr, kState, kpDescription, ... ) TELEMETRY_REQUIRED( tmSetLockStateMinTimeEx( context, buf, kpFilename, kLine, kPtr, kState, kpDescription, ##__VA_ARGS__ ) )


//void tmThreadName( HTELEMETRY cx,TmU32 const kThreadID,char const * kpNameFormat,...  );
//
//Description
//	Sets the name of the specified thread.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kThreadID [in] thread identifier of the thread to name. Use 0 to specify the current thread.
//	kpNameFormat [in] name of the thread. This may contain printf-style format specifiers.
#define TM_THREAD_NAME( context, kThreadID, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmThreadName( context, kThreadID, kpNameFormat, ##__VA_ARGS__ ) )


//void tmLockName( HTELEMETRY cx,void const* kPtr,char const * kpNameFormat,...  );
//
//Description
//	Sets the name of the specified locking object.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kPtr [in] identifier for the lock. Usually you can supply a pointer/HANDLE.
//	kpNameFormat [in] name of the lock. This may contain printf-style format specifiers.
#define TM_LOCK_NAME( context, kPtr, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmLockName( context, kPtr, kpNameFormat, ##__VA_ARGS__ ) )


//void tmEmitAccumulationZone( HTELEMETRY cx,TmU32 const kZoneFlags,TmU64 * pStart,int const kCount,TmU64 const kTotal,char const * kpZoneFormat,...  );
//
//Description
//	Emits an accumulation zone.
//
//Parameters
//	cx [in] handle to a valid telemetry context
//	kZoneFlags [in] reserved, must be 0
//	pStart [out] pointer to start time variable retrieved with a prior call to tmGetAccumulationStart. This value is modified.
//	kCount [in] number of times the accumulation zone was called
//	kTotal [in] total amount of time spent in the accumulation zone
//	kpZoneFormat [in] name of the accumulation zone. This may contain printf-style format specifiers.
#define TM_EMIT_ACCUMULATION_ZONE( context, kZoneFlags, pStart, kCount, kTotal, kpZoneFormat, ... ) TELEMETRY_REQUIRED( tmEmitAccumulationZone( context, kZoneFlags, pStart, kCount, kTotal, kpZoneFormat, ##__VA_ARGS__ ) )


//void tmSetVariable( HTELEMETRY cx,char const * kpKey,char const * kpValueFormat,...  );
//
//Description
//	Binds a string to a key name and sends to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpKey [in] pointer to string identify the key being set
//	kpValueFormat [in] pointer to string to bind to the associated key. This may contain printf-style format specifiers.
#define TM_SET_VARIABLE( context, kpKey, kpValueFormat, ... ) TELEMETRY_REQUIRED( tmSetVariable( context, kpKey, kpValueFormat, ##__VA_ARGS__ ) )


//void tmSetTimelineSectionName( HTELEMETRY cx,char const * kpNameFormat,...  );
//
//Description
//	Changes the name of the global state.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpNameFormat [in] name of the current state. This may contain printf-style format specifiers.
#define TM_SET_TIMELINE_SECTION_NAME( context, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmSetTimelineSectionName( context, kpNameFormat, ##__VA_ARGS__ ) )


//void tmEnter( HTELEMETRY cx,TmU32 const kFlags,char const * kpZoneName,...  );
//
//Description
//	Notify Telemetry that a zone is being entered.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kFlags [in] Bitwise OR of TmZoneFlag
//	kpZoneName [in] name of the zone. This may contain printf-style format specifiers.
#define TM_ENTER( context, kFlags, kpZoneName, ... ) TELEMETRY_REQUIRED( tmEnter( context, kFlags, kpZoneName, ##__VA_ARGS__ ) )


//void tmEnterEx( HTELEMETRY cx,TmU64 * pMatchId,TmU32 const kThreadId,TmU64 const kThreshold,char const * kpFilename,int const kLine,TmU32 const kFlags,char const * kpZoneName,...  );
//
//Description
//	Notify Telemetry that a zone is being entered, but with explicit file and line information.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	pMatchId [in] pointer variable to assign a match ID, pass NULL if you are not Zones: Runtime Zone Filtering by Duration.
//	kThreadId [in] thread ID for this zone, pass 0 for current thread
//	kThreshold [in] microseconds that the zone must span before being sent to server.
//	kpFilename [in] filename
//	kLine [in] line number
//	kFlags [in] Bitwise OR of TmZoneFlag
//	kpZoneName [in] name of the zone. This may contain printf-style format specifiers.
#define TM_ENTER_EX( context, pMatchId, kThreadId, kThreshold, kpFilename, kLine, kFlags, kpZoneName, ... ) TELEMETRY_REQUIRED( tmEnterEx( context, pMatchId, kThreadId, kThreshold, kpFilename, kLine, kFlags, kpZoneName, ##__VA_ARGS__ ) )


//void tmAlloc( HTELEMETRY cx,void const * kPtr,TmU64 const kSize,char const * kpDescription,...  );
//
//Description
//	Notify Telemetry that memory has been allocated.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kPtr [in] address of the memory that has been allocated
//	kSize [in] size of memory that has been allocated
//	kpDescription [in] textual description of the allocation, e.g. "vertex array" or "texture data". This may contain printf-style format specifiers.
#define TM_ALLOC( context, kPtr, kSize, kpDescription, ... ) TELEMETRY_REQUIRED( tmAlloc( context, kPtr, kSize, kpDescription, ##__VA_ARGS__ ) )


//void tmAllocEx( HTELEMETRY cx,char const * kpFilename,int const kLineNumber,void const * kPtr,TmU64 const kSize,char const * kpDescription,...  );
//
//Description
//	Notify Telemetry that memory has been allocated, but with explicit file and line information.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpFilename [in] name of the file
//	kLineNumber [in] line number of the allocation
//	kPtr [in] address of the memory that has been allocated
//	kSize [in] size of memory that has been allocated
//	kpDescription [in] textual description of the allocation, e.g. "vertex array" or "texture data". This may contain printf-style format specifiers.
#define TM_ALLOC_EX( context, kpFilename, kLineNumber, kPtr, kSize, kpDescription, ... ) TELEMETRY_REQUIRED( tmAllocEx( context, kpFilename, kLineNumber, kPtr, kSize, kpDescription, ##__VA_ARGS__ ) )


//void tmMessage( HTELEMETRY cx,TmU32 const kFlags,char const * kpFormatString,...  );
//
//Description
//	Send a message to the server for later viewing in the Visualizer.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kFlags [in] Bitwise OR of TmMessageFlag
//	kpFormatString [in] message string (not to exceed TM_MAX_STRING in length). This may contain printf-style format specifiers. Note that the message may have a Telemetry object path in it so that it can be hierarchically displayed with other messages.
#define TM_MESSAGE( context, kFlags, kpFormatString, ... ) TELEMETRY_REQUIRED( tmMessage( context, kFlags, kpFormatString, ##__VA_ARGS__ ) )
#define TM_LOG( context, kpFormatString, ... ) TM_MESSAGE( context, TMMF_SEVERITY_LOG, kpFormatString, ##__VA_ARGS__ )
#define TM_WARNING( context, kpFormatString, ... ) TM_MESSAGE( context, TMMF_SEVERITY_WARNING, kpFormatString, ##__VA_ARGS__ )
#define TM_ERROR( context, kpFormatString, ... ) TM_MESSAGE( context, TMMF_SEVERITY_ERROR, kpFormatString, ##__VA_ARGS__ )



//void tmPlot( HTELEMETRY cx,TmPlotType const kType,float const kValue,char const * kpNameFormat,...  );
//
//Description
//	Sends a floating point plot value to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kType [in] type of the plot (used to specify how the Visualizer will format the plot's value).
//	kValue [in] the value of the plot at this moment in time
//	kpNameFormat [in] name of the plot. You can use Telemetry object paths to create plot hierarchies on the Visualizer's plot filter tree. You can also specify plot scaling groups by appending group names in parenthesis to your plot's name, e.g. "renderer/memory/meshes(rendermem)" and "renderer/memory/textures(rendermem)" would be scaled using the same min/max values. This may contain printf-style format specifiers.
#define TM_PLOT( context, kType, kFlags, kValue, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmPlot( context, kType, kFlags, kValue, kpNameFormat, ##__VA_ARGS__ ) )


//Identical to tmPlot()
//void tmPlotF32( HTELEMETRY cx,TmPlotType const kType,float const kValue,char const * kpNameFormat,...  );
//
//Description
//	Sends a floating point plot value to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kType [in] type of the plot (used to specify how the Visualizer will format the plot's value).
//	kValue [in] the value of the plot at this moment in time
//	kpNameFormat [in] name of the plot. You can use Telemetry object paths to create plot hierarchies on the Visualizer's plot filter tree. You can also specify plot scaling groups by appending group names in parenthesis to your plot's name, e.g. "renderer/memory/meshes(rendermem)" and "renderer/memory/textures(rendermem)" would be scaled using the same min/max values. This may contain printf-style format specifiers.
#define TM_PLOT_F32( context, kType, kFlags, kValue, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmPlotF32( context, kType, kFlags, kValue, kpNameFormat, ##__VA_ARGS__ ) )


//void tmPlotF64( HTELEMETRY cx,TmPlotType const kType,double const kValue,char const * kpNameFormat,...  );
//
//Description
//	Sends a double precision floating point plot value to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kType [in] type of the plot (used to specify how the Visualizer will format the plot's value).
//	kValue [in] the value of the plot at this moment in time
//	kpNameFormat [in] name of the plot. You can use Telemetry object paths to create plot hierarchies on the Visualizer's plot filter tree. You can also specify plot scaling groups by appending group names in parenthesis to your plot's name, e.g. "renderer/memory/meshes(rendermem)" and "renderer/memory/textures(rendermem)" would be scaled using the same min/max values. This may contain printf-style format specifiers.
#define TM_PLOT_F64( context, kType, kFlags, kValue, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmPlotF64( context, kType, kFlags, kValue, kpNameFormat, ##__VA_ARGS__ ) )


//void tmPlotI32( HTELEMETRY cx,TmPlotType const kType,TmI32 const kValue,char const * kpNameFormat,...  );
//
//Description
//	Sends an signed 32-bit value to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kType [in] type of the plot (used to specify how the Visualizer will format the plot's value).
//	kValue [in] the value of the plot at this moment in time
//	kpNameFormat [in] name of the plot. You can use Telemetry object paths to create plot hierarchies on the Visualizer's plot filter tree. You can also specify plot scaling groups by appending group names in parenthesis to your plot's name, e.g. "renderer/memory/meshes(rendermem)" and "renderer/memory/textures(rendermem)" would be scaled using the same min/max values. This may contain printf-style format specifiers.
#define TM_PLOT_I32( context, kType, kFlags, kValue, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmPlotI32( context, kType, kFlags, kValue, kpNameFormat, ##__VA_ARGS__ ) )


//void tmPlotU32( HTELEMETRY cx,TmPlotType const kType,TmU32 const kValue,char const * kpNameFormat,...  );
//
//Description
//	Sends an unsigned 32-bit value to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kType [in] type of the plot (used to specify how the Visualizer will format the plot's value).
//	kValue [in] the value of the plot at this moment in time
//	kpNameFormat [in] name of the plot. You can use Telemetry object paths to create plot hierarchies on the Visualizer's plot filter tree. You can also specify plot scaling groups by appending group names in parenthesis to your plot's name, e.g. "renderer/memory/meshes(rendermem)" and "renderer/memory/textures(rendermem)" would be scaled using the same min/max values. This may contain printf-style format specifiers.
#define TM_PLOT_U32( context, kType, kFlags, kValue, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmPlotU32( context, kType, kFlags, kValue, kpNameFormat, ##__VA_ARGS__ ) )


//void tmPlotI64( HTELEMETRY cx,TmPlotType const kType,TmI64 const kValue,char const * kpNameFormat,...  );
//
//Description
//	Sends an signed 64-bit value to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kType [in] type of the plot (used to specify how the Visualizer will format the plot's value).
//	kValue [in] the value of the plot at this moment in time
//	kpNameFormat [in] name of the plot. You can use Telemetry object paths to create plot hierarchies on the Visualizer's plot filter tree. You can also specify plot scaling groups by appending group names in parenthesis to your plot's name, e.g. "renderer/memory/meshes(rendermem)" and "renderer/memory/textures(rendermem)" would be scaled using the same min/max values. This may contain printf-style format specifiers.
#define TM_PLOT_I64( context, kType, kFlags, kValue, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmPlotI64( context, kType, kFlags, kValue, kpNameFormat, ##__VA_ARGS__ ) )


//void tmPlotU64( HTELEMETRY cx,TmPlotType const kType,TmU64 const kValue,char const * kpNameFormat,...  );
//
//Description
//	Sends an unsigned 64-bit value to the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kType [in] type of the plot (used to specify how the Visualizer will format the plot's value).
//	kValue [in] the value of the plot at this moment in time
//	kpNameFormat [in] name of the plot. You can use Telemetry object paths to create plot hierarchies on the Visualizer's plot filter tree. You can also specify plot scaling groups by appending group names in parenthesis to your plot's name, e.g. "renderer/memory/meshes(rendermem)" and "renderer/memory/textures(rendermem)" would be scaled using the same min/max values. This may contain printf-style format specifiers.
#define TM_PLOT_U64( context, kType, kFlags, kValue, kpNameFormat, ... ) TELEMETRY_REQUIRED( tmPlotU64( context, kType, kFlags, kValue, kpNameFormat, ##__VA_ARGS__ ) )


//void tmBlob( HTELEMETRY cx,void const * kpData,int const kDataSize,char const * kpPluginIdentifier,char const * kpBlobName,...  );
//
//Description
//	Sends an arbitrary application specified blob of binary data for storage on the server. 
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kpData [in] the actual blob data
//	kDataSize [in] size of kpData in bytes
//	kpPluginIdentifier [in] identifier of the plugin used to visualize this data (must match the plugin's internal identifier).
//	kpBlobName [in] the name of the blob. This may contain printf-style format specifiers.
#define TM_BLOB( context, kpData, kDataSize, kpPluginIdentifier, kpBlobName, ...) TELEMETRY_REQUIRED( tmBlob( context, kpData, kDataSize, kpPluginIdentifier, kpBlobName, ##__VA_ARGS__ ) )


//void tmDisjointBlob( HTELEMETRY cx,int const kNumPieces,void const ** kpData,int const * kDataSizes,char const* kpPluginIdentifier,char const * kpBlobName,...  );
//
//Description
//	Sends an arbitrary application specified blob of binary data for storage on the server.
//
//Parameters
//	cx [in] handle to a valid Telemetry context
//	kNumPieces [in] number of elements in kpData and kDataSizes
//	kpData [in] array of actual data blobs
//	kDataSizes [in] array of data blob sizes in bytes
//	kpPluginIdentifier [in] identifier of the plugin used to visualize this data (must match the plugin's internal identifier).
//	kpBlobName [in] the name of the blob. This may contain printf-style format specifiers.
#define TM_DISJOINT_BLOB( context, kNumPieces, kpData, kDataSizes, kpPluginIdentifier, kpBlobName, ... ) TELEMETRY_REQUIRED( tmDisjointBlob( context, kNumPieces, kpData, kDataSizes, kpPluginIdentifier, kpBlobName, ##__VA_ARGS__ ) )












#if !defined( RAD_TELEMETRY_ENABLED )

//
// Telemetry is disabled.
//

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

#define TelemetrySetLockName( _ctx, _location, _description ) 

#else

//
// Telemetry is enabled.
//

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
		TM_TRY_LOCK( TELEMETRY_LEVEL1, m_plocation, "%s", m_description );
	}
	~CTelemetryLock()
	{
		Unlocked();
	}
	void Locked()
	{
		TM_END_TRY_LOCK( TELEMETRY_LEVEL1, m_plocation, TMLR_SUCCESS );
		TM_SET_LOCK_STATE( TELEMETRY_LEVEL1, m_plocation, TMLS_LOCKED, "%s Locked", m_description );
	}
	void Unlocked()
	{
		if( m_plocation )
		{
			TM_SET_LOCK_STATE( TELEMETRY_LEVEL1, m_plocation, TMLS_RELEASED, "%s Released", m_description );
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
			TM_MESSAGE( TELEMETRY_LEVEL0, TMMF_ICON_NOTE | TMMF_SEVERITY_WARNING, "(dota/spike)%s %.2fms %t", m_message, time, tmSendCallStack( TELEMETRY_LEVEL0, 0 ) );
		}
	}

private:
	TmU64 time0;
	float m_threshold;
	const char *m_message;
};

#endif // RAD_TELEMETRY_ENABLED

#endif // VPROF_TELEMETRY_H

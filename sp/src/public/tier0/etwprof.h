//========= Copyright Valve Corporation, All rights reserved. ============//
//
// ETW (Event Tracing for Windows) profiling helpers.
// This allows easy insertion of Generic Event markers into ETW/xperf tracing
// which then aids in analyzing the traces and finding performance problems.
// The usage patterns are to use ETWBegin and ETWEnd (typically through the
// convenience class CETWScope) to bracket time-consuming operations. In addition
// ETWFrameMark marks the beginning of each frame, and ETWMark can be used to
// mark other notable events. More event types and providers can be added as needed.
// When recording xperf profiles add Valve-Main+Valve-FrameRate to the list of
// user-mode providers and be sure to register the providers with this sequence
// of commands:
//    xcopy /y game\bin\tier0.dll %temp%
//    wevtutil um src\tier0\ValveETWProvider.man
//    wevtutil im src\tier0\ValveETWProvider.man
//
//===============================================================================

#ifndef ETWPROF_H
#define ETWPROF_H
#if defined( COMPILER_MSVC )
#pragma once
#endif

#include "tier0/platform.h"

#ifdef	IS_WINDOWS_PC
// ETW support should be compiled in for all Windows PC platforms. It isn't
// supported on Windows XP but that is determined at run-time.
#define	ETW_MARKS_ENABLED
#endif

#ifdef	ETW_MARKS_ENABLED

// Insert a single event to mark a point in an ETW trace. The return value is a 64-bit
// time stamp.
PLATFORM_INTERFACE int64 ETWMark( const char *pMessage );
// Optionally do full printf formatting of the mark string. This will be more expensive,
// but only when tracing is enabled.
PLATFORM_INTERFACE void ETWMarkPrintf( PRINTF_FORMAT_STRING const char *pMessage, ... ) FMTFUNCTION( 1, 2 );
// Optionally specify one to four floats. They will show up in separate columns in
// summary tables to allow sorting and easier transfer to spreadsheets.
PLATFORM_INTERFACE void ETWMark1F( const char *pMessage, float data1 );
PLATFORM_INTERFACE void ETWMark2F( const char *pMessage, float data1, float data2 );
PLATFORM_INTERFACE void ETWMark3F( const char *pMessage, float data1, float data2, float data3 );
PLATFORM_INTERFACE void ETWMark4F( const char *pMessage, float data1, float data2, float data3, float data4 );
// Optionally specify one to four ints. They will show up in separate columns in
// summary tables to allow sorting and easier transfer to spreadsheets.
PLATFORM_INTERFACE void ETWMark1I( const char *pMessage, int data1 );
PLATFORM_INTERFACE void ETWMark2I( const char *pMessage, int data1, int data2 );
PLATFORM_INTERFACE void ETWMark3I( const char *pMessage, int data1, int data2, int data3 );
PLATFORM_INTERFACE void ETWMark4I( const char *pMessage, int data1, int data2, int data3, int data4 );
// Optionally specify one to two strings. They will show up in separate columns in
// summary tables to allow sorting and easier transfer to spreadsheets.
PLATFORM_INTERFACE void ETWMark1S( const char *pMessage, const char* data1 );
PLATFORM_INTERFACE void ETWMark2S( const char *pMessage, const char* data1, const char* data2 );

// Insert a begin event to mark the start of some work. The return value is a 64-bit
// time stamp which should be passed to the corresponding ETWEnd function.
PLATFORM_INTERFACE int64 ETWBegin( const char *pMessage );

// Insert a paired end event to mark the end of some work.
PLATFORM_INTERFACE int64 ETWEnd( const char *pMessage, int64 nStartTime );

// Mark the start of the next render frame. bIsServerProcess must be passed
// in consistently for a particular process.
PLATFORM_INTERFACE void ETWRenderFrameMark( bool bIsServerProcess );
// Mark the start of the next simulation frame. bIsServerProcess must be passed
// in consistently for a particular process.
PLATFORM_INTERFACE void ETWSimFrameMark( bool bIsServerProcess );
// Return the frame number recorded in the ETW trace -- useful for synchronizing
// other profile information to the ETW trace.
PLATFORM_INTERFACE int ETWGetRenderFrameNumber();

PLATFORM_INTERFACE void ETWMouseDown( int nWhichButton, int nX, int nY );
PLATFORM_INTERFACE void ETWMouseUp( int nWhichButton, int nX, int nY );
PLATFORM_INTERFACE void ETWMouseMove( int nX, int nY );
PLATFORM_INTERFACE void ETWMouseWheel( int nWheelDelta, int nX, int nY );
PLATFORM_INTERFACE void ETWKeyDown( int nScanCode, int nVirtualCode, const char *pChar );

PLATFORM_INTERFACE void ETWSendPacket( const char *pTo, int nWireSize, int nOutSequenceNR, int nOutSequenceNrAck );
PLATFORM_INTERFACE void ETWThrottled();
PLATFORM_INTERFACE void ETWReadPacket( const char *pFrom, int nWireSize, int nInSequenceNR, int nOutSequenceNRAck );

// This class calls the ETW Begin and End functions in order to insert a
// pair of events to bracket some work.
class CETWScope
{
public:
	CETWScope( const char *pMessage )
		: m_pMessage( pMessage )
	{
		m_nStartTime = ETWBegin( pMessage );
	}
	~CETWScope()
	{
		ETWEnd( m_pMessage, m_nStartTime );
	}
private:
	// Private and unimplemented to disable copying.
	CETWScope( const CETWScope& rhs );
	CETWScope& operator=( const CETWScope& rhs );

	const char* m_pMessage;
	int64 m_nStartTime;
};

#else

inline int64 ETWMark( const char* ) { return 0; }
inline void ETWMarkPrintf( const char *, ... ) { return; }
inline void ETWMark1F( const char *, float ) { }
inline void ETWMark2F( const char *, float , float ) { }
inline void ETWMark3F( const char *, float , float , float ) { }
inline void ETWMark4F( const char *, float , float , float , float ) { }
inline void ETWMark1I( const char *, int ) { }
inline void ETWMark2I( const char *, int , int ) { }
inline void ETWMark3I( const char *, int , int , int ) { }
inline void ETWMark4I( const char *, int , int , int , int ) { }
// Optionally specify one to two strings. They will show up in separate columns in
// summary tables to allow sorting and easier transfer to spreadsheets.
inline void ETWMark1S( const char *, const char* ) { }
inline void ETWMark2S( const char *, const char* , const char* ) { }

inline int64 ETWBegin( const char* ) { return 0; }
inline int64 ETWEnd( const char*, int64 ) { return 0; }
inline void ETWRenderFrameMark( bool  ) {}
inline void ETWSimFrameMark( bool  ) {}
inline int ETWGetRenderFrameNumber() { return 0; }

inline void ETWMouseDown( int nWhichButton, int nX, int nY ) {}
inline void ETWMouseUp( int nWhichButton, int nX, int nY ) {}
inline void ETWMouseMove( int nX, int nY ) {}
inline void ETWMouseWheel( int nWheelDelta, int nX, int nY ) {}
inline void ETWKeyDown( int nScanCode, int nVirtualCode, const char *pChar ) {}

inline void ETWSendPacket( const char *pTo, int nWireSize, int nOutSequenceNR, int nOutSequenceNrAck ) {}
inline void ETWThrottled() {}
inline void ETWReadPacket( const char *pFrom, int nWireSize, int nInSequenceNR, int nOutSequenceNRAck ) {}

// This class calls the ETW Begin and End functions in order to insert a
// pair of events to bracket some work.
class CETWScope
{
public:
	CETWScope( const char* )
	{
	}
private:
	// Private and unimplemented to disable copying.
	CETWScope( const CETWScope& rhs );
	CETWScope& operator=( const CETWScope& rhs );
};

#endif

#endif // ETWPROF_H

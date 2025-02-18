//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:  
//
// $NoKeywords: $
//
//=============================================================================//
#ifndef DBG_H
#define DBG_H

#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "dbgflag.h"
#include "platform.h"
#include <math.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef POSIX
#define __cdecl
#endif

//-----------------------------------------------------------------------------
// dll export stuff
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define DBG_INTERFACE	DLL_EXPORT
#define DBG_OVERLOAD	DLL_GLOBAL_EXPORT
#define DBG_CLASS		DLL_CLASS_EXPORT
#else
#define DBG_INTERFACE	DLL_IMPORT
#define DBG_OVERLOAD	DLL_GLOBAL_IMPORT
#define DBG_CLASS		DLL_CLASS_IMPORT
#endif

#else // BUILD_AS_DLL

#define DBG_INTERFACE	extern
#define DBG_OVERLOAD	
#define DBG_CLASS		
#endif // BUILD_AS_DLL


class Color;


//-----------------------------------------------------------------------------
// Usage model for the Dbg library
//
// 1. Spew.
// 
//   Spew can be used in a static and a dynamic mode. The static
//   mode allows us to display assertions and other messages either only
//   in debug builds, or in non-release builds. The dynamic mode allows us to
//   turn on and off certain spew messages while the application is running.
// 
//   Static Spew messages:
//
//     Assertions are used to detect and warn about invalid states
//     Spews are used to display a particular status/warning message.
//
//     To use an assertion, use
//
//     Assert( (f == 5) );
//     AssertMsg( (f == 5), ("F needs to be %d here!\n", 5) );
//     AssertFunc( (f == 5), BadFunc() );
//     AssertEquals( f, 5 );
//     AssertFloatEquals( f, 5.0f, 1e-3 );
//
//     The first will simply report that an assertion failed on a particular
//     code file and line. The second version will display a print-f formatted message 
//	   along with the file and line, the third will display a generic message and
//     will also cause the function BadFunc to be executed, and the last two
//	   will report an error if f is not equal to 5 (the last one asserts within
//	   a particular tolerance).
//
//     To use a warning, use
//      
//     Warning("Oh I feel so %s all over\n", "yummy");
//
//     Warning will do its magic in only Debug builds. To perform spew in *all*
//     builds, use RelWarning.
//
//	   Three other spew types, Msg, Log, and Error, are compiled into all builds.
//	   These error types do *not* need two sets of parenthesis.
//
//	   Msg( "Isn't this exciting %d?", 5 );
//	   Error( "I'm just thrilled" );
//
//   Dynamic Spew messages
//
//     It is possible to dynamically turn spew on and off. Dynamic spew is 
//     identified by a spew group and priority level. To turn spew on for a 
//     particular spew group, use SpewActivate( "group", level ). This will 
//     cause all spew in that particular group with priority levels <= the 
//     level specified in the SpewActivate function to be printed. Use DSpew 
//     to perform the spew:
//
//     DWarning( "group", level, "Oh I feel even yummier!\n" );
//
//     Priority level 0 means that the spew will *always* be printed, and group
//     '*' is the default spew group. If a DWarning is encountered using a group 
//     whose priority has not been set, it will use the priority of the default 
//     group. The priority of the default group is initially set to 0.      
//
//   Spew output
//   
//     The output of the spew system can be redirected to an externally-supplied
//     function which is responsible for outputting the spew. By default, the 
//     spew is simply printed using printf.
//
//     To redirect spew output, call SpewOutput.
//
//     SpewOutputFunc( OutputFunc );
//
//     This will cause OutputFunc to be called every time a spew message is
//     generated. OutputFunc will be passed a spew type and a message to print.
//     It must return a value indicating whether the debugger should be invoked,
//     whether the program should continue running, or whether the program 
//     should abort. 
//
// 2. Code activation
//
//   To cause code to be run only in debug builds, use DBG_CODE:
//   An example is below.
//
//   DBG_CODE(
//				{
//					int x = 5;
//					++x;
//				}
//           ); 
//
//   Code can be activated based on the dynamic spew groups also. Use
//  
//   DBG_DCODE( "group", level,
//              { int x = 5; ++x; }
//            );
//
// 3. Breaking into the debugger.
//
//   To cause an unconditional break into the debugger in debug builds only, use DBG_BREAK
//
//   DBG_BREAK();
//
//	 You can force a break in any build (release or debug) using
//
//	 DebuggerBreak();
//-----------------------------------------------------------------------------

/* Various types of spew messages */
// I'm sure you're asking yourself why SPEW_ instead of DBG_ ?
// It's because DBG_ is used all over the place in windows.h
// For example, DBG_CONTINUE is defined. Feh.
enum SpewType_t
{
	SPEW_MESSAGE = 0,
	SPEW_WARNING,
	SPEW_ASSERT,
	SPEW_ERROR,
	SPEW_LOG,

	SPEW_TYPE_COUNT
};

enum SpewRetval_t
{
	SPEW_DEBUGGER = 0,
	SPEW_CONTINUE,
	SPEW_ABORT
};

/* type of externally defined function used to display debug spew */
typedef SpewRetval_t (*SpewOutputFunc_t)( SpewType_t spewType, const tchar *pMsg );

/* Used to redirect spew output */
DBG_INTERFACE void   SpewOutputFunc( SpewOutputFunc_t func );

/* Used to get the current spew output function */
DBG_INTERFACE SpewOutputFunc_t GetSpewOutputFunc( void );

/* This is the default spew fun, which is used if you don't specify one */
DBG_INTERFACE SpewRetval_t DefaultSpewFunc( SpewType_t type, const tchar *pMsg );

/* Same as the default spew func, but returns SPEW_ABORT for asserts */
DBG_INTERFACE SpewRetval_t DefaultSpewFuncAbortOnAsserts( SpewType_t type, const tchar *pMsg );

/* Should be called only inside a SpewOutputFunc_t, returns groupname, level, color */
DBG_INTERFACE const tchar* GetSpewOutputGroup( void );
DBG_INTERFACE int GetSpewOutputLevel( void );
DBG_INTERFACE const Color* GetSpewOutputColor( void );

/* Used to manage spew groups and subgroups */
DBG_INTERFACE void   SpewActivate( const tchar* pGroupName, int level );
DBG_INTERFACE bool   IsSpewActive( const tchar* pGroupName, int level );

/* Used to display messages, should never be called directly. */
DBG_INTERFACE void   _SpewInfo( SpewType_t type, const tchar* pFile, int line );
DBG_INTERFACE SpewRetval_t   _SpewMessage( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE SpewRetval_t   _DSpewMessage( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE SpewRetval_t   ColorSpewMessage( SpewType_t type, const Color *pColor, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE SpewRetval_t   ColorSpewMessage2( SpewType_t type, const Color &color, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void _ExitOnFatalAssert( const tchar* pFile, int line );
DBG_INTERFACE bool ShouldUseNewAssertDialog();

DBG_INTERFACE bool SetupWin32ConsoleIO();

// Returns true if they want to break in the debugger.
DBG_INTERFACE bool DoNewAssertDialog( const tchar *pFile, int line, const tchar *pExpression );

// Allows the assert dialogs to be turned off from code
DBG_INTERFACE bool AreAllAssertsDisabled();
DBG_INTERFACE void SetAllAssertsDisabled( bool bAssertsEnabled );

// Provides a callback that is called on asserts regardless of spew levels
typedef void (*AssertFailedNotifyFunc_t)( const char *pchFile, int nLine, const char *pchMessage );
DBG_INTERFACE void SetAssertFailedNotifyFunc( AssertFailedNotifyFunc_t func );
DBG_INTERFACE void CallAssertFailedNotifyFunc( const char *pchFile, int nLine, const char *pchMessage );

/* True if -hushasserts was passed on command line. */
DBG_INTERFACE bool HushAsserts();

#if defined( USE_SDL )
DBG_INTERFACE void SetAssertDialogParent( struct SDL_Window *window );
DBG_INTERFACE struct SDL_Window * GetAssertDialogParent();
#endif

/* Used to define macros, never use these directly. */

#ifdef _PREFAST_
	// When doing /analyze builds define _AssertMsg to be __analysis_assume. This tells
	// the compiler to assume that the condition is true, which helps to suppress many
	// warnings. This define is done in debug and release builds.
	// The unfortunate !! is necessary because otherwise /analyze is incapable of evaluating
	// all of the logical expressions that the regular compiler can handle.
	// Include _msg in the macro so that format errors in it are detected.
	#define _AssertMsg( _exp, _msg, _executeExp, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
	#define  _AssertMsgOnce( _exp, _msg, _bFatal ) do { __analysis_assume( !!(_exp) ); _msg; } while (0)
	// Force asserts on for /analyze so that we get a __analysis_assume of all of the constraints.
	#define DBGFLAG_ASSERT
	#define DBGFLAG_ASSERTFATAL
	#define DBGFLAG_ASSERTDEBUG
#else
	#define  _AssertMsg( _exp, _msg, _executeExp, _bFatal )	\
		do {																\
			if (!(_exp)) 													\
			{ 																\
				_SpewInfo( SPEW_ASSERT, __TFILE__, __LINE__ );				\
				SpewRetval_t retAssert = _SpewMessage("%s", static_cast<const char*>( _msg ));	\
				CallAssertFailedNotifyFunc( __TFILE__, __LINE__, _msg );					\
				_executeExp; 												\
				if ( retAssert == SPEW_DEBUGGER)									\
				{															\
					if ( !ShouldUseNewAssertDialog() || DoNewAssertDialog( __TFILE__, __LINE__, _msg ) ) \
					{														\
						DebuggerBreak();									\
					}														\
					if ( _bFatal )											\
					{														\
						_ExitOnFatalAssert( __TFILE__, __LINE__ );			\
					}														\
				}															\
			}																\
		} while (0)

	#define  _AssertMsgOnce( _exp, _msg, _bFatal ) \
		do {																\
			static bool fAsserted;											\
			if (!fAsserted )												\
			{ 																\
				_AssertMsg( _exp, _msg, (fAsserted = true), _bFatal );		\
			}																\
		} while (0)
#endif

/* Spew macros... */

// AssertFatal macros
// AssertFatal is used to detect an unrecoverable error condition.
// If enabled, it may display an assert dialog (if DBGFLAG_ASSERTDLG is turned on or running under the debugger),
// and always terminates the application

#ifdef DBGFLAG_ASSERTFATAL

#define  AssertFatal( _exp )									_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), true )
#define  AssertFatalOnce( _exp )								_AssertMsgOnce( _exp, _T("Assertion Failed: ") _T(#_exp), true )
#define  AssertFatalMsg( _exp, _msg, ... )						_AssertMsg( _exp, (const tchar *)CDbgFmtMsg( _msg, ##__VA_ARGS__ ), ((void)0), true )
#define  AssertFatalMsgOnce( _exp, _msg )						_AssertMsgOnce( _exp, _msg, true )
#define  AssertFatalFunc( _exp, _f )							_AssertMsg( _exp, _T("Assertion Failed: " _T(#_exp), _f, true )
#define  AssertFatalEquals( _exp, _expectedValue )				AssertFatalMsg2( (_exp) == (_expectedValue), _T("Expected %d but got %d!"), (_expectedValue), (_exp) ) 
#define  AssertFatalFloatEquals( _exp, _expectedValue, _tol )   AssertFatalMsg2( fabs((_exp) - (_expectedValue)) <= (_tol), _T("Expected %f but got %f!"), (_expectedValue), (_exp) )
#define  VerifyFatal( _exp )									AssertFatal( _exp )
#define  VerifyEqualsFatal( _exp, _expectedValue )				AssertFatalEquals( _exp, _expectedValue )

#define  AssertFatalMsg1( _exp, _msg, a1 )									AssertFatalMsg( _exp, _msg, a1 )
#define  AssertFatalMsg2( _exp, _msg, a1, a2 )								AssertFatalMsg( _exp, _msg, a1, a2 )
#define  AssertFatalMsg3( _exp, _msg, a1, a2, a3 )							AssertFatalMsg( _exp, _msg, a1, a2, a3 )
#define  AssertFatalMsg4( _exp, _msg, a1, a2, a3, a4 )						AssertFatalMsg( _exp, _msg, a1, a2, a3, a4 )
#define  AssertFatalMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5 )
#define  AssertFatalMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6 )
#define  AssertFatalMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )
#define  AssertFatalMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )
#define  AssertFatalMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	AssertFatalMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )

#else // DBGFLAG_ASSERTFATAL

#define  AssertFatal( _exp )									((void)0)
#define  AssertFatalOnce( _exp )								((void)0)
#define  AssertFatalMsg( _exp, _msg )							((void)0)
#define  AssertFatalMsgOnce( _exp, _msg )						((void)0)
#define  AssertFatalFunc( _exp, _f )							((void)0)
#define  AssertFatalEquals( _exp, _expectedValue )				((void)0)
#define  AssertFatalFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  VerifyFatal( _exp )									(_exp)
#define  VerifyEqualsFatal( _exp, _expectedValue )				(_exp)

#define  AssertFatalMsg1( _exp, _msg, a1 )									((void)0)
#define  AssertFatalMsg2( _exp, _msg, a1, a2 )								((void)0)
#define  AssertFatalMsg3( _exp, _msg, a1, a2, a3 )							((void)0)
#define  AssertFatalMsg4( _exp, _msg, a1, a2, a3, a4 )						((void)0)
#define  AssertFatalMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					((void)0)
#define  AssertFatalMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertFatalMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			((void)0)
#define  AssertFatalMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		((void)0)
#define  AssertFatalMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	((void)0)

#endif // DBGFLAG_ASSERTFATAL

// Assert macros
// Assert is used to detect an important but survivable error.
// It's only turned on when DBGFLAG_ASSERT is true.

#ifdef DBGFLAG_ASSERT

#define  Assert( _exp )           							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), false )
#define  AssertMsg( _exp, _msg, ... )  						_AssertMsg( _exp, (const tchar *)CDbgFmtMsg( _msg, ##__VA_ARGS__ ), ((void)0), false )
#define  AssertOnce( _exp )       							_AssertMsgOnce( _exp, _T("Assertion Failed: ") _T(#_exp), false )
#define  AssertMsgOnce( _exp, _msg )  						_AssertMsgOnce( _exp, _msg, false )
#define  AssertFunc( _exp, _f )   							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), _f, false )
#define  AssertEquals( _exp, _expectedValue )              	AssertMsg2( (_exp) == (_expectedValue), _T("Expected %d but got %d!"), (_expectedValue), (_exp) ) 
#define  AssertFloatEquals( _exp, _expectedValue, _tol )  	AssertMsg2( fabs((_exp) - (_expectedValue)) <= (_tol), _T("Expected %f but got %f!"), (_expectedValue), (_exp) )
#define  Verify( _exp )           							Assert( _exp )
#define  VerifyMsg1( _exp, _msg, a1 )						AssertMsg1( _exp, _msg, a1 )
#define	 VerifyMsg2( _exp, _msg, a1, a2 )					AssertMsg2( _exp, _msg, a1, a2 )
#define	 VerifyMsg3( _exp, _msg, a1, a2, a3 )				AssertMsg3( _exp, _msg, a1, a2, a3 )
#define  VerifyEquals( _exp, _expectedValue )           	AssertEquals( _exp, _expectedValue )
#define  DbgVerify( _exp )           						Assert( _exp )

#define  AssertMsg1( _exp, _msg, a1 )									AssertMsg( _exp, _msg, a1 )
#define  AssertMsg2( _exp, _msg, a1, a2 )								AssertMsg( _exp, _msg, a1, a2 )
#define  AssertMsg3( _exp, _msg, a1, a2, a3 )							AssertMsg( _exp, _msg, a1, a2, a3 )
#define  AssertMsg4( _exp, _msg, a1, a2, a3, a4 )						AssertMsg( _exp, _msg, a1, a2, a3, a4 )
#define  AssertMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					AssertMsg( _exp, _msg, a1, a2, a3, a4, a5 )
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6 )
#define  AssertMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )
#define  AssertMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )
#define  AssertMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	AssertMsg( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )

#else // DBGFLAG_ASSERT

#define  Assert( _exp )										((void)0)
#define  AssertOnce( _exp )									((void)0)
#define  AssertMsg( _exp, _msg, ... )						((void)0)
#define  AssertMsgOnce( _exp, _msg )						((void)0)
#define  AssertFunc( _exp, _f )								((void)0)
#define  AssertEquals( _exp, _expectedValue )				((void)0)
#define  AssertFloatEquals( _exp, _expectedValue, _tol )	((void)0)
#define  Verify( _exp )										(_exp)
#define	 VerifyMsg1( _exp, _msg, a1 )						(_exp)
#define	 VerifyMsg2( _exp, _msg, a1, a2 )					(_exp)
#define	 VerifyMsg3( _exp, _msg, a1, a2, a3 )				(_exp)
#define  VerifyEquals( _exp, _expectedValue )           	(_exp)
#define  DbgVerify( _exp )									(_exp)

#define  AssertMsg1( _exp, _msg, a1 )									((void)0)
#define  AssertMsg2( _exp, _msg, a1, a2 )								((void)0)
#define  AssertMsg3( _exp, _msg, a1, a2, a3 )							((void)0)
#define  AssertMsg4( _exp, _msg, a1, a2, a3, a4 )						((void)0)
#define  AssertMsg5( _exp, _msg, a1, a2, a3, a4, a5 )					((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg6( _exp, _msg, a1, a2, a3, a4, a5, a6 )				((void)0)
#define  AssertMsg7( _exp, _msg, a1, a2, a3, a4, a5, a6, a7 )			((void)0)
#define  AssertMsg8( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8 )		((void)0)
#define  AssertMsg9( _exp, _msg, a1, a2, a3, a4, a5, a6, a7, a8, a9 )	((void)0)

#endif // DBGFLAG_ASSERT

// The Always version of the assert macros are defined even when DBGFLAG_ASSERT is not, 
// so they will be available even in release.
#define  AssertAlways( _exp )           							_AssertMsg( _exp, _T("Assertion Failed: ") _T(#_exp), ((void)0), false )
#define  AssertMsgAlways( _exp, _msg )  							_AssertMsg( _exp, _msg, ((void)0), false )

// Stringify a number
#define V_STRINGIFY_INTERNAL(x) #x
// Extra level of indirection needed when passing in a macro to avoid getting the macro name instead of value
#define V_STRINGIFY(x) V_STRINGIFY_INTERNAL(x)

// Macros to help decorate warnings or errors with the location in code
#define FILE_LINE_FUNCTION_STRING __FILE__ "(" V_STRINGIFY(__LINE__) "):" __FUNCTION__ ":"
#define FILE_LINE_STRING __FILE__ "(" V_STRINGIFY(__LINE__) "):"
#define FUNCTION_LINE_STRING __FUNCTION__ "(" V_STRINGIFY(__LINE__) "): "

// Handy define for inserting clickable messages into the build output.
// Use like this:
// #pragma MESSAGE("Some message")
#define MESSAGE(msg) message(__FILE__ "(" V_STRINGIFY(__LINE__) "): " msg)


#if !defined( _X360 ) || !defined( _RETAIL )

/* These are always compiled in */
DBG_INTERFACE void Msg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void DMsg( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void MsgV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

DBG_INTERFACE void Warning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void DWarning( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void WarningV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

DBG_INTERFACE void Log( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void DLog( const tchar *pGroupName, int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void LogV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

#ifdef Error
// p4.cpp does a #define Error Warning and in that case the Error prototype needs to
// be consistent with the Warning prototype.
DBG_INTERFACE void Error( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
#else
DBG_INTERFACE void NORETURN Error( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void NORETURN ErrorV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist );

#endif

#else

inline void Msg( ... ) {}
inline void DMsg( ... ) {}
inline void MsgV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}
inline void Warning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) {}
inline void WarningV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}
inline void DWarning( ... ) {}
inline void Log( ... ) {}
inline void DLog( ... ) {}
inline void LogV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}
inline void Error( ... ) {}
inline void ErrorV( PRINTF_FORMAT_STRING const tchar *pMsg, va_list arglist ) {}

#endif

// You can use this macro like a runtime assert macro.
// If the condition fails, then Error is called with the message. This macro is called
// like AssertMsg, where msg must be enclosed in parenthesis:
//
// ErrorIfNot( bCondition, ("a b c %d %d %d", 1, 2, 3) );
#define ErrorIfNot( condition, msg ) \
	if ( condition )		\
		;					\
	else 					\
	{						\
		Error msg;			\
	}

#if !defined( _X360 ) || !defined( _RETAIL )

/* A couple of super-common dynamic spew messages, here for convenience */
/* These looked at the "developer" group */
DBG_INTERFACE void DevMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void DevWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void DevLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );

/* default level versions (level 1) */
DBG_OVERLOAD void DevMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void DevWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void DevLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );

/* These looked at the "console" group */
DBG_INTERFACE void ConColorMsg( int level, const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 3, 4 );
DBG_INTERFACE void ConMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void ConWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void ConLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );

/* default console version (level 1) */
DBG_OVERLOAD void ConColorMsg( const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_OVERLOAD void ConMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void ConWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_OVERLOAD void ConLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );

/* developer console version (level 2) */
DBG_INTERFACE void ConDColorMsg( const Color& clr, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void ConDMsg( PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void ConDWarning( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );
DBG_INTERFACE void ConDLog( PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 1, 2 );

/* These looked at the "network" group */
DBG_INTERFACE void NetMsg( int level, PRINTF_FORMAT_STRING const tchar* pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void NetWarning( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );
DBG_INTERFACE void NetLog( int level, PRINTF_FORMAT_STRING const tchar *pMsg, ... ) FMTFUNCTION( 2, 3 );

void ValidateSpew( class CValidator &validator );

#else

inline void DevMsg( ... ) {}
inline void DevWarning( ... ) {}
inline void DevLog( ... ) {}
inline void ConMsg( ... ) {}
inline void ConLog( ... ) {}
inline void NetMsg( ... ) {}
inline void NetWarning( ... ) {}
inline void NetLog( ... ) {}

#endif

DBG_INTERFACE void COM_TimestampedLog( PRINTF_FORMAT_STRING char const *fmt, ... ) FMTFUNCTION( 1, 2 );

/* Code macros, debugger interface */

#ifdef DBGFLAG_ASSERT

#define DBG_CODE( _code )            if (0) ; else { _code }
#define DBG_CODE_NOSCOPE( _code )	 _code
#define DBG_DCODE( _g, _l, _code )   if (IsSpewActive( _g, _l )) { _code } else {}
#define DBG_BREAK()                  DebuggerBreak()	/* defined in platform.h */ 

#else /* not _DEBUG */

#define DBG_CODE( _code )            ((void)0)
#define DBG_CODE_NOSCOPE( _code )	 
#define DBG_DCODE( _g, _l, _code )   ((void)0)
#define DBG_BREAK()                  ((void)0)

#endif /* _DEBUG */

//-----------------------------------------------------------------------------

#ifndef _RETAIL
class CScopeMsg
{
public:
	CScopeMsg( const char *pszScope )
	{
		m_pszScope = pszScope;
		Msg( "%s { ", pszScope );
	}
	~CScopeMsg()
	{
		Msg( "} %s", m_pszScope );
	}
	const char *m_pszScope;
};
#define SCOPE_MSG( msg ) CScopeMsg scopeMsg( msg )
#else
#define SCOPE_MSG( msg )
#endif

//-----------------------------------------------------------------------------
// Utilities to suppress warnings or other annotations

// Note a variable is possibly unused to avoid analyzer warnings
template< typename T > static FORCEINLINE void NoteUnused( const T& foo ) { return; }

// NOTE: On GCC / Clang, assert_cast can sometimes fire even if the type is correct. We should just workaround these.
// The situation where this would occur is 
// 1. You create an object of a low level type in a DLL, and it really gets created there.
// 2. You pass it across a DLL boundary
// 3. You use assert_cast to verify it in the second DLL boundary (where it also could've been created).
#ifdef _DEBUG
template<typename DEST_POINTER_TYPE, typename SOURCE_POINTER_TYPE>
inline DEST_POINTER_TYPE assert_cast(SOURCE_POINTER_TYPE* pSource)
{
    Assert( static_cast<DEST_POINTER_TYPE>(pSource) == dynamic_cast<DEST_POINTER_TYPE>(pSource) );
    return static_cast<DEST_POINTER_TYPE>(pSource);
}
#else
#define assert_cast static_cast
#endif

//-----------------------------------------------------------------------------
// Templates to assist in validating pointers:

// Have to use these stubs so we don't have to include windows.h here.

DBG_INTERFACE void _AssertValidReadPtr( void* ptr, int count = 1 );
DBG_INTERFACE void _AssertValidWritePtr( void* ptr, int count = 1 );
DBG_INTERFACE void _AssertValidReadWritePtr( void* ptr, int count = 1 );
DBG_INTERFACE void AssertValidStringPtr( const tchar* ptr, int maxchar = 0xFFFFFF );

#ifdef DBGFLAG_ASSERT

FORCEINLINE void AssertValidReadPtr( const void* ptr, int count = 1 )	    { _AssertValidReadPtr( (void*)ptr, count ); }
FORCEINLINE void AssertValidWritePtr( const void* ptr, int count = 1 )		{ _AssertValidWritePtr( (void*)ptr, count ); }
FORCEINLINE void AssertValidReadWritePtr( const void* ptr, int count = 1 )	{ _AssertValidReadWritePtr( (void*)ptr, count ); }

#else

FORCEINLINE void AssertValidReadPtr( const void* ptr, int count = 1 )			 { }
FORCEINLINE void AssertValidReadPtr( const void* ptr, size_t count )			 { }
FORCEINLINE void AssertValidWritePtr( const void* ptr, int count = 1 )		     { }
FORCEINLINE void AssertValidReadWritePtr( const void* ptr, int count = 1 )	     { }
#define AssertValidStringPtr AssertValidReadPtr

#endif

#define AssertValidThis() AssertValidReadWritePtr(this,sizeof(*this))

//-----------------------------------------------------------------------------
// Macro to protect functions that are not reentrant

#ifdef _DEBUG
class CReentryGuard
{
public:
	CReentryGuard(int *pSemaphore)
	 : m_pSemaphore(pSemaphore)
	{
		++(*m_pSemaphore);
	}
	
	~CReentryGuard()
	{
		--(*m_pSemaphore);
	}
	
private:
	int *m_pSemaphore;
};

#define ASSERT_NO_REENTRY() \
	static int fSemaphore##__LINE__; \
	Assert( !fSemaphore##__LINE__ ); \
	CReentryGuard ReentryGuard##__LINE__( &fSemaphore##__LINE__ )
#else
#define ASSERT_NO_REENTRY()
#endif

//-----------------------------------------------------------------------------
//
// Purpose: Inline string formatter
//

#include "tier0/valve_off.h"
class CDbgFmtMsg
{
public:
	CDbgFmtMsg(PRINTF_FORMAT_STRING const tchar *pszFormat, ...) FMTFUNCTION( 2, 3 )
	{ 
		va_list arg_ptr;

		va_start(arg_ptr, pszFormat);
		_vsntprintf(m_szBuf, sizeof(m_szBuf)-1, pszFormat, arg_ptr);
		va_end(arg_ptr);

		m_szBuf[sizeof(m_szBuf)-1] = 0;
	}

	operator const tchar *() const				
	{ 
		return m_szBuf; 
	}

private:
	tchar m_szBuf[256];
};
#include "tier0/valve_on.h"

//-----------------------------------------------------------------------------
//
// Purpose: Embed debug info in each file.
//
#if defined( _WIN32 ) && !defined( _X360 )

	#ifdef _DEBUG
		#pragma comment(compiler)
	#endif

#endif

//-----------------------------------------------------------------------------
//
// Purpose: Wrap around a variable to create a simple place to put a breakpoint
//

#ifdef _DEBUG

template< class Type >
class CDataWatcher
{
public:
	const Type& operator=( const Type &val ) 
	{ 
		return Set( val ); 
	}
	
	const Type& operator=( const CDataWatcher<Type> &val ) 
	{ 
		return Set( val.m_Value ); 
	}
	
	const Type& Set( const Type &val )
	{
		// Put your breakpoint here
		m_Value = val;
		return m_Value;
	}
	
	Type& GetForModify()
	{
		return m_Value;
	}
	
	const Type& operator+=( const Type &val ) 
	{
		return Set( m_Value + val ); 
	}
	
	const Type& operator-=( const Type &val ) 
	{
		return Set( m_Value - val ); 
	}
	
	const Type& operator/=( const Type &val ) 
	{
		return Set( m_Value / val ); 
	}
	
	const Type& operator*=( const Type &val ) 
	{
		return Set( m_Value * val ); 
	}
	
	const Type& operator^=( const Type &val ) 
	{
		return Set( m_Value ^ val ); 
	}
	
	const Type& operator|=( const Type &val ) 
	{
		return Set( m_Value | val ); 
	}
	
	const Type& operator++()
	{
		return (*this += 1);
	}
	
	Type operator--()
	{
		return (*this -= 1);
	}
	
	Type operator++( int ) // postfix version..
	{
		Type val = m_Value;
		(*this += 1);
		return val;
	}
	
	Type operator--( int ) // postfix version..
	{
		Type val = m_Value;
		(*this -= 1);
		return val;
	}
	
	// For some reason the compiler only generates type conversion warnings for this operator when used like 
	// CNetworkVarBase<unsigned tchar> = 0x1
	// (it warns about converting from an int to an unsigned char).
	template< class C >
	const Type& operator&=( C val ) 
	{ 
		return Set( m_Value & val ); 
	}
	
	operator const Type&() const 
	{
		return m_Value; 
	}
	
	const Type& Get() const 
	{
		return m_Value; 
	}
	
	const Type* operator->() const 
	{
		return &m_Value; 
	}
	
	Type m_Value;
	
};

#else

template< class Type >
class CDataWatcher
{
private:
	CDataWatcher(); // refuse to compile in non-debug builds
};

#endif

//-----------------------------------------------------------------------------


// This is horrible, but we don't want to integrate CS:GO new logging system atm.

#ifdef BUILDING_VPC
#define Log_Warning( ignore, ... )	::Warning( __VA_ARGS__ );
#define Log_Msg( ignore, ... )		::Msg( __VA_ARGS__ );
#define Log_Error( ignore, ... )		::Error( __VA_ARGS__ );
#else
#define Log_Warning( ignore, ... )	::Warning( __VA_ARGS__ ); ::Log( __VA_ARGS__ );
#define Log_Msg( ignore, ... )		::Msg( __VA_ARGS__ ); ::Log( __VA_ARGS__ );
#define Log_Error( ignore, ... )		::Error( __VA_ARGS__ );
#endif
#define Log_Warning_Color( ignore, ... )	::ColorSpewMessage2( SPEW_WARNING, __VA_ARGS__ );
#define Log_Msg_Color( ignore, ... )	::ColorSpewMessage2( SPEW_MESSAGE, __VA_ARGS__ );
#define Log_Error_Color( ignore, ... )	::ColorSpewMessage2( SPEW_ERROR, __VA_ARGS__ );
#define DECLARE_LOGGING_CHANNEL( ... );
#define DEFINE_LOGGING_CHANNEL_NO_TAGS( ... );
#define Plat_FatalError( ... ) do { Log_Error( LOG_GENERAL, __VA_ARGS__ ); Plat_ExitProcess( EXIT_FAILURE ); } while( 0 )

#endif /* DBG_H */

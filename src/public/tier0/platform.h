//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//
//===========================================================================//

#ifndef PLATFORM_H
#define PLATFORM_H

#if defined(__x86_64__) || defined(_WIN64)
#define PLATFORM_64BITS 1
#endif

#if defined(__GCC__) || defined(__GNUC__)
#define COMPILER_GCC 1
#endif

#ifdef __clang__
#define COMPILER_CLANG 1
#endif

#if defined( _X360 )
	#define NO_STEAM
	#define NO_VOICE
	// for the 360, the ppc platform and the rtos are tightly coupled
	// setup the 360 environment here !once! for much less leaf module include wackiness
	// these are critical order and purposely appear *before* anything else
	#define _XBOX
#include <xtl.h>
	#include <xaudio2.h>
	#include <xbdm.h>
#include <Xgraphics.h>
	#include <xui.h>
	#include <pmcpbsetup.h>
#include <XMAHardwareAbstraction.h>
	#undef _XBOX
#endif

#define __STDC_LIMIT_MACROS
#include <stdint.h>

#include "wchartypes.h"
#include "basetypes.h"
#include "tier0/valve_off.h"

//-----------------------------------------------------------------------------
// This macro predates universal static_assert support in our toolchains
#define COMPILE_TIME_ASSERT( pred ) static_assert( pred, "Compile time assert constraint is not true: " #pred )

// ASSERT_INVARIANT used to be needed in order to allow COMPILE_TIME_ASSERTs at global
// scope. However the new COMPILE_TIME_ASSERT macro supports that by default.
#define ASSERT_INVARIANT( pred )	COMPILE_TIME_ASSERT( pred )

#ifdef _WIN32
#pragma once
#endif

// feature enables
#define NEW_SOFTWARE_LIGHTING

#ifdef POSIX
// need this for _alloca
#include <alloca.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>
#endif

#include <malloc.h>
#include <new>

// need this for memset
#include <string.h>

#include "tier0/valve_minmax_on.h"	// GCC 4.2.2 headers screw up our min/max defs.

// Detect the architecture we are running on
#if defined(__arm__) || defined( __aarch64__ ) || defined(_M_ARM) || defined(_M_ARM64)
	#define PLATFORM_ARM 1
#elif  defined(_M_X64) || defined(__x86_64__)
  #define PLATFORM_INTEL
  #define PLATFORM_X86 64
#elif defined(_M_IX86) || defined(__i386__)
  #define PLATFORM_INTEL
  #define PLATFORM_X86 32
#else
	#error Unknown processor architecture.
#endif

#ifdef _RETAIL
#define IsRetail() true
#else
#define IsRetail() false
#endif

#if defined( DX_TO_GL_ABSTRACTION ) && defined( USE_DXVK )
#include <windows.h>
#endif

#ifdef _DEBUG
#define IsRelease() false
#define IsDebug() true
#else
#define IsRelease() true
#define IsDebug() false
#endif

// Deprecating, infavor of IsX360() which will revert to IsXbox()
// after confidence of xbox 1 code flush
#define IsXbox()	false

#ifdef _WIN32
	#define IsLinux() false
	#define IsOSX() false
	#define IsPosix() false
	#define PLATFORM_WINDOWS 1 // Windows PC or Xbox 360
	#ifndef _X360
		#define IsWindows() true
		#define IsPC() true
		#define IsConsole() false
		#define IsX360() false
		#define IsPS3() false
		#define IS_WINDOWS_PC 1
		#define PLATFORM_WINDOWS_PC 1 // Windows PC
		#ifdef _WIN64
			#define IsPlatformWindowsPC64() true
			#define IsPlatformWindowsPC32() false
			#define PLATFORM_WINDOWS_PC64 1
		#else
			#define IsPlatformWindowsPC64() false
			#define IsPlatformWindowsPC32() true
			#define PLATFORM_WINDOWS_PC32 1
		#endif
	#else
		#define PLATFORM_X360 1
		#ifndef _CONSOLE
			#define _CONSOLE
		#endif
		#define IsWindows() false
		#define IsPC() false
		#define IsConsole() true
		#define IsX360() true
		#define IsPS3() false
	#endif
#elif defined(POSIX)
	#define IsPC() true
	#define IsWindows() false
	#define IsConsole() false
	#define IsX360() false
	#define IsPS3() false
	#if defined( LINUX )
		#define IsLinux() true
	#else
		#define IsLinux() false
	#endif
	
	#if defined( OSX )
		#define IsOSX() true
	#else
		#define IsOSX() false
	#endif
	
	#define IsPosix() true
#else
	#error
#endif

#if PLATFORM_WINDOWS_PC

# if PLATFORM_64BITS
#  define PLATFORM_DIR "\\x64"
# else
#  define PLATFORM_DIR ""
# endif

//#elif PLATFORM_LINUX
#elif LINUX

# if PLATFORM_64BITS
#  define PLATFORM_DIR "/linux64"
# else
#  define PLATFORM_DIR ""
# endif

//#elif PLATFORM_OSX
#elif OSX

#if PLATFORM_ARM
#  define PLATFORM_DIR "/osxarm64"
#else
# if PLATFORM_64BITS
#  define PLATFORM_DIR "/osx64"
# else
#  define PLATFORM_DIR ""
# endif
#endif

#else
# error "Define a platform dir for me!"
#endif

#define PLATFORM_BIN_DIR "bin" PLATFORM_DIR

typedef unsigned char uint8;
typedef signed char int8;

#if defined( _WIN32 )

	typedef __int16					int16;
	typedef unsigned __int16		uint16;
	typedef __int32					int32;
	typedef unsigned __int32		uint32;
	typedef __int64					int64;
	typedef unsigned __int64		uint64;

	#ifdef PLATFORM_64BITS
		typedef __int64 intp;				// intp is an integer that can accomodate a pointer
		typedef unsigned __int64 uintp;		// (ie, sizeof(intp) >= sizeof(int) && sizeof(intp) >= sizeof(void *)
	#else
		typedef __int32 intp;
		typedef unsigned __int32 uintp;
	#endif

	#if defined( _X360 )
		#ifdef __m128
			#undef __m128
		#endif
		#define __m128				__vector4
	#endif

	// Use this to specify that a function is an override of a virtual function.
	// This lets the compiler catch cases where you meant to override a virtual
	// function but you accidentally changed the function signature and created
	// an overloaded function. Usage in function declarations is like this:
	// int GetData() const OVERRIDE;
	#define OVERRIDE override

#else // _WIN32

	typedef short					int16;
	typedef unsigned short			uint16;
	typedef int						int32;
	typedef unsigned int			uint32;
	typedef long long				int64;
	typedef unsigned long long		uint64;
	#ifdef PLATFORM_64BITS
		typedef long long			intp;
		typedef unsigned long long	uintp;
	#else
		typedef int					intp;
		typedef unsigned int		uintp;
	#endif
	#ifndef USE_DXVK
	typedef void *HWND;
	#endif

	// Avoid redefinition warnings if a previous header defines this.
	#undef OVERRIDE
	#if __cplusplus >= 201103L
		#define OVERRIDE override
		#if defined(__clang__)
			// warning: 'override' keyword is a C++11 extension [-Wc++11-extensions]
			// Disabling this warning is less intrusive than enabling C++11 extensions
			#pragma GCC diagnostic ignored "-Wc++11-extensions"
		#endif
	#else
		#define OVERRIDE
	#endif

    // [u]int64 are actually defined as 'long long' and gcc 64-bit
    // doesn't automatically consider them the same as 'long int'.
    // Changing the types for [u]int64 is complicated by
    // there being many definitions, so we just
    // define a 'long int' here and use it in places that would
    // otherwise confuse the compiler.
    typedef long int lint64;
    typedef unsigned long int ulint64;

	// include nullptr_t in global namespace
	typedef decltype( nullptr ) nullptr_t;

#endif // else _WIN32

//-----------------------------------------------------------------------------
// Set up platform type defines.
//-----------------------------------------------------------------------------
#if defined( PLATFORM_X360 ) || defined( _PS3 )
	#if !defined( _GAMECONSOLE )
		#define _GAMECONSOLE
	#endif
	#define IsPC()			false
	#define IsGameConsole()	true
#else
	#define IsPC()			true
	#define IsGameConsole()	false
#endif

#ifdef PLATFORM_64BITS
	#define IsPlatform64Bits()	true
#else
	#define IsPlatform64Bits()	false
#endif

// From steam/steamtypes.h
// RTime32
// We use this 32 bit time representing real world time.
// It offers 1 second resolution beginning on January 1, 1970 (Unix time)
typedef uint32 RTime32;

typedef float				float32;
typedef double				float64;

// for when we don't care about how many bits we use
typedef unsigned int		uint;

#ifdef _MSC_VER
#pragma once
// Ensure that everybody has the right compiler version installed. The version
// number can be obtained by looking at the compiler output when you type 'cl'
// and removing the last two digits and the periods: 16.00.40219.01 becomes 160040219
#if _MSC_FULL_VER > 180000000
	#if _MSC_FULL_VER < 180030723
		#error You must install VS 2013 Update 3
	#endif
#elif _MSC_FULL_VER > 160000000
	#if _MSC_FULL_VER < 160040219
		#error You must install VS 2010 SP1
	#endif
#else
	#if _MSC_FULL_VER < 140050727
		#error You must install VS 2005 SP1
	#endif
#endif
#endif

// This can be used to ensure the size of pointers to members when declaring
// a pointer type for a class that has only been forward declared
#ifdef _MSC_VER
#define SINGLE_INHERITANCE __single_inheritance
#define MULTIPLE_INHERITANCE __multiple_inheritance
#else
#define SINGLE_INHERITANCE
#define MULTIPLE_INHERITANCE
#endif

#ifdef _MSC_VER
#define NO_VTABLE __declspec( novtable )
#else
#define NO_VTABLE
#endif

#ifdef _MSC_VER
	// This indicates that a function never returns, which helps with
	// generating accurate compiler warnings
	#define NORETURN				__declspec( noreturn )
#else
	#define NORETURN
#endif

// This can be used to declare an abstract (interface only) class.
// Classes marked abstract should not be instantiated.  If they are, and access violation will occur.
//
// Example of use:
//
// abstract_class CFoo
// {
//      ...
// }
//
// MSDN __declspec(novtable) documentation: http://msdn.microsoft.com/library/default.asp?url=/library/en-us/vclang/html/_langref_novtable.asp
//
// Note: NJS: This is not enabled for regular PC, due to not knowing the implications of exporting a class with no no vtable.
//       It's probable that this shouldn't be an issue, but an experiment should be done to verify this.
//
#ifndef _X360
#define abstract_class class
#else
#define abstract_class class NO_VTABLE
#endif


// MSVC CRT uses 0x7fff while gcc uses MAX_INT, leading to mismatches between platforms
// As a result, we pick the least common denominator here.  This should be used anywhere
// you might typically want to use RAND_MAX
#define VALVE_RAND_MAX 0x7fff

/*
FIXME: Enable this when we no longer fear change =)

// need these for the limits
#include <limits.h>
#include <float.h>

// Maximum and minimum representable values
#define  INT8_MAX			SCHAR_MAX
#define  INT16_MAX			SHRT_MAX
#define  INT32_MAX			LONG_MAX
#define  INT64_MAX			(((int64)~0) >> 1)

#define  INT8_MIN			SCHAR_MIN
#define  INT16_MIN			SHRT_MIN
#define  INT32_MIN			LONG_MIN
#define  INT64_MIN			(((int64)1) << 63)

#define  UINT8_MAX			((uint8)~0)
#define  UINT16_MAX			((uint16)~0)
#define  UINT32_MAX			((uint32)~0)
#define  UINT64_MAX			((uint64)~0)

#define  UINT8_MIN			0
#define  UINT16_MIN			0
#define  UINT32_MIN			0
#define  UINT64_MIN			0

#ifndef  UINT_MIN
#define  UINT_MIN			UINT32_MIN
#endif

#define  FLOAT32_MAX		FLT_MAX
#define  FLOAT64_MAX		DBL_MAX

#define  FLOAT32_MIN FLT_MIN
#define  FLOAT64_MIN DBL_MIN
*/

// portability / compiler settings
#if defined(_WIN32) && !defined(WINDED)

#if defined(_M_IX86)
#define __i386__	1
#endif

#elif defined( POSIX ) && !defined( USE_DXVK )
#if defined( OSX ) && defined( CARBON_WORKAROUND )
#define DWORD unsigned int
#else
typedef unsigned int DWORD;
#endif
typedef unsigned short WORD;
typedef void * HINSTANCE;
#define _MAX_PATH PATH_MAX
#define __cdecl
#define __stdcall
#define __declspec

#endif // defined(_WIN32) && !defined(WINDED)

#ifdef POSIX
typedef unsigned int *LPDWORD;
#endif

#define MAX_FILEPATH 512 

// Defines MAX_PATH
#ifndef MAX_PATH
#define MAX_PATH  260
#endif

#ifdef _WIN32
#define MAX_UNICODE_PATH 32767
#else
#define MAX_UNICODE_PATH MAX_PATH
#endif

#define MAX_UNICODE_PATH_IN_UTF8 MAX_UNICODE_PATH*4

#if !defined( offsetof )
	#ifdef __GNUC__
		#define offsetof( type, var ) __builtin_offsetof( type, var )
	#else
		#define offsetof(s,m)	(size_t)&(((s *)0)->m)
	#endif
#endif // !defined( offsetof )


#define ALIGN_VALUE( val, alignment ) ( ( val + alignment - 1 ) & ~( alignment - 1 ) ) //  need macro for constant expression

// Used to step into the debugger
#if defined( _WIN32 ) && !defined( _X360 )
#define DebuggerBreak()  __debugbreak()
#elif defined( _X360 )
#define DebuggerBreak() DebugBreak()
#else
	// On OSX, SIGTRAP doesn't really stop the thread cold when debugging.
	// So if being debugged, use INT3 which is precise.
#ifdef OSX
#define DebuggerBreak()  if ( Plat_IsInDebugSession() ) { __asm ( "int $3" ); } else { raise(SIGTRAP); }
#else
#define DebuggerBreak()  raise(SIGTRAP)
#endif
#endif
#define	DebuggerBreakIfDebugging() if ( !Plat_IsInDebugSession() ) ; else DebuggerBreak()

#define	DebuggerBreakIfDebugging_StagingOnly()

// Allows you to specify code that should only execute if we are in a staging build. Otherwise the code noops.
#define STAGING_ONLY_EXEC( _exec ) do { } while (0)

// C functions for external declarations that call the appropriate C++ methods
#ifndef EXPORT
	#ifdef _WIN32
		#define EXPORT	_declspec( dllexport )
	#else
		#define EXPORT	/* */
	#endif
#endif

#if defined __i386__ && !defined __linux__
	#define id386	1
#else
	#define id386	0
#endif  // __i386__

// decls for aligning data
#ifdef _WIN32
        #define DECL_ALIGN(x) __declspec(align(x))

#elif GNUC
	#define DECL_ALIGN(x) __attribute__((aligned(x)))
#else
        #define DECL_ALIGN(x) /* */
#endif

#if defined( GNUC )
// gnuc has the align decoration at the end
#define ALIGN4
#define ALIGN8 
#define ALIGN16
#define ALIGN32
#define ALIGN128
#define ALIGN_N( _align_ )

#undef ALIGN16_POST
#define ALIGN4_POST DECL_ALIGN(4)
#define ALIGN8_POST DECL_ALIGN(8)
#define ALIGN16_POST DECL_ALIGN(16)
#define ALIGN32_POST DECL_ALIGN(32)
#define ALIGN128_POST DECL_ALIGN(128)
#define ALIGN_N_POST( _align_ ) DECL_ALIGN( _align_ )
#else
// MSVC has the align at the start of the struct
// PS3 SNC supports both
#define ALIGN4 DECL_ALIGN(4)
#define ALIGN8 DECL_ALIGN(8)
#define ALIGN16 DECL_ALIGN(16)
#define ALIGN32 DECL_ALIGN(32)
#define ALIGN128 DECL_ALIGN(128)
#define ALIGN_N( _align_ ) DECL_ALIGN( _align_ )

#define ALIGN4_POST
#define ALIGN8_POST
#define ALIGN16_POST
#define ALIGN32_POST
#define ALIGN128_POST
#define ALIGN_N_POST( _align_ )
#endif

// !!! NOTE: if you get a compile error here, you are using VALIGNOF on an abstract type :NOTE !!!
#define VALIGNOF_PORTABLE( type ) ( sizeof( AlignOf_t<type> ) - sizeof( type ) )

#if defined( COMPILER_GCC ) || defined( COMPILER_MSVC )
#define VALIGNOF( type ) __alignof( type )
#define VALIGNOF_TEMPLATE_SAFE( type ) VALIGNOF_PORTABLE( type )
#else
#error "PORT: Code only tested with MSVC! Must validate with new compiler, and use built-in keyword if available."
#endif

// Pull in the /analyze code annotations.
#include "annotations.h"

//-----------------------------------------------------------------------------
// Convert int<-->pointer, avoiding 32/64-bit compiler warnings:
//-----------------------------------------------------------------------------
#define INT_TO_POINTER( i ) (void *)( ( i ) + (char *)NULL )
#define POINTER_TO_INT( p ) ( (int)(uintp)( p ) )


//-----------------------------------------------------------------------------
// Stack-based allocation related helpers
//-----------------------------------------------------------------------------
#if defined( GNUC )
	#define stackalloc( _size )		alloca( ALIGN_VALUE( _size, 16 ) )
#if defined(OSX)
	#define mallocsize( _p )	( malloc_size( _p ) )
#elif defined(_LINUX)
	#define mallocsize( _p )	( malloc_usable_size( _p ) )
#else
#error
#endif
#elif defined ( _WIN32 )
	#define stackalloc( _size )		_alloca( ALIGN_VALUE( _size, 16 ) )
	#define mallocsize( _p )		( _msize( _p ) )
#endif

#define  stackfree( _p )			0

// Linux had a few areas where it didn't construct objects in the same order that Windows does.
// So when CVProfile::CVProfile() would access g_pMemAlloc, it would crash because the allocator wasn't initalized yet.
#ifdef POSIX
	#define CONSTRUCT_EARLY __attribute__((init_priority(101)))
#else
	#define CONSTRUCT_EARLY
	#endif

#if defined(_MSC_VER)
	#define SELECTANY __declspec(selectany)
	#define RESTRICT __restrict
	#define RESTRICT_FUNC __declspec(restrict)
	#define FMTFUNCTION( a, b )
#elif defined(GNUC)
	#define SELECTANY __attribute__((weak))
	#if defined(LINUX) && !defined(DEDICATED)
		#define RESTRICT
	#else
		#define RESTRICT __restrict
	#endif
	#define RESTRICT_FUNC
	// squirrel.h does a #define printf DevMsg which leads to warnings when we try
	// to use printf as the prototype format function. Using __printf__ instead.
	#define FMTFUNCTION( fmtargnumber, firstvarargnumber ) __attribute__ (( format( __printf__, fmtargnumber, firstvarargnumber )))
#else
	#define SELECTANY static
	#define RESTRICT
	#define RESTRICT_FUNC
	#define FMTFUNCTION( a, b )
#endif

#if defined( _WIN32 )

	// Used for dll exporting and importing
	#define DLL_EXPORT				extern "C" __declspec( dllexport )
	#define DLL_IMPORT				extern "C" __declspec( dllimport )

	// Can't use extern "C" when DLL exporting a class
	#define DLL_CLASS_EXPORT		__declspec( dllexport )
	#define DLL_CLASS_IMPORT		__declspec( dllimport )

	// Can't use extern "C" when DLL exporting a global
	#define DLL_GLOBAL_EXPORT		extern __declspec( dllexport )
	#define DLL_GLOBAL_IMPORT		extern __declspec( dllimport )

	#define DLL_LOCAL

#elif defined GNUC
// Used for dll exporting and importing
#define  DLL_EXPORT   extern "C" __attribute__ ((visibility("default")))
#define  DLL_IMPORT   extern "C"

// Can't use extern "C" when DLL exporting a class
#define  DLL_CLASS_EXPORT __attribute__ ((visibility("default")))
#define  DLL_CLASS_IMPORT

// Can't use extern "C" when DLL exporting a global
#define  DLL_GLOBAL_EXPORT   extern __attribute ((visibility("default")))
#define  DLL_GLOBAL_IMPORT   extern

#define  DLL_LOCAL __attribute__ ((visibility("hidden")))

#else
#error "Unsupported Platform."
#endif

// Used for standard calling conventions
#if defined( _WIN32 ) && !defined( _X360 )
	#define  STDCALL				__stdcall
	#define  FASTCALL				__fastcall
	#define  FORCEINLINE			__forceinline
	// GCC 3.4.1 has a bug in supporting forced inline of templated functions
	// this macro lets us not force inlining in that case
	#define  FORCEINLINE_TEMPLATE		__forceinline
#elif defined( _X360 )
	#define  STDCALL				__stdcall
	#ifdef FORCEINLINE
		#undef FORCEINLINE
#endif 
	#define  FORCEINLINE			__forceinline
	#define  FORCEINLINE_TEMPLATE		__forceinline
	#else
		#define  STDCALL
	#define  FASTCALL
	#ifdef _LINUX_DEBUGGABLE
		#define  FORCEINLINE
	#else
			#define  FORCEINLINE inline __attribute__ ((always_inline))
		#endif
	#define FORCEINLINE_TEMPLATE	FORCEINLINE
//	#define  __stdcall			__attribute__ ((__stdcall__))
#endif

#if ( defined(__SANITIZE_ADDRESS__) && __SANITIZE_ADDRESS__ )
	#define NO_ASAN __attribute__((no_sanitize("address")))
	#define NO_ASAN_FORCEINLINE NO_ASAN inline
#else
	#define NO_ASAN
	#define NO_ASAN_FORCEINLINE FORCEINLINE
#endif

// Force a function call site -not- to inlined. (useful for profiling)
#define DONT_INLINE(a) (((int)(a)+1)?(a):(a))

// Pass hints to the compiler to prevent it from generating unnessecary / stupid code
// in certain situations.  Several compilers other than MSVC also have an equivilent
// construct.
//
// Essentially the 'Hint' is that the condition specified is assumed to be true at
// that point in the compilation.  If '0' is passed, then the compiler assumes that
// any subsequent code in the same 'basic block' is unreachable, and thus usually
// removed.
#ifdef _MSC_VER
	#define HINT(THE_HINT)	__assume((THE_HINT))
#else
	#define HINT(THE_HINT)	0
#endif

// Marks the codepath from here until the next branch entry point as unreachable,
// and asserts if any attempt is made to execute it.
#define UNREACHABLE() { Assert(0); HINT(0); }

// In cases where no default is present or appropriate, this causes MSVC to generate
// as little code as possible, and throw an assertion in debug.
#define NO_DEFAULT default: UNREACHABLE();


#ifdef _WIN32

// Remove warnings from warning level 4.
#pragma warning(disable : 4514) // warning C4514: 'acosl' : unreferenced inline function has been removed
#pragma warning(disable : 4100) // warning C4100: 'hwnd' : unreferenced formal parameter
#pragma warning(disable : 4127) // warning C4127: conditional expression is constant
#pragma warning(disable : 4512) // warning C4512: 'InFileRIFF' : assignment operator could not be generated
#pragma warning(disable : 4611) // warning C4611: interaction between '_setjmp' and C++ object destruction is non-portable
#pragma warning(disable : 4710) // warning C4710: function 'x' not inlined
#pragma warning(disable : 4702) // warning C4702: unreachable code
#pragma warning(disable : 4505) // unreferenced local function has been removed
#pragma warning(disable : 4239) // nonstandard extension used : 'argument' ( conversion from class Vector to class Vector& )
#pragma warning(disable : 4097) // typedef-name 'BaseClass' used as synonym for class-name 'CFlexCycler::CBaseFlex'
#pragma warning(disable : 4324) // Padding was added at the end of a structure
#pragma warning(disable : 4244) // type conversion warning.
#pragma warning(disable : 4305)	// truncation from 'const double ' to 'float '
#pragma warning(disable : 4786)	// Disable warnings about long symbol names
#pragma warning(disable : 4250) // 'X' : inherits 'Y::Z' via dominance
#pragma warning(disable : 4201) // nonstandard extension used : nameless struct/union
#pragma warning(disable : 4481) // warning C4481: nonstandard extension used: override specifier 'override'
#pragma warning(disable : 4748) // warning C4748: /GS can not protect parameters and local variables from local buffer overrun because optimizations are disabled in function

#if _MSC_VER >= 1300
#pragma warning(disable : 4511)	// Disable warnings about private copy constructors
#pragma warning(disable : 4121)	// warning C4121: 'symbol' : alignment of a member was sensitive to packing
#pragma warning(disable : 4530)	// warning C4530: C++ exception handler used, but unwind semantics are not enabled. Specify /EHsc (disabled due to std headers having exception syntax)
#endif

#if _MSC_VER >= 1400
#pragma warning(disable : 4996)	// functions declared deprecated
#endif


#endif // _WIN32

#if defined( LINUX ) && ((__GNUC__ * 100) + __GNUC_MINOR__) >= 406
  // based on some Jonathan Wakely macros on the net...
  #define GCC_DIAG_STR(s) #s
  #define GCC_DIAG_JOINSTR(x,y) GCC_DIAG_STR(x ## y)
  #define GCC_DIAG_DO_PRAGMA(x) _Pragma (#x)
  #define GCC_DIAG_PRAGMA(x)	GCC_DIAG_DO_PRAGMA(GCC diagnostic x)

  #define GCC_DIAG_PUSH_OFF(x)	GCC_DIAG_PRAGMA(push) GCC_DIAG_PRAGMA(ignored GCC_DIAG_JOINSTR(-W,x))
  #define GCC_DIAG_POP()		GCC_DIAG_PRAGMA(pop)
#else
  #define GCC_DIAG_PUSH_OFF(x)
  #define GCC_DIAG_POP()
#endif

#ifdef LINUX
#pragma GCC diagnostic ignored "-Wconversion-null"	// passing NULL to non-pointer argument 1
#pragma GCC diagnostic ignored "-Wpointer-arith"	// NULL used in arithmetic. Ie, vpanel == NULL where VPANEL is uint.
#pragma GCC diagnostic ignored "-Wswitch"				// enumeration values not handled in switch
#endif

#ifdef OSX
#pragma GCC diagnostic ignored "-Wconversion-null"			// passing NULL to non-pointer argument 1
#pragma GCC diagnostic ignored "-Wnull-arithmetic"			// NULL used in arithmetic. Ie, vpanel == NULL where VPANEL is uint.
#pragma GCC diagnostic ignored "-Wswitch-enum"				// enumeration values not handled in switch
#pragma GCC diagnostic ignored "-Wswitch"					// enumeration values not handled in switch
#endif


// When we port to 64 bit, we'll have to resolve the int, ptr vs size_t 32/64 bit problems...
#if !defined( _WIN64 )
#pragma warning( disable : 4267 )	// conversion from 'size_t' to 'int', possible loss of data
#pragma warning( disable : 4311 )	// pointer truncation from 'char *' to 'int'
#pragma warning( disable : 4312 )	// conversion from 'unsigned int' to 'memhandle_t' of greater size
#endif


#ifdef POSIX
#define _stricmp stricmp
#define strcmpi stricmp
#define stricmp strcasecmp
#define _vsnprintf vsnprintf
#define _alloca alloca
#ifdef _snprintf
#undef _snprintf
#endif
#define _snprintf snprintf
#define GetProcAddress dlsym
#define _chdir chdir
#define _strnicmp strnicmp
#define strnicmp strncasecmp
#define _getcwd getcwd
#define _snwprintf swprintf
#define swprintf_s swprintf
#define wcsicmp _wcsicmp
#define _wcsicmp wcscmp
#define _finite finite
#define _tempnam tempnam
#define _unlink unlink
#define _access access
#define _mkdir(dir) mkdir( dir, S_IRWXU | S_IRWXG | S_IRWXO )
#define _wtoi(arg) wcstol(arg, NULL, 10)
#define _wtoi64(arg) wcstoll(arg, NULL, 10)

#define _O_RDONLY O_RDONLY

#define _stat stat
#define _open open
#define _lseek lseek
#define _read read
#define _close close

#ifndef USE_DXVK
typedef uintp HMODULE;
typedef void *HANDLE;
#endif
#endif

//-----------------------------------------------------------------------------
// fsel
//-----------------------------------------------------------------------------
#ifndef _X360

static FORCEINLINE float fsel(float fComparand, float fValGE, float fLT)
{
	return fComparand >= 0 ? fValGE : fLT;
}
static FORCEINLINE double fsel(double fComparand, double fValGE, double fLT)
{
	return fComparand >= 0 ? fValGE : fLT;
}

#else

// __fsel(double fComparand, double fValGE, double fLT) == fComparand >= 0 ? fValGE : fLT
// this is much faster than if ( aFloat > 0 ) { x = .. }
#define fsel __fsel

#endif


//-----------------------------------------------------------------------------
// FP exception handling
//-----------------------------------------------------------------------------
//#define CHECK_FLOAT_EXCEPTIONS		1

#if !defined( _X360 )
#if defined( _MSC_VER )

	#if defined( PLATFORM_WINDOWS_PC64 )
		inline void SetupFPUControlWord()
		{
		}
	#else
		inline void SetupFPUControlWordForceExceptions()
		{
			// use local to get and store control word
			uint16 tmpCtrlW;
			__asm
			{
				fnclex						/* clear all current exceptions */
				fnstcw word ptr [tmpCtrlW]	/* get current control word */
				and [tmpCtrlW], 0FCC0h		/* Keep infinity control + rounding control */
				or [tmpCtrlW], 0230h		/* set to 53-bit, mask only inexact, underflow */
				fldcw word ptr [tmpCtrlW]	/* put new control word in FPU */
			}
		}

		#ifdef CHECK_FLOAT_EXCEPTIONS

			inline void SetupFPUControlWord()
			{
				SetupFPUControlWordForceExceptions();
			}

		#else

			inline void SetupFPUControlWord()
			{
				// use local to get and store control word
				uint16 tmpCtrlW;
				__asm
				{
					fnstcw word ptr [tmpCtrlW]	/* get current control word */
					and [tmpCtrlW], 0FCC0h		/* Keep infinity control + rounding control */
					or [tmpCtrlW], 023Fh		/* set to 53-bit, mask only inexact, underflow */
					fldcw word ptr [tmpCtrlW]	/* put new control word in FPU */
				}
			}

		#endif
	#endif

#else

	inline void SetupFPUControlWord()
	{
		__volatile unsigned short int __cw;
		__asm __volatile ("fnstcw %0" : "=m" (__cw));
		__cw = __cw & 0x0FCC0;	// keep infinity control, keep rounding mode
		__cw = __cw | 0x023F;	// set 53-bit, no exceptions
		__asm __volatile ("fldcw %0" : : "m" (__cw));
	}

#endif // _MSC_VER

#else

	#ifdef _DEBUG
		FORCEINLINE bool IsFPUControlWordSet()
		{
			float f = 0.996f;
			union
			{
				double flResult;
				int pResult[2];
			};
			flResult = __fctiw( f );
			return ( pResult[1] == 1 );
		}
	#endif

	inline void SetupFPUControlWord()
	{
		// Set round-to-nearest in FPSCR
		// (cannot assemble, must use op-code form)
		__emit( 0xFF80010C );	// mtfsfi  7,0

		// Favour compatibility over speed (make sure the VPU set to Java-compliant mode)
		// NOTE: the VPU *always* uses round-to-nearest
			__vector4  a = { 0.0f, 0.0f, 0.0f, 0.0f };
			a;				//	Avoid compiler warning
			__asm
		{
			mtvscr a;	// Clear the Vector Status & Control Register to zero
		}
	}

#endif // _X360

//-----------------------------------------------------------------------------
// Portability casting
//-----------------------------------------------------------------------------
template < typename Tdst, typename Tsrc > FORCEINLINE Tdst size_cast( Tsrc val )
{
	static_assert( sizeof( Tdst ) <= sizeof( uint64 ) && sizeof( Tsrc ) <= sizeof( uint64 ), "Okay in my defense there weren't any types larger than 64-bits when this code was written." );
	return ( Tdst )val;
}

//-----------------------------------------------------------------------------
// Purpose: Standard functions for handling endian-ness
//-----------------------------------------------------------------------------

//-------------------------------------
// Basic swaps
//-------------------------------------

template <typename T>
inline T WordSwapC( T w )
{
   uint16 temp;

   COMPILE_TIME_ASSERT( sizeof( T ) == sizeof(uint16) );

   temp  = ((*((uint16 *)&w) & 0xff00) >> 8);
   temp |= ((*((uint16 *)&w) & 0x00ff) << 8);

   return *((T*)&temp);
}

template <typename T>
inline T DWordSwapC( T dw )
{
   uint32 temp;

   COMPILE_TIME_ASSERT( sizeof( T ) == sizeof(uint32) );

   temp  =   *((uint32 *)&dw) 				>> 24;
   temp |= ((*((uint32 *)&dw) & 0x00FF0000) >> 8);
   temp |= ((*((uint32 *)&dw) & 0x0000FF00) << 8);
   temp |= ((*((uint32 *)&dw) & 0x000000FF) << 24);

   return *((T*)&temp);
}

template <typename T>
inline T QWordSwapC( T dw )
{
	// Assert sizes passed to this are already correct, otherwise
	// the cast to uint64 * below is unsafe and may have wrong results 
	// or even crash.
	COMPILE_TIME_ASSERT( sizeof( dw ) == sizeof(uint64) );

	uint64 temp;

	temp  =   *((uint64 *)&dw) 				         >> 56;
	temp |= ((*((uint64 *)&dw) & 0x00FF000000000000ull) >> 40);
	temp |= ((*((uint64 *)&dw) & 0x0000FF0000000000ull) >> 24);
	temp |= ((*((uint64 *)&dw) & 0x000000FF00000000ull) >> 8);
	temp |= ((*((uint64 *)&dw) & 0x00000000FF000000ull) << 8);
	temp |= ((*((uint64 *)&dw) & 0x0000000000FF0000ull) << 24);
	temp |= ((*((uint64 *)&dw) & 0x000000000000FF00ull) << 40);
	temp |= ((*((uint64 *)&dw) & 0x00000000000000FFull) << 56);

	return *((T*)&temp);
}

//-------------------------------------
// Fast swaps
//-------------------------------------

#if defined _MSC_VER		// MSVC (What about MinGW and Clang for Windows)

#define WordSwap(d) _byteswap_ushort(d)
#define DWordSwap(d) ((uint32)(_byteswap_ulong( (unsigned long) d)))
#define QWordSwap(d) _byteswap_uint64(d)

#elif defined __GNUC__		// GCC or Clang

#define WordSwap(d) __builtin_bswap16(d)
#define DWordSwap(d) __builtin_bswap32(d)
#define QWordSwap(d) __builtin_bswap64(d)

#else						// N/A, native code

#pragma message( "TODO: Using non-intrinsic byteswap functions..." )

#define WordSwap WordSwapC
#define DWordSwap DWordSwapC
#define QWordSwap QWordSwapC

#endif

//-------------------------------------
// The typically used methods.
//-------------------------------------

#define VALVE_LITTLE_ENDIAN 1

// If a swapped float passes through the fpu, the bytes may get changed.
// Prevent this by swapping floats as DWORDs.
#define SafeSwapFloat( pOut, pIn )	(*((uint*)pOut) = DWordSwap( *((uint*)pIn) ))

#if defined(VALVE_LITTLE_ENDIAN)

#define BigShort( val )				WordSwap( val )
#define BigWord( val )				WordSwap( val )
#define BigLong( val )				DWordSwap( val )
#define BigDWord( val )				DWordSwap( val )
#define LittleShort( val )			( val )
#define LittleWord( val )			( val )
#define LittleLong( val )			( val )
#define LittleDWord( val )			( val )
#define LittleQWord( val )			( val )
#define SwapShort( val )			BigShort( val )
#define SwapWord( val )				BigWord( val )
#define SwapLong( val )				BigLong( val )
#define SwapDWord( val )			BigDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		SafeSwapFloat( pOut, pIn )
#define LittleFloat( pOut, pIn )	( *pOut = *pIn )
#define SwapFloat( pOut, pIn )		BigFloat( pOut, pIn )

#elif defined(VALVE_BIG_ENDIAN)

#define BigShort( val )				( val )
#define BigWord( val )				( val )
#define BigLong( val )				( val )
#define BigDWord( val )				( val )
#define LittleShort( val )			WordSwap( val )
#define LittleWord( val )			WordSwap( val )
#define LittleLong( val )			DWordSwap( val )
#define LittleDWord( val )			DWordSwap( val )
#define LittleQWord( val )			QWordSwap( val )
#define SwapShort( val )			LittleShort( val )
#define SwapWord( val )				LittleWord( val )
#define SwapLong( val )				LittleLong( val )
#define SwapDWord( val )			LittleDWord( val )

// Pass floats by pointer for swapping to avoid truncation in the fpu
#define BigFloat( pOut, pIn )		( *pOut = *pIn )
#define LittleFloat( pOut, pIn )	SafeSwapFloat( pOut, pIn )
#define SwapFloat( pOut, pIn )		LittleFloat( pOut, pIn )

#else

// @Note (toml 05-02-02): this technique expects the compiler to
// optimize the expression and eliminate the other path. On any new
// platform/compiler this should be tested.
inline short BigShort( short val )		{ int test = 1; return ( *(char *)&test == 1 ) ? WordSwap( val )  : val; }
inline uint16 BigWord( uint16 val )		{ int test = 1; return ( *(char *)&test == 1 ) ? WordSwap( val )  : val; }
inline long BigLong( long val )			{ int test = 1; return ( *(char *)&test == 1 ) ? DWordSwap( val ) : val; }
inline uint32 BigDWord( uint32 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? DWordSwap( val ) : val; }
inline short LittleShort( short val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : WordSwap( val ); }
inline uint16 LittleWord( uint16 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : WordSwap( val ); }
inline long LittleLong( long val )		{ int test = 1; return ( *(char *)&test == 1 ) ? val : DWordSwap( val ); }
inline uint32 LittleDWord( uint32 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : DWordSwap( val ); }
inline uint64 LittleQWord( uint64 val )	{ int test = 1; return ( *(char *)&test == 1 ) ? val : QWordSwap( val ); }
inline short SwapShort( short val )					{ return WordSwap( val ); }
inline uint16 SwapWord( uint16 val )				{ return WordSwap( val ); }
inline long SwapLong( long val )					{ return DWordSwap( val ); }
inline uint32 SwapDWord( uint32 val )				{ return DWordSwap( val ); }

// Pass floats by pointer for swapping to avoid truncation in the fpu
inline void BigFloat( float *pOut, const float *pIn )		{ int test = 1; ( *(char *)&test == 1 ) ? SafeSwapFloat( pOut, pIn ) : ( *pOut = *pIn ); }
inline void LittleFloat( float *pOut, const float *pIn )	{ int test = 1; ( *(char *)&test == 1 ) ? ( *pOut = *pIn ) : SafeSwapFloat( pOut, pIn ); }
inline void SwapFloat( float *pOut, const float *pIn )		{ SafeSwapFloat( pOut, pIn ); }

#endif

#if _X360
FORCEINLINE unsigned long LoadLittleDWord( const unsigned long *base, unsigned int dwordIndex )
		{
			return __loadwordbytereverse( dwordIndex<<2, base );
		}

FORCEINLINE void StoreLittleDWord( unsigned long *base, unsigned int dwordIndex, unsigned long dword )
		{
			__storewordbytereverse( dword, dwordIndex<<2, base );
		}
#else
	FORCEINLINE uint32 LoadLittleDWord( uint32 *base, unsigned int dwordIndex )
	{
		return LittleDWord( base[dwordIndex] );
	}

	FORCEINLINE void StoreLittleDWord( uint32 *base, unsigned int dwordIndex, uint32 dword )
	{
		base[dwordIndex] = LittleDWord(dword);
	}
#endif

inline uint64 CastPtrToUint64( const void* p )
{
	return (uint64) ( (uintp) p );
}

inline int64 CastPtrToInt64( const void* p )
{
	return (int64) ( (uintp) p );
}

//-----------------------------------------------------------------------------
// DLL export for platform utilities
//-----------------------------------------------------------------------------
#ifndef STATIC_TIER0

#ifdef TIER0_DLL_EXPORT
#define PLATFORM_INTERFACE	DLL_EXPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_EXPORT
#define PLATFORM_CLASS		DLL_CLASS_EXPORT
#else
#define PLATFORM_INTERFACE	DLL_IMPORT
#define PLATFORM_OVERLOAD	DLL_GLOBAL_IMPORT
#define PLATFORM_CLASS		DLL_CLASS_IMPORT
#endif

#else	// BUILD_AS_DLL

#define PLATFORM_INTERFACE	extern
#define PLATFORM_OVERLOAD
#define PLATFORM_CLASS

#endif	// BUILD_AS_DLL

typedef class CSysModule* PlatModule_t;
#define PLAT_MODULE_INVALID ((PlatModule_t)0)

// When in benchmark mode, the timer returns a simple incremented value each time you call it.
//
// It should not be changed after startup unless you really know what you're doing. The only place
// that should do this is the benchmark code itself so it can output a legit duration.
PLATFORM_INTERFACE void				Plat_SetBenchmarkMode( bool bBenchmarkMode );	
PLATFORM_INTERFACE bool				Plat_IsInBenchmarkMode();


PLATFORM_INTERFACE double			Plat_FloatTime();		// Returns time in seconds since the module was loaded.
PLATFORM_INTERFACE uint32			Plat_MSTime();			// Time in milliseconds.
PLATFORM_INTERFACE uint64			Plat_USTime();			// Time in microseconds.
PLATFORM_INTERFACE char *			Plat_ctime( const time_t *timep, char *buf, size_t bufsize );
PLATFORM_INTERFACE void				Plat_GetModuleFilename( char *pOut, int nMaxBytes );

PLATFORM_INTERFACE void				Plat_ExitProcess( int nCode );

typedef uint32 strlen_t;

PLATFORM_INTERFACE void Plat_getwd( char *pWorkingDirectory, strlen_t nBufLen );

PLATFORM_INTERFACE void Plat_chdir( const char *pDir );

PLATFORM_INTERFACE char const * Plat_GetEnv( char const *pEnvVarName );

PLATFORM_INTERFACE bool Plat_GetExecutablePath( char *pBuff, strlen_t nBuff );

PLATFORM_INTERFACE bool Plat_FileExists( const char *pFileName );

//called to exit the process due to a fatal error. This allows for the application to handle providing a hook as well which can be called
//before exiting
PLATFORM_INTERFACE void				Plat_ExitProcessWithError( int nCode, bool bGenerateMinidump = false );

//sets the callback that will be triggered by Plat_ExitProcessWithError. NULL is valid. The return value true indicates that
//the exit has been handled and no further processing should be performed. False will cause a minidump to be generated, and the process
//to be terminated
typedef bool (*ExitProcessWithErrorCBFn)( int nCode );
PLATFORM_INTERFACE void				Plat_SetExitProcessWithErrorCB( ExitProcessWithErrorCBFn pfnCB );

PLATFORM_INTERFACE struct tm *		Plat_gmtime( const time_t *timep, struct tm *result );
PLATFORM_INTERFACE time_t			Plat_timegm( struct tm *timeptr );
PLATFORM_INTERFACE struct tm *		Plat_localtime( const time_t *timep, struct tm *result );

#if defined( _WIN32 ) && defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
	extern "C" unsigned __int64 __rdtsc();
	#pragma intrinsic(__rdtsc)
#endif

inline uint64 Plat_Rdtsc()
{
#if defined( _X360 )
	return ( uint64 )__mftb32();
#elif defined( _WIN64 )
	return ( uint64 )__rdtsc();
#elif defined( _WIN32 )
  #if defined( _MSC_VER ) && ( _MSC_VER >= 1400 )
	return ( uint64 )__rdtsc();
  #else
    __asm rdtsc;
	__asm ret;
  #endif
#elif defined( __i386__ )
	uint64 val;
	__asm__ __volatile__ ( "rdtsc" : "=A" (val) );
	return val;
#elif defined( __x86_64__ )
	uint32 lo, hi;
	__asm__ __volatile__ ( "rdtsc" : "=a" (lo), "=d" (hi));
	return ( ( ( uint64 )hi ) << 32 ) | lo;
#else
	#error
#endif
}

// b/w compatibility
#define Sys_FloatTime Plat_FloatTime

// Protect against bad auto operator=
#define DISALLOW_OPERATOR_EQUAL( _classname )			\
	private:											\
		_classname &operator=( const _classname & );	\
	public:

// Define a reasonable operator=
#define IMPLEMENT_OPERATOR_EQUAL( _classname )			\
	public:												\
		_classname &operator=( const _classname &src )	\
		{												\
			memcpy( this, &src, sizeof(_classname) );	\
			return *this;								\
		}

// Processor Information:
struct CPUInformation
{
	int	 m_Size;		// Size of this structure, for forward compatability.

	bool m_bRDTSC : 1,	// Is RDTSC supported?
		 m_bCMOV  : 1,  // Is CMOV supported?
		 m_bFCMOV : 1,  // Is FCMOV supported?
		 m_bSSE	  : 1,	// Is SSE supported?
		 m_bSSE2  : 1,	// Is SSE2 Supported?
		 m_b3DNow : 1,	// Is 3DNow! Supported?
		 m_bMMX   : 1,	// Is MMX supported?
		 m_bHT	  : 1;	// Is HyperThreading supported?

	uint8 m_nLogicalProcessors;		// Number op logical processors.
	uint8 m_nPhysicalProcessors;	// Number of physical processors

	bool m_bSSE3 : 1,
		 m_bSSSE3 : 1,
		 m_bSSE4a : 1,
		 m_bSSE41 : 1,
		 m_bSSE42 : 1,
		 m_bAVX   : 1;  // Is AVX supported?

	int64 m_Speed;						// In cycles per second.

	char* m_szProcessorID;				// Processor vendor Identification.

	uint32 m_nModel;
	uint32 m_nFeatures[ 3 ];

	char* m_szProcessorBrand;			// Processor brand string, if available

	uint32 m_nL1CacheSizeKb;
	uint32 m_nL1CacheDesc;
	uint32 m_nL2CacheSizeKb;
	uint32 m_nL2CacheDesc;
	uint32 m_nL3CacheSizeKb;
	uint32 m_nL3CacheDesc;

	CPUInformation()
	{
		memset( this, 0, sizeof( *this ) );
	}
};

// Have to return a pointer, not a reference, because references are not compatible with the
// extern "C" implied by PLATFORM_INTERFACE.
PLATFORM_INTERFACE const CPUInformation* GetCPUInformation();

#define MEMORY_INFORMATION_VERSION 0

struct MemoryInformation
{
	int m_nStructVersion;

	uint m_nPhysicalRamMbTotal;
	uint m_nPhysicalRamMbAvailable;
	
	uint m_nVirtualRamMbTotal;
	uint m_nVirtualRamMbAvailable;

	inline MemoryInformation()
	{
		memset( this, 0, sizeof( *this ) );
		m_nStructVersion = MEMORY_INFORMATION_VERSION;
	}
};

// Returns true if the passed in MemoryInformation structure was filled out, otherwise false.
PLATFORM_INTERFACE bool GetMemoryInformation( MemoryInformation *pOutMemoryInfo );

PLATFORM_INTERFACE float GetCPUUsage();

PLATFORM_INTERFACE void GetCurrentDate( int *pDay, int *pMonth, int *pYear );

//-----------------------------------------------------------------------------
// D3DX
//-----------------------------------------------------------------------------

#define DXABSTRACT_BREAK_ON_ERROR() DebuggerBreak()

// ---------------------------------------------------------------------------------- //
// Performance Monitoring Events - L2 stats etc...
// ---------------------------------------------------------------------------------- //
PLATFORM_INTERFACE void InitPME();
PLATFORM_INTERFACE void ShutdownPME();

//-----------------------------------------------------------------------------
// Thread related functions
//-----------------------------------------------------------------------------

// Sets a hardware data breakpoint on the given address. Currently Win32-only.
// Specify 1, 2, or 4 bytes for nWatchBytes; pass 0 to unregister the address.
PLATFORM_INTERFACE void	Plat_SetHardwareDataBreakpoint( const void *pAddress, int nWatchBytes, bool bBreakOnRead );

// Apply current hardware data breakpoints to a newly created thread.
PLATFORM_INTERFACE void	Plat_ApplyHardwareDataBreakpointsToNewThread( unsigned long dwThreadID );

//-----------------------------------------------------------------------------
// Process related functions
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE const tchar *Plat_GetCommandLine();
#ifndef _WIN32
// helper function for OS's that don't have a ::GetCommandLine() call
PLATFORM_INTERFACE void Plat_SetCommandLine( const char *cmdLine );
#endif
PLATFORM_INTERFACE const char *Plat_GetCommandLineA();

//-----------------------------------------------------------------------------
// Security related functions
//-----------------------------------------------------------------------------
// Ensure that the hardware key's drivers have been installed.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKeyDriver();

// Ok, so this isn't a very secure way to verify the hardware key for now.  It
// is primarially depending on the fact that all the binaries have been wrapped
// with the secure wrapper provided by the hardware keys vendor.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKey();

// The same as above, but notifies user with a message box when the key isn't in
// and gives him an opportunity to correct the situation.
PLATFORM_INTERFACE bool Plat_VerifyHardwareKeyPrompt();

// Can be called in real time, doesn't perform the verify every frame.  Mainly just
// here to allow the game to drop out quickly when the key is removed, rather than
// allowing the wrapper to pop up it's own blocking dialog, which the engine doesn't
// like much.
PLATFORM_INTERFACE bool Plat_FastVerifyHardwareKey();

//-----------------------------------------------------------------------------
// Just logs file and line to simple.log
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE void* Plat_SimpleLog( const tchar* file, int line );

#if _X360
#define Plat_FastMemset XMemSet
#define Plat_FastMemcpy XMemCpy
#else
#define Plat_FastMemset memset
#define Plat_FastMemcpy memcpy
#endif

//-----------------------------------------------------------------------------
// Returns true if debugger attached, false otherwise
//-----------------------------------------------------------------------------
#if defined(_WIN32) || defined(LINUX) || defined(OSX)
PLATFORM_INTERFACE bool Plat_IsInDebugSession();
PLATFORM_INTERFACE void Plat_DebugString( const char * );
#else
inline bool Plat_IsInDebugSession( bool bForceRecheck = false ) { return false; }
#define Plat_DebugString(s) ((void)0)
#endif

//-----------------------------------------------------------------------------
// Returns true if running on a 64 bit (windows) OS
//-----------------------------------------------------------------------------
PLATFORM_INTERFACE bool Is64BitOS();


//-----------------------------------------------------------------------------
// XBOX Components valid in PC compilation space
//-----------------------------------------------------------------------------

#define XBOX_DVD_SECTORSIZE			2048
#define XBOX_DVD_ECC_SIZE			32768 // driver reads in quantum ECC blocks
#define XBOX_HDD_SECTORSIZE			512

// Custom windows messages for Xbox input
#define WM_XREMOTECOMMAND					(WM_USER + 100)
#define WM_XCONTROLLER_KEY					(WM_USER + 101)
#define WM_SYS_UI							(WM_USER + 102)
#define WM_SYS_SIGNINCHANGED				(WM_USER + 103)
#define WM_SYS_STORAGEDEVICESCHANGED		(WM_USER + 104)
#define WM_SYS_PROFILESETTINGCHANGED		(WM_USER + 105)
#define WM_SYS_MUTELISTCHANGED				(WM_USER + 106)
#define WM_SYS_INPUTDEVICESCHANGED			(WM_USER + 107)
#define WM_SYS_INPUTDEVICECONFIGCHANGED		(WM_USER + 108)
#define WM_LIVE_CONNECTIONCHANGED			(WM_USER + 109)
#define WM_LIVE_INVITE_ACCEPTED				(WM_USER + 110)
#define WM_LIVE_LINK_STATE_CHANGED			(WM_USER + 111)
#define WM_LIVE_CONTENT_INSTALLED			(WM_USER + 112)
#define WM_LIVE_MEMBERSHIP_PURCHASED		(WM_USER + 113)
#define WM_LIVE_VOICECHAT_AWAY				(WM_USER + 114)
#define WM_LIVE_PRESENCE_CHANGED			(WM_USER + 115)
#define WM_FRIENDS_PRESENCE_CHANGED			(WM_USER + 116)
#define WM_FRIENDS_FRIEND_ADDED				(WM_USER + 117)
#define WM_FRIENDS_FRIEND_REMOVED			(WM_USER + 118)
#define WM_CUSTOM_GAMEBANNERPRESSED			(WM_USER + 119)
#define WM_CUSTOM_ACTIONPRESSED				(WM_USER + 120)
#define WM_XMP_STATECHANGED					(WM_USER + 121)
#define WM_XMP_PLAYBACKBEHAVIORCHANGED		(WM_USER + 122)
#define WM_XMP_PLAYBACKCONTROLLERCHANGED	(WM_USER + 123)

inline const char *GetPlatformExt( void )
{
	return IsX360() ? ".360" : "";
}

// flat view, 6 hw threads
#define XBOX_PROCESSOR_0			( 1<<0 )
#define XBOX_PROCESSOR_1			( 1<<1 )
#define XBOX_PROCESSOR_2			( 1<<2 )
#define XBOX_PROCESSOR_3			( 1<<3 )
#define XBOX_PROCESSOR_4			( 1<<4 )
#define XBOX_PROCESSOR_5			( 1<<5 )

// core view, 3 cores with 2 hw threads each
#define XBOX_CORE_0_HWTHREAD_0		XBOX_PROCESSOR_0
#define XBOX_CORE_0_HWTHREAD_1		XBOX_PROCESSOR_1
#define XBOX_CORE_1_HWTHREAD_0		XBOX_PROCESSOR_2
#define XBOX_CORE_1_HWTHREAD_1		XBOX_PROCESSOR_3
#define XBOX_CORE_2_HWTHREAD_0		XBOX_PROCESSOR_4
#define XBOX_CORE_2_HWTHREAD_1		XBOX_PROCESSOR_5

//-----------------------------------------------------------------------------
// Include additional dependant header components.
//-----------------------------------------------------------------------------
#include "tier0/fasttimer.h"

#if defined( _X360 )
#include "xbox/xbox_core.h"
#endif

//-----------------------------------------------------------------------------
// There is no requirement that a va_list be usable in multiple calls,
// but the Steam code does this.  Linux64 does not support reuse, whereas
// Windows does, so Linux64 breaks on code that was written and working
// on Windows.  Fortunately Linux has va_copy, which provides a simple
// way to let a va_list be used multiple times.  Unfortunately Windows
// does not have va_copy, so here we provide things to hide the difference.
//-----------------------------------------------------------------------------

class CReuseVaList
{
public:
    CReuseVaList( va_list List )
    {
#if defined(LINUX) || defined(OSX)
        va_copy( m_ReuseList, List );
#else
        m_ReuseList = List;
#endif
    }
    ~CReuseVaList()
    {
#if defined(LINUX) || defined(OSX)
        va_end( m_ReuseList );
#endif
    }

    va_list m_ReuseList;
};

//-----------------------------------------------------------------------------
// Methods to invoke the constructor, copy constructor, and destructor
//-----------------------------------------------------------------------------

template <class T>
inline T* Construct( T* pMemory )
{
	return reinterpret_cast<T*>(::new( pMemory ) T);
}

template <class T, typename ARG1>
inline T* Construct( T* pMemory, ARG1 a1 )
{
	return reinterpret_cast<T*>(::new( pMemory ) T( a1 ));
}

template <class T, typename ARG1, typename ARG2>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2 )
{
	return reinterpret_cast<T*>(::new( pMemory ) T( a1, a2 ));
}

template <class T, typename ARG1, typename ARG2, typename ARG3>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3 )
{
	return reinterpret_cast<T*>(::new( pMemory ) T( a1, a2, a3 ));
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4 )
{
	return reinterpret_cast<T*>(::new( pMemory ) T( a1, a2, a3, a4 ));
}

template <class T, typename ARG1, typename ARG2, typename ARG3, typename ARG4, typename ARG5>
inline T* Construct( T* pMemory, ARG1 a1, ARG2 a2, ARG3 a3, ARG4 a4, ARG5 a5 )
{
	return reinterpret_cast<T*>(::new( pMemory ) T( a1, a2, a3, a4, a5 ));
}

template <class T, class P>
inline void ConstructOneArg( T* pMemory, P const& arg)
{
	::new( pMemory ) T(arg);
}

template <class T, class P1, class P2 >
inline void ConstructTwoArg( T* pMemory, P1 const& arg1, P2 const& arg2)
{
	::new( pMemory ) T(arg1, arg2);
}

template <class T, class P1, class P2, class P3 >
inline void ConstructThreeArg( T* pMemory, P1 const& arg1, P2 const& arg2, P3 const& arg3)
{
	::new( pMemory ) T(arg1, arg2, arg3);
}

template <class T>
inline T* CopyConstruct( T* pMemory, T const& src )
{
	return reinterpret_cast<T*>(::new( pMemory ) T(src));
}

template <class T>
inline void Destruct( T* pMemory )
{
	pMemory->~T();

#ifdef _DEBUG
	memset( reinterpret_cast<void*>( pMemory ), 0xDD, sizeof(T) );
#endif
}

// The above will error when binding to a type of: foo(*)[] -- there is no provision in c++ for knowing how many objects
// to destruct without preserving the count and calling the necessary destructors.
template <class T, size_t N>
inline void Destruct( T (*pMemory)[N] )
{
	for ( size_t i = 0; i < N; i++ )
	{
		(pMemory[i])->~T();
	}

#ifdef _DEBUG
	memset( reinterpret_cast<void*>( pMemory ), 0xDD, sizeof(*pMemory) );
#endif
}

template <typename T> struct RemoveReference_      { using Type = T; };
template <typename T> struct RemoveReference_<T&>  { using Type = T; };
template <typename T> struct RemoveReference_<T&&> { using Type = T; };

template <typename T>
using RemoveReference = typename RemoveReference_<T>::Type;

template <typename T>
constexpr RemoveReference<T>&& Move(T&& arg)
{
	return static_cast<RemoveReference<T>&&>(arg);
}

// misyl: Shamelessly nicked from Source 2 =)
//
//--------------------------------------------------------------------------------------------------
// RunCodeAtScopeExit
//
// Example:
//	int *x = new int;
//	RunCodeAtScopeExit( delete x )
//--------------------------------------------------------------------------------------------------
template <typename LambdaType>
class CScopeGuardLambdaImpl
{
public:
	explicit CScopeGuardLambdaImpl( LambdaType&& lambda ) : m_lambda( Move( lambda ) ) { }
	~CScopeGuardLambdaImpl() { m_lambda(); }
private:
	LambdaType m_lambda;
};

//--------------------------------------------------------------------------------------------------
template <typename LambdaType>
CScopeGuardLambdaImpl< LambdaType > MakeScopeGuardLambda( LambdaType&& lambda )
{
	return CScopeGuardLambdaImpl< LambdaType >( Move( lambda ) );
}

//--------------------------------------------------------------------------------------------------
#define RunLambdaAtScopeExit2( VarName, ... )		const auto VarName( MakeScopeGuardLambda( __VA_ARGS__ ) ); (void)VarName
#define RunLambdaAtScopeExit( ... )					RunLambdaAtScopeExit2( UNIQUE_ID, __VA_ARGS__ )
#define RunCodeAtScopeExit( ... )					RunLambdaAtScopeExit( [&]() { __VA_ARGS__ ; } )

//
// GET_OUTER()
//
// A platform-independent way for a contained class to get a pointer to its
// owner. If you know a class is exclusively used in the context of some
// "outer" class, this is a much more space efficient way to get at the outer
// class than having the inner class store a pointer to it.
//
//	class COuter
//	{
//		class CInner // Note: this does not need to be a nested class to work
//		{
//			void PrintAddressOfOuter()
//			{
//				printf( "Outer is at 0x%x\n", GET_OUTER( COuter, m_Inner ) );
//			}
//		};
//
//		CInner m_Inner;
//		friend class CInner;
//	};

#define GET_OUTER( OuterType, OuterMember ) \
   ( ( OuterType * ) ( (uint8 *)this - offsetof( OuterType, OuterMember ) ) )


/*	TEMPLATE_FUNCTION_TABLE()

    (Note added to platform.h so platforms that correctly support templated
	 functions can handle portions as templated functions rather than wrapped
	 functions)

	Helps automate the process of creating an array of function
	templates that are all specialized by a single integer.
	This sort of thing is often useful in optimization work.

	For example, using TEMPLATE_FUNCTION_TABLE, this:

	TEMPLATE_FUNCTION_TABLE(int, Function, ( int blah, int blah ), 10)
	{
		return argument * argument;
	}

	is equivilent to the following:

	(NOTE: the function has to be wrapped in a class due to code
	generation bugs involved with directly specializing a function
	based on a constant.)

	template<int argument>
	class FunctionWrapper
	{
	public:
		int Function( int blah, int blah )
		{
			return argument*argument;
		}
	}

	typedef int (*FunctionType)( int blah, int blah );

	class FunctionName
	{
	public:
		enum { count = 10 };
		FunctionType functions[10];
	};

	FunctionType FunctionName::functions[] =
	{
		FunctionWrapper<0>::Function,
		FunctionWrapper<1>::Function,
		FunctionWrapper<2>::Function,
		FunctionWrapper<3>::Function,
		FunctionWrapper<4>::Function,
		FunctionWrapper<5>::Function,
		FunctionWrapper<6>::Function,
		FunctionWrapper<7>::Function,
		FunctionWrapper<8>::Function,
		FunctionWrapper<9>::Function
	};
*/

PLATFORM_INTERFACE bool vtune( bool resume );


#define TEMPLATE_FUNCTION_TABLE(RETURN_TYPE, NAME, ARGS, COUNT)			\
																		\
typedef RETURN_TYPE (FASTCALL *__Type_##NAME) ARGS;						\
																		\
template<const int nArgument>											\
struct __Function_##NAME												\
{																		\
	static RETURN_TYPE FASTCALL Run ARGS;								\
};																		\
																		\
template <const int i>														\
struct __MetaLooper_##NAME : __MetaLooper_##NAME<i-1>					\
{																		\
	__Type_##NAME func;													\
	inline __MetaLooper_##NAME() { func = __Function_##NAME<i>::Run; }	\
};																		\
																		\
template<>																\
struct __MetaLooper_##NAME<0>											\
{																		\
	__Type_##NAME func;													\
	inline __MetaLooper_##NAME() { func = __Function_##NAME<0>::Run; }	\
};																		\
																		\
class NAME																\
{																		\
private:																\
    static const __MetaLooper_##NAME<COUNT> m;							\
public:																	\
	enum { count = COUNT };												\
	static const __Type_##NAME* functions;								\
};																		\
const __MetaLooper_##NAME<COUNT> NAME::m;								\
const __Type_##NAME* NAME::functions = (__Type_##NAME*)&m;				\
template<const int nArgument>													\
RETURN_TYPE FASTCALL __Function_##NAME<nArgument>::Run ARGS


#define LOOP_INTERCHANGE(BOOLEAN, CODE)\
	if( (BOOLEAN) )\
	{\
		CODE;\
	} else\
	{\
		CODE;\
	}

//-----------------------------------------------------------------------------
// Dynamic libs support
//-----------------------------------------------------------------------------
#if 0 // defined( PLATFORM_WINDOWS_PC )

PLATFORM_INTERFACE void *Plat_GetProcAddress( const char *pszModule, const char *pszName );

template <typename FUNCPTR_TYPE>
class CDynamicFunction
{
public:
	CDynamicFunction( const char *pszModule, const char *pszName, FUNCPTR_TYPE pfnFallback = NULL )
	{
		m_pfn = pfnFallback;
		void *pAddr = Plat_GetProcAddress( pszModule, pszName );
		if ( pAddr )
		{
			m_pfn = (FUNCPTR_TYPE)pAddr;
		}
	}

	operator bool()			{ return m_pfn != NULL;	}
	bool operator !()		{ return !m_pfn;	}
	operator FUNCPTR_TYPE()	{ return m_pfn; }

private:
	FUNCPTR_TYPE m_pfn;
};
#endif


// Watchdog timer support. Call Plat_BeginWatchdogTimer( nn ) to kick the timer off.  if you don't call
// Plat_EndWatchdogTimer within nn seconds, the program will kick off an exception.  This is for making
// sure that hung dedicated servers abort (and restart) instead of staying hung. Calling
// Plat_EndWatchdogTimer more than once or when there is no active watchdog is fine. Only does anything
// under linux right now. It should be possible to implement this functionality in windows via a
// thread, if desired.
PLATFORM_INTERFACE void Plat_BeginWatchdogTimer( int nSecs );
PLATFORM_INTERFACE void Plat_EndWatchdogTimer( void );
PLATFORM_INTERFACE int Plat_GetWatchdogTime( void );

typedef void (*Plat_WatchDogHandlerFunction_t)(void);
PLATFORM_INTERFACE void Plat_SetWatchdogHandlerFunction( Plat_WatchDogHandlerFunction_t function );

/// Get some random bytes from a secure source of high entropy
PLATFORM_INTERFACE void SecureRandomBytes( void *pDest, size_t cbSize );

// Use ValidateAlignment to sanity-check alignment usage when allocating arrays of an aligned type
#define ALIGN_ASSERT( pred ) { COMPILE_TIME_ASSERT( pred ); }
template< class T, int ALIGN >
inline void ValidateAlignmentExplicit(void)
{
	// Alignment must be a power of two
	ALIGN_ASSERT((ALIGN & (ALIGN - 1)) == 0);
	// Alignment must not imply gaps in the array (which the CUtlMemory pattern does not allow for)
	ALIGN_ASSERT(ALIGN <= sizeof(T));
	// Alignment must be a multiple of the size of the object type, or elements will *NOT* be aligned!
	ALIGN_ASSERT((sizeof(T) % ALIGN) == 0);
	// Alignment should be a multiple of the base alignment of T
//	ALIGN_ASSERT((ALIGN % VALIGNOF(T)) == 0);
	// Alignment must not be bigger than the maximum declared alignment used by DECLARE_ALIGNED_BYTE_ARRAY
	// (if you hit this, just add more powers of 2 below and increase this limit)
	ALIGN_ASSERT( ALIGN <= 128 );
}
template< class T > inline void ValidateAlignment(void) { ValidateAlignmentExplicit<T, VALIGNOF(T)>(); }

// Portable alternative to __alignof
template<class T> struct AlignOf_t { AlignOf_t(){} AlignOf_t & operator=(const AlignOf_t &) { return *this; } byte b; T t; };

template < size_t NUM, class T, int ALIGN > struct AlignedByteArrayExplicit_t{};
template < size_t NUM, class T > struct AlignedByteArray_t : public AlignedByteArrayExplicit_t< NUM, T, VALIGNOF_TEMPLATE_SAFE(T) > {};

#define DECLARE_ALIGNED_BYTE_ARRAY( ALIGN ) \
	template < size_t NUM, class T > \
	struct ALIGN_N( ALIGN ) AlignedByteArrayExplicit_t< NUM, T, ALIGN > \
	{ \
		/* NOTE: verify alignment in the constructor (which may be wrong if this is heap-allocated, for ALIGN > MEMALLOC_MAX_AUTO_ALIGN) */ \
		AlignedByteArrayExplicit_t()	{ if ( (ALIGN-1) & (size_t)this ) DebuggerBreakIfDebugging(); } \
		T *			Base( void )		{ ValidateAlignmentExplicit<T,ALIGN>(); return (T *)&m_Data; } \
		const T *	Base( void ) const	{ ValidateAlignmentExplicit<T,ALIGN>(); return (const T *)&m_Data; } \
	private: \
		byte m_Data[ NUM*sizeof( T ) ]; \
	} ALIGN_N_POST( ALIGN );

DECLARE_ALIGNED_BYTE_ARRAY(1);
DECLARE_ALIGNED_BYTE_ARRAY(2);
DECLARE_ALIGNED_BYTE_ARRAY(4);
DECLARE_ALIGNED_BYTE_ARRAY(8);
DECLARE_ALIGNED_BYTE_ARRAY(16);
DECLARE_ALIGNED_BYTE_ARRAY(32);
DECLARE_ALIGNED_BYTE_ARRAY(64);
DECLARE_ALIGNED_BYTE_ARRAY(128);

//-----------------------------------------------------------------------------

#include "tier0/valve_on.h"

#if defined(TIER0_DLL_EXPORT)
extern "C" int V_tier0_stricmp(const char *s1, const char *s2 );
#undef stricmp
#undef strcmpi
#define stricmp(s1,s2) V_tier0_stricmp( s1, s2 )
#define strcmpi(s1,s2) V_tier0_stricmp( s1, s2 )
#endif

// misyl: Pad the lightmap atlas so we can do r_lightmap_bicubic and have bicubic sampling as an option.
// I am so evil I put it in platform.h, but a few places need this define and I didn't want to think too
// hard about it.
// >:)
#define PAD_LIGHTMAP_ATLAS 1

#endif /* PLATFORM_H */

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Insert this file into all projects using the memory system
// It will cause that project to use the shader memory allocator
//
// $NoKeywords: $
//=============================================================================//


#if !defined(STEAM) && !defined(NO_MALLOC_OVERRIDE)

#undef PROTECTED_THINGS_ENABLE   // allow use of _vsnprintf

#if defined( _WIN32 ) && !defined( _X360 )
#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "tier0/dbg.h"
#include "tier0/memalloc.h"
#include <string.h>
#include <stdio.h>
#include "memdbgoff.h"

#ifdef _WIN32
// ARG: crtdbg is necessary for certain definitions below,
// but it also redefines malloc as a macro in release.
// To disable this, we gotta define _DEBUG before including it.. BLEAH!
#define _DEBUG 1
#include "crtdbg.h"
#ifdef NDEBUG
#undef _DEBUG
#endif

// Turn this back off in release mode.
#ifdef NDEBUG
#undef _DEBUG
#endif
#elif POSIX
#define __cdecl
#endif

#if defined( _WIN32 ) && !defined( _X360 )
const char *MakeModuleFileName()
{
	if ( g_pMemAlloc->IsDebugHeap() )
	{
		char *pszModuleName = (char *)HeapAlloc( GetProcessHeap(), 0, MAX_PATH ); // small leak, debug only

		MEMORY_BASIC_INFORMATION mbi;
		static int dummy;
		VirtualQuery( &dummy, &mbi, sizeof(mbi) );

		GetModuleFileName( reinterpret_cast<HMODULE>(mbi.AllocationBase), pszModuleName, MAX_PATH );
		char *pDot = strrchr( pszModuleName, '.' );
		if ( pDot )
		{
			char *pSlash = strrchr( pszModuleName, '\\' );
			if ( pSlash )
			{
				pszModuleName = pSlash + 1;
				*pDot = 0;
			}
		}

		return pszModuleName;
	}
	return NULL;
}

static void *AllocUnattributed( size_t nSize )
{
	static const char *pszOwner = MakeModuleFileName();

	if ( !pszOwner )
		return g_pMemAlloc->Alloc(nSize);
	else
		return g_pMemAlloc->Alloc(nSize, pszOwner, 0);
}

static void *ReallocUnattributed( void *pMem, size_t nSize )
{
	static const char *pszOwner = MakeModuleFileName();

	if ( !pszOwner )
		return g_pMemAlloc->Realloc(pMem, nSize);
	else
		return g_pMemAlloc->Realloc(pMem, nSize, pszOwner, 0);
}

#else
#define MakeModuleFileName() NULL
inline void *AllocUnattributed( size_t nSize )
{
	return g_pMemAlloc->Alloc(nSize);
}

inline void *ReallocUnattributed( void *pMem, size_t nSize )
{
	return g_pMemAlloc->Realloc(pMem, nSize);
}
#endif

//-----------------------------------------------------------------------------
// Standard functions in the CRT that we're going to override to call our allocator
//-----------------------------------------------------------------------------
#if defined(_WIN32) && !defined(_STATIC_LINKED)
// this magic only works under win32
// under linux this malloc() overrides the libc malloc() and so we
// end up in a recursion (as g_pMemAlloc->Alloc() calls malloc)
#if _MSC_VER >= 1400
#define ALLOC_CALL _CRTNOALIAS _CRTRESTRICT 
#define FREE_CALL _CRTNOALIAS 
#else
#define ALLOC_CALL
#define FREE_CALL
#endif

extern "C"
{
	
ALLOC_CALL void *malloc( size_t nSize )
{
	return AllocUnattributed( nSize );
}

FREE_CALL void free( void *pMem )
{
	g_pMemAlloc->Free(pMem);
}

ALLOC_CALL void *realloc( void *pMem, size_t nSize )
{
	return ReallocUnattributed( pMem, nSize );
}

ALLOC_CALL void *calloc( size_t nCount, size_t nElementSize )
{
	void *pMem = AllocUnattributed( nElementSize * nCount );
	memset(pMem, 0, nElementSize * nCount);
	return pMem;
}

} // end extern "C"

//-----------------------------------------------------------------------------
// Non-standard MSVC functions that we're going to override to call our allocator
//-----------------------------------------------------------------------------
extern "C"
{

// 64-bit
#ifdef _WIN64
void* __cdecl _malloc_base( size_t nSize )
{
	return AllocUnattributed( nSize );
}
#else
void *_malloc_base( size_t nSize )
{
	return AllocUnattributed( nSize );
}
#endif

void *_calloc_base( size_t nSize )
{
	void *pMem = AllocUnattributed( nSize );
	memset(pMem, 0, nSize);
	return pMem;
}

void *_realloc_base( void *pMem, size_t nSize )
{
	return ReallocUnattributed( pMem, nSize );
}

void *_recalloc_base( void *pMem, size_t nSize )
{
	void *pMemOut = ReallocUnattributed( pMem, nSize );
	memset(pMemOut, 0, nSize);
	return pMemOut;
}

void _free_base( void *pMem )
{
	g_pMemAlloc->Free(pMem);
}

void *__cdecl _expand_base( void *pMem, size_t nNewSize, int nBlockUse )
{
	Assert( 0 );
	return NULL;
}

// crt
void * __cdecl _malloc_crt(size_t size)
{
	return AllocUnattributed( size );
}

void * __cdecl _calloc_crt(size_t count, size_t size)
{
	return _calloc_base( count * size );
}

void * __cdecl _realloc_crt(void *ptr, size_t size)
{
	return _realloc_base( ptr, size );
}

void * __cdecl _recalloc_crt(void *ptr, size_t count, size_t size)
{
	return _recalloc_base( ptr, size * count );
}

ALLOC_CALL void * __cdecl _recalloc ( void * memblock, size_t count, size_t size )
{
	void *pMem = ReallocUnattributed( memblock, size * count );
	memset( pMem, 0, size * count );
	return pMem;
}

size_t _msize_base( void *pMem )
{
	return g_pMemAlloc->GetSize(pMem);
}

size_t _msize( void *pMem )
{
	return _msize_base(pMem);
}

size_t msize( void *pMem )
{
	return g_pMemAlloc->GetSize(pMem);
}

void *__cdecl _heap_alloc( size_t nSize )
{
	return AllocUnattributed( nSize );
}

void *__cdecl _nh_malloc( size_t nSize, int )
{
	return AllocUnattributed( nSize );
}

void *__cdecl _expand( void *pMem, size_t nSize )
{
	Assert( 0 );
	return NULL;
}

unsigned int _amblksiz = 16; //BYTES_PER_PARA;

#if _MSC_VER >= 1400
HANDLE _crtheap = (HANDLE)1;	// PatM Can't be 0 or CRT pukes
int __active_heap = 1;
#endif //  _MSC_VER >= 1400

size_t __cdecl _get_sbh_threshold( void )
{
	return 0;
}

int __cdecl _set_sbh_threshold( size_t )
{
	return 0;
}

int _heapchk()
{
	return g_pMemAlloc->heapchk();
}

int _heapmin()
{
	return 1;
}

int __cdecl _heapadd( void *, size_t )
{
	return 0;
}

int __cdecl _heapset( unsigned int )
{
	return 0;
}

size_t __cdecl _heapused( size_t *, size_t * )
{
	return 0;
}

#ifdef _WIN32
int __cdecl _heapwalk( _HEAPINFO * )
{
	return 0;
}
#endif

} // end extern "C"


//-----------------------------------------------------------------------------
// Debugging functions that we're going to override to call our allocator
// NOTE: These have to be here for release + debug builds in case we
// link to a debug static lib!!!
//-----------------------------------------------------------------------------

extern "C"
{
	
void *malloc_db( size_t nSize, const char *pFileName, int nLine )
{
	return g_pMemAlloc->Alloc(nSize, pFileName, nLine);
}

void free_db( void *pMem, const char *pFileName, int nLine )
{
	g_pMemAlloc->Free(pMem, pFileName, nLine);
}

void *realloc_db( void *pMem, size_t nSize, const char *pFileName, int nLine )
{
	return g_pMemAlloc->Realloc(pMem, nSize, pFileName, nLine);
}
	
} // end extern "C"

//-----------------------------------------------------------------------------
// These methods are standard MSVC heap initialization + shutdown methods
//-----------------------------------------------------------------------------
extern "C"
{

#if !defined( _X360 )
	int __cdecl _heap_init()
	{
		return g_pMemAlloc != NULL;
	}

	void __cdecl _heap_term()
	{
	}
#endif

}
#endif


//-----------------------------------------------------------------------------
// Prevents us from using an inappropriate new or delete method,
// ensures they are here even when linking against debug or release static libs
//-----------------------------------------------------------------------------
#ifndef NO_MEMOVERRIDE_NEW_DELETE
#ifdef OSX
void *__cdecl operator new( size_t nSize ) throw (std::bad_alloc)
#else
void *__cdecl operator new( size_t nSize )
#endif
{
	return AllocUnattributed( nSize );
}

void *__cdecl operator new( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
{
	return g_pMemAlloc->Alloc(nSize, pFileName, nLine);
}

#ifdef OSX
void __cdecl operator delete( void *pMem ) throw()
#else
void __cdecl operator delete( void *pMem )
#endif
{
	g_pMemAlloc->Free( pMem );
}

#ifdef OSX
void *__cdecl operator new[]( size_t nSize ) throw (std::bad_alloc)
#else
void *__cdecl operator new[]( size_t nSize )
#endif
{
	return AllocUnattributed( nSize );
}

void *__cdecl operator new[] ( size_t nSize, int nBlockUse, const char *pFileName, int nLine )
{
	return g_pMemAlloc->Alloc(nSize, pFileName, nLine);
}

#ifdef OSX
void __cdecl operator delete[]( void *pMem ) throw()
#else
void __cdecl operator delete[]( void *pMem )
#endif
{
	g_pMemAlloc->Free( pMem );
}
#endif


//-----------------------------------------------------------------------------
// Override some debugging allocation methods in MSVC
// NOTE: These have to be here for release + debug builds in case we
// link to a debug static lib!!!
//-----------------------------------------------------------------------------
#ifndef _STATIC_LINKED
#ifdef _WIN32

// This here just hides the internal file names, etc of allocations
// made in the c runtime library
#define CRT_INTERNAL_FILE_NAME "C-runtime internal"

class CAttibCRT
{
public:
	CAttibCRT(int nBlockUse) : m_nBlockUse(nBlockUse)
	{
		if (m_nBlockUse == _CRT_BLOCK)
		{
			g_pMemAlloc->PushAllocDbgInfo(CRT_INTERNAL_FILE_NAME, 0);
		}
	}
	
	~CAttibCRT()
	{
		if (m_nBlockUse == _CRT_BLOCK)
		{
			g_pMemAlloc->PopAllocDbgInfo();
		}
	}
	
private:
	int m_nBlockUse;
};


#define AttribIfCrt() CAttibCRT _attrib(nBlockUse)
#elif defined(POSIX)
#define AttribIfCrt()
#endif // _WIN32


extern "C"
{
	
void *__cdecl _nh_malloc_dbg( size_t nSize, int nFlag, int nBlockUse,
								const char *pFileName, int nLine )
{
	AttribIfCrt();
	return g_pMemAlloc->Alloc(nSize, pFileName, nLine);
}

void *__cdecl _malloc_dbg( size_t nSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	AttribIfCrt();
	return g_pMemAlloc->Alloc(nSize, pFileName, nLine);
}

#if defined( _X360 )
void *__cdecl _calloc_dbg_impl( size_t nNum, size_t nSize, int nBlockUse, 
								const char * szFileName, int nLine, int * errno_tmp )
{
	return _calloc_dbg( nNum, nSize, nBlockUse, szFileName, nLine );
}
#endif

void *__cdecl _calloc_dbg( size_t nNum, size_t nSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	AttribIfCrt();
	void *pMem = g_pMemAlloc->Alloc(nSize * nNum, pFileName, nLine);
	memset(pMem, 0, nSize * nNum);
	return pMem;
}

void *__cdecl _calloc_dbg_impl( size_t nNum, size_t nSize, int nBlockUse, 
	const char * szFileName, int nLine, int * errno_tmp )
{
	return _calloc_dbg( nNum, nSize, nBlockUse, szFileName, nLine );
}

void *__cdecl _realloc_dbg( void *pMem, size_t nNewSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	AttribIfCrt();
	return g_pMemAlloc->Realloc(pMem, nNewSize, pFileName, nLine);
}

void *__cdecl _expand_dbg( void *pMem, size_t nNewSize, int nBlockUse,
							const char *pFileName, int nLine )
{
	Assert( 0 );
	return NULL;
}

void __cdecl _free_dbg( void *pMem, int nBlockUse )
{
	AttribIfCrt();
	g_pMemAlloc->Free(pMem);
}

size_t __cdecl _msize_dbg( void *pMem, int nBlockUse )
{
#ifdef _WIN32
	return _msize(pMem);
#elif POSIX
	Assert( "_msize_dbg unsupported" );
	return 0;
#endif
}


#ifdef _WIN32

#if defined(_DEBUG) && _MSC_VER >= 1300
// X360TBD: aligned and offset allocations may be important on the 360

// aligned base
ALLOC_CALL void *__cdecl _aligned_malloc_base( size_t size, size_t align )
{
	return MemAlloc_AllocAligned( size, align );
}

ALLOC_CALL void *__cdecl _aligned_realloc_base( void *ptr, size_t size, size_t align )
{
	return MemAlloc_ReallocAligned( ptr, size, align );
}

ALLOC_CALL void *__cdecl _aligned_recalloc_base( void *ptr, size_t size, size_t align )
{
	Error( "Unsupported function\n" );
	return NULL;
}

FREE_CALL void __cdecl _aligned_free_base( void *ptr )
{
	MemAlloc_FreeAligned( ptr );
}

// aligned
ALLOC_CALL void * __cdecl _aligned_malloc( size_t size, size_t align )
{
	return _aligned_malloc_base(size, align);
}

ALLOC_CALL void *__cdecl _aligned_realloc(void *memblock, size_t size, size_t align)
{
    return _aligned_realloc_base(memblock, size, align);
}

ALLOC_CALL void * __cdecl _aligned_recalloc( void * memblock, size_t count, size_t size, size_t align )
{
    return _aligned_recalloc_base(memblock, count * size, align);
}

FREE_CALL void __cdecl _aligned_free( void *memblock )
{
    _aligned_free_base(memblock);
}

// aligned offset base
ALLOC_CALL void * __cdecl _aligned_offset_malloc_base( size_t size, size_t align, size_t offset )
{
	Assert( IsPC() || 0 );
	return NULL;
}

ALLOC_CALL void * __cdecl _aligned_offset_realloc_base( void * memblock, size_t size, size_t align, size_t offset)
{
	Assert( IsPC() || 0 );
	return NULL;
}

ALLOC_CALL void * __cdecl _aligned_offset_recalloc_base( void * memblock, size_t size, size_t align, size_t offset)
{
	Assert( IsPC() || 0 );
	return NULL;
}

// aligned offset
ALLOC_CALL void *__cdecl _aligned_offset_malloc(size_t size, size_t align, size_t offset)
{
    return _aligned_offset_malloc_base( size, align, offset );
}

ALLOC_CALL void *__cdecl _aligned_offset_realloc(void *memblock, size_t size, size_t align, size_t offset)
{
    return _aligned_offset_realloc_base( memblock, size, align, offset );
}

ALLOC_CALL void * __cdecl _aligned_offset_recalloc( void * memblock, size_t count, size_t size, size_t align, size_t offset )
{
    return _aligned_offset_recalloc_base( memblock, count * size, align, offset );
}

#endif // _MSC_VER >= 1400

#endif

} // end extern "C"


//-----------------------------------------------------------------------------
// Override some the _CRT debugging allocation methods in MSVC
//-----------------------------------------------------------------------------
#ifdef _WIN32

extern "C"
{
	
int _CrtDumpMemoryLeaks(void)
{
	return 0;
}

_CRT_DUMP_CLIENT _CrtSetDumpClient( _CRT_DUMP_CLIENT dumpClient )
{
	return NULL;
}

int _CrtSetDbgFlag( int nNewFlag )
{
	return g_pMemAlloc->CrtSetDbgFlag( nNewFlag );
}

// 64-bit port.
#define AFNAME(var) __p_ ## var
#define AFRET(var)  &var

int _crtDbgFlag = _CRTDBG_ALLOC_MEM_DF;
int* AFNAME(_crtDbgFlag)(void)
{
	return AFRET(_crtDbgFlag);
}

long _crtBreakAlloc;      /* Break on this allocation */
long* AFNAME(_crtBreakAlloc) (void)
{
	return AFRET(_crtBreakAlloc);
}

void __cdecl _CrtSetDbgBlockType( void *pMem, int nBlockUse )
{
	DebuggerBreak();
}

_CRT_ALLOC_HOOK __cdecl _CrtSetAllocHook( _CRT_ALLOC_HOOK pfnNewHook )
{
	DebuggerBreak();
	return NULL;
}

long __cdecl _CrtSetBreakAlloc( long lNewBreakAlloc )
{
	return g_pMemAlloc->CrtSetBreakAlloc( lNewBreakAlloc );
}
					 
int __cdecl _CrtIsValidHeapPointer( const void *pMem )
{
	return g_pMemAlloc->CrtIsValidHeapPointer( pMem );
}

int __cdecl _CrtIsValidPointer( const void *pMem, unsigned int size, int access )
{
	return g_pMemAlloc->CrtIsValidPointer( pMem, size, access );
}

int __cdecl _CrtCheckMemory( void )
{
	// FIXME: Remove this when we re-implement the heap
	return g_pMemAlloc->CrtCheckMemory( );
}

int __cdecl _CrtIsMemoryBlock( const void *pMem, unsigned int nSize,
    long *plRequestNumber, char **ppFileName, int *pnLine )
{
	DebuggerBreak();
	return 1;
}

int __cdecl _CrtMemDifference( _CrtMemState *pState, const _CrtMemState * oldState, const _CrtMemState * newState )
{
	DebuggerBreak();
	return FALSE;
}

void __cdecl _CrtMemDumpStatistics( const _CrtMemState *pState )
{
	DebuggerBreak();	
}

void __cdecl _CrtMemCheckpoint( _CrtMemState *pState )
{
	// FIXME: Remove this when we re-implement the heap
	g_pMemAlloc->CrtMemCheckpoint( pState );
}

void __cdecl _CrtMemDumpAllObjectsSince( const _CrtMemState *pState )
{
	DebuggerBreak();
}

void __cdecl _CrtDoForAllClientObjects( void (*pfn)(void *, void *), void * pContext )
{
	DebuggerBreak();
}


//-----------------------------------------------------------------------------
// Methods in dbgrpt.cpp 
//-----------------------------------------------------------------------------
long _crtAssertBusy = -1;

int __cdecl _CrtSetReportMode( int nReportType, int nReportMode )
{
	return g_pMemAlloc->CrtSetReportMode( nReportType, nReportMode );
}

_HFILE __cdecl _CrtSetReportFile( int nRptType, _HFILE hFile )
{
	return (_HFILE)g_pMemAlloc->CrtSetReportFile( nRptType, hFile );
}

_CRT_REPORT_HOOK __cdecl _CrtSetReportHook( _CRT_REPORT_HOOK pfnNewHook )
{
	return (_CRT_REPORT_HOOK)g_pMemAlloc->CrtSetReportHook( pfnNewHook );
}

int __cdecl _CrtDbgReport( int nRptType, const char * szFile,
        int nLine, const char * szModule, const char * szFormat, ... )
{
	static char output[1024];
	va_list args;
	if ( szFormat )
	{
		va_start( args, szFormat );
		_vsnprintf( output, sizeof( output )-1, szFormat, args );
		va_end( args );
	}
	else
	{
		output[0] = 0;
	}

	return g_pMemAlloc->CrtDbgReport( nRptType, szFile, nLine, szModule, output );
}

#if _MSC_VER >= 1400

// Configure VS so that it will record crash dumps on pure-call violations
// and invalid parameter handlers.
// If you manage to call a pure-virtual function (easily done if you indirectly
// call a pure-virtual function from the base-class constructor or destructor)
// or if you invoke the invalid parameter handler (printf(NULL); is one way)
// then no crash dump will be created.
// This crash redirects the handlers for these two events so that crash dumps
// are created.
//
// The ErrorHandlerRegistrar object must be in memoverride.cpp so that it will
// be placed in every DLL and EXE. This is required because each DLL and EXE
// gets its own copy of the C run-time and these overrides are set on a per-CRT
// basis.

/*
// This sample code will cause pure-call and invalid_parameter violations and
// was used for testing:
class Base
{
public:
	virtual void PureFunction() = 0;

	Base()
	{
		NonPureFunction();
	}

	void NonPureFunction()
	{
		PureFunction();
	}
};

class Derived : public Base
{
public:
	void PureFunction() OVERRIDE
	{
	}
};

void PureCallViolation()
{
	Derived derived;
}

void InvalidParameterViolation()
{
	printf( NULL );
}
*/

#include <stdlib.h>
#include "minidump.h"

// Disable compiler optimizations. If we don't do this then VC++ generates code
// that confuses the Visual Studio debugger and causes it to display completely
// random call stacks. That makes the minidumps excruciatingly hard to understand.
#pragma optimize("", off)

// Write a minidump file, unless running under the debugger in which case break
// into the debugger.
// The "int dummy" parameter is so that the callers can be unique so that the
// linker won't use its /opt:icf optimization to collapse them together. This
// makes reading the call stack easier.
void __cdecl WriteMiniDumpOrBreak( int dummy, const char *pchName )
{
	if ( Plat_IsInDebugSession() )
	{
		__debugbreak();
		// Continue at your peril...
	}
	else
	{
		WriteMiniDump( pchName );
		// Call Plat_ExitProcess so we don't continue in a bad state. 
		TerminateProcess(GetCurrentProcess(), 0);
	}
}

void __cdecl VPureCall()
{
	WriteMiniDumpOrBreak( 0, "PureClass" );
}

void VInvalidParameterHandler(const wchar_t* expression,
   const wchar_t* function, 
   const wchar_t* file, 
   unsigned int line, 
   uintptr_t pReserved)
{
	WriteMiniDumpOrBreak( 1, "InvalidParameterHandler" );
}

// Restore compiler optimizations.
#pragma optimize("", on)

// Helper class for registering error callbacks. See above for details.
class ErrorHandlerRegistrar
{
public:
	ErrorHandlerRegistrar();
} s_ErrorHandlerRegistration;

ErrorHandlerRegistrar::ErrorHandlerRegistrar()
{
	_set_purecall_handler( VPureCall );
	_set_invalid_parameter_handler( VInvalidParameterHandler );
}

#if defined( _DEBUG )
 
// wrapper which passes no debug info; not available in debug
#ifndef	SUPPRESS_INVALID_PARAMETER_NO_INFO
void __cdecl _invalid_parameter_noinfo(void)
{
    Assert(0);
}
#endif

#endif /* defined( _DEBUG ) */

#if defined( _DEBUG ) || defined( USE_MEM_DEBUG )

int __cdecl __crtMessageWindowW( int nRptType, const wchar_t * szFile, const wchar_t * szLine,
								 const wchar_t * szModule, const wchar_t * szUserMessage )
{
	Assert(0);
	return 0;
}

int __cdecl _CrtDbgReportV( int nRptType, const wchar_t *szFile, int nLine, 
						    const wchar_t *szModule, const wchar_t *szFormat, va_list arglist )
{
	Assert(0);
	return 0;
}

int __cdecl _CrtDbgReportW( int nRptType, const wchar_t *szFile, int nLine, 
						    const wchar_t *szModule, const wchar_t *szFormat, ...)
{
	Assert(0);
	return 0;
}

int __cdecl _VCrtDbgReportA( int nRptType, const wchar_t * szFile, int nLine, 
							 const wchar_t * szModule, const wchar_t * szFormat, va_list arglist )
{
	Assert(0);
	return 0;
}

int __cdecl _CrtSetReportHook2( int mode, _CRT_REPORT_HOOK pfnNewHook )
{
	_CrtSetReportHook( pfnNewHook );
	return 0;
}
 

#endif  /* defined( _DEBUG ) || defined( USE_MEM_DEBUG ) */

extern "C" int __crtDebugCheckCount = FALSE;

extern "C" int __cdecl _CrtSetCheckCount( int fCheckCount )
{
    int oldCheckCount = __crtDebugCheckCount;
    return oldCheckCount;
}

extern "C" int __cdecl _CrtGetCheckCount( void )
{
    return __crtDebugCheckCount;
}

// aligned offset debug
extern "C" void * __cdecl _aligned_offset_recalloc_dbg( void * memblock, size_t count, size_t size, size_t align, size_t offset, const char * f_name, int line_n )
{
	Assert( IsPC() || 0 );
	void *pMem = ReallocUnattributed( memblock, size * count );
	memset( pMem, 0, size * count );
	return pMem;
}

extern "C" void * __cdecl _aligned_recalloc_dbg( void *memblock, size_t count, size_t size, size_t align, const char * f_name, int line_n )
{
    return _aligned_offset_recalloc_dbg(memblock, count, size, align, 0, f_name, line_n);
}

extern "C" void * __cdecl _recalloc_dbg ( void * memblock, size_t count, size_t size, int nBlockUse, const char * szFileName, int nLine )
{
	return _aligned_offset_recalloc_dbg(memblock, count, size, 0, 0, szFileName, nLine);
}

_CRT_REPORT_HOOK __cdecl _CrtGetReportHook( void )
{
	return NULL;
}

#endif
int __cdecl _CrtReportBlockType(const void * pUserData)
{
	return 0;
}


} // end extern "C"
#endif // _WIN32

// Most files include this file, so when it's used it adds an extra .ValveDbg section,
// to help identify debug binaries.
#ifdef _WIN32
	#ifndef NDEBUG // _DEBUG
		#pragma data_seg("ValveDBG") 
		volatile const char* DBG = "*** DEBUG STUB ***";                     
	#endif
#endif

#endif

// Extras added prevent dbgheap.obj from being included - DAL
#ifdef _WIN32

extern "C"
{
size_t __crtDebugFillThreshold = 0;

extern "C" void * __cdecl _heap_alloc_base (size_t size) {
    assert(0);
	return NULL;
}


void * __cdecl _heap_alloc_dbg( size_t nSize, int nBlockUse, const char * szFileName, int nLine)
{
		return _heap_alloc(nSize);
}

// 64-bit
#ifdef _WIN64
static void * __cdecl realloc_help( void * pUserData, size_t * pnNewSize, int nBlockUse,const char * szFileName,
				int nLine, int fRealloc )
{
		assert(0); // Shouldn't be needed
		return NULL;
}
#else
static void * __cdecl realloc_help( void * pUserData, size_t nNewSize, int nBlockUse, const char * szFileName,
                  int nLine, int fRealloc)
{
		assert(0); // Shouldn't be needed
		return NULL;
}
#endif

void __cdecl _free_nolock( void * pUserData)
{
		// I don't think the second param is used in memoverride
        _free_dbg(pUserData, 0);
}

void __cdecl _free_dbg_nolock( void * pUserData, int nBlockUse)
{
        _free_dbg(pUserData, 0);
}

_CRT_ALLOC_HOOK __cdecl _CrtGetAllocHook ( void)
{
		assert(0); 
        return NULL;
}

static int __cdecl CheckBytes( unsigned char * pb, unsigned char bCheck, size_t nSize)
{
        int bOkay = TRUE;
        return bOkay;
}


_CRT_DUMP_CLIENT __cdecl _CrtGetDumpClient ( void)
{
		assert(0); 
        return NULL;
}

#if _MSC_VER >= 1400
static void __cdecl _printMemBlockData( _locale_t plocinfo, _CrtMemBlockHeader * pHead)
{
}

static void __cdecl _CrtMemDumpAllObjectsSince_stat( const _CrtMemState * state, _locale_t plocinfo)
{
}
#endif
void * __cdecl _aligned_malloc_dbg( size_t size, size_t align, const char * f_name, int line_n)
{
    return _aligned_malloc(size, align);
}

void * __cdecl _aligned_realloc_dbg( void *memblock, size_t size, size_t align,
               const char * f_name, int line_n)
{
    return _aligned_realloc(memblock, size, align);
}

void * __cdecl _aligned_offset_malloc_dbg( size_t size, size_t align, size_t offset,
              const char * f_name, int line_n)
{
    return _aligned_offset_malloc(size, align, offset);
}

void * __cdecl _aligned_offset_realloc_dbg( void * memblock, size_t size, size_t align, 
                 size_t offset, const char * f_name, int line_n)
{
    return _aligned_offset_realloc(memblock, size, align, offset);
}

void __cdecl _aligned_free_dbg( void * memblock)
{
    _aligned_free(memblock);
}

size_t __cdecl _CrtSetDebugFillThreshold( size_t _NewDebugFillThreshold)
{
	assert(0);
    return 0;
}

//===========================================
// NEW!!! 64-bit

char * __cdecl _strdup ( const char * string )
{
	int nSize = (int)strlen(string) + 1;
	// Check for integer underflow.
	if ( nSize <= 0 )
		return NULL;
	char *pCopy = (char*)AllocUnattributed( nSize );
	if ( pCopy )
		memcpy( pCopy, string, nSize );
	return pCopy;
}

#if 0
_TSCHAR * __cdecl _tfullpath_dbg ( _TSCHAR *UserBuf, const _TSCHAR *path, size_t maxlen, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return NULL;
}

_TSCHAR * __cdecl _tfullpath ( _TSCHAR *UserBuf, const _TSCHAR *path, size_t maxlen )
{
	Assert(0);
	return NULL;
}

_TSCHAR * __cdecl _tgetdcwd_lk_dbg ( int drive, _TSCHAR *pnbuf, int maxlen, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return NULL;
}

_TSCHAR * __cdecl _tgetdcwd_nolock ( int drive, _TSCHAR *pnbuf, int maxlen )
{
	Assert(0);
	return NULL;
}

errno_t __cdecl _tdupenv_s_helper ( _TSCHAR **pBuffer, size_t *pBufferSizeInTChars, const _TSCHAR *varname, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return 0;
}

errno_t __cdecl _tdupenv_s_helper ( _TSCHAR **pBuffer, size_t *pBufferSizeInTChars, const _TSCHAR *varname )
{
	Assert(0);
	return 0;
}

_TSCHAR * __cdecl _ttempnam_dbg ( const _TSCHAR *dir, const _TSCHAR *pfx, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return 0;
}

_TSCHAR * __cdecl _ttempnam ( const _TSCHAR *dir, const _TSCHAR *pfx )
{
	Assert(0);
	return 0;
}
#endif

wchar_t * __cdecl _wcsdup_dbg ( const wchar_t * string, int nBlockUse, const char * szFileName, int nLine )
{
	Assert(0);
	return 0;
}

wchar_t * __cdecl _wcsdup ( const wchar_t * string )
{
	Assert(0);
	return 0;
}

} // end extern "C"

#if _MSC_VER >= 1400

//-----------------------------------------------------------------------------
// 	XBox Memory Allocator Override
//-----------------------------------------------------------------------------
#if defined( _X360 )
#if defined( _DEBUG ) || defined( USE_MEM_DEBUG )
#include "utlmap.h"

MEMALLOC_DEFINE_EXTERNAL_TRACKING( XMem );

CThreadFastMutex g_XMemAllocMutex;

void XMemAlloc_RegisterAllocation( void *p, DWORD dwAllocAttributes )
{
	if ( !g_pMemAlloc )
	{
		// core xallocs cannot be journaled until system is ready
		return;
	}

	AUTO_LOCK_FM( g_XMemAllocMutex );
	int size = XMemSize( p, dwAllocAttributes );
	MemAlloc_RegisterExternalAllocation( XMem, p, size );
}

void XMemAlloc_RegisterDeallocation( void *p, DWORD dwAllocAttributes )
{
	if ( !g_pMemAlloc )
	{
		// core xallocs cannot be journaled until system is ready
		return;
	}

	AUTO_LOCK_FM( g_XMemAllocMutex );
	int size = XMemSize( p, dwAllocAttributes );
	MemAlloc_RegisterExternalDeallocation( XMem, p, size );
}

#else

#define XMemAlloc_RegisterAllocation( p, a )	((void)0)
#define XMemAlloc_RegisterDeallocation( p, a )	((void)0)

#endif

//-----------------------------------------------------------------------------
//	XMemAlloc
//
//	XBox Memory Allocator Override
//-----------------------------------------------------------------------------
LPVOID WINAPI XMemAlloc( SIZE_T dwSize, DWORD dwAllocAttributes )
{
	LPVOID	ptr;
	XALLOC_ATTRIBUTES *pAttribs = (XALLOC_ATTRIBUTES *)&dwAllocAttributes;
	bool bPhysical = ( pAttribs->dwMemoryType == XALLOC_MEMTYPE_PHYSICAL );

	if ( !bPhysical && !pAttribs->dwHeapTracksAttributes && pAttribs->dwAllocatorId != eXALLOCAllocatorId_XUI )
	{
		MEM_ALLOC_CREDIT();
		switch ( pAttribs->dwAlignment )
		{
		case XALLOC_ALIGNMENT_4:
			ptr = g_pMemAlloc->Alloc( dwSize );
			break;
		case XALLOC_ALIGNMENT_8:
			ptr = MemAlloc_AllocAligned( dwSize, 8 );
			break;
		case XALLOC_ALIGNMENT_DEFAULT:
		case XALLOC_ALIGNMENT_16:
		default:
			ptr = MemAlloc_AllocAligned( dwSize, 16 );
			break;
		}
		if ( pAttribs->dwZeroInitialize != 0 )
		{
			memset( ptr, 0, XMemSize( ptr, dwAllocAttributes ) );
		}
		return ptr;
	}

	ptr = XMemAllocDefault( dwSize, dwAllocAttributes );
	if ( ptr )
	{
		XMemAlloc_RegisterAllocation( ptr, dwAllocAttributes );
	}

	return ptr;
}

//-----------------------------------------------------------------------------
//	XMemFree
//
//	XBox Memory Allocator Override
//-----------------------------------------------------------------------------
VOID WINAPI XMemFree( PVOID pAddress, DWORD dwAllocAttributes )
{
	if ( !pAddress )
	{
		return;
	}

	XALLOC_ATTRIBUTES *pAttribs = (XALLOC_ATTRIBUTES *)&dwAllocAttributes;
	bool bPhysical = ( pAttribs->dwMemoryType == XALLOC_MEMTYPE_PHYSICAL );

	if ( !bPhysical && !pAttribs->dwHeapTracksAttributes && pAttribs->dwAllocatorId != eXALLOCAllocatorId_XUI )
	{
		switch ( pAttribs->dwAlignment )
		{
		case XALLOC_ALIGNMENT_4:
			return g_pMemAlloc->Free( pAddress );
		default:
			return MemAlloc_FreeAligned( pAddress );
		}
		return;
	}

	XMemAlloc_RegisterDeallocation( pAddress, dwAllocAttributes );

	XMemFreeDefault( pAddress, dwAllocAttributes );
}

//-----------------------------------------------------------------------------
//	XMemSize
//
//	XBox Memory Allocator Override
//-----------------------------------------------------------------------------
SIZE_T WINAPI XMemSize( PVOID pAddress, DWORD dwAllocAttributes )
{
	XALLOC_ATTRIBUTES *pAttribs = (XALLOC_ATTRIBUTES *)&dwAllocAttributes;
	bool bPhysical = ( pAttribs->dwMemoryType == XALLOC_MEMTYPE_PHYSICAL );

	if ( !bPhysical && !pAttribs->dwHeapTracksAttributes && pAttribs->dwAllocatorId != eXALLOCAllocatorId_XUI )
	{
		switch ( pAttribs->dwAlignment )
		{
		case XALLOC_ALIGNMENT_4:
			return g_pMemAlloc->GetSize( pAddress );
		default:
			return MemAlloc_GetSizeAligned( pAddress );
		}
	}

	return XMemSizeDefault( pAddress, dwAllocAttributes );
}
#endif // _X360

#define MAX_LANG_LEN        64  /* max language name length */
#define MAX_CTRY_LEN        64  /* max country name length */
#define MAX_MODIFIER_LEN    0   /* max modifier name length - n/a */
#define MAX_LC_LEN          (MAX_LANG_LEN+MAX_CTRY_LEN+MAX_MODIFIER_LEN+3)

#if _MSC_VER >= 1700 // VS 11
// Copied from C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\crt\src\mtdll.h
#ifndef _SETLOC_STRUCT_DEFINED
struct _is_ctype_compatible {
        unsigned long id;
        int is_clike;
};

typedef struct setloc_struct {
    /* getqloc static variables */
    wchar_t *pchLanguage;
    wchar_t *pchCountry;
    int iLocState;
    int iPrimaryLen;
    BOOL bAbbrevLanguage;
    BOOL bAbbrevCountry;
    UINT        _cachecp;
    wchar_t     _cachein[MAX_LC_LEN];
    wchar_t     _cacheout[MAX_LC_LEN];
    /* _setlocale_set_cat (LC_CTYPE) static variable */
    struct _is_ctype_compatible _Loc_c[5];
    wchar_t _cacheLocaleName[LOCALE_NAME_MAX_LENGTH];
} _setloc_struct, *_psetloc_struct;
#define _SETLOC_STRUCT_DEFINED
#endif  /* _SETLOC_STRUCT_DEFINED */

_CRTIMP extern unsigned long __cdecl __threadid(void);
#define _threadid   (__threadid())
_CRTIMP extern uintptr_t __cdecl __threadhandle(void);
#define _threadhandle   (__threadhandle())

/* Structure for each thread's data */

struct _tiddata {
    unsigned long   _tid;       /* thread ID */


    uintptr_t _thandle;         /* thread handle */

    int     _terrno;            /* errno value */
    unsigned long   _tdoserrno; /* _doserrno value */
    unsigned int    _fpds;      /* Floating Point data segment */
    unsigned long   _holdrand;  /* rand() seed value */
    char *      _token;         /* ptr to strtok() token */
    wchar_t *   _wtoken;        /* ptr to wcstok() token */
    unsigned char * _mtoken;    /* ptr to _mbstok() token */

    /* following pointers get malloc'd at runtime */
    char *      _errmsg;        /* ptr to strerror()/_strerror() buff */
    wchar_t *   _werrmsg;       /* ptr to _wcserror()/__wcserror() buff */
    char *      _namebuf0;      /* ptr to tmpnam() buffer */
    wchar_t *   _wnamebuf0;     /* ptr to _wtmpnam() buffer */
    char *      _namebuf1;      /* ptr to tmpfile() buffer */
    wchar_t *   _wnamebuf1;     /* ptr to _wtmpfile() buffer */
    char *      _asctimebuf;    /* ptr to asctime() buffer */
    wchar_t *   _wasctimebuf;   /* ptr to _wasctime() buffer */
    void *      _gmtimebuf;     /* ptr to gmtime() structure */
    char *      _cvtbuf;        /* ptr to ecvt()/fcvt buffer */
    unsigned char _con_ch_buf[MB_LEN_MAX];
                                /* ptr to putch() buffer */
    unsigned short _ch_buf_used;   /* if the _con_ch_buf is used */

    /* following fields are needed by _beginthread code */
    void *      _initaddr;      /* initial user thread address */
    void *      _initarg;       /* initial user thread argument */

    /* following three fields are needed to support signal handling and
     * runtime errors */
    void *      _pxcptacttab;   /* ptr to exception-action table */
    void *      _tpxcptinfoptrs; /* ptr to exception info pointers */
    int         _tfpecode;      /* float point exception code */

    /* pointer to the copy of the multibyte character information used by
     * the thread */
    pthreadmbcinfo  ptmbcinfo;

    /* pointer to the copy of the locale informaton used by the thead */
    pthreadlocinfo  ptlocinfo;
    int         _ownlocale;     /* if 1, this thread owns its own locale */

    /* following field is needed by NLG routines */
    unsigned long   _NLG_dwCode;

    /*
     * Per-Thread data needed by C++ Exception Handling
     */
    void *      _terminate;     /* terminate() routine */
    void *      _unexpected;    /* unexpected() routine */
    void *      _translator;    /* S.E. translator */
    void *      _purecall;      /* called when pure virtual happens */
    void *      _curexception;  /* current exception */
    void *      _curcontext;    /* current exception context */
    int         _ProcessingThrow; /* for uncaught_exception */
    void *      _curexcspec;    /* for handling exceptions thrown from std::unexpected */
#if defined (_M_X64) || defined (_M_ARM)
    void *      _pExitContext;
    void *      _pUnwindContext;
    void *      _pFrameInfoChain;
#if defined (_WIN64)
    unsigned __int64    _ImageBase;
    unsigned __int64    _ThrowImageBase;
#else  /* defined (_WIN64) */
    unsigned __int32    _ImageBase;
    unsigned __int32    _ThrowImageBase;
#endif  /* defined (_WIN64) */
    void *      _pForeignException;
#elif defined (_M_IX86)
    void *      _pFrameInfoChain;
#endif  /* defined (_M_IX86) */
    _setloc_struct _setloc_data;

    void *      _reserved1;     /* nothing */
    void *      _reserved2;     /* nothing */
    void *      _reserved3;     /* nothing */
#ifdef _M_IX86
    void *      _reserved4;     /* nothing */
    void *      _reserved5;     /* nothing */
#endif  /* _M_IX86 */

    int _cxxReThrow;        /* Set to True if it's a rethrown C++ Exception */

    unsigned long __initDomain;     /* initial domain used by _beginthread[ex] for managed function */
};
#else
struct _is_ctype_compatible {
        unsigned long id;
        int is_clike;
};

typedef struct setloc_struct {
    /* getqloc static variables */
    char *pchLanguage;
    char *pchCountry;
    int iLcidState;
    int iPrimaryLen;
    BOOL bAbbrevLanguage;
    BOOL bAbbrevCountry;
    LCID lcidLanguage;
    LCID lcidCountry;
    /* expand_locale static variables */
    LC_ID       _cacheid;
    UINT        _cachecp;
    char        _cachein[MAX_LC_LEN];
    char        _cacheout[MAX_LC_LEN];
    /* _setlocale_set_cat (LC_CTYPE) static variable */
    struct _is_ctype_compatible _Lcid_c[5];
} _setloc_struct, *_psetloc_struct;

struct _tiddata {
    unsigned long   _tid;       /* thread ID */


    uintptr_t _thandle;         /* thread handle */

    int     _terrno;            /* errno value */
    unsigned long   _tdoserrno; /* _doserrno value */
    unsigned int    _fpds;      /* Floating Point data segment */
    unsigned long   _holdrand;  /* rand() seed value */
    char *      _token;         /* ptr to strtok() token */
    wchar_t *   _wtoken;        /* ptr to wcstok() token */
    unsigned char * _mtoken;    /* ptr to _mbstok() token */

    /* following pointers get malloc'd at runtime */
    char *      _errmsg;        /* ptr to strerror()/_strerror() buff */
    wchar_t *   _werrmsg;       /* ptr to _wcserror()/__wcserror() buff */
    char *      _namebuf0;      /* ptr to tmpnam() buffer */
    wchar_t *   _wnamebuf0;     /* ptr to _wtmpnam() buffer */
    char *      _namebuf1;      /* ptr to tmpfile() buffer */
    wchar_t *   _wnamebuf1;     /* ptr to _wtmpfile() buffer */
    char *      _asctimebuf;    /* ptr to asctime() buffer */
    wchar_t *   _wasctimebuf;   /* ptr to _wasctime() buffer */
    void *      _gmtimebuf;     /* ptr to gmtime() structure */
    char *      _cvtbuf;        /* ptr to ecvt()/fcvt buffer */
    unsigned char _con_ch_buf[MB_LEN_MAX];
                                /* ptr to putch() buffer */
    unsigned short _ch_buf_used;   /* if the _con_ch_buf is used */

    /* following fields are needed by _beginthread code */
    void *      _initaddr;      /* initial user thread address */
    void *      _initarg;       /* initial user thread argument */

    /* following three fields are needed to support signal handling and
     * runtime errors */
    void *      _pxcptacttab;   /* ptr to exception-action table */
    void *      _tpxcptinfoptrs; /* ptr to exception info pointers */
    int         _tfpecode;      /* float point exception code */

    /* pointer to the copy of the multibyte character information used by
     * the thread */
    pthreadmbcinfo  ptmbcinfo;

    /* pointer to the copy of the locale informaton used by the thead */
    pthreadlocinfo  ptlocinfo;
    int         _ownlocale;     /* if 1, this thread owns its own locale */

    /* following field is needed by NLG routines */
    unsigned long   _NLG_dwCode;

    /*
     * Per-Thread data needed by C++ Exception Handling
     */
    void *      _terminate;     /* terminate() routine */
    void *      _unexpected;    /* unexpected() routine */
    void *      _translator;    /* S.E. translator */
    void *      _purecall;      /* called when pure virtual happens */
    void *      _curexception;  /* current exception */
    void *      _curcontext;    /* current exception context */
    int         _ProcessingThrow; /* for uncaught_exception */
    void *              _curexcspec;    /* for handling exceptions thrown from std::unexpected */
#if defined (_M_IA64) || defined (_M_AMD64)
    void *      _pExitContext;
    void *      _pUnwindContext;
    void *      _pFrameInfoChain;
    unsigned __int64    _ImageBase;
#if defined (_M_IA64)
    unsigned __int64    _TargetGp;
#endif  /* defined (_M_IA64) */
    unsigned __int64    _ThrowImageBase;
    void *      _pForeignException;
#elif defined (_M_IX86)
    void *      _pFrameInfoChain;
#endif  /* defined (_M_IX86) */
    _setloc_struct _setloc_data;

    void *      _encode_ptr;    /* EncodePointer() routine */
    void *      _decode_ptr;    /* DecodePointer() routine */

    void *      _reserved1;     /* nothing */
    void *      _reserved2;     /* nothing */
    void *      _reserved3;     /* nothing */

    int _cxxReThrow;        /* Set to True if it's a rethrown C++ Exception */

    unsigned long __initDomain;     /* initial domain used by _beginthread[ex] for managed function */
};
#endif

typedef struct _tiddata * _ptiddata;

class _LocaleUpdate
{
    _locale_tstruct localeinfo;
    _ptiddata ptd;
    bool updated;
    public:
    _LocaleUpdate(_locale_t plocinfo)
        : updated(false)
    {
		/*
        if (plocinfo == NULL)
        {
            ptd = _getptd();
            localeinfo.locinfo = ptd->ptlocinfo;
            localeinfo.mbcinfo = ptd->ptmbcinfo;

            __UPDATE_LOCALE(ptd, localeinfo.locinfo);
            __UPDATE_MBCP(ptd, localeinfo.mbcinfo);
            if (!(ptd->_ownlocale & _PER_THREAD_LOCALE_BIT))
            {
                ptd->_ownlocale |= _PER_THREAD_LOCALE_BIT;
                updated = true;
            }
        }
        else
        {
            localeinfo=*plocinfo;
        }
		*/
    }
    ~_LocaleUpdate()
    {
//        if (updated)
//	        ptd->_ownlocale = ptd->_ownlocale & ~_PER_THREAD_LOCALE_BIT;
    }
    _locale_t GetLocaleT()
    {
        return &localeinfo;
    }
};


#pragma warning(push)
#pragma warning(disable: 4483)
#if _MSC_FULL_VER >= 140050415
#define _NATIVE_STARTUP_NAMESPACE  __identifier("<CrtImplementationDetails>")
#else  /* _MSC_FULL_VER >= 140050415 */
#define _NATIVE_STARTUP_NAMESPACE __CrtImplementationDetails
#endif  /* _MSC_FULL_VER >= 140050415 */

namespace _NATIVE_STARTUP_NAMESPACE
{
    class NativeDll
    {
    private:
        static const unsigned int ProcessDetach   = 0;
        static const unsigned int ProcessAttach   = 1;
        static const unsigned int ThreadAttach    = 2;
        static const unsigned int ThreadDetach    = 3;
        static const unsigned int ProcessVerifier = 4;

    public:

        inline static bool IsInDllMain()
        {
            return false;
        }

        inline static bool IsInProcessAttach()
        {
            return false;
        }

        inline static bool IsInProcessDetach()
        {
            return false;
        }

        inline static bool IsInVcclrit()
        {
            return false;
        }

        inline static bool IsSafeForManagedCode()
        {
            if (!IsInDllMain())
            {
                return true;
            }

            if (IsInVcclrit())
            {
                return true;
            }

            return !IsInProcessAttach() && !IsInProcessDetach();
        }
    };
}
#pragma warning(pop)

#endif // _MSC_VER >= 1400

#endif // !STEAM && !NO_MALLOC_OVERRIDE

#endif // _WIN32

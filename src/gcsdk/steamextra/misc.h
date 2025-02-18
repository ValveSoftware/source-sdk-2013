//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Miscellaneous platform-specific code
//
//=============================================================================


#ifndef MISC_H
#define MISC_H

// Random number utilities
uint32 UNRandFast();
char CHRandFast();
void SetRandSeed( uint64 ulRandSeed );
uint64 GetRandSeed();
void RandMem(void *dest, int count);

bool IsVTTAccountName( const char *szAccountName );

float SafeCalcPct( uint64 ulNumerator, uint64 ulDenominator );

bool BRejectDueToBacklog( int nBacklogCur, int nBacklogThreshold, int nBacklogLimit, int iItem );

#define SAFE_CLOSE_HANDLE( x )   if ( INVALID_HANDLE_VALUE != ( x ) ) {  CloseHandle( x ); ( x ) = INVALID_HANDLE_VALUE; }
#define SAFE_DELETE( x )		 if ( NULL != ( x ) ) { delete ( x ); ( x ) = NULL; }
#define SAFE_CLOSE_HCONNECTION( x )   if ( 0 != ( x ) ) { CNet::BClose( ( x ) ); ( x ) = 0; }
#define SAFE_RELEASE( x )		if ( NULL != ( x ) ) { ( x )->Release(); x = NULL; }

#define DECLARE_STEAM_CLASS_SIMPLE( className, baseClassName ) \
	typedef baseClassName BaseClass; \
	typedef className ThisClass;	\

#define DECLARE_STEAM_CLASS_NOBASE( className ) \
	typedef className ThisClass;	\

// Type for our memory output debugging.
class IDisplayMemPoolStats
{
public:
	virtual void Display( const char *pszClassName, uint64 ulClassSize, uint32 unPoolInstanceCount, uint64 ulPoolMemoryUsage, uint32 unPoolPeakInstanceCount, uint64 ulPoolPeakMemoryUsage ) = 0;
};

// A class for registering functions that will print memory usage information
typedef void (*DumpMemFn_t)( IDisplayMemPoolStats * );
class CDumpMemFnReg
{
public:
	static CDumpMemFnReg *sm_Head;

	CDumpMemFnReg( DumpMemFn_t fn )
		: m_fn( fn ),
		m_pNext( sm_Head )
	{
		sm_Head = this;
	}

	DumpMemFn_t m_fn;
	CDumpMemFnReg *m_pNext;
};

// Helper macros for creating and using CClassMemoryPool on our frequently allocated objects
#define DECLARE_CLASS_MEMPOOL( className )										\
private:																		\
	static CUtlMemoryPool sm_classMemPool;										\
public:																			\
	static void* operator new ( size_t nSize );									\
	static void* operator new ( size_t nSize, int nBlockUse, const char *pFileName, int nLine );	\
	static void operator delete ( void *pMem );									\
	static void DumpMemStats( IDisplayMemPoolStats *pDisplayer );				\


#define IMPLEMENT_CLASS_MEMPOOL( className, initSize, growMode )				\
CUtlMemoryPool className::sm_classMemPool( sizeof( className ), ( initSize ), ( growMode ), MEM_ALLOC_CLASSNAME( className ) );	\
																				\
void* className::operator new ( size_t nSize )									\
{																				\
	if ( nSize != sizeof( className ) )											\
	{																			\
		EmitError( SPEW_CONSOLE, #className"::operator new() called on wrong size! Expected %llu, Got %llu\n", (uint64)sizeof( className ), (uint64)nSize );	\
		return NULL;															\
	}																			\
																				\
	return sm_classMemPool.Alloc();												\
}																				\
																				\
void* className::operator new ( size_t nSize, int nBlockUse, const char *pFileName, int nLine )	\
{																				\
	if ( nSize != sizeof( className ) )											\
	{																			\
		EmitError( SPEW_CONSOLE, #className"::operator new() called on wrong size! Expected %llu, Got %llu\n", (uint64)sizeof( className ), (uint64)nSize );	\
		return NULL;															\
	}																			\
																				\
	return sm_classMemPool.Alloc();												\
}																				\
																				\
void className::operator delete ( void *pMem )									\
{																				\
	sm_classMemPool.Free( (className *)pMem );									\
}																				\
																				\
void className::DumpMemStats( IDisplayMemPoolStats *pDisplayer )				\
{																				\
	Assert( pDisplayer );														\
	pDisplayer->Display															\
	(																			\
		#className,																\
		sizeof( className ),													\
		sm_classMemPool.Count(),												\
		sm_classMemPool.Count() * sizeof( className ),							\
		( ( sm_classMemPool.PeakCount() + ( initSize ) ) / ( initSize ) ) * ( initSize ) , \
		( ( sm_classMemPool.PeakCount() + ( initSize ) ) / ( initSize ) ) * ( initSize ) * sizeof( className )	\
	);																			\
}																				\
																				\
static CDumpMemFnReg s_##className##RegDumpMemory( &className::DumpMemStats );

// useful macro for rendering an IP
#define iptod(x) ((x)>>24&0xff), ((x)>>16&0xff), ((x)>>8&0xff), ((x)&0xff)

#endif // MISC_H

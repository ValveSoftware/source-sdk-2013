//========= Copyright Valve Corporation, All rights reserved. ============//
#ifndef GLMDEBUG_H
#define	GLMDEBUG_H

#include "tier0/platform.h"

// include this anywhere you need to be able to compile-out code related specifically to GLM debugging.

// we expect DEBUG to be driven by the build system so you can include this header anywhere.
// when we come out, GLMDEBUG will be defined to a value - 0, 1, or 2
// 0 means no GLM debugging is possible
// 1 means it's possible and resulted from being a debug build
// 2 means it's possible and resulted from being manually forced on for a release build

#ifdef POSIX
	#ifndef GLMDEBUG
		#ifdef DEBUG
			#define GLMDEBUG 1	// normally 1 here, testing
		#else
			// #define GLMDEBUG 2			// don't check this in enabled..
		#endif
		
		#ifndef GLMDEBUG
			#define GLMDEBUG 0
		#endif
	#endif
#else
	#ifndef GLMDEBUG
		#define GLMDEBUG 0
	#endif
#endif


//===============================================================================
// debug channels

enum EGLMDebugChannel
{
	ePrintf,
	eDebugger,
	eGLProfiler
};

#if GLMDEBUG
	// make all these prototypes disappear in non GLMDEBUG
	void	GLMDebugInitialize( bool forceReinit=false );

	bool	GLMDetectOGLP( void );
	bool	GLMDetectGDB( void );
	uint	GLMDetectAvailableChannels( void );

	uint	GLMDebugChannelMask( uint *newValue = NULL );
		// note that GDB and OGLP can both come and go during run - forceCheck will allow that to be detected.
		// mask returned is in form of 1<<n, n from EGLMDebugChannel
#endif

//===============================================================================
// debug message flavors

enum EGLMDebugFlavor
{
	eAllFlavors,		// 0
	eDebugDump,			// 1 debug dump flavor								-D-
	eTenure,			// 2 code tenures									> <
	eComment,			// 3 one off messages								---
	eMatrixData,		// 4 matrix data									-M-
	eShaderData,		// 5 shader data (params)							-S-
	eFrameBufData,		// 6 FBO data (attachments)							-F-
	eDXStuff,			// 7 dxabstract spew								-X-
	eAllocations,		// 8 tracking allocs and frees						-A-
	eSlowness,			// 9 slow things happening (srgb flips..)			-Z-
	eDefaultFlavor,		// not specified (no marker)
	eFlavorCount
};
uint	GLMDebugFlavorMask( uint *newValue = NULL );

// make all these prototypes disappear in non GLMDEBUG
#if GLMDEBUG
		// these are unconditional outputs, they don't interrogate the string
	void	GLMStringOut( const char *string );
	void	GLMStringOutIndented( const char *string, int indentColumns );

	#ifdef TOGL_DLL_EXPORT
			// these will look at the string to guess its flavor: <, >, ---, -M-, -S- 
		DLL_EXPORT void	GLMPrintfVA( const char *fmt, va_list vargs );
		DLL_EXPORT void	GLMPrintf( const char *fmt, ... );
	#else
		DLL_IMPORT void	GLMPrintfVA( const char *fmt, va_list vargs );
		DLL_IMPORT void	GLMPrintf( const char *fmt, ... );
	#endif

		// these take an explicit flavor with a default value
	void	GLMPrintStr( const char *str, EGLMDebugFlavor flavor = eDefaultFlavor );

	#define	GLMPRINTTEXT_NUMBEREDLINES	0x80000000
	void	GLMPrintText( const char *str, EGLMDebugFlavor flavor = eDefaultFlavor, uint options=0  );			// indent each newline

	int		GLMIncIndent( int indentDelta );
	int		GLMGetIndent( void );
	void	GLMSetIndent( int indent );

#endif

// helpful macro if you are in a position to call GLM functions directly (i.e. you live in materialsystem / shaderapidx9)
#if GLMDEBUG
	#define	GLMPRINTF(args)			GLMPrintf args
	#define	GLMPRINTSTR(args)		GLMPrintStr args
	#define	GLMPRINTTEXT(args)		GLMPrintText args
#else
	#define	GLMPRINTF(args)	
	#define	GLMPRINTSTR(args)
	#define	GLMPRINTTEXT(args)
#endif


//===============================================================================
// knob twiddling
#ifdef TOGL_DLL_EXPORT
	DLL_EXPORT float	GLMKnob( char *knobname, float *setvalue );	// Pass NULL to not-set the knob value
	DLL_EXPORT float	GLMKnobToggle( char *knobname );
#else
	DLL_IMPORT float	GLMKnob( char *knobname, float *setvalue );	// Pass NULL to not-set the knob value
	DLL_IMPORT float	GLMKnobToggle( char *knobname );
#endif

//===============================================================================
// other stuff

#if GLMDEBUG
void GLMTriggerDebuggerBreak();
inline void GLMDebugger( void )
{
	if (GLMDebugChannelMask() & (1<<eDebugger))
	{
#ifdef OSX
		asm {int 3 };
#else
		DebuggerBreak();
#endif
	}
	
	if (GLMDebugChannelMask() & (1<<eGLProfiler))
	{
		GLMTriggerDebuggerBreak();
	}
}
#else
	#define	GLMDebugger() do { } while(0)
#endif

// helpers for CGLSetOption - no op if no profiler
void	GLMProfilerClearTrace( void );
void	GLMProfilerEnableTrace( bool enable );

// helpers for CGLSetParameter - no op if no profiler
void	GLMProfilerDumpState( void );

void CheckGLError( int line );

#endif // GLMDEBUG_H

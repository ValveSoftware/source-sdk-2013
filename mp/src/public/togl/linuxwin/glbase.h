//========= Copyright Valve Corporation, All rights reserved. ============//
//                       TOGL CODE LICENSE
//
//  Copyright 2011-2014 Valve Corporation
//  All Rights Reserved.
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to deal
//  in the Software without restriction, including without limitation the rights
//  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//  copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
//  THE SOFTWARE.
//
// glbase.h
//
//===============================================================================

#ifndef GLBASE_H
#define	GLBASE_H

#ifdef DX_TO_GL_ABSTRACTION

#undef HAVE_GL_ARB_SYNC

#ifndef OSX
#define HAVE_GL_ARB_SYNC 1
#endif

#ifdef USE_SDL
#include "SDL_opengl.h"
#endif

#ifdef OSX
#include <OpenGL/CGLCurrent.h>
#include <ApplicationServices/ApplicationServices.h>
#endif

#ifdef DX_TO_GL_ABSTRACTION
	#ifndef WIN32
	#define Debugger DebuggerBreak
	#endif
	#undef CurrentTime

	#if defined( USE_SDL )
		#include "SDL.h"
	#endif
#endif

//===============================================================================
// glue to call out to Obj-C land (these are in glmgrcocoa.mm)
#ifdef OSX
	typedef void _PseudoNSGLContext;					// aka NSOpenGLContext
	typedef _PseudoNSGLContext	*PseudoNSGLContextPtr;
	
	CGLContextObj	GetCGLContextFromNSGL( PseudoNSGLContextPtr nsglCtx );
#endif

// Set TOGL_SUPPORT_NULL_DEVICE to 1 to support the NULL ref device
#define TOGL_SUPPORT_NULL_DEVICE 0

#if TOGL_SUPPORT_NULL_DEVICE
	#define TOGL_NULL_DEVICE_CHECK if( m_params.m_deviceType == D3DDEVTYPE_NULLREF ) return S_OK;
	#define TOGL_NULL_DEVICE_CHECK_RET_VOID if( m_params.m_deviceType == D3DDEVTYPE_NULLREF ) return;
#else
	#define TOGL_NULL_DEVICE_CHECK
	#define TOGL_NULL_DEVICE_CHECK_RET_VOID
#endif

// GL_ENABLE_INDEX_VERIFICATION enables index range verification on all dynamic IB/VB's (obviously slow)
#define GL_ENABLE_INDEX_VERIFICATION 0

// GL_ENABLE_UNLOCK_BUFFER_OVERWRITE_DETECTION (currently win32 only) - If 1, VirtualAlloc/VirtualProtect is used to detect cases where the app locks a buffer, copies the ptr away, unlocks, then tries to later write to the buffer.
#define GL_ENABLE_UNLOCK_BUFFER_OVERWRITE_DETECTION 0

#define GL_BATCH_TELEMETRY_ZONES 0

// GL_BATCH_PERF_ANALYSIS - Enables gl_batch_vis, and various per-batch telemetry statistics messages. 
#define GL_BATCH_PERF_ANALYSIS 0
#define GL_BATCH_PERF_ANALYSIS_WRITE_PNGS 0

// GL_TELEMETRY_ZONES - Causes every single OpenGL call to generate a telemetry event
#define GL_TELEMETRY_ZONES 0

// GL_DUMP_ALL_API_CALLS - Causes a debug message to be printed for every API call if s_bDumpCalls bool is set to 1
#define GL_DUMP_ALL_API_CALLS 0

// Must also enable PIX_ENABLE to use GL_TELEMETRY_GPU_ZONES.
#define GL_TELEMETRY_GPU_ZONES 0

// Records global # of OpenGL calls/total cycles spent inside GL
#define GL_TRACK_API_TIME GL_BATCH_PERF_ANALYSIS

#define GL_USE_EXECUTE_HELPER_FOR_ALL_API_CALLS ( GL_TELEMETRY_ZONES || GL_TRACK_API_TIME || GL_DUMP_ALL_API_CALLS )

#if GL_BATCH_PERF_ANALYSIS
	#define GL_BATCH_PERF(...) __VA_ARGS__
#else
	#define GL_BATCH_PERF(...)
#endif

#define	kGLMUserClipPlanes			2
#define kGLMScratchFBOCount			4

#endif // DX_TO_GL_ABSTRACTION

#endif // GLBASE_H

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
// glmdisplay.h
// display related stuff - used by both GLMgr and the CocoaMgr
//
//===============================================================================

#ifndef GLMDISPLAY_H
#define	GLMDISPLAY_H

#pragma once

#ifdef USE_SDL
#include "SDL_opengl.h"
#endif

#ifdef OSX
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/CGLTypes.h>
#include <OpenGL/CGLRenderers.h>
#include <OpenGL/CGLCurrent.h>
#endif

#ifdef MAC_OS_X_VERSION_10_9
typedef uint32_t CGDirectDisplayID;
typedef uint32_t CGOpenGLDisplayMask;
typedef double CGRefreshRate;
#endif

typedef void _PseudoNSGLContext;					// aka NSOpenGLContext
typedef _PseudoNSGLContext	*PseudoNSGLContextPtr;

struct GLMDisplayModeInfoFields
{
	uint	m_modePixelWidth;
	uint	m_modePixelHeight;
	uint	m_modeRefreshHz;
	// are we even going to talk about bit depth... not yet
};

struct GLMDisplayInfoFields
{
#ifdef OSX
	CGDirectDisplayID				m_cgDisplayID;
	CGOpenGLDisplayMask				m_glDisplayMask;		// result of CGDisplayIDToOpenGLDisplayMask on the cg_displayID.
#endif
	uint							m_displayPixelWidth;
	uint							m_displayPixelHeight;	
};

struct GLMRendererInfoFields
{
	/*properties of interest and their desired values.
	
	   kCGLRPFullScreen          =  54,		true
	   kCGLRPAccelerated         =  73,		true
	   kCGLRPWindow              =  80,		true

	   kCGLRPRendererID          =  70,		informational
	   kCGLRPDisplayMask         =  84,		informational
	   kCGLRPBufferModes         = 100,		informational
	   kCGLRPColorModes          = 103,		informational
	   kCGLRPAccumModes          = 104,		informational
	   kCGLRPDepthModes          = 105,		informational
	   kCGLRPStencilModes        = 106,		informational
	   kCGLRPMaxAuxBuffers       = 107,		informational
	   kCGLRPMaxSampleBuffers    = 108,		informational
	   kCGLRPMaxSamples          = 109,		informational
	   kCGLRPSampleModes         = 110,		informational
	   kCGLRPSampleAlpha         = 111,		informational
	   kCGLRPVideoMemory         = 120,		informational
	   kCGLRPTextureMemory       = 121,		informational
	   kCGLRPRendererCount       = 128		number of renderers in the CGLRendererInfoObj under examination

	   kCGLRPOffScreen           =  53,		D/C
	   kCGLRPRobust              =  75,		FALSE or D/C - aka we're asking for no-fallback
	   kCGLRPBackingStore        =  76,		D/C
	   kCGLRPMPSafe              =  78,		D/C
	   kCGLRPMultiScreen         =  81,		D/C
	   kCGLRPCompliant           =  83,		D/C
	*/


	//--------------------------- info we have from CGL renderer queries, IOKit, Gestalt
	//--------------------------- these are set up in the displayDB by CocoaMgr
	GLint	m_fullscreen;
	GLint	m_accelerated;
	GLint	m_windowed;
	
	GLint	m_rendererID;
	GLint	m_displayMask;
	GLint	m_bufferModes;
	GLint	m_colorModes;
	GLint	m_accumModes;
	GLint	m_depthModes;
	GLint	m_stencilModes;

	GLint	m_maxAuxBuffers;
	GLint	m_maxSampleBuffers;
	GLint	m_maxSamples;
	GLint	m_sampleModes;
	GLint	m_sampleAlpha;

	GLint	m_vidMemory;
	GLint	m_texMemory;
	
	uint	m_pciVendorID;
	uint	m_pciDeviceID;
	char	m_pciModelString[64];
	char	m_driverInfoString[64];

	//--------------------------- OS version related - set up by CocoaMgr

	// OS version found
	uint	m_osComboVersion;			// 0x00XXYYZZ : XX major, YY minor, ZZ minor minor : 10.6.3 --> 0x000A0603.  10.5.8 --> 0x000A0508.
		
	//--------------------------- shorthands - also set up by CocoaMgr - driven by vendorid / deviceid

	bool	m_ati;
	bool	m_atiR5xx;
	bool	m_atiR6xx;
	bool	m_atiR7xx;
	bool	m_atiR8xx;
	bool	m_atiNewer;
	
	bool	m_intel;
	bool	m_intel95x;
	bool	m_intel3100;
	bool	m_intelHD4000;

	bool	m_nv;
	bool	m_nvG7x;
	bool	m_nvG8x;
	bool	m_nvNewer;
	
	//--------------------------- context query results - left blank in the display DB - but valid in a GLMContext (call ctx->Caps() to get a const ref)
	
	// booleans	
	bool	m_hasGammaWrites;			// aka glGetBooleanv(GL_FRAMEBUFFER_SRGB_CAPABLE_EXT) / glEnable(GL_FRAMEBUFFER_SRGB_EXT)
	bool	m_hasMixedAttachmentSizes;	// aka ARB_fbo in 10.6.3 - test for min OS vers, then exported ext string
	bool	m_hasBGRA;					// aka GL_BGRA vertex attribs in 10.6.3 -  - test for min OS vers, then exported ext string
	bool	m_hasNewFullscreenMode;		// aka 10.6.x "big window" fullscreen mode
	bool	m_hasNativeClipVertexMode;	// aka GLSL gl_ClipVertex does not fall back to SW- OS version and folklore-based
	bool	m_hasOcclusionQuery;		// occlusion query: do you speak it ?!
	bool	m_hasFramebufferBlit;		// framebuffer blit: know what I'm sayin?!
	bool	m_hasPerfPackage1;			// means new MTGL, fast OQ, fast uniform upload, NV can resolve flipped (late summer 2010 post 10.6.4 update)
	
	// counts
	int		m_maxAniso;					// aniso limit - context query
	
	// other exts
	bool	m_hasBindableUniforms;
	int		m_maxVertexBindableUniforms;
	int		m_maxFragmentBindableUniforms;
	int		m_maxBindableUniformSize;
	
	bool	m_hasUniformBuffers;
	
	// runtime options that aren't negotiable once set
	bool	m_hasDualShaders;			// must supply CLI arg "-glmdualshaders" or we go GLSL only

	//--------------------------- " can'ts " - specific problems that need to be worked around

	bool	m_cantBlitReliably;		// Intel chipsets have problems blitting sRGB sometimes
	bool	m_cantAttachSRGB;		// NV G8x on 10.5.8 can't have srgb tex on FBO color - separate issue from hasGammaWrites
	bool	m_cantResolveFlipped;	// happens on NV in 10.6.4 and prior - console variable "gl_can_resolve_flipped" can overrule
	bool	m_cantResolveScaled;	// happens everywhere per GL spec but may be relaxed some day - console variable "gl_can_resolve_scaled" can overrule
	bool	m_costlyGammaFlips;		// this means that sRGB sampling state affects shader code gen, resulting in state-dependent code regen


	//--------------------------- " bads " - known bad drivers
	bool	m_badDriver1064NV;		// this is the bad NVIDIA driver on 10.6.4 - stutter, tex corruption, black screen issues
	bool    m_badDriver108Intel;	// this is the bad Intel HD4000 driver on 10.8 - intermittent crash on GLSL compilation.
};



#endif

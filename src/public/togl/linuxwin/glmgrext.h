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
// glmgrext.h
// helper file for extension testing and runtime importing of entry points
//
//===============================================================================

#pragma once

#ifdef OSX
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif defined(DX_TO_GL_ABSTRACTION)
#include <GL/gl.h>
#include <GL/glext.h>
#else
#error
#endif

#ifndef GL_EXT_framebuffer_sRGB
	#define GL_FRAMEBUFFER_SRGB_EXT                 0x8DB9
	#define GL_FRAMEBUFFER_SRGB_CAPABLE_EXT         0x8DBA
#endif

#ifndef ARB_texture_rg
	#define GL_COMPRESSED_RED                 0x8225
	#define GL_COMPRESSED_RG                  0x8226
	#define GL_RG                             0x8227
	#define GL_RG_INTEGER                     0x8228
	#define GL_R8                             0x8229
	#define GL_R16                            0x822A
	#define GL_RG8                            0x822B
	#define GL_RG16                           0x822C
	#define GL_R16F                           0x822D
	#define GL_R32F                           0x822E
	#define GL_RG16F                          0x822F
	#define GL_RG32F                          0x8230
	#define GL_R8I                            0x8231
	#define GL_R8UI                           0x8232
	#define GL_R16I                           0x8233
	#define GL_R16UI                          0x8234
	#define GL_R32I                           0x8235
	#define GL_R32UI                          0x8236
	#define GL_RG8I                           0x8237
	#define GL_RG8UI                          0x8238
	#define GL_RG16I                          0x8239
	#define GL_RG16UI                         0x823A
	#define GL_RG32I                          0x823B
	#define GL_RG32UI                         0x823C
#endif

#ifndef GL_EXT_bindable_uniform
	#define GL_UNIFORM_BUFFER_EXT             0x8DEE
#endif

// unpublished extension enums (thus the "X")

// from EXT_framebuffer_multisample_blit_scaled..
#define XGL_SCALED_RESOLVE_FASTEST_EXT 0x90BA
#define XGL_SCALED_RESOLVE_NICEST_EXT 0x90BB

#ifndef GL_TEXTURE_MINIMIZE_STORAGE_APPLE
#define GL_TEXTURE_MINIMIZE_STORAGE_APPLE  0x85B6
#endif

#ifndef GL_ALL_COMPLETED_NV
#define GL_ALL_COMPLETED_NV               0x84F2
#endif

#ifndef GL_MAP_READ_BIT
#define GL_MAP_READ_BIT                   0x0001
#endif

#ifndef GL_MAP_WRITE_BIT
#define GL_MAP_WRITE_BIT                  0x0002
#endif

#ifndef GL_MAP_INVALIDATE_RANGE_BIT
#define GL_MAP_INVALIDATE_RANGE_BIT       0x0004
#endif

#ifndef GL_MAP_INVALIDATE_BUFFER_BIT
#define GL_MAP_INVALIDATE_BUFFER_BIT      0x0008
#endif

#ifndef GL_MAP_FLUSH_EXPLICIT_BIT
#define GL_MAP_FLUSH_EXPLICIT_BIT         0x0010
#endif

#ifndef GL_MAP_UNSYNCHRONIZED_BIT
#define GL_MAP_UNSYNCHRONIZED_BIT         0x0020
#endif

#ifndef GL_MAP_PERSISTENT_BIT
#define GL_MAP_PERSISTENT_BIT				0x0040
#endif

#ifndef GL_MAP_COHERENT_BIT
#define GL_MAP_COHERENT_BIT					0x0080
#endif


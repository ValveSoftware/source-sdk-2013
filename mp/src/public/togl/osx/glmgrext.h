//========= Copyright Valve Corporation, All rights reserved. ============//
//
// glmgrext.h
// helper file for extension testing and runtime importing of entry points
//
//===============================================================================

#pragma once

#ifdef OSX
#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#elif defined(LINUX)
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


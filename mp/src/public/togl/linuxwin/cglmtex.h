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
// cglmtex.h
//	GLMgr textures
//
//===============================================================================

#ifndef CGLMTEX_H
#define	CGLMTEX_H

#pragma once

#ifdef OSX
#include "glmgrbasics.h"
#endif
#include "tier1/utlhash.h"
#include "tier1/utlmap.h"

//===============================================================================

// forward declarations

class	GLMContext;
class	GLMTester;
class	CGLMTexLayoutTable;
class	CGLMTex;
class	CGLMFBO;

struct	IDirect3DSurface9;

#if GLMDEBUG
extern CGLMTex *g_pFirstCGMLTex;
#endif

// For GL_EXT_texture_sRGB_decode
#ifndef GL_TEXTURE_SRGB_DECODE_EXT
#define GL_TEXTURE_SRGB_DECODE_EXT 0x8A48
#endif

#ifndef GL_DECODE_EXT
#define GL_DECODE_EXT             0x8A49
#endif

#ifndef GL_SKIP_DECODE_EXT
#define GL_SKIP_DECODE_EXT        0x8A4A
#endif

//===============================================================================

struct GLMTexFormatDesc
{
	const char	*m_formatSummary;	// for debug visibility
	
	D3DFORMAT	m_d3dFormat;		// what D3D knows it as; see public/bitmap/imageformat.h
	
	GLenum		m_glIntFormat;		// GL internal format
	GLenum		m_glIntFormatSRGB;	// internal format if SRGB flavor
	GLenum		m_glDataFormat;		// GL data format
	GLenum		m_glDataType;		// GL data type
	
	int			m_chunkSize;		// 1 or 4 - 4 is used for compressed textures
	int			m_bytesPerSquareChunk;	// how many bytes for the smallest quantum (m_chunkSize x m_chunkSize)
									// this description lets us calculate size cleanly without conditional logic for compression
};
const GLMTexFormatDesc *GetFormatDesc( D3DFORMAT format );

//===============================================================================

// utility function for generating slabs of texels. mostly for test.
typedef struct
{
	// in
	D3DFORMAT	m_format;
	void		*m_dest;			// dest address
	int			m_chunkCount;		// square chunk count (single texels or compressed blocks)
	int			m_byteCountLimit;	// caller expectation of max number of bytes to write out
	float		r,g,b,a;			// color desired
	
	// out
	int			m_bytesWritten;
}	GLMGenTexelParams;

// return true if successful
bool	GLMGenTexels( GLMGenTexelParams *params );


//===============================================================================

struct GLMTexLayoutSlice
{
	int	m_xSize,m_ySize,m_zSize;		//texel dimensions of this slice
	int	m_storageOffset;				//where in the storage slab does this slice live
	int	m_storageSize;					//how much storage does this slice occupy
};

enum EGLMTexFlags
{
	kGLMTexMipped		=	0x01,
	kGLMTexMippedAuto	=	0x02,
	kGLMTexRenderable	=	0x04,
	kGLMTexIsStencil	=	0x08,
	kGLMTexIsDepth		=	0x10,
	kGLMTexSRGB			=	0x20,
	kGLMTexMultisampled	=	0x40,		// has an RBO backing it.  Cannot combine with Mipped, MippedAuto.  One slice maximum, only targeting GL_TEXTURE_2D.
										// actually not 100% positive on the mipmapping, the RBO itself can't be mipped, but the resulting texture could
										// have mipmaps generated.
};

//===============================================================================

struct GLMTexLayoutKey
{
	// input values: held const, these are the hash key for the form map
	GLenum				m_texGLTarget;				// flavor of texture: GL_TEXTURE_2D, GL_TEXTURE_3D, GLTEXTURE_CUBE_MAP
	D3DFORMAT			m_texFormat;				// D3D texel format
	unsigned long		m_texFlags;					// mipped, autogen mips, render target, ... ?
	unsigned long		m_texSamples;				// zero for a plain tex, 2/4/6/8 for "MSAA tex" (RBO backed)
	int					m_xSize,m_ySize,m_zSize;	// size of base mip
};

bool LessFunc_GLMTexLayoutKey( const GLMTexLayoutKey &a, const GLMTexLayoutKey &b );

#define	GLM_TEX_MAX_MIPS	14
#define	GLM_TEX_MAX_FACES	6
#define	GLM_TEX_MAX_SLICES	(GLM_TEX_MAX_MIPS * GLM_TEX_MAX_FACES)

#pragma warning( push )
#pragma warning( disable : 4200 )

struct GLMTexLayout
{
	char		*m_layoutSummary;	// for debug visibility

	// const inputs used for hashing
	GLMTexLayoutKey		m_key;
	
	// refcount
	int					m_refCount;

	// derived values:	
	GLMTexFormatDesc	*m_format;					// format specific info
	int					m_mipCount;					// derived by starying at base size and working down towards 1x1
	int					m_faceCount;				// 1 for 2d/3d, 6 for cubemap
	int					m_sliceCount;				// product of faces and mips
	int					m_storageTotalSize;			// size of storage slab required
	
	// slice array
	GLMTexLayoutSlice	m_slices[0];				// dynamically allocated 2-d array [faces][mips]
};

#pragma warning( pop )

class	CGLMTexLayoutTable
{
public:
					CGLMTexLayoutTable();
	
	GLMTexLayout	*NewLayoutRef( GLMTexLayoutKey *pDesiredKey );		// pass in a pointer to layout key - receive ptr to completed layout
	void			DelLayoutRef( GLMTexLayout *layout );		// pass in pointer to completed layout.  refcount is dropped.
	
	void			DumpStats( void );
protected:
	CUtlMap< GLMTexLayoutKey, GLMTexLayout* >	m_layoutMap;
};

//===============================================================================

// a sampler specifies desired state for drawing on a given sampler index
// this is the combination of a texture choice and a set of sampler parameters
// see http://msdn.microsoft.com/en-us/library/bb172602(VS.85).aspx

struct GLMTexLockParams
{
	// input params which identify the slice of interest
	CGLMTex		*m_tex;
	int			m_face;
	int			m_mip;
	
	// identifies the region of the slice
	GLMRegion	m_region;
	
	// tells GLM to force re-read of the texels back from GL
	// i.e. "I know I stepped on those texels with a draw or blit - the GLM copy is stale"
	bool		m_readback;
};

struct GLMTexLockDesc
{
	GLMTexLockParams	m_req;	// form of the lock request
	
	bool				m_active;				// set true at lock time. cleared at unlock time.

	int					m_sliceIndex;			// which slice in the layout
	int					m_sliceBaseOffset;		// where is that in the texture data
	int					m_sliceRegionOffset;	// offset to the start (lowest address corner) of the region requested
};

//===============================================================================

#define	GLM_SAMPLER_COUNT	16

#define GLM_MAX_PIXEL_TEX_SAMPLERS	16
#define GLM_MAX_VERTEX_TEX_SAMPLERS	0
typedef CBitVec<GLM_SAMPLER_COUNT> CTexBindMask;

enum EGLMTexSliceFlag
{
	kSliceValid			=	0x01,	// slice has been teximage'd in whole at least once - set to 0 initially
	kSliceStorageValid	=	0x02,	// if backing store is available, this slice's data is a valid copy - set to 0 initially
	kSliceLocked		=	0x04,	// are one or more locks outstanding on this slice
	kSliceFullyDirty	=	0x08,	// does the slice need to be fully downloaded at unlock time (disregard dirty rects)
};

//===============================================================================

#define GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS		(2)
#define GLM_PACKED_SAMPLER_PARAMS_MIN_FILTER_BITS	(2)
#define GLM_PACKED_SAMPLER_PARAMS_MAG_FILTER_BITS	(2)
#define GLM_PACKED_SAMPLER_PARAMS_MIP_FILTER_BITS	(2)
#define GLM_PACKED_SAMPLER_PARAMS_MIN_LOD_BITS		(4)
#define GLM_PACKED_SAMPLER_PARAMS_MAX_ANISO_BITS	(5)
#define GLM_PACKED_SAMPLER_PARAMS_COMPARE_MODE_BITS	(1)
#define GLM_PACKED_SAMPLER_PARAMS_SRGB_BITS			(1)

struct GLMTexPackedSamplingParams
{
	uint32 m_addressU		: GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS;
	uint32 m_addressV		: GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS;
	uint32 m_addressW		: GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS;

	uint32 m_minFilter		: GLM_PACKED_SAMPLER_PARAMS_MIN_FILTER_BITS;
	uint32 m_magFilter		: GLM_PACKED_SAMPLER_PARAMS_MAG_FILTER_BITS;
	uint32 m_mipFilter		: GLM_PACKED_SAMPLER_PARAMS_MIP_FILTER_BITS;

	uint32 m_minLOD			: GLM_PACKED_SAMPLER_PARAMS_MIN_LOD_BITS;
	uint32 m_maxAniso		: GLM_PACKED_SAMPLER_PARAMS_MAX_ANISO_BITS;
	uint32 m_compareMode	: GLM_PACKED_SAMPLER_PARAMS_COMPARE_MODE_BITS;
	uint32 m_srgb			: GLM_PACKED_SAMPLER_PARAMS_SRGB_BITS;
	uint32 m_isValid		: 1;
};

struct GLMTexSamplingParams
{
	union
	{
		GLMTexPackedSamplingParams m_packed;
		uint32 m_bits;
	};

	uint32 m_borderColor;

	FORCEINLINE bool operator== (const GLMTexSamplingParams& rhs ) const
	{
		return ( m_bits == rhs.m_bits ) && ( m_borderColor == rhs.m_borderColor );
	}

	FORCEINLINE void SetToDefaults()
	{
		m_bits = 0;
		m_borderColor = 0;
		m_packed.m_addressU = D3DTADDRESS_WRAP;
		m_packed.m_addressV = D3DTADDRESS_WRAP;
		m_packed.m_addressW = D3DTADDRESS_WRAP;
		m_packed.m_minFilter = D3DTEXF_POINT;
		m_packed.m_magFilter = D3DTEXF_POINT;
		m_packed.m_mipFilter = D3DTEXF_NONE;
		m_packed.m_maxAniso = 1;
		m_packed.m_compareMode = 0;
		m_packed.m_isValid = true;
	}

#ifndef OSX
	FORCEINLINE void SetToSamplerObject( GLuint nSamplerObject ) const
	{
		static const GLenum dxtogl_addressMode[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, (GLenum)-1 };
		static const GLenum dxtogl_magFilter[4] = { GL_NEAREST,	GL_NEAREST,	GL_LINEAR, GL_LINEAR };
		static const GLenum dxtogl_minFilter[4][4] = // indexed by _D3DTEXTUREFILTERTYPE on both axes: [row is min filter][col is mip filter]. 
		{
			/* min = D3DTEXF_NONE */		{	GL_NEAREST,		GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST_MIPMAP_LINEAR,	(GLenum)-1	},		// D3DTEXF_NONE we just treat like POINT
			/* min = D3DTEXF_POINT */		{	GL_NEAREST,		GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST_MIPMAP_LINEAR,	(GLenum)-1	},
			/* min = D3DTEXF_LINEAR */		{	GL_LINEAR,		GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	(GLenum)-1	},
			/* min = D3DTEXF_ANISOTROPIC */	{	GL_LINEAR,		GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	(GLenum)-1	},		// no diff from prior row, set maxAniso to effect the sampling
		};

		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_WRAP_S, dxtogl_addressMode[m_packed.m_addressU] );
		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_WRAP_T, dxtogl_addressMode[m_packed.m_addressV] );
		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_WRAP_R, dxtogl_addressMode[m_packed.m_addressW] );
		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_MIN_FILTER, dxtogl_minFilter[m_packed.m_minFilter][m_packed.m_mipFilter] );
		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_MAG_FILTER, dxtogl_magFilter[m_packed.m_magFilter] );
		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_packed.m_maxAniso );

		float flBorderColor[4] = { 0, 0, 0, 0 };
		if ( m_borderColor )
		{
			flBorderColor[0] = ((m_borderColor >> 16) & 0xFF) * (1.0f/255.0f);	//R
			flBorderColor[1] = ((m_borderColor >>  8) & 0xFF) * (1.0f/255.0f);	//G
			flBorderColor[2] = ((m_borderColor      ) & 0xFF) * (1.0f/255.0f);	//B
			flBorderColor[3] = ((m_borderColor >> 24) & 0xFF) * (1.0f/255.0f);	//A
		}
		gGL->glSamplerParameterfv( nSamplerObject, GL_TEXTURE_BORDER_COLOR, flBorderColor ); // <-- this crashes ATI's driver, remark it out
		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_MIN_LOD, m_packed.m_minLOD );
		gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_COMPARE_MODE_ARB, m_packed.m_compareMode ? GL_COMPARE_R_TO_TEXTURE_ARB : GL_NONE );
		if ( m_packed.m_compareMode )
		{
			gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );
		}
		if ( gGL->m_bHave_GL_EXT_texture_sRGB_decode )
		{
			gGL->glSamplerParameteri( nSamplerObject, GL_TEXTURE_SRGB_DECODE_EXT, m_packed.m_srgb ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT );
		}
	}
#endif // !OSX

	inline void DeltaSetToTarget( GLenum target, const GLMTexSamplingParams &curState )
	{
		static const GLenum dxtogl_addressMode[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, (GLenum)-1 };
		static const GLenum dxtogl_magFilter[4] = { GL_NEAREST,	GL_NEAREST,	GL_LINEAR, GL_LINEAR };
		static const GLenum dxtogl_minFilter[4][4] = // indexed by _D3DTEXTUREFILTERTYPE on both axes: [row is min filter][col is mip filter]. 
		{
			/* min = D3DTEXF_NONE */		{	GL_NEAREST,		GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST_MIPMAP_LINEAR,	(GLenum)-1	},		// D3DTEXF_NONE we just treat like POINT
			/* min = D3DTEXF_POINT */		{	GL_NEAREST,		GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST_MIPMAP_LINEAR,	(GLenum)-1	},
			/* min = D3DTEXF_LINEAR */		{	GL_LINEAR,		GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	(GLenum)-1	},
			/* min = D3DTEXF_ANISOTROPIC */	{	GL_LINEAR,		GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	(GLenum)-1	},		// no diff from prior row, set maxAniso to effect the sampling
		};

		if ( m_packed.m_addressU != curState.m_packed.m_addressU )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_WRAP_S, dxtogl_addressMode[m_packed.m_addressU] );
		}

		if ( m_packed.m_addressV != curState.m_packed.m_addressV )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_WRAP_T, dxtogl_addressMode[m_packed.m_addressV] );
		}

		if ( m_packed.m_addressW != curState.m_packed.m_addressW )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_WRAP_R, dxtogl_addressMode[m_packed.m_addressW] );
		}

		if ( ( m_packed.m_minFilter != curState.m_packed.m_minFilter ) || 
			 ( m_packed.m_magFilter != curState.m_packed.m_magFilter ) ||
			 ( m_packed.m_mipFilter != curState.m_packed.m_mipFilter ) ||
			 ( m_packed.m_maxAniso != curState.m_packed.m_maxAniso ) )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_MIN_FILTER, dxtogl_minFilter[m_packed.m_minFilter][m_packed.m_mipFilter] );
			gGL->glTexParameteri( target, GL_TEXTURE_MAG_FILTER, dxtogl_magFilter[m_packed.m_magFilter] );
			gGL->glTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_packed.m_maxAniso );
		}

		if ( m_borderColor != curState.m_borderColor )
		{
			float flBorderColor[4] = { 0, 0, 0, 0 };
			if ( m_borderColor )
			{
				flBorderColor[0] = ((m_borderColor >> 16) & 0xFF) * (1.0f/255.0f);	//R
				flBorderColor[1] = ((m_borderColor >>  8) & 0xFF) * (1.0f/255.0f);	//G
				flBorderColor[2] = ((m_borderColor      ) & 0xFF) * (1.0f/255.0f);	//B
				flBorderColor[3] = ((m_borderColor >> 24) & 0xFF) * (1.0f/255.0f);	//A
			}
		
			gGL->glTexParameterfv( target, GL_TEXTURE_BORDER_COLOR, flBorderColor ); // <-- this crashes ATI's driver, remark it out
		}

		if ( m_packed.m_minLOD != curState.m_packed.m_minLOD )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_MIN_LOD, m_packed.m_minLOD );
		}

		if ( m_packed.m_compareMode != curState.m_packed.m_compareMode )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_COMPARE_MODE_ARB, m_packed.m_compareMode ? GL_COMPARE_R_TO_TEXTURE_ARB : GL_NONE );
			if ( m_packed.m_compareMode )
			{
				gGL->glTexParameteri( target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );
			}
		}

		if ( ( gGL->m_bHave_GL_EXT_texture_sRGB_decode ) && ( m_packed.m_srgb != curState.m_packed.m_srgb ) )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_SRGB_DECODE_EXT, m_packed.m_srgb ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT );
		}
	}

	inline void SetToTarget( GLenum target )
	{
		static const GLenum dxtogl_addressMode[] = { GL_REPEAT, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_BORDER, (GLenum)-1 };
		static const GLenum dxtogl_magFilter[4] = { GL_NEAREST,	GL_NEAREST,	GL_LINEAR, GL_LINEAR };
		static const GLenum dxtogl_minFilter[4][4] = // indexed by _D3DTEXTUREFILTERTYPE on both axes: [row is min filter][col is mip filter]. 
		{
			/* min = D3DTEXF_NONE */		{	GL_NEAREST,		GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST_MIPMAP_LINEAR,	(GLenum)-1	},		// D3DTEXF_NONE we just treat like POINT
			/* min = D3DTEXF_POINT */		{	GL_NEAREST,		GL_NEAREST_MIPMAP_NEAREST,	GL_NEAREST_MIPMAP_LINEAR,	(GLenum)-1	},
			/* min = D3DTEXF_LINEAR */		{	GL_LINEAR,		GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	(GLenum)-1	},
			/* min = D3DTEXF_ANISOTROPIC */	{	GL_LINEAR,		GL_LINEAR_MIPMAP_NEAREST,	GL_LINEAR_MIPMAP_LINEAR,	(GLenum)-1	},		// no diff from prior row, set maxAniso to effect the sampling
		};

		gGL->glTexParameteri( target, GL_TEXTURE_WRAP_S, dxtogl_addressMode[m_packed.m_addressU] );
		gGL->glTexParameteri( target, GL_TEXTURE_WRAP_T, dxtogl_addressMode[m_packed.m_addressV] );
		gGL->glTexParameteri( target, GL_TEXTURE_WRAP_R, dxtogl_addressMode[m_packed.m_addressW] );
		gGL->glTexParameteri( target, GL_TEXTURE_MIN_FILTER, dxtogl_minFilter[m_packed.m_minFilter][m_packed.m_mipFilter] );
		gGL->glTexParameteri( target, GL_TEXTURE_MAG_FILTER, dxtogl_magFilter[m_packed.m_magFilter] );
		gGL->glTexParameteri( target, GL_TEXTURE_MAX_ANISOTROPY_EXT, m_packed.m_maxAniso );

		float flBorderColor[4] = { 0, 0, 0, 0 };
		if ( m_borderColor )
		{
			flBorderColor[0] = ((m_borderColor >> 16) & 0xFF) * (1.0f/255.0f);	//R
			flBorderColor[1] = ((m_borderColor >>  8) & 0xFF) * (1.0f/255.0f);	//G
			flBorderColor[2] = ((m_borderColor      ) & 0xFF) * (1.0f/255.0f);	//B
			flBorderColor[3] = ((m_borderColor >> 24) & 0xFF) * (1.0f/255.0f);	//A
		}
		gGL->glTexParameterfv( target, GL_TEXTURE_BORDER_COLOR, flBorderColor ); // <-- this crashes ATI's driver, remark it out
		gGL->glTexParameteri( target, GL_TEXTURE_MIN_LOD, m_packed.m_minLOD );
		gGL->glTexParameteri( target, GL_TEXTURE_COMPARE_MODE_ARB, m_packed.m_compareMode ? GL_COMPARE_R_TO_TEXTURE_ARB : GL_NONE );
		if ( m_packed.m_compareMode )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_COMPARE_FUNC_ARB, GL_LEQUAL );
		}
		if ( gGL->m_bHave_GL_EXT_texture_sRGB_decode )
		{
			gGL->glTexParameteri( target, GL_TEXTURE_SRGB_DECODE_EXT, m_packed.m_srgb ? GL_DECODE_EXT : GL_SKIP_DECODE_EXT );
		}
	}
};

//===============================================================================

class CGLMTex
{

public:

	void					Lock( GLMTexLockParams *params, char** addressOut, int* yStrideOut, int *zStrideOut );
	void					Unlock( GLMTexLockParams *params );
	GLuint                                  GetTexName() { return m_texName; }
	
protected:
	friend class GLMContext;			// only GLMContext can make CGLMTex objects
	friend class GLMTester;
	friend class CGLMFBO;

	friend struct IDirect3DDevice9;
	friend struct IDirect3DBaseTexture9;
	friend struct IDirect3DTexture9;
	friend struct IDirect3DSurface9;
	friend struct IDirect3DCubeTexture9;
	friend struct IDirect3DVolumeTexture9;
	
			CGLMTex( GLMContext *ctx, GLMTexLayout *layout, uint levels, const char *debugLabel = NULL );
			~CGLMTex( );
	
	int						CalcSliceIndex( int face, int mip );
	void					CalcTexelDataOffsetAndStrides( int sliceIndex, int x, int y, int z, int *offsetOut, int *yStrideOut, int *zStrideOut );
		
	void					ReadTexels( GLMTexLockDesc *desc, bool readWholeSlice=true );
	void					WriteTexels( GLMTexLockDesc *desc, bool writeWholeSlice=true, bool noDataWrite=false );
		// last param lets us send NULL data ptr (only legal with uncompressed formats, beware)
		// this helps out ResetSRGB.

#if defined( OSX )
	void					HandleSRGBMismatch( bool srgb, int &srgbFlipCount );
	void					ResetSRGB( bool srgb, bool noDataWrite );
	// re-specify texture format to match desired sRGB form
	// noWrite means send NULL for texel source addresses instead of actual data - ideal for RT's
#endif
				
	bool					IsRBODirty() const;
	void					ForceRBONonDirty();
	void					ForceRBODirty();
			
		// re-specify texture format to match desired sRGB form
		// noWrite means send NULL for texel source addresses instead of actual data - ideal for RT's

	GLuint					m_texName;			// name of this texture in the context
	GLenum					m_texGLTarget;
	uint					m_nSamplerType;		// SAMPLER_2D, etc.
	
	GLMTexSamplingParams	m_SamplingParams;

	GLMTexLayout			*m_layout;		// layout of texture (shared across all tex with same layout)
	
	uint					m_nLastResolvedBatchCounter;
					
	int						m_minActiveMip;//index of lowest mip that has been written.  used to drive setting of GL_TEXTURE_MAX_LEVEL.
	int						m_maxActiveMip;//index of highest mip that has been written.  used to drive setting of GL_TEXTURE_MAX_LEVEL.
						
	GLMContext				*m_ctx;			// link back to parent context
		
	
	CGLMFBO                 *m_pBlitSrcFBO;
	CGLMFBO                 *m_pBlitDstFBO;
	GLuint					m_rboName;		// name of MSAA RBO backing the tex if MSAA enabled (or zero)
													
	int						m_rtAttachCount; // how many RT's have this texture attached somewhere

	char					*m_backing;		// backing storage if available
	
	int						m_lockCount;	// lock reqs are stored in the GLMContext for tracking

	CUtlVector<unsigned char>	m_sliceFlags;
			
	char					*m_debugLabel;	// strdup() of debugLabel passed in, or NULL
	
	bool					m_texClientStorage;	// was CS selected for texture
	bool					m_texPreloaded;		// has it been kicked into VRAM with GLMContext::PreloadTex yet

	int						m_srgbFlipCount;
#if GLMDEBUG
	CGLMTex					*m_pPrevTex;
	CGLMTex					*m_pNextTex;
#endif
};

#endif

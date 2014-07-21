//========= Copyright Valve Corporation, All rights reserved. ============//
//
// cglmtex.h
//	GLMgr textures
//
//===============================================================================

#ifndef CGLMTEX_H
#define	CGLMTEX_H

#pragma once

#include "tier1/utlhash.h"
#include "tier1/utlmap.h"

//===============================================================================

// forward declarations

class	GLMContext;
class	GLMTester;
class	CGLMTexLayoutTable;
class	CGLMTex;
class	CGLMFBO;

class	IDirect3DSurface9;

//===============================================================================

struct GLMTexFormatDesc
{
	char		*m_formatSummary;	// for debug visibility
	
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


class	CGLMTexLayoutTable
{
public:
					CGLMTexLayoutTable();
	
	GLMTexLayout	*NewLayoutRef( GLMTexLayoutKey *key );		// pass in a pointer to layout key - receive ptr to completed layout
	void			DelLayoutRef( GLMTexLayout *layout );		// pass in pointer to completed layout.  refcount is dropped.
	
	void			DumpStats( void );
protected:
	CUtlMap< GLMTexLayoutKey, GLMTexLayout* >	m_layoutMap;
};

//===============================================================================

// a sampler specifies desired state for drawing on a given sampler index
// this is the combination of a texture choice and a set of sampler parameters
// see http://msdn.microsoft.com/en-us/library/bb172602(VS.85).aspx


struct	GLMTexSamplingParams
{
	GLenum	m_addressModes[3];	// S, T, R
	GLfloat	m_borderColor[4];	// R,G,B,A

	GLenum	m_magFilter;
	GLenum	m_minFilter;
	
	GLfloat	m_mipmapBias;
	GLint	m_minMipLevel;
	GLint	m_maxMipLevel;
	GLint	m_maxAniso;
	GLenum	m_compareMode;		// only used for depth and stencil type textures
	bool	m_srgb;				// srgb texture read... 
};

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

typedef CBitVec<GLM_SAMPLER_COUNT> CTexBindMask;

enum EGLMTexSliceFlag
{
	kSliceValid			=	0x01,	// slice has been teximage'd in whole at least once - set to 0 initially
	kSliceStorageValid	=	0x02,	// if backing store is available, this slice's data is a valid copy - set to 0 initially
	kSliceLocked		=	0x04,	// are one or more locks outstanding on this slice
	kSliceFullyDirty	=	0x08,	// does the slice need to be fully downloaded at unlock time (disregard dirty rects)
};

class CGLMTex
{

public:

	void					Lock( GLMTexLockParams *params, char** addressOut, int* yStrideOut, int *zStrideOut );
	void					Unlock( GLMTexLockParams *params );
	
protected:
	friend class GLMContext;			// only GLMContext can make CGLMTex objects
	friend class GLMTester;
	friend class CGLMFBO;

	friend class IDirect3DDevice9;
	friend class IDirect3DBaseTexture9;
	friend class IDirect3DTexture9;
	friend class IDirect3DSurface9;
	friend class IDirect3DCubeTexture9;
	friend class IDirect3DVolumeTexture9;
	
			CGLMTex( GLMContext *ctx, GLMTexLayout *layout, GLMTexSamplingParams *sampling, char *debugLabel = NULL );
			~CGLMTex( );
	
	int						CalcSliceIndex( int face, int mip );
	void					CalcTexelDataOffsetAndStrides( int sliceIndex, int x, int y, int z, int *offsetOut, int *yStrideOut, int *zStrideOut );

	void					ApplySamplingParams( GLMTexSamplingParams *params, bool noCheck=FALSE );

	void					ReadTexels( GLMTexLockDesc *desc, bool readWholeSlice=true );
	void					WriteTexels( GLMTexLockDesc *desc, bool writeWholeSlice=true, bool noDataWrite=false );
		// last param lets us send NULL data ptr (only legal with uncompressed formats, beware)
		// this helps out ResetSRGB.
	
	void					ResetSRGB( bool srgb, bool noDataWrite );
		// re-specify texture format to match desired sRGB form
		// noWrite means send NULL for texel source addresses instead of actual data - ideal for RT's

	GLMTexLayout			*m_layout;		// layout of texture (shared across all tex with same layout)
	int						m_minActiveMip;//index of lowest mip that has been written.  used to drive setting of GL_TEXTURE_MAX_LEVEL.
	int						m_maxActiveMip;//index of highest mip that has been written.  used to drive setting of GL_TEXTURE_MAX_LEVEL.
				
	GLMTexSamplingParams	m_sampling;		// mirror of sampling params currently embodied in the texture
											// (consult this at draw time, in order to know if changes need to be made)
						
	GLMContext				*m_ctx;			// link back to parent context

	GLuint					m_texName;			// name of this texture in the context
	bool					m_texClientStorage;	// was CS selecetd for texture
	bool					m_texPreloaded;		// has it been kicked into VRAM with GLMContext::PreloadTex yet

	GLuint					m_rboName;		// name of MSAA RBO backing the tex if MSAA enabled (or zero)
	bool					m_rboDirty;		// has RBO been drawn on - i.e. needs to be blitted back to texture if texture is going to be sampled from
	
	CTexBindMask			m_bindPoints;	// true for each place in the parent ctx where currently
											// bound (indexed via EGLMTexCtxBindingIndex)
										
	int						m_rtAttachCount; // how many RT's have this texture attached somewhere

	char					*m_backing;		// backing storage if available
	
	int						m_lockCount;	// lock reqs are stored in the GLMContext for tracking

	CUtlVector<unsigned char>	m_sliceFlags;
	
	char					*m_debugLabel;	// strdup() of debugLabel passed in, or NULL
};


#endif

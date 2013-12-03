//========= Copyright Valve Corporation, All rights reserved. ============//
//
// glmgr.h
//	singleton class, common basis for managing GL contexts
//	responsible for tracking adapters and contexts
//
//===============================================================================

#ifndef GLMGR_H
#define	GLMGR_H

#pragma once

#undef HAVE_GL_ARB_SYNC
#ifdef LINUX
#define HAVE_GL_ARB_SYNC 1
#endif

#include "glentrypoints.h"
#include "glmdebug.h"
#include "glmdisplay.h"
#include "glmgrext.h"
#include "glmgrbasics.h"
#include "cglmtex.h"
#include "cglmfbo.h"
#include "cglmprogram.h"
#include "cglmbuffer.h"
#include "cglmquery.h"

#include "tier0/vprof_telemetry.h"
#include "materialsystem/ishader.h"
#include "dxabstract_types.h"


#ifdef LINUX
#define Debugger DebuggerBreak
#undef CurrentTime

// prevent some conflicts in SDL headers...
#undef M_PI
#include <stdint.h>
#ifndef _STDINT_H_
#define _STDINT_H_ 1
#endif

#include "SDL/SDL.h"
#endif

//===============================================================================
// glue to call out to Obj-C land (these are in glmgrcocoa.mm)
#ifdef OSX
PseudoNSGLContextPtr  GetCurrentNSGLContext( );
CGLContextObj         GetCGLContextFromNSGL( PseudoNSGLContextPtr nsglCtx );
#endif

#include "tier0/dynfunction.h"

//===============================================================================

// parrot the D3D present parameters, more or less... "adapter" translates into "active display index" per the m_activeDisplayCount below.
class GLMDisplayParams
{
	public:

		// presumption, these indices are in sync with the current display DB that GLMgr has handy
	//int					m_rendererIndex;		// index of renderer (-1 if root context)
	//int					m_displayIndex;			// index of display in renderer - for FS
	//int					m_modeIndex;			// index of mode in display - for FS

	void				*m_focusWindow;			// (VD3DHWND aka WindowRef) - what window does this context display into
	
	bool				m_fsEnable;				// fullscreen on or not
	bool				m_vsyncEnable;			// vsync on or not

	// height and width have to match the display mode info if full screen.
	
	uint				m_backBufferWidth;		// pixel width (aka screen h-resolution if full screen)
	uint				m_backBufferHeight;		// pixel height (aka screen v-resolution if full screen)
	D3DFORMAT           m_backBufferFormat;		// pixel format
	uint				m_multiSampleCount;		// 0 means no MSAA, 2 means 2x MSAA, etc
		// uint				m_multiSampleQuality;	// no MSAA quality control yet
		
	bool				m_enableAutoDepthStencil;	// generally set to 'TRUE' per CShaderDeviceDx8::SetPresentParameters
	D3DFORMAT			m_autoDepthStencilFormat;
	
	uint				m_fsRefreshHz;			// if full screen, this refresh rate (likely 0 for LCD's)
	
	//uint				m_rootRendererID;		// only used if m_rendererIndex is -1.
	//uint				m_rootDisplayMask;		// only used if m_rendererIndex is -1.

	bool				m_mtgl;					// enable multi threaded GL driver
};

//===============================================================================

class GLMgr
{
public:
	
	//===========================================================================
	// class methods - singleton
	static void		NewGLMgr( void );			// instantiate singleton..
	static GLMgr	*aGLMgr( void );			// return singleton..
	static void		DelGLMgr( void );			// tear down singleton..

	//===========================================================================
	// plain methods

					#if 0				// turned all these off while new approach is coded
						void			RefreshDisplayDB( void );	// blow away old display DB, make a new one
						GLMDisplayDB	*GetDisplayDB( void );		// get a ptr to the one GLMgr keeps.  only valid til next refresh.

							// eligible renderers will be ranked by desirability starting at index 0 within the db
							// within each renderer, eligible displays will be ranked some kind of desirability (area? dist from menu bar?) 
							// within each display, eligible modes will be ranked by descending areas
						
							// calls supplying indices are implicitly making reference to the current DB
						bool			CaptureDisplay( int rendIndex, int displayIndex, bool captureAll );		// capture one display or all displays
						void			ReleaseDisplays( void );												// release all captures
						
						int				GetDisplayMode( int rendIndex, int displayIndex );						// retrieve current display res (returns modeIndex)
						void			SetDisplayMode( GLMDisplayParams *params );								// set the display res (only useful for FS)
					#endif
	
	GLMContext		*NewContext( GLMDisplayParams *params );		// this will have to change
	void			DelContext( GLMContext *context );

		// with usage of CGLMacro.h we could dispense with the "current context" thing
		// and just declare a member variable of GLMContext, allowing each glXXX call to be routed directly
		// to the correct context
	void		SetCurrentContext( GLMContext *context );	// make current in calling thread only
	GLMContext	*GetCurrentContext( void );
		
protected:
	friend class GLMContext;

	GLMgr();
	~GLMgr();
};


//===========================================================================//

// helper function to do enable or disable in one step
inline void glSetEnable( GLenum which, bool enable )
{
	if (enable)
		gGL->glEnable(which);
	else
		gGL->glDisable(which);
}

// helper function for int vs enum clarity
inline void glGetEnumv( GLenum which, GLenum *dst )
{
	gGL->glGetIntegerv( which, (int*)dst );
}

//===========================================================================//
//
// types to support the GLMContext
//
//===========================================================================//

// Each state set/get path we are providing caching for, needs its own struct and a comparison operator.
// we also provide an enum of how many such types there are, handy for building dirty masks etc.

// shorthand macros
#define	EQ(fff) ( (src.fff) == (fff) )

//rasterizer
struct GLAlphaTestEnable_t		{ GLint		enable;													bool operator==(const GLAlphaTestEnable_t& src)		const { return EQ(enable);									} };
struct GLAlphaTestFunc_t		{ GLenum	func; GLclampf ref;										bool operator==(const GLAlphaTestFunc_t& src)		const { return EQ(func) && EQ(ref);							} };
struct GLCullFaceEnable_t		{ GLint		enable;													bool operator==(const GLCullFaceEnable_t& src)		const { return EQ(enable);									} };
struct GLCullFrontFace_t		{ GLenum	value;													bool operator==(const GLCullFrontFace_t& src)		const { return EQ(value);									} };
struct GLPolygonMode_t			{ GLenum	values[2];												bool operator==(const GLPolygonMode_t& src)			const { return EQ(values[0]) && EQ(values[1]);				} };
struct GLDepthBias_t			{ GLfloat	factor;		GLfloat units;								bool operator==(const GLDepthBias_t& src)			const { return EQ(factor) && EQ(units);						} };
struct GLScissorEnable_t		{ GLint		enable;													bool operator==(const GLScissorEnable_t& src)		const { return EQ(enable);									} };
struct GLScissorBox_t			{ GLint		x,y;		GLsizei width, height;						bool operator==(const GLScissorBox_t& src)			const { return EQ(x) && EQ(y) && EQ(width) && EQ(height);	} };
struct GLAlphaToCoverageEnable_t{ GLint		enable;													bool operator==(const GLAlphaToCoverageEnable_t& src) const { return EQ(enable);								} };
struct GLViewportBox_t			{ GLint		x,y;		GLsizei width, height;						bool operator==(const GLViewportBox_t& src)			const { return EQ(x) && EQ(y) && EQ(width) && EQ(height);	} };
struct GLViewportDepthRange_t	{ GLdouble	near,far;												bool operator==(const GLViewportDepthRange_t& src)	const { return EQ(near) && EQ(far);							} };
struct GLClipPlaneEnable_t		{ GLint		enable;													bool operator==(const GLClipPlaneEnable_t& src)		const { return EQ(enable);									} };
struct GLClipPlaneEquation_t	{ GLfloat	x,y,z,w;												bool operator==(const GLClipPlaneEquation_t& src)	const { return EQ(x) && EQ(y) && EQ(z) && EQ(w);			} };

//blend
struct GLColorMaskSingle_t		{ char		r,g,b,a;												bool operator==(const GLColorMaskSingle_t& src)		const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLColorMaskMultiple_t	{ char		r,g,b,a;												bool operator==(const GLColorMaskMultiple_t& src)	const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLBlendEnable_t			{ GLint		enable;													bool operator==(const GLBlendEnable_t& src)			const { return EQ(enable);									} };
struct GLBlendFactor_t			{ GLenum	srcfactor,dstfactor;									bool operator==(const GLBlendFactor_t& src)			const { return EQ(srcfactor) && EQ(dstfactor);				} };
struct GLBlendEquation_t		{ GLenum	equation;												bool operator==(const GLBlendEquation_t& src)		const { return EQ(equation);								} };
struct GLBlendColor_t			{ GLfloat	r,g,b,a;												bool operator==(const GLBlendColor_t& src)			const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLBlendEnableSRGB_t		{ GLint		enable;													bool operator==(const GLBlendEnableSRGB_t& src)		const { return EQ(enable);									} };

//depth
struct GLDepthTestEnable_t		{ GLint		enable;													bool operator==(const GLDepthTestEnable_t& src)		const { return EQ(enable);									} };
struct GLDepthFunc_t			{ GLenum	func;													bool operator==(const GLDepthFunc_t& src)			const { return EQ(func);									} };
struct GLDepthMask_t			{ char		mask;													bool operator==(const GLDepthMask_t& src)			const { return EQ(mask);									} };

//stencil
struct GLStencilTestEnable_t	{ GLint		enable;													bool operator==(const GLStencilTestEnable_t& src)	const { return EQ(enable);									} };
struct GLStencilFunc_t			{ GLenum	frontfunc, backfunc;	GLint ref;  GLuint mask;		bool operator==(const GLStencilFunc_t& src)			const { return EQ(frontfunc) && EQ(backfunc) && EQ(ref) && EQ(mask); } };
struct GLStencilOp_t			{ GLenum	sfail;	GLenum dpfail; GLenum dppass;					bool operator==(const GLStencilOp_t& src)			const { return EQ(sfail) && EQ(dpfail) && EQ(dppass);		} };
struct GLStencilWriteMask_t		{ GLint		mask;													bool operator==(const GLStencilWriteMask_t& src)	const { return EQ(mask);									} };

//clearing
struct GLClearColor_t			{  GLfloat	r,g,b,a;												bool operator==(const GLClearColor_t& src)			const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLClearDepth_t			{  GLdouble	d;														bool operator==(const GLClearDepth_t& src)			const { return EQ(d);										} };
struct GLClearStencil_t			{  GLint	s;														bool operator==(const GLClearStencil_t& src)		const { return EQ(s);										} };

#undef EQ

enum EGLMStateBlockType
{
	kGLAlphaTestEnable,
	kGLAlphaTestFunc,
	
	kGLCullFaceEnable,
	kGLCullFrontFace,

	kGLPolygonMode,

	kGLDepthBias,
	
	kGLScissorEnable,
	kGLScissorBox,

	kGLViewportBox,
	kGLViewportDepthRange,
	
	kGLClipPlaneEnable,
	kGLClipPlaneEquation,
	
	kGLColorMaskSingle,
	kGLColorMaskMultiple,

	kGLBlendEnable,
	kGLBlendFactor,	
	kGLBlendEquation,
	kGLBlendColor,
	kGLBlendEnableSRGB,

	kGLDepthTestEnable,
	kGLDepthFunc,
	kGLDepthMask,

	kGLStencilTestEnable,
	kGLStencilFunc,
	kGLStencilOp,	
	kGLStencilWriteMask,

	kGLClearColor,
	kGLClearDepth,
	kGLClearStencil,

	kGLAlphaToCoverageEnable,
	
	kGLMStateBlockLimit
};

//===========================================================================//

// templated functions representing GL R/W bottlenecks
// one set of set/get/getdefault is instantiated for each of the GL*** types above.

// use these from the non array state objects
template<typename T>	void GLContextSet( T *src );
template<typename T>	void GLContextGet( T *dst );
template<typename T>	void GLContextGetDefault( T *dst );

// use these from the array state objects
template<typename T>	void GLContextSetIndexed( T *src, int index );
template<typename T>	void GLContextGetIndexed( T *dst, int index );
template<typename T>	void GLContextGetDefaultIndexed( T *dst, int index );

//===========================================================================//

// caching state object template.  One of these is instantiated in the context per unique struct type above
template<typename T> class GLState
{
	public:
	
		GLState<T>()
		{
			dirty = false;
			memset( &data, 0, sizeof(data) );
		};

		// write: client src into cache
		// common case is both false.  dirty is calculated, context write is deferred.
		void	Write( T *src, bool noCompare=false, bool noDefer=false )
		{
			if (noCompare)
			{
				dirty = true;
			}
			else
			{
				// only == is implemented, so test for equal and negate
				// note, you only set dirty if mismatch, you never clear it until flush
				if ( !(data == *src) )
				{
					dirty = true;
				}
			}
			
			data = *src;
			
			if (noDefer)
			{
				Flush( true );	// dirty becomes false
			}
		};
		
		// write cache->context if dirty or forced.
		void Flush( bool noDefer=false )
		{
			if (dirty || noDefer)
			{
				GLContextSet( &data );
				GLMCheckError();
					// good place for some error checking here
				dirty = false;
			}
		};
		
		// default: write default value to cache, optionally write through
		void Default( bool noDefer=false )
		{
			GLContextGetDefault( &data );	// read default values directly to our cache copy
			dirty = true;
			Flush(noDefer);
		};

		// read: sel = 0 for cache, 1 for context
		void Read( T *dst, int sel )
		{
			if (sel==0)
			{
				*dst = data;
			}
			else
			{
				GLContextGet( dst );
				GLMCheckError();
			}
		};
		
		// check: verify that context equals cache, return true if mismatched or if illegal values seen
		bool Check ( void )
		{
			T		temp;
			bool	result;

			GLContextGet( &temp );
			GLMCheckError();
			result = !(temp == data);
			return result;
		};
		
	protected:
		T		data;
		bool	dirty;
};

// caching state object template - with multiple values behind it that are indexed
template<typename T, int COUNT> class GLStateArray
{
	public:
	
		GLStateArray<T,COUNT>()
		{
			memset( &dirty, 0, sizeof(dirty) );
			memset( &data, 0, sizeof(data) );
		};

		// write: client src into cache
		// common case is both false.  dirty is calculated, context write is deferred.
		void WriteIndex( T *src, int index, bool noCompare=false, bool noDefer=false )
		{
			if (noCompare)
			{
				dirty[index] = true;
			}
			else
			{
				// only == is implemented, so test for equal and negate
				// note, you only set dirty if mismatch, you never clear it until flush
				if (! (data[index] == *src) )
				{
					dirty[index] = true;
				}
			}
			
			data[index] = *src;
			
			if (noDefer)
			{
				FlushIndex( index, true );	// dirty becomes false
			}
		};
		
		// write cache->context if dirty or forced.
		void FlushIndex( int index, bool noDefer=false )
		{
			if (dirty[index] || noDefer)
			{
				GLContextSetIndexed( &data[index], index );
				GLMCheckError();
				dirty[index] = false;
			}
		};
		
		// write all slots in the array
		void Flush( bool noDefer=false )
		{
			for( int i=0; i<COUNT; i++)
			{
				FlushIndex( i, noDefer );
			}
		}
		
		// default: write default value to cache, optionally write through
		void DefaultIndex( int index, bool noDefer=false )
		{
			GLContextGetDefaultIndexed( &data[index], index );	// read default values directly to our cache copy
			dirty[index] = true;
			Flush(noDefer);
		};
		
		void Default( void )
		{
			for( int i=0; i<COUNT; i++)
			{
				DefaultIndex( i );
			}			
		}

		// read: sel = 0 for cache, 1 for context
		void ReadIndex( T *dst, int index, int sel )
		{
			if (sel==0)
			{
				*dst = data[index];
			}
			else
			{
				GLContextGetIndexed( dst, index );
				GLMCheckError();
			}
		};
		
		// check: verify that context equals cache, return true if mismatched or if illegal values seen
		bool CheckIndex( int index )
		{
			T		temp;
			bool	result;

			GLContextGetIndexed( &temp, index );
			GLMCheckError();
			result = !(temp == data[index]);
			
			return result;
		};

		bool Check( void )
		{
			T		temp;
			bool	result = false;

			for( int i=0; i<COUNT; i++)
			{
				result |= CheckIndex( i );
			}
			
			return result;
		};
		
	protected:
		T		data	[COUNT];
		bool	dirty	[COUNT];
};


//===========================================================================//

struct	GLMTexSampler
{
	GLMTexSamplingParams	m_samp;
	CGLMTex					*m_drawTex;		// tex which must be bound at time of draw
	CGLMTex					*m_boundTex;	// tex which is actually bound now (if does not match, a rebind is needed to draw)
};

#ifdef NEVER
//===========================================================================//

enum	GLMVertexAttributeIndex
{
	kGLMGenericAttr00 = 0,
	kGLMGenericAttr01,
	kGLMGenericAttr02,
	kGLMGenericAttr03,
	kGLMGenericAttr04,
	kGLMGenericAttr05,
	kGLMGenericAttr06,
	kGLMGenericAttr07,
	kGLMGenericAttr08,
	kGLMGenericAttr09,
	kGLMGenericAttr10,
	kGLMGenericAttr11,
	kGLMGenericAttr12,
	kGLMGenericAttr13,
	kGLMGenericAttr14,
	kGLMGenericAttr15,

	kGLMVertexAttributeIndexMax			// ideally < 32
};

struct GLMVertexAttributeDesc			// all the info you need to do vertex setup for one attribute
{
	CGLMBuffer				*m_buffer;	// NULL allowed in which case m_offset is the full 32-bit pointer.. so you can draw from plain RAM if desired
	GLuint					m_datasize;	// comp count of the attribute (1-4)
	GLenum					m_datatype;	// data type of the attribute (GL_FLOAT, GL_UNSIGNED_BYTE, etc)
	GLuint					m_stride;
	GLuint					m_offset;	// net offset to attribute 'zero' within the buffer.
	GLboolean				m_normalized;	// apply to any fixed point data that needs normalizing, esp color bytes
	
		// may need a seed value at some point to be able to disambiguate re-lifed buffers holding same pointer
		// simpler alternative is to do shoot-down inside the vertex/index buffer free calls.
		// I'd rather not have to have each attribute fiddling a ref count on the buffer to which it refers..

#define	EQ(fff) ( (src.fff) == (fff) )
	// test in decreasing order of likelihood of difference, but do not include the buffer revision as caller is not supplying it..
	bool operator==(const GLMVertexAttributeDesc& src)		const { return EQ(m_buffer) && EQ(m_offset) && EQ(m_stride) && EQ(m_datatype) && EQ(m_normalized) && EQ(m_datasize);	}
#undef EQ

	uint					m_bufferRevision;	// only set in GLM context's copy, to disambiguate references that are same offset / same buffer but cross an orphan event
};

// GLMContext will maintain one of these structures inside the context to represent the current state.
// Client can supply a new one when it wants to change the setup.
//FIXME GLMContext can do the work to migrate from old setup to new setup as efficiently as possible (but it doesn't yet)
#endif

struct GLMVertexSetup
{
	uint	m_attrMask;					// which attrs are enabled (1<<n) mask where n is a GLMVertexAttributeIndex.
	
	GLMVertexAttributeDesc	m_attrs[ kGLMVertexAttributeIndexMax ];

	// copied in from dxabstract, not strictly needed for operation, helps debugging
	unsigned char	m_vtxAttribMap[16];

		/* high nibble is usage per _D3DDECLUSAGE
			typedef enum _D3DDECLUSAGE
			{
				D3DDECLUSAGE_POSITION		= 0,
				D3DDECLUSAGE_BLENDWEIGHT	= 1,
				D3DDECLUSAGE_BLENDINDICES	= 2,
				D3DDECLUSAGE_NORMAL			= 3,
				D3DDECLUSAGE_PSIZE			= 4,
				D3DDECLUSAGE_TEXCOORD		= 5,
				D3DDECLUSAGE_TANGENT		= 6,
				D3DDECLUSAGE_BINORMAL		= 7,
				D3DDECLUSAGE_TESSFACTOR		= 8,
				D3DDECLUSAGE_PLUGH			= 9,	// mystery value
				D3DDECLUSAGE_COLOR			= 10,
				D3DDECLUSAGE_FOG			= 11,
				D3DDECLUSAGE_DEPTH			= 12,
				D3DDECLUSAGE_SAMPLE			= 13,
			} D3DDECLUSAGE;
		
			low nibble is usageindex (i.e. POSITION0, POSITION1, etc)
			array position is attrib number.
		*/
};

//===========================================================================//

//FIXME magic numbers here

#define	kGLMProgramParamFloat4Limit	256
#define	kGLMProgramParamBoolLimit	16
#define	kGLMProgramParamInt4Limit	16

#define	kGLMVertexProgramParamFloat4Limit	256
#define	kGLMFragmentProgramParamFloat4Limit	32

struct GLMProgramParamsF
{
	float	m_values[kGLMProgramParamFloat4Limit][4];		// float4's 256 of them
	uint	m_dirtySlotCount;								// index of slot past highest dirty (assume 0 for base of range)
};

struct GLMProgramParamsB
{
	int		m_values[kGLMProgramParamBoolLimit];			// bools, 4 of them
	uint	m_dirtySlotCount;
};

struct GLMProgramParamsI
{
	int		m_values[kGLMProgramParamInt4Limit][4];			// int4s, 16 of them
	uint	m_dirtySlotCount;
};

enum EGLMParamWriteMode
{
	eParamWriteAllSlots,			// glUniform4fv of the maximum size	(not recommended if shader is down-sizing the decl)
	eParamWriteShaderSlots,			// glUniform4fv of the active slot count ("highwater")
	eParamWriteShaderSlotsOptional,	// glUniform4fv of the active slot count ("highwater") - but only if at least one has been written - it's optional
	eParamWriteDirtySlotRange		// glUniform4fv of the 0-N range where N is highest dirty slot
};

enum EGLMAttribWriteMode
{
	eAttribWriteAll,
	eAttribWriteDirty
};

//===========================================================================//

#if GLMDEBUG
enum EGLMDebugCallSite
{
	eBeginFrame,		// inside begin frame func - frame number has been inc'd, batch number should be -1
	eClear,				// inside clear func
	eDrawElements,		// inside repeat loop, prior to draw call - batch numberhas been inc'd
	eEndFrame,			// end frame
	ePresent			// before showing pixels
};

// caller should zero one of these out and fill in the m_caller before invoking the hook
struct GLMDebugHookInfo
{
		// info from the caller to the debug hook
	EGLMDebugCallSite	m_caller;

	
		// state the hook uses to keep track of progress within a single run of the caller		
	int					m_iteration;	// which call to the hook is this.  if it's zero, it precedes any action in the caller.

	
		// bools used to communicate between caller and hook
	bool				m_loop;			// hook tells caller to loop around again (don't exit)
	bool				m_holding;		// current mood of hook, are we holding on this batch (i.e. rerun)
	
		// specific info for a draw call
	GLenum				m_drawMode;
	GLuint				m_drawStart;
	GLuint				m_drawEnd;
	GLsizei				m_drawCount;
	GLenum				m_drawType;
	const GLvoid		*m_drawIndices;
};
#endif

//===========================================================================//

#define	kGLMUserClipPlanes			2
#define kGLMScratchFBOCount			4

class	GLMContext
{
	public:
		// set/check current context (perq for many other calls)
		void	MakeCurrent( bool bRenderThread = false );
		void	ReleaseCurrent( bool bRenderThread = false );

		// CheckCurrent has been removed (it no longer compiled). To minimize churn I'm leaving
		// the inline NOP version.
		// DO NOT change this to non-inlined. It's called all over the place from very hot codepaths.
		FORCEINLINE void CheckCurrent( void ) { }
		
		void							PopulateCaps( void );	// fill out later portions of renderer info record which need context queries
		void							DumpCaps( void );		// printf all the caps info (you can call this in release too)
		const GLMRendererInfoFields&	Caps( void );			// peek at the caps record
	
		// state cache/mirror
		void	SetDefaultStates( void );
		void	FlushStates( bool noDefer = false );
		void	VerifyStates( void );

		// textures
		// Lock and Unlock reqs go directly to the tex object
		CGLMTex	*NewTex( GLMTexLayoutKey *key, char *debugLabel=NULL );
		void	DelTex( CGLMTex	*tex );	

			// options for Blit (replacement for ResolveTex and BlitTex)
			// pass NULL for dstTex if you want to target GL_BACK with the blit.  You get y-flip with that, don't change the dstrect yourself.		
		void	Blit2( CGLMTex *srcTex, GLMRect *srcRect, int srcFace, int srcMip, CGLMTex *dstTex, GLMRect *dstRect, int dstFace, int dstMip, uint filter );

			// tex blit (via FBO blit)
		void	BlitTex( CGLMTex *srcTex, GLMRect *srcRect, int srcFace, int srcMip, CGLMTex *dstTex, GLMRect *dstRect, int dstFace, int dstMip, uint filter, bool useBlitFB = true );

			//	MSAA resolve - we do this in GLMContext because it has to do a bunch of FBO/blit gymnastics
		void	ResolveTex( CGLMTex *tex, bool forceDirty=false );	
		
			// texture pre-load (residency forcing) - normally done one-time but you can force it
		void	PreloadTex( CGLMTex *tex, bool force=false );

		// samplers
		void	SetSamplerTex( int sampler, CGLMTex *tex );
		void	SetSamplerParams( int sampler, GLMTexSamplingParams *params );

		// render targets (FBO's)
		CGLMFBO	*NewFBO( void );
		void	DelFBO( CGLMFBO *fbo );		
		void	SetDrawingFBO( CGLMFBO *fbo );	// as with samplers, the notion of the target FBO is latched til draw time and then checked

		// programs
		CGLMProgram	*NewProgram( EGLMProgramType type, char *progString );
		void	DelProgram( CGLMProgram *prog );
		void	NullProgram( void );											// de-ac all shader state
		
		void	SetDrawingProgram( EGLMProgramType type, CGLMProgram *prog );	// set NULL for no program
		void	SetDrawingLang( EGLMProgramLang lang, bool immediate=false );	// choose ARB or GLSL.  immediate=false defers lang change to top of frame
		
		void	LinkShaderPair( CGLMProgram *vp, CGLMProgram *fp );			// ensure this combo has been linked and is in the GLSL pair cache
		void	ClearShaderPairCache( void );								// call this to shoot down all the linked pairs
		void	QueryShaderPair( int index, GLMShaderPairInfo *infoOut );	// this lets you query the shader pair cache for saving its state
		
		// buffers
		// Lock and Unlock reqs go directly to the buffer object
		CGLMBuffer	*NewBuffer( EGLMBufferType type, uint size, uint options );
		void	DelBuffer( CGLMBuffer *buff );

		void	SetIndexBuffer( CGLMBuffer *buff );		
		void	SetVertexAttributes( GLMVertexSetup *setup );
			// note, no API is exposed for setting a single attribute source.
			// come prepared with a complete block of attributes to use.
			
		// Queries
		CGLMQuery	*NewQuery( GLMQueryParams *params );
		void		DelQuery( CGLMQuery *query );
			
		// "slot" means a vec4-sized thing
		// these write into .env parameter space
		void	SetProgramParametersF( EGLMProgramType type, uint baseSlot, float *slotData, uint slotCount );	// take vec4f's
		void	SetProgramParametersB( EGLMProgramType type, uint baseSlot, int  *slotData, uint boolCount );	// take "BOOL" aka int
		void	SetProgramParametersI( EGLMProgramType type, uint baseSlot, int  *slotData, uint slotCount );	// take int4s

		// state sync
		void	FlushDrawStates( bool shadersOn=true );				// pushes all drawing state - samplers, tex, programs, etc.
		
		// drawing
		void	DrawRangeElements(	GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
		void	CheckNative( void );
		
		// clearing
		void	Clear( bool color, unsigned long colorValue, bool depth, float depthValue, bool stencil, unsigned int stencilValue, GLScissorBox_t *rect = NULL );
		
		// display
		//void	SetVSyncEnable( bool vsyncOn );
		//void	SetFullScreen( bool fsOn, int screenIndex );		// will be latched for next BeginFrame
		//void	ActivateFullScreen( bool fsOn, int screenIndex );	// will be called by BeginFrame
		bool	SetDisplayParams( GLMDisplayParams *params );		// either the first time setup, or a change to new setup
		
		void	Present( CGLMTex *tex );		// somewhat hardwired for the time being

		// mode switch / reset
		void	Reset( void );							// not a lot of args for now..
		
		// writers for the state block inputs
		
		void	WriteAlphaTestEnable		( GLAlphaTestEnable_t *src );
		void	WriteAlphaTestFunc			( GLAlphaTestFunc_t *src );
		void	WriteCullFaceEnable			( GLCullFaceEnable_t *src );
		void	WriteCullFrontFace			( GLCullFrontFace_t *src );
		void	WritePolygonMode			( GLPolygonMode_t *src );
		void	WriteDepthBias				( GLDepthBias_t *src );
		void	WriteClipPlaneEnable		( GLClipPlaneEnable_t *src, int which );
		void	WriteClipPlaneEquation		( GLClipPlaneEquation_t *src, int which );
		void	WriteScissorEnable			( GLScissorEnable_t *src );
		void	WriteScissorBox				( GLScissorBox_t *src );
		void	WriteAlphaToCoverageEnable	( GLAlphaToCoverageEnable_t *src );
		void	WriteViewportBox			( GLViewportBox_t *src );
		void	WriteViewportDepthRange		( GLViewportDepthRange_t *src );
		void	WriteColorMaskSingle		( GLColorMaskSingle_t *src );
		void	WriteColorMaskMultiple		( GLColorMaskMultiple_t *src, int which );
		void	WriteBlendEnable			( GLBlendEnable_t *src );
		void	WriteBlendFactor			( GLBlendFactor_t *src );
		void	WriteBlendEquation			( GLBlendEquation_t *src );
		void	WriteBlendColor				( GLBlendColor_t *src );
		void	WriteBlendEnableSRGB		( GLBlendEnableSRGB_t *src );
		void	WriteDepthTestEnable		( GLDepthTestEnable_t *src );
		void	WriteDepthFunc				( GLDepthFunc_t *src );
		void	WriteDepthMask				( GLDepthMask_t *src );
		void	WriteStencilTestEnable		( GLStencilTestEnable_t *src );
		void	WriteStencilFunc			( GLStencilFunc_t *src );
		void	WriteStencilOp				( GLStencilOp_t *src, int which );
		void	WriteStencilWriteMask		( GLStencilWriteMask_t *src );
		void	WriteClearColor				( GLClearColor_t *src );
		void	WriteClearDepth				( GLClearDepth_t *src );
		void	WriteClearStencil			( GLClearStencil_t *src );


		// debug stuff
		void	BeginFrame( void );
		void	EndFrame( void );
		
		// new interactive debug stuff
#if GLMDEBUG
		void	DebugDump( GLMDebugHookInfo *info, uint options, uint vertDumpMode );
		void	DebugHook( GLMDebugHookInfo *info );
		void	DebugPresent( void );
		void	DebugClear( void );
#endif

		FORCEINLINE DWORD GetCurrentOwnerThreadId() const { return m_nCurOwnerThreadId; }
		
	protected:
		friend class GLMgr;				// only GLMgr can make GLMContext objects
		friend class GLMRendererInfo;	// only GLMgr can make GLMContext objects
		friend class CGLMTex;			// tex needs to be able to do binds
		friend class CGLMFBO;			// fbo needs to be able to do binds
		friend class CGLMProgram;
		friend class CGLMShaderPair;
		friend class CGLMShaderPairCache;
		friend class CGLMBuffer;
		friend class GLMTester;			// tester class needs access back into GLMContext
		
		friend class IDirect3D9;
		friend class IDirect3DDevice9;
		
		// methods------------------------------------------
		
				// old GLMContext( GLint displayMask, GLint rendererID, PseudoNSGLContextPtr nsglShareCtx );
				GLMContext( GLMDisplayParams *params );
				~GLMContext();
		
		// textures
		void	SelectTMU( int tmu );																// wrapper for glActiveTexture()
		int		BindTexToTMU( CGLMTex *tex, int tmu, bool noCheck=false );
		
		// render targets / FBO's
		void	BindFBOToCtx( CGLMFBO *fbo, GLenum bindPoint = GL_FRAMEBUFFER_EXT );				// you can also choose GL_READ_FRAMEBUFFER_EXT / GL_DRAW_FRAMEBUFFER_EXT
		
		// programs
		//void	BindProgramToCtx( EGLMProgramType type, CGLMProgram *prog );						// will set program mode enable appropriately

		// buffers
		void	BindBufferToCtx( EGLMBufferType type, CGLMBuffer *buff, bool force = false );		// does not twiddle any enables.

		// debug font
		void	GenDebugFontTex( void );
		void	DrawDebugText( float x, float y, float z, float drawCharWidth, float drawCharHeight, char *string );
		
		// members------------------------------------------
		
		// context
		DWORD							m_nCurOwnerThreadId;

		GLMRendererInfoFields			m_caps;
	
		bool							m_displayParamsValid;		// is there a param block copied in yet
		GLMDisplayParams				m_displayParams;			// last known display config, either via constructor, or by SetDisplayParams...
		
#if defined(USE_SDL)
		int								m_pixelFormatAttribs[100];	// more than enough
		void *							m_ctx;
#endif

		// texture form table
		CGLMTexLayoutTable				*m_texLayoutTable;

		// context state mirrors

		GLState<GLAlphaTestEnable_t>	m_AlphaTestEnable;
		
		GLState<GLAlphaTestFunc_t>		m_AlphaTestFunc;
		
		GLState<GLCullFaceEnable_t>		m_CullFaceEnable;			
		GLState<GLCullFrontFace_t>		m_CullFrontFace;	
		GLState<GLPolygonMode_t>		m_PolygonMode;
		
		GLState<GLDepthBias_t>			m_DepthBias;		

		GLStateArray<GLClipPlaneEnable_t,kGLMUserClipPlanes>	m_ClipPlaneEnable;
		GLStateArray<GLClipPlaneEquation_t,kGLMUserClipPlanes>	m_ClipPlaneEquation;	// dxabstract puts them directly into param slot 253(0) and 254(1)
		
		GLState<GLScissorEnable_t>		m_ScissorEnable;	
		GLState<GLScissorBox_t>			m_ScissorBox;

		GLState<GLAlphaToCoverageEnable_t> m_AlphaToCoverageEnable;	
		
		GLState<GLViewportBox_t>		m_ViewportBox;		
		GLState<GLViewportDepthRange_t>	m_ViewportDepthRange;
		
		GLState<GLColorMaskSingle_t>	m_ColorMaskSingle;	
		GLStateArray<GLColorMaskMultiple_t,8>	m_ColorMaskMultiple;	// need an official constant for the color buffers limit
		
		GLState<GLBlendEnable_t>		m_BlendEnable;		
		GLState<GLBlendFactor_t>		m_BlendFactor;		
		GLState<GLBlendEquation_t>		m_BlendEquation;	
		GLState<GLBlendColor_t>			m_BlendColor;		
		GLState<GLBlendEnableSRGB_t>	m_BlendEnableSRGB;		// write to this one to transmit intent to write SRGB encoded pixels to drawing FB
		bool							m_FakeBlendEnableSRGB;	// writes to above will be shunted here if fake SRGB is in effect.
		
		GLState<GLDepthTestEnable_t>	m_DepthTestEnable;	
		GLState<GLDepthFunc_t>			m_DepthFunc;		
		GLState<GLDepthMask_t>			m_DepthMask;		
		
		GLState<GLStencilTestEnable_t>	m_StencilTestEnable;	// global stencil test enable
		GLState<GLStencilFunc_t>		m_StencilFunc;			// holds front and back stencil funcs
		GLStateArray<GLStencilOp_t,2>	m_StencilOp;			// indexed: 0=front 1=back
		GLState<GLStencilWriteMask_t>	m_StencilWriteMask;		
		
		GLState<GLClearColor_t>			m_ClearColor;		
		GLState<GLClearDepth_t>			m_ClearDepth;		
		GLState<GLClearStencil_t>		m_ClearStencil;		
		
		// texture bindings and sampler setup
		int								m_activeTexture;		// mirror for glActiveTexture
		GLMTexSampler					m_samplers[GLM_SAMPLER_COUNT];
	
		// texture lock tracking - CGLMTex objects share usage of this
		CUtlVector< GLMTexLockDesc >	m_texLocks;
		
		// render target binding - check before draw
		// similar to tex sampler mechanism, we track "bound" from "chosen for drawing" separately,
		// so binding for creation/setup need not disrupt any notion of what will be used at draw time
		
		CGLMFBO							*m_boundDrawFBO;		// FBO on GL_DRAW_FRAMEBUFFER bind point
		CGLMFBO							*m_boundReadFBO;		// FBO on GL_READ_FRAMEBUFFER bind point
																// ^ both are set if you bind to GL_FRAMEBUFFER_EXT
		
		CGLMFBO							*m_drawingFBO;			// what FBO should be bound at draw time (to both read/draw bp's).

		CGLMFBO							*m_blitReadFBO;
		CGLMFBO							*m_blitDrawFBO;			// scratch FBO's for framebuffer blit
		
		CGLMFBO							*m_scratchFBO[ kGLMScratchFBOCount ];	// general purpose FBO's for internal use
		
		CUtlVector< CGLMFBO* >			m_fboTable;				// each live FBO goes in the table
		
		// program bindings
		EGLMProgramLang					m_drawingLangAtFrameStart;	// selector for start of frame (spills into m_drawingLang)
		EGLMProgramLang					m_drawingLang;				// selector for which language we desire to draw with on the next batch
		CGLMProgram						*m_drawingProgram[ kGLMNumProgramTypes ];
		
		GLMProgramParamsF				m_programParamsF[ kGLMNumProgramTypes ];
		GLMProgramParamsB				m_programParamsB[ kGLMNumProgramTypes ];	// two banks, but only the vertex one is used
		GLMProgramParamsI				m_programParamsI[ kGLMNumProgramTypes ];	// two banks, but only the vertex one is used
		EGLMParamWriteMode				m_paramWriteMode;
		
		CGLMProgram						*m_nullFragmentProgram;		// write opaque black.  Activate when caller asks for null FP

		CGLMProgram						*m_preloadTexVertexProgram;			// programs to help preload textures (dummies)
		CGLMProgram						*m_preload2DTexFragmentProgram;
		CGLMProgram						*m_preload3DTexFragmentProgram;
		CGLMProgram						*m_preloadCubeTexFragmentProgram;

		CGLMProgram						*m_boundProgram[ kGLMNumProgramTypes ];

		CGLMShaderPairCache				*m_pairCache;				// GLSL only
		CGLMShaderPair					*m_boundPair;				// GLSL only
		uint							m_boundPairRevision;		// GLSL only
		GLhandleARB						m_boundPairProgram;			// GLSL only

		// buffer bindings
		CGLMBuffer						*m_lastKnownBufferBinds[ kGLMNumBufferTypes ];				// tracked per bind point for dupe-bind-absorb
		GLMVertexAttributeDesc			m_lastKnownVertexAttribs[ kGLMVertexAttributeIndexMax ];	// tracked per attrib for dupe-set-absorb
		uint							m_lastKnownVertexAttribMask;								// tracked for dupe-enable-absorb
		
		CGLMBuffer						*m_drawIndexBuffer;	// ... ? do we need dupe tracking for index buffer setup? ?
		
		GLMVertexSetup					m_drawVertexSetup;

		EGLMAttribWriteMode				m_attribWriteMode;
		
		bool							m_slowCheckEnable;		// turn this on or no native checking is done ("-glmassertslow" or "-glmsspewslow")
		bool							m_slowAssertEnable;		// turn this on to assert on a non-native batch	"-glmassertslow"
		bool							m_slowSpewEnable;	// turn this on to log non-native batches to stdout "-glmspewslow"
		
		// debug font texture
		CGLMTex							*m_debugFontTex;		// might be NULL unless you call GenDebugFontTex
		CGLMBuffer						*m_debugFontIndices;	// up to 1024 indices (256 chars times 4)
		CGLMBuffer						*m_debugFontVertices;	// up to 1024 verts

		// batch/frame debugging support
		int								m_debugFrameIndex;			// init to -1. Increment at BeginFrame
		int								m_debugBatchIndex;			// init to -1. Increment at any draw call

#if GLMDEBUG
		// interactive (DebugHook) debug support

			// using these you can implement frame advance, batch single step, and batch rewind (let it run til next frame and hold on prev batch #)
		int								m_holdFrameBegin;		// -1 if no hold req'd, otherwise # of frame to hold at (at beginframe time)
		int								m_holdFrameEnd;			// -1 if no hold req'd, otherwise # of frame to hold at (at endframe time)

		int								m_holdBatch,m_holdBatchFrame;	// -1 if no hold, else # of batch&frame to hold at (both must be set)
																// these can be expired/cleared to -1 if the frame passes without a hit
																// may be desirable to re-pause in that event, as user was expecting a hold to occur

		bool							m_debugDelayEnable;		// allow sleep delay
		uint							m_debugDelay;			// sleep time per hook call in microseconds (for usleep())
		
			// pre-draw global toggles / options
		bool							m_autoClearColor,m_autoClearDepth,m_autoClearStencil;
		float							m_autoClearColorValues[4];
		
			// debug knobs
		int								m_selKnobIndex;
		float							m_selKnobMinValue,m_selKnobMaxValue,m_selKnobIncrement;
#endif

};

struct GLMTestParams
{
	GLMContext	*m_ctx;
	int			*m_testList;			// -1 termed
	
	bool		m_glErrToDebugger;
	bool		m_glErrToConsole;
	
	bool		m_intlErrToDebugger;
	bool		m_intlErrToConsole;
	
	int			m_frameCount;			// how many frames to test.
};

class GLMTester
{
	public:
	
	GLMTester(GLMTestParams *params);
	~GLMTester();
	

	// optionally callable by test routines to get basic drawables wired up
	void	StdSetup( void );
	void	StdCleanup( void );
	
	// callable by test routines to clear the frame or present it
	void	Clear( void );
	void	Present( int seed );

	// error reporting
	void	CheckGLError( char *comment );					// obey m_params setting for console / debugger response
	void	InternalError( int errcode, char *comment );	// if errcode!=0, obey m_params setting for console / debugger response
	
	void	RunTests();

	void	RunOneTest( int testindex );

	// test routines themselves
	void	Test0();
	void	Test1();
	void	Test2();
	void	Test3();

	GLMTestParams	m_params;		// copy of caller's params, do not mutate...
	
	// std-setup stuff
	int				m_drawWidth, m_drawHeight;
	CGLMFBO			*m_drawFBO;
	CGLMTex			*m_drawColorTex;
	CGLMTex			*m_drawDepthTex;
};

class CShowPixelsParams
{
public:
	GLuint					m_srcTexName;
	int						m_width,m_height;
	bool					m_vsyncEnable;
	bool					m_fsEnable;		// want receiving view to be full screen.  for now, just target the main screen.  extend later.
	bool					m_useBlit;		// use FBO blit - sending context says it is available.
	bool					m_noBlit;		// the back buffer has already been populated by the caller (perhaps via direct MSAA resolve from multisampled RT tex)
	bool					m_onlySyncView;	// react to full/windowed state change only, do not present bits
};


#define	kMaxCrawlFrames	100
#define	kMaxCrawlText		(kMaxCrawlFrames * 256)
class CStackCrawlParams
{
	public:
	uint					m_frameLimit;							// input: max frames to retrieve
	uint					m_frameCount;							// output: frames found
	void					*m_crawl[kMaxCrawlFrames];				// call site addresses
	char					*m_crawlNames[kMaxCrawlFrames];			// pointers into text following, one per decoded name
	char					m_crawlText[kMaxCrawlText];
};

#endif

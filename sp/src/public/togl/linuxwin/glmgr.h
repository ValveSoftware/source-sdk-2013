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

#include "glbase.h"
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
#include "materialsystem/IShader.h"
#include "dxabstract_types.h"
#include "tier0/icommandline.h"

//===============================================================================

#define GLM_OPENGL_VENDOR_ID 1
#define GLM_OPENGL_DEFAULT_DEVICE_ID 1
#define GLM_OPENGL_LOW_PERF_DEVICE_ID 2

extern void GLMDebugPrintf( const char *pMsg, ... );

extern uint g_nTotalDrawsOrClears, g_nTotalVBLockBytes, g_nTotalIBLockBytes;

#if GL_TELEMETRY_GPU_ZONES
struct TelemetryGPUStats_t
{
	uint m_nTotalBufferLocksAndUnlocks;
	uint m_nTotalTexLocksAndUnlocks;
	uint m_nTotalBlit2;
	uint m_nTotalResolveTex;
	uint m_nTotalPresent;

	inline void Clear() { memset( this, 0, sizeof( *this ) ); }
	inline uint GetTotal() const { return m_nTotalBufferLocksAndUnlocks + m_nTotalTexLocksAndUnlocks + m_nTotalBlit2 + m_nTotalResolveTex + m_nTotalPresent; }
};
extern TelemetryGPUStats_t g_TelemetryGPUStats;
#endif

struct GLMRect;
typedef void *PseudoGLContextPtr;

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
	
	GLMContext		*NewContext( IDirect3DDevice9 *pDevice, GLMDisplayParams *params );		// this will have to change
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
FORCEINLINE void glSetEnable( GLenum which, bool enable )
{
	if (enable)
		gGL->glEnable(which);
	else
		gGL->glDisable(which);
}

// helper function for int vs enum clarity
FORCEINLINE void glGetEnumv( GLenum which, GLenum *dst )
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
struct GLAlphaTestEnable_t		{ GLint		enable;													inline bool operator==(const GLAlphaTestEnable_t& src)		const { return EQ(enable);									} };
struct GLAlphaTestFunc_t		{ GLenum	func; GLclampf ref;										inline bool operator==(const GLAlphaTestFunc_t& src)		const { return EQ(func) && EQ(ref);							} };
struct GLCullFaceEnable_t		{ GLint		enable;													inline bool operator==(const GLCullFaceEnable_t& src)		const { return EQ(enable);									} };
struct GLCullFrontFace_t		{ GLenum	value;													inline bool operator==(const GLCullFrontFace_t& src)		const { return EQ(value);									} };
struct GLPolygonMode_t			{ GLenum	values[2];												inline bool operator==(const GLPolygonMode_t& src)			const { return EQ(values[0]) && EQ(values[1]);				} };
struct GLDepthBias_t			{ GLfloat	factor;		GLfloat units;								inline bool operator==(const GLDepthBias_t& src)			const { return EQ(factor) && EQ(units);						} };
struct GLScissorEnable_t		{ GLint		enable;													inline bool operator==(const GLScissorEnable_t& src)		const { return EQ(enable);									} };
struct GLScissorBox_t			{ GLint		x,y;		GLsizei width, height;						inline bool operator==(const GLScissorBox_t& src)			const { return EQ(x) && EQ(y) && EQ(width) && EQ(height);	} };
struct GLAlphaToCoverageEnable_t{ GLint		enable;													inline bool operator==(const GLAlphaToCoverageEnable_t& src) const { return EQ(enable);								} };
struct GLViewportBox_t			{ GLint		x,y;		GLsizei width, height; uint widthheight;	inline bool operator==(const GLViewportBox_t& src)			const { return EQ(x) && EQ(y) && EQ(width) && EQ(height);	} };
struct GLViewportDepthRange_t	{ GLdouble	flNear,flFar;											inline bool operator==(const GLViewportDepthRange_t& src)	const { return EQ(flNear) && EQ(flFar);						} };
struct GLClipPlaneEnable_t		{ GLint		enable;													inline bool operator==(const GLClipPlaneEnable_t& src)		const { return EQ(enable);									} };
struct GLClipPlaneEquation_t	{ GLfloat	x,y,z,w;												inline bool operator==(const GLClipPlaneEquation_t& src)	const { return EQ(x) && EQ(y) && EQ(z) && EQ(w);			} };

//blend
struct GLColorMaskSingle_t		{ char		r,g,b,a;												inline bool operator==(const GLColorMaskSingle_t& src)		const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLColorMaskMultiple_t	{ char		r,g,b,a;												inline bool operator==(const GLColorMaskMultiple_t& src)	const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLBlendEnable_t			{ GLint		enable;													inline bool operator==(const GLBlendEnable_t& src)			const { return EQ(enable);									} };
struct GLBlendFactor_t			{ GLenum	srcfactor,dstfactor;									inline bool operator==(const GLBlendFactor_t& src)			const { return EQ(srcfactor) && EQ(dstfactor);				} };
struct GLBlendEquation_t		{ GLenum	equation;												inline bool operator==(const GLBlendEquation_t& src)		const { return EQ(equation);								} };
struct GLBlendColor_t			{ GLfloat	r,g,b,a;												inline bool operator==(const GLBlendColor_t& src)			const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLBlendEnableSRGB_t		{ GLint		enable;													inline bool operator==(const GLBlendEnableSRGB_t& src)		const { return EQ(enable);									} };

//depth
struct GLDepthTestEnable_t		{ GLint		enable;													inline bool operator==(const GLDepthTestEnable_t& src)		const { return EQ(enable);									} };
struct GLDepthFunc_t			{ GLenum	func;													inline bool operator==(const GLDepthFunc_t& src)			const { return EQ(func);									} };
struct GLDepthMask_t			{ char		mask;													inline bool operator==(const GLDepthMask_t& src)			const { return EQ(mask);									} };

//stencil
struct GLStencilTestEnable_t	{ GLint		enable;													inline bool operator==(const GLStencilTestEnable_t& src)	const { return EQ(enable);									} };
struct GLStencilFunc_t			{ GLenum	frontfunc, backfunc;	GLint ref;  GLuint mask;		inline bool operator==(const GLStencilFunc_t& src)			const { return EQ(frontfunc) && EQ(backfunc) && EQ(ref) && EQ(mask); } };
struct GLStencilOp_t			{ GLenum	sfail;	GLenum dpfail; GLenum dppass;					inline bool operator==(const GLStencilOp_t& src)			const { return EQ(sfail) && EQ(dpfail) && EQ(dppass);		} };
struct GLStencilWriteMask_t		{ GLint		mask;													inline bool operator==(const GLStencilWriteMask_t& src)	const { return EQ(mask);									} };

//clearing
struct GLClearColor_t			{  GLfloat	r,g,b,a;												inline bool operator==(const GLClearColor_t& src)			const { return EQ(r) && EQ(g) && EQ(b) && EQ(a);			} };
struct GLClearDepth_t			{  GLdouble	d;														inline bool operator==(const GLClearDepth_t& src)			const { return EQ(d);										} };
struct GLClearStencil_t			{  GLint	s;														inline bool operator==(const GLClearStencil_t& src)		const { return EQ(s);										} };

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

//===============================================================================
// template specializations for each type of state

//                                                                      --- GLAlphaTestEnable ---
FORCEINLINE void GLContextSet( GLAlphaTestEnable_t *src )
{
	glSetEnable( GL_ALPHA_TEST, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLAlphaTestEnable_t *dst )
{
	dst->enable = gGL->glIsEnabled( GL_ALPHA_TEST );
}

FORCEINLINE void GLContextGetDefault( GLAlphaTestEnable_t *dst )
{
	dst->enable = GL_FALSE;	
}

//                                                                      --- GLAlphaTestFunc ---
FORCEINLINE void GLContextSet( GLAlphaTestFunc_t *src )
{
	gGL->glAlphaFunc( src->func, src->ref );
}

FORCEINLINE void GLContextGet( GLAlphaTestFunc_t *dst )
{
	glGetEnumv( GL_ALPHA_TEST_FUNC, &dst->func );
	gGL->glGetFloatv( GL_ALPHA_TEST_REF, &dst->ref );
}

FORCEINLINE void GLContextGetDefault( GLAlphaTestFunc_t *dst )
{
	dst->func = GL_ALWAYS;
	dst->ref = 0.0f;
}

//                                                                      --- GLAlphaToCoverageEnable ---
FORCEINLINE void GLContextSet( GLAlphaToCoverageEnable_t *src )
{
	glSetEnable( GL_SAMPLE_ALPHA_TO_COVERAGE_ARB, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLAlphaToCoverageEnable_t *dst )
{
	dst->enable = gGL->glIsEnabled( GL_SAMPLE_ALPHA_TO_COVERAGE_ARB );
}

FORCEINLINE void GLContextGetDefault( GLAlphaToCoverageEnable_t *dst )
{
	dst->enable = GL_FALSE;	
}

//                                                                      --- GLCullFaceEnable ---
FORCEINLINE void GLContextSet( GLCullFaceEnable_t *src )
{
	glSetEnable( GL_CULL_FACE, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLCullFaceEnable_t *dst )
{
	dst->enable = gGL->glIsEnabled( GL_CULL_FACE );
}

FORCEINLINE void GLContextGetDefault( GLCullFaceEnable_t *dst )
{
	dst->enable = GL_TRUE;	
}


//                                                                      --- GLCullFrontFace ---
FORCEINLINE void GLContextSet( GLCullFrontFace_t *src )
{
	gGL->glFrontFace( src->value );	// legal values are GL_CW or GL_CCW
}

FORCEINLINE void GLContextGet( GLCullFrontFace_t *dst )
{
	glGetEnumv( GL_FRONT_FACE, &dst->value );
}

FORCEINLINE void GLContextGetDefault( GLCullFrontFace_t *dst )
{
	dst->value = GL_CCW;
}


//                                                                      --- GLPolygonMode ---
FORCEINLINE void GLContextSet( GLPolygonMode_t *src )
{
	gGL->glPolygonMode( GL_FRONT, src->values[0] );
	gGL->glPolygonMode( GL_BACK, src->values[1] );
}

FORCEINLINE void GLContextGet( GLPolygonMode_t *dst )
{
	glGetEnumv( GL_POLYGON_MODE, &dst->values[0] );

}

FORCEINLINE void GLContextGetDefault( GLPolygonMode_t *dst )
{
	dst->values[0] = dst->values[1] = GL_FILL;
}


//                                                                      --- GLDepthBias ---
// note the implicit enable / disable.
// if you set non zero values, it is enabled, otherwise not.
FORCEINLINE void GLContextSet( GLDepthBias_t *src )
{
	bool enable = (src->factor != 0.0f) || (src->units != 0.0f);

	glSetEnable( GL_POLYGON_OFFSET_FILL, enable );
	gGL->glPolygonOffset( src->factor, src->units );
}

FORCEINLINE void GLContextGet( GLDepthBias_t *dst )
{
	gGL->glGetFloatv		( GL_POLYGON_OFFSET_FACTOR, &dst->factor );
	gGL->glGetFloatv		( GL_POLYGON_OFFSET_UNITS, &dst->units );
}

FORCEINLINE void GLContextGetDefault( GLDepthBias_t *dst )
{
	dst->factor		= 0.0;
	dst->units		= 0.0;
}


//                                                                      --- GLScissorEnable ---
FORCEINLINE void GLContextSet( GLScissorEnable_t *src )
{
	glSetEnable( GL_SCISSOR_TEST, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLScissorEnable_t *dst )
{
	dst->enable = gGL->glIsEnabled( GL_SCISSOR_TEST );
}

FORCEINLINE void GLContextGetDefault( GLScissorEnable_t *dst )
{
	dst->enable = GL_FALSE;
}


//                                                                      --- GLScissorBox ---
FORCEINLINE void GLContextSet( GLScissorBox_t *src )
{
	gGL->glScissor ( src->x, src->y, src->width, src->height );
}

FORCEINLINE void GLContextGet( GLScissorBox_t *dst )
{
	gGL->glGetIntegerv ( GL_SCISSOR_BOX, &dst->x );
}

FORCEINLINE void GLContextGetDefault( GLScissorBox_t *dst )
{
	// hmmmm, good question?  we can't really know a good answer so we pick a silly one
	// and the client better come back with a better answer later.
	dst->x = dst->y = 0;
	dst->width = dst->height = 16;
}


//                                                                      --- GLViewportBox ---

FORCEINLINE void GLContextSet( GLViewportBox_t *src )
{
	Assert( src->width == (int)( src->widthheight & 0xFFFF ) );
	Assert( src->height == (int)( src->widthheight >> 16 ) );
	gGL->glViewport (src->x, src->y, src->width, src->height );
}

FORCEINLINE void GLContextGet( GLViewportBox_t *dst )
{
	gGL->glGetIntegerv	( GL_VIEWPORT, &dst->x ); 
	dst->widthheight = dst->width | ( dst->height << 16 );
}

FORCEINLINE void GLContextGetDefault( GLViewportBox_t *dst )
{
	// as with the scissor box, we don't know yet, so pick a silly one and change it later
	dst->x = dst->y = 0;
	dst->width = dst->height = 16;
	dst->widthheight = dst->width | ( dst->height << 16 );
}


//                                                                      --- GLViewportDepthRange ---
FORCEINLINE void GLContextSet( GLViewportDepthRange_t *src )
{
	gGL->glDepthRange	( src->flNear, src->flFar );
}

FORCEINLINE void GLContextGet( GLViewportDepthRange_t *dst )
{
	gGL->glGetDoublev	( GL_DEPTH_RANGE, &dst->flNear );
}

FORCEINLINE void GLContextGetDefault( GLViewportDepthRange_t *dst )
{
	dst->flNear = 0.0;
	dst->flFar = 1.0;
}

//                                                                      --- GLClipPlaneEnable ---
FORCEINLINE void GLContextSetIndexed( GLClipPlaneEnable_t *src, int index )
{
#if GLMDEBUG
	if (CommandLine()->FindParm("-caps_noclipplanes"))
	{
		if (GLMKnob("caps-key",NULL) > 0.0)
		{
			// caps ON means NO clipping
			src->enable = false;
		}
	}
#endif
	glSetEnable( GL_CLIP_PLANE0 + index, src->enable != 0 );
}

FORCEINLINE void GLContextGetIndexed( GLClipPlaneEnable_t *dst, int index )
{
	dst->enable = gGL->glIsEnabled( GL_CLIP_PLANE0 + index );
}

FORCEINLINE void GLContextGetDefaultIndexed( GLClipPlaneEnable_t *dst, int index )
{
	dst->enable = 0;
}



//                                                                      --- GLClipPlaneEquation ---
FORCEINLINE void GLContextSetIndexed( GLClipPlaneEquation_t *src, int index )
{
	// shove into glGlipPlane
	GLdouble coeffs[4] = { src->x, src->y, src->z, src->w };

	gGL->glClipPlane( GL_CLIP_PLANE0 + index, coeffs );
}

FORCEINLINE void GLContextGetIndexed( GLClipPlaneEquation_t *dst, int index )
{
	DebuggerBreak();	 // do this later
	//	glClipPlane( GL_CLIP_PLANE0 + index, coeffs );
	//	GLdouble coeffs[4] = { src->x, src->y, src->z, src->w };
}

FORCEINLINE void GLContextGetDefaultIndexed( GLClipPlaneEquation_t *dst, int index )
{
	dst->x = 1.0;
	dst->y = 0.0;
	dst->z = 0.0;
	dst->w = 0.0;
}


//                                                                      --- GLColorMaskSingle ---
FORCEINLINE void GLContextSet( GLColorMaskSingle_t *src )
{
	gGL->glColorMask( src->r, src->g, src->b, src->a );
}

FORCEINLINE void GLContextGet( GLColorMaskSingle_t *dst )
{
	gGL->glGetBooleanv( GL_COLOR_WRITEMASK, (GLboolean*)&dst->r);
}

FORCEINLINE void GLContextGetDefault( GLColorMaskSingle_t *dst )
{
	dst->r = dst->g = dst->b = dst->a = 1;
}


//                                                                      --- GLColorMaskMultiple ---
FORCEINLINE void GLContextSetIndexed( GLColorMaskMultiple_t *src, int index )
{
	gGL->glColorMaskIndexedEXT ( index, src->r, src->g, src->b, src->a );
}

FORCEINLINE void GLContextGetIndexed( GLColorMaskMultiple_t *dst, int index )
{
	gGL->glGetBooleanIndexedvEXT ( GL_COLOR_WRITEMASK, index, (GLboolean*)&dst->r );
}

FORCEINLINE void GLContextGetDefaultIndexed( GLColorMaskMultiple_t *dst, int index )
{
	dst->r = dst->g = dst->b = dst->a = 1;
}


//                                                                      --- GLBlendEnable ---
FORCEINLINE void GLContextSet( GLBlendEnable_t *src )
{
	glSetEnable( GL_BLEND, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLBlendEnable_t *dst )
{
	dst->enable = gGL->glIsEnabled( GL_BLEND );
}

FORCEINLINE void GLContextGetDefault( GLBlendEnable_t *dst )
{
	dst->enable = GL_FALSE;
}


//                                                                      --- GLBlendFactor ---
FORCEINLINE void GLContextSet( GLBlendFactor_t *src )
{
	gGL->glBlendFunc ( src->srcfactor, src->dstfactor );
}

FORCEINLINE void GLContextGet( GLBlendFactor_t *dst )
{
	glGetEnumv	( GL_BLEND_SRC, &dst->srcfactor );
	glGetEnumv	( GL_BLEND_DST, &dst->dstfactor );
}

FORCEINLINE void GLContextGetDefault( GLBlendFactor_t *dst )
{
	dst->srcfactor = GL_ONE;
	dst->dstfactor = GL_ZERO;
}


//                                                                      --- GLBlendEquation ---
FORCEINLINE void GLContextSet( GLBlendEquation_t *src )
{
	gGL->glBlendEquation ( src->equation );
}

FORCEINLINE void GLContextGet( GLBlendEquation_t *dst )
{
	glGetEnumv	( GL_BLEND_EQUATION, &dst->equation );
}

FORCEINLINE void GLContextGetDefault( GLBlendEquation_t *dst )
{
	dst->equation = GL_FUNC_ADD;
}


//                                                                      --- GLBlendColor ---
FORCEINLINE void GLContextSet( GLBlendColor_t *src )
{
	gGL->glBlendColor ( src->r, src->g, src->b, src->a );
}

FORCEINLINE void GLContextGet( GLBlendColor_t *dst )
{
	gGL->glGetFloatv	( GL_BLEND_COLOR, &dst->r );
}

FORCEINLINE void GLContextGetDefault( GLBlendColor_t *dst )
{
	//solid white
	dst->r = dst->g = dst->b = dst->a = 1.0;
}


//                                                                      --- GLBlendEnableSRGB ---

#define GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING	0x8210
#define GL_COLOR_ATTACHMENT0						0x8CE0

FORCEINLINE void GLContextSet( GLBlendEnableSRGB_t *src )
{
#if GLMDEBUG
	// just check in debug... this is too expensive to look at on MTGL
	if (src->enable)
	{
		GLboolean	srgb_capable = false;
		gGL->glGetBooleanv( GL_FRAMEBUFFER_SRGB_CAPABLE_EXT, &srgb_capable);

		if (src->enable && !srgb_capable)
		{
			GLMPRINTF(("-Z- srgb-state-set FBO conflict: attempt to enable SRGB on non SRGB capable FBO config"));
		}
	}
#endif
	// this query is not useful unless you have the ARB_framebuffer_srgb ext.
	//GLint encoding = 0;
	//pfnglGetFramebufferAttachmentParameteriv( GL_DRAW_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_COLOR_ENCODING, &encoding );

	glSetEnable( GL_FRAMEBUFFER_SRGB_EXT, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLBlendEnableSRGB_t *dst )
{
	//dst->enable = glIsEnabled( GL_FRAMEBUFFER_SRGB_EXT );
	dst->enable = true; // wtf ?
}

FORCEINLINE void GLContextGetDefault( GLBlendEnableSRGB_t *dst )
{
	dst->enable = GL_FALSE;
}


//                                                                      --- GLDepthTestEnable ---
FORCEINLINE void GLContextSet( GLDepthTestEnable_t *src )
{
	glSetEnable( GL_DEPTH_TEST, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLDepthTestEnable_t *dst )
{
	dst->enable = gGL->glIsEnabled( GL_DEPTH_TEST );
}

FORCEINLINE void GLContextGetDefault( GLDepthTestEnable_t *dst )
{
	dst->enable = GL_FALSE;
}


//                                                                      --- GLDepthFunc ---
FORCEINLINE void GLContextSet( GLDepthFunc_t *src )
{
	gGL->glDepthFunc				( src->func );
}

FORCEINLINE void GLContextGet( GLDepthFunc_t *dst )
{
	glGetEnumv				( GL_DEPTH_FUNC, &dst->func );
}

FORCEINLINE void GLContextGetDefault( GLDepthFunc_t *dst )
{
	dst->func = GL_GEQUAL;
}


//                                                                      --- GLDepthMask ---
FORCEINLINE void GLContextSet( GLDepthMask_t *src )
{
	gGL->glDepthMask ( src->mask );
}

FORCEINLINE void GLContextGet( GLDepthMask_t *dst )
{
	gGL->glGetBooleanv ( GL_DEPTH_WRITEMASK, (GLboolean*)&dst->mask );
}

FORCEINLINE void GLContextGetDefault( GLDepthMask_t *dst )
{
	dst->mask = GL_TRUE;
}


//                                                                      --- GLStencilTestEnable ---
FORCEINLINE void GLContextSet( GLStencilTestEnable_t *src )
{
	glSetEnable( GL_STENCIL_TEST, src->enable != 0 );
}

FORCEINLINE void GLContextGet( GLStencilTestEnable_t *dst )
{
	dst->enable = gGL->glIsEnabled( GL_STENCIL_TEST );
}

FORCEINLINE void GLContextGetDefault( GLStencilTestEnable_t *dst )
{
	dst->enable = GL_FALSE;
}


//                                                                      --- GLStencilFunc ---
FORCEINLINE void GLContextSet( GLStencilFunc_t *src )
{
	if (src->frontfunc == src->backfunc)
		gGL->glStencilFuncSeparate( GL_FRONT_AND_BACK, src->frontfunc, src->ref, src->mask);
	else
	{
		gGL->glStencilFuncSeparate( GL_FRONT, src->frontfunc, src->ref, src->mask);
		gGL->glStencilFuncSeparate( GL_BACK, src->backfunc, src->ref, src->mask);
	}
}

FORCEINLINE void GLContextGet( GLStencilFunc_t *dst )
{
	glGetEnumv		( GL_STENCIL_FUNC, &dst->frontfunc );
	glGetEnumv		( GL_STENCIL_BACK_FUNC, &dst->backfunc );
	gGL->glGetIntegerv	( GL_STENCIL_REF, &dst->ref );
	gGL->glGetIntegerv	( GL_STENCIL_VALUE_MASK, (GLint*)&dst->mask );
}

FORCEINLINE void GLContextGetDefault( GLStencilFunc_t *dst )
{
	dst->frontfunc	= GL_ALWAYS;
	dst->backfunc	= GL_ALWAYS;
	dst->ref		= 0;
	dst->mask		= 0xFFFFFFFF;
}


//                                                                      --- GLStencilOp --- indexed 0=front, 1=back

FORCEINLINE void GLContextSetIndexed( GLStencilOp_t *src, int index )
{
	GLenum face = (index==0) ? GL_FRONT : GL_BACK;
	gGL->glStencilOpSeparate( face, src->sfail, src->dpfail, src->dppass );
}

FORCEINLINE void GLContextGetIndexed( GLStencilOp_t *dst, int index )
{
	glGetEnumv		( (index==0) ? GL_STENCIL_FAIL : GL_STENCIL_BACK_FAIL, &dst->sfail );
	glGetEnumv		( (index==0) ? GL_STENCIL_PASS_DEPTH_FAIL : GL_STENCIL_BACK_PASS_DEPTH_FAIL, &dst->dpfail );
	glGetEnumv		( (index==0) ? GL_STENCIL_PASS_DEPTH_PASS : GL_STENCIL_BACK_PASS_DEPTH_PASS, &dst->dppass );
}

FORCEINLINE void GLContextGetDefaultIndexed( GLStencilOp_t *dst, int index )
{
	dst->sfail = dst->dpfail = dst->dppass = GL_KEEP;
}


//                                                                      --- GLStencilWriteMask ---
FORCEINLINE void GLContextSet( GLStencilWriteMask_t *src )
{
	gGL->glStencilMask( src->mask );
}

FORCEINLINE void GLContextGet( GLStencilWriteMask_t *dst )
{
	gGL->glGetIntegerv	( GL_STENCIL_WRITEMASK, &dst->mask );
}

FORCEINLINE void GLContextGetDefault( GLStencilWriteMask_t *dst )
{
	dst->mask = 0xFFFFFFFF;
}


//                                                                      --- GLClearColor ---
FORCEINLINE void GLContextSet( GLClearColor_t *src )
{
	gGL->glClearColor( src->r, src->g, src->b, src->a );
}

FORCEINLINE void GLContextGet( GLClearColor_t *dst )
{
	gGL->glGetFloatv		( GL_COLOR_CLEAR_VALUE, &dst->r );
}

FORCEINLINE void GLContextGetDefault( GLClearColor_t *dst )
{
	dst->r = dst->g = dst->b = 0.5;
	dst->a = 1.0;
}


//                                                                      --- GLClearDepth ---
FORCEINLINE void GLContextSet( GLClearDepth_t *src )
{
	gGL->glClearDepth ( src->d );
}

FORCEINLINE void GLContextGet( GLClearDepth_t *dst )
{
	gGL->glGetDoublev ( GL_DEPTH_CLEAR_VALUE, &dst->d );
}

FORCEINLINE void GLContextGetDefault( GLClearDepth_t *dst )
{
	dst->d = 1.0;
}


//                                                                      --- GLClearStencil ---
FORCEINLINE void GLContextSet( GLClearStencil_t *src )
{
	gGL->glClearStencil( src->s );
}

FORCEINLINE void GLContextGet( GLClearStencil_t *dst )
{
	gGL->glGetIntegerv	( GL_STENCIL_CLEAR_VALUE, &dst->s );
}

FORCEINLINE void GLContextGetDefault( GLClearStencil_t *dst )
{
	dst->s = 0;
}

//===========================================================================//

// caching state object template.  One of these is instantiated in the context per unique struct type above
template<typename T> class GLState
{
	public:
		inline GLState()
		{
			memset( &data, 0, sizeof(data) );
			Default();
		}
		
		FORCEINLINE void Flush()
		{
			// immediately blast out the state - it makes no sense to delta it or do anything fancy because shaderapi, dxabstract, and OpenGL itself does this for us (and OpenGL calls with multithreaded drivers are very cheap)
			GLContextSet( &data );
		}
				
		// write: client src into cache
		// common case is both false.  dirty is calculated, context write is deferred.
		FORCEINLINE void Write( const T *src )
		{
			data = *src;
			Flush();
		}
						
		// default: write default value to cache, optionally write through
		inline void Default( bool noDefer=false )
		{
			GLContextGetDefault( &data );	// read default values directly to our cache copy
			Flush();
		}

		// read: sel = 0 for cache, 1 for context
		inline void Read( T *dst, int sel )
		{
			if (sel==0)
				*dst = data;
			else
				GLContextGet( dst );
		}
		
		// check: verify that context equals cache, return true if mismatched or if illegal values seen
		inline bool Check ( void )
		{
			T		temp;
			bool	result;

			GLContextGet( &temp );
			result = !(temp == data);
			return result;
		}

		FORCEINLINE const T &GetData() const { return data; }
		
	protected:
		T data;
};

// caching state object template - with multiple values behind it that are indexed
template<typename T, int COUNT> class GLStateArray
{
	public:
		inline GLStateArray()
		{
			memset( &data, 0, sizeof(data) );
			Default();
		}

		// write cache->context if dirty or forced.
		FORCEINLINE void FlushIndex( int index )
		{
			// immediately blast out the state - it makes no sense to delta it or do anything fancy because shaderapi, dxabstract, and OpenGL itself does this for us (and OpenGL calls with multithreaded drivers are very cheap)
			GLContextSetIndexed( &data[index], index );
		};

		// write: client src into cache
		// common case is both false.  dirty is calculated, context write is deferred.
		FORCEINLINE void WriteIndex( T *src, int index )
		{
			data[index] = *src;
			FlushIndex( index );	// dirty becomes false
		};
						
		// write all slots in the array
		FORCEINLINE void Flush()
		{
			for( int i=0; i < COUNT; i++)
			{
				FlushIndex( i );
			}
		}
		
		// default: write default value to cache, optionally write through
		inline void DefaultIndex( int index )
		{
			GLContextGetDefaultIndexed( &data[index], index );	// read default values directly to our cache copy
			Flush();
		};
		
		inline void Default( void )
		{
			for( int i=0; i<COUNT; i++)
			{
				DefaultIndex( i );
			}			
		}

		// read: sel = 0 for cache, 1 for context
		inline void ReadIndex( T *dst, int index, int sel )
		{
			if (sel==0)
				*dst = data[index];
			else
				GLContextGetIndexed( dst, index );
		};
		
		// check: verify that context equals cache, return true if mismatched or if illegal values seen
		inline bool CheckIndex( int index )
		{
			T		temp;
			bool	result;

			GLContextGetIndexed( &temp, index );
			result = !(temp == data[index]);
			
			return result;
		};

		inline bool Check( void )
		{
			//T		temp;
			bool	result = false;

			for( int i=0; i<COUNT; i++)
			{
				result |= CheckIndex( i );
			}
			
			return result;
		};
		
	protected:
		T		data[COUNT];
};


//===========================================================================//



struct	GLMTexSampler
{
	CGLMTex *m_pBoundTex;				// tex which is actually bound now
	GLMTexSamplingParams m_samp;		// current 2D sampler state
};

// GLMContext will maintain one of these structures inside the context to represent the current state.
// Client can supply a new one when it wants to change the setup.
//FIXME GLMContext can do the work to migrate from old setup to new setup as efficiently as possible (but it doesn't yet)

struct GLMVertexSetup
{
	uint m_attrMask;					// which attrs are enabled (1<<n) mask where n is a GLMVertexAttributeIndex.
	
	GLMVertexAttributeDesc m_attrs[ kGLMVertexAttributeIndexMax ];

	// copied in from dxabstract, not strictly needed for operation, helps debugging
	unsigned char m_vtxAttribMap[16];

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
			
	int	m_firstDirtySlotNonBone;
	int	m_dirtySlotHighWaterNonBone;						// index of slot past highest dirty non-bone register (assume 0 for base of range)

	int m_dirtySlotHighWaterBone;							// index of slot past highest dirty bone register (0=first bone reg, which is DXABSTRACT_VS_FIRST_BONE_SLOT)
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

class CFlushDrawStatesStats
{
public:
	CFlushDrawStatesStats() 
	{  
		Clear(); 
	}

	void Clear()
	{
		memset(this, 0, sizeof(*this));
	}

	uint m_nTotalBatchFlushes;
	uint m_nTotalProgramPairChanges;

	uint m_nNumChangedSamplers;
	uint m_nNumSamplingParamsChanged;
	uint m_nIndexBufferChanged;
	uint m_nVertexBufferChanged;

	uint m_nFirstVSConstant;
	uint m_nNumVSConstants;
	uint m_nNumVSBoneConstants;
	uint m_nFirstPSConstant;
	uint m_nNumPSConstants;
	uint m_nNewPS;
	uint m_nNewVS;
};

//===========================================================================//

#ifndef GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD
#define GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD 0x9160
#endif

#define GLMGR_PINNED_MEMORY_BUFFER_SIZE ( 6 * 1024 * 1024 )

class CPinnedMemoryBuffer
{
	CPinnedMemoryBuffer( const CPinnedMemoryBuffer & );
	CPinnedMemoryBuffer & operator= ( const CPinnedMemoryBuffer & );

public:
	CPinnedMemoryBuffer() : m_pRawBuf( NULL ), m_pBuf( NULL ), m_nSize( 0 ), m_nOfs( 0 ), m_nBufferObj( 0 ), m_nSyncObj( 0 )
	{
	}

	~CPinnedMemoryBuffer()
	{
		Deinit();
	}

	bool Init( uint nSize )
	{
		Deinit();
				
		// Guarantee 64KB alignment
		m_pRawBuf = malloc( nSize + 65535 );
		m_pBuf = reinterpret_cast<void *>((reinterpret_cast<uint64>(m_pRawBuf) + 65535) & (~65535));
		m_nSize = nSize;
		m_nOfs = 0;

		gGL->glGenBuffersARB( 1, &m_nBufferObj );
		gGL->glBindBufferARB( GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, m_nBufferObj );

		gGL->glBufferDataARB( GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, m_nSize, m_pBuf, GL_STREAM_COPY );
		
		return true;
	}

	void Deinit()
	{
		if ( !m_pRawBuf )
			return;
		
		BlockUntilNotBusy();
				
		gGL->glBindBufferARB(GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, m_nBufferObj );

		gGL->glBufferDataARB( GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 0, (void*)NULL, GL_STREAM_COPY );
		
		gGL->glBindBufferARB( GL_EXTERNAL_VIRTUAL_MEMORY_BUFFER_AMD, 0 );
		
		gGL->glDeleteBuffersARB( 1, &m_nBufferObj );
		m_nBufferObj = 0;

		free( m_pRawBuf );
		m_pRawBuf = NULL;
		m_pBuf = NULL;

		m_nSize = 0;
		m_nOfs = 0;
	}

	inline uint GetSize() const { return m_nSize; }
	inline uint GetOfs() const { return m_nOfs; }
	inline uint GetBytesRemaining() const { return m_nSize - m_nOfs; }
	inline void *GetPtr() const { return m_pBuf; }
	inline GLuint GetHandle() const { return m_nBufferObj; }

	void InsertFence()
	{
		if ( m_nSyncObj  )
		{
			gGL->glDeleteSync( m_nSyncObj );
		}

		m_nSyncObj = gGL->glFenceSync( GL_SYNC_GPU_COMMANDS_COMPLETE, 0 );
	}

	void BlockUntilNotBusy()
	{
		if ( m_nSyncObj )
		{
			gGL->glClientWaitSync( m_nSyncObj, GL_SYNC_FLUSH_COMMANDS_BIT, 3000000000000ULL );
			
			gGL->glDeleteSync( m_nSyncObj );

			m_nSyncObj = 0;
		}
		m_nOfs = 0;
	}
		
	void Append( uint nSize )
	{
		m_nOfs += nSize;
		Assert( m_nOfs <= m_nSize );
	}
		
private:
	void *m_pRawBuf;
	void *m_pBuf;
	uint m_nSize;
	uint m_nOfs;
	
	GLuint m_nBufferObj;

	GLsync m_nSyncObj;
};

//===========================================================================//

class GLMContext
{
	public:
		// set/check current context (perq for many other calls)
		void	MakeCurrent( bool bRenderThread = false );
		void	ReleaseCurrent( bool bRenderThread = false );

		// CheckCurrent has been removed (it no longer compiled on Linux). To minimize churn I'm leaving
		// the inline NOP version.
		// DO NOT change this to non-inlined. It's called all over the place from very hot codepaths.
		FORCEINLINE void CheckCurrent( void ) { }
		
		void							PopulateCaps( void );	// fill out later portions of renderer info record which need context queries
		void							DumpCaps( void );		// printf all the caps info (you can call this in release too)
		const GLMRendererInfoFields&	Caps( void );			// peek at the caps record
	
		// state cache/mirror
		void	SetDefaultStates( void );
		void    ForceFlushStates();

		void	VerifyStates( void );

		// textures
		// Lock and Unlock reqs go directly to the tex object
		CGLMTex	*NewTex( GLMTexLayoutKey *key, const char *debugLabel=NULL );
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
		FORCEINLINE void SetSamplerTex( int sampler, CGLMTex *tex );
				
		FORCEINLINE void SetSamplerDirty( int sampler );
		FORCEINLINE void SetSamplerMinFilter( int sampler, GLenum Value );
		FORCEINLINE void SetSamplerMagFilter( int sampler, GLenum Value );
		FORCEINLINE void SetSamplerMipFilter( int sampler, GLenum Value );
		FORCEINLINE void SetSamplerAddressU( int sampler, GLenum Value );
		FORCEINLINE void SetSamplerAddressV( int sampler, GLenum Value );
		FORCEINLINE void SetSamplerAddressW( int sampler, GLenum Value );
		FORCEINLINE void SetSamplerStates( int sampler, GLenum AddressU, GLenum AddressV, GLenum AddressW, GLenum minFilter, GLenum magFilter, GLenum mipFilter );
		FORCEINLINE void SetSamplerBorderColor( int sampler, DWORD Value );
		FORCEINLINE void SetSamplerMipMapLODBias( int sampler, DWORD Value );
		FORCEINLINE void SetSamplerMaxMipLevel( int sampler, DWORD Value );
		FORCEINLINE void SetSamplerMaxAnisotropy( int sampler, DWORD Value );
		FORCEINLINE void SetSamplerSRGBTexture( int sampler, DWORD Value );
		FORCEINLINE void SetShadowFilter( int sampler, DWORD Value );
		
		// render targets (FBO's)
		CGLMFBO	*NewFBO( void );
		void	DelFBO( CGLMFBO *fbo );		
		
		// programs
		CGLMProgram	*NewProgram( EGLMProgramType type, char *progString, const char *pShaderName );
		void	DelProgram( CGLMProgram *pProg );
		void	NullProgram( void );											// de-ac all shader state
		
		FORCEINLINE void SetVertexProgram( CGLMProgram *pProg );	
		FORCEINLINE void SetFragmentProgram( CGLMProgram *pProg );
		FORCEINLINE void SetProgram( EGLMProgramType nProgType, CGLMProgram *pProg ) { m_drawingProgram[nProgType] = pProg; m_bDirtyPrograms = true; }
		
		void	SetDrawingLang( EGLMProgramLang lang, bool immediate=false );	// choose ARB or GLSL.  immediate=false defers lang change to top of frame
		
		void	LinkShaderPair( CGLMProgram *vp, CGLMProgram *fp );			// ensure this combo has been linked and is in the GLSL pair cache
		void	ClearShaderPairCache( void );								// call this to shoot down all the linked pairs
		void	QueryShaderPair( int index, GLMShaderPairInfo *infoOut );	// this lets you query the shader pair cache for saving its state
		
		// buffers
		// Lock and Unlock reqs go directly to the buffer object
		CGLMBuffer	*NewBuffer( EGLMBufferType type, uint size, uint options );
		void	DelBuffer( CGLMBuffer *buff );

		FORCEINLINE void SetIndexBuffer( CGLMBuffer *buff ) { BindIndexBufferToCtx( buff );	}

		// FIXME: Remove this, it's no longer used
		FORCEINLINE void SetVertexAttributes( GLMVertexSetup *setup )
		{
			// we now just latch the vert setup and then execute on it at flushdrawstatestime if shaders are enabled.
			if ( setup )
			{
				m_drawVertexSetup = *setup;
			}
			else
			{
				memset( &m_drawVertexSetup, 0, sizeof( m_drawVertexSetup ) );
			}
		}

		// note, no API is exposed for setting a single attribute source.
		// come prepared with a complete block of attributes to use.
			
		// Queries
		CGLMQuery *NewQuery( GLMQueryParams *params );
		void DelQuery( CGLMQuery *query );
			
		// "slot" means a vec4-sized thing
		// these write into .env parameter space
		FORCEINLINE void SetProgramParametersF( EGLMProgramType type, uint baseSlot, float *slotData, uint slotCount );
		FORCEINLINE void SetProgramParametersB( EGLMProgramType type, uint baseSlot, int  *slotData, uint boolCount );	// take "BOOL" aka int
		FORCEINLINE void SetProgramParametersI( EGLMProgramType type, uint baseSlot, int  *slotData, uint slotCount );	// take int4s

		// state sync
		// If lazyUnbinding is true, unbound samplers will not actually be unbound to the GL device.
		FORCEINLINE void FlushDrawStates( uint nStartIndex, uint nEndIndex, uint nBaseVertex );				// pushes all drawing state - samplers, tex, programs, etc.
		void FlushDrawStatesNoShaders();
				
		// drawing
		FORCEINLINE void DrawRangeElements(	GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices, uint baseVertex, CGLMBuffer *pIndexBuf );
		void DrawRangeElementsNonInline(	GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices, uint baseVertex, CGLMBuffer *pIndexBuf );

		void	CheckNative( void );
		
		// clearing
		void	Clear( bool color, unsigned long colorValue, bool depth, float depthValue, bool stencil, unsigned int stencilValue, GLScissorBox_t *rect = NULL );
		
		// display
		//void	SetVSyncEnable( bool vsyncOn );
		//void	SetFullScreen( bool fsOn, int screenIndex );		// will be latched for next BeginFrame
		//void	ActivateFullScreen( bool fsOn, int screenIndex );	// will be called by BeginFrame
		bool	SetDisplayParams( GLMDisplayParams *params );		// either the first time setup, or a change to new setup
		
		void	Present( CGLMTex *tex );		// somewhat hardwired for the time being

		// Called when IDirect3DDevice9::Reset() is called.
		void	Reset();							
										
		// writers for the state block inputs
		
		FORCEINLINE void	WriteAlphaTestEnable( GLAlphaTestEnable_t *src ) { m_AlphaTestEnable.Write( src ); }
		FORCEINLINE void	WriteAlphaTestFunc( GLAlphaTestFunc_t *src ) { m_AlphaTestFunc.Write( src ); }
		FORCEINLINE void	WriteAlphaToCoverageEnable( GLAlphaToCoverageEnable_t *src ) { m_AlphaToCoverageEnable.Write( src ); }
		FORCEINLINE void	WriteCullFaceEnable( GLCullFaceEnable_t *src ) { m_CullFaceEnable.Write( src ); }
		FORCEINLINE void	WriteCullFrontFace( GLCullFrontFace_t *src ) { m_CullFrontFace.Write( src ); }
		FORCEINLINE void	WritePolygonMode( GLPolygonMode_t *src ) { m_PolygonMode.Write( src ); }
		FORCEINLINE void	WriteDepthBias( GLDepthBias_t *src ) { m_DepthBias.Write( src ); }
		FORCEINLINE void	WriteClipPlaneEnable( GLClipPlaneEnable_t *src, int which ) { m_ClipPlaneEnable.WriteIndex( src, which ); }
		FORCEINLINE void	WriteClipPlaneEquation( GLClipPlaneEquation_t *src, int which ) { m_ClipPlaneEquation.WriteIndex( src, which ); }
		FORCEINLINE void	WriteScissorEnable( GLScissorEnable_t *src ) { m_ScissorEnable.Write( src ); }
		FORCEINLINE void	WriteScissorBox( GLScissorBox_t *src ) { m_ScissorBox.Write( src ); }
		FORCEINLINE void	WriteViewportBox( GLViewportBox_t *src ) { m_ViewportBox.Write( src ); }
		FORCEINLINE void	WriteViewportDepthRange( GLViewportDepthRange_t *src ) { m_ViewportDepthRange.Write( src ); }
		FORCEINLINE void	WriteColorMaskSingle( GLColorMaskSingle_t *src ) { m_ColorMaskSingle.Write( src ); }
		FORCEINLINE void	WriteColorMaskMultiple( GLColorMaskMultiple_t *src, int which ) { m_ColorMaskMultiple.WriteIndex( src, which ); }
		FORCEINLINE void	WriteBlendEnable( GLBlendEnable_t *src ) { m_BlendEnable.Write( src ); }
		FORCEINLINE void	WriteBlendFactor( GLBlendFactor_t *src ) { m_BlendFactor.Write( src ); }
		FORCEINLINE void	WriteBlendEquation( GLBlendEquation_t *src ) { m_BlendEquation.Write( src ); }
		FORCEINLINE void	WriteBlendColor( GLBlendColor_t *src ) { m_BlendColor.Write( src ); }

		FORCEINLINE void	WriteBlendEnableSRGB( GLBlendEnableSRGB_t *src ) 
		{
			if (m_caps.m_hasGammaWrites)	// only if caps allow do we actually push it through to the extension
			{
				m_BlendEnableSRGB.Write( src );
			}
			else
			{
				m_FakeBlendEnableSRGB = src->enable != 0;
			}	
			// note however that we're still tracking what this mode should be, so FlushDrawStates can look at it and adjust the pixel shader
			// if fake SRGB mode is in place (m_caps.m_hasGammaWrites is false)
		}

		FORCEINLINE void	WriteDepthTestEnable( GLDepthTestEnable_t *src ) { m_DepthTestEnable.Write( src ); }
		FORCEINLINE void	WriteDepthFunc( GLDepthFunc_t *src ) { m_DepthFunc.Write( src ); }
		FORCEINLINE void	WriteDepthMask( GLDepthMask_t *src ) { m_DepthMask.Write( src ); }
		FORCEINLINE void	WriteStencilTestEnable( GLStencilTestEnable_t *src ) { m_StencilTestEnable.Write( src ); }
		FORCEINLINE void	WriteStencilFunc( GLStencilFunc_t *src ) { m_StencilFunc.Write( src ); }
		FORCEINLINE void	WriteStencilOp( GLStencilOp_t *src, int which ) { m_StencilOp.WriteIndex( src, which ); }
		FORCEINLINE void	WriteStencilWriteMask( GLStencilWriteMask_t *src ) { m_StencilWriteMask.Write( src ); }
		FORCEINLINE void	WriteClearColor( GLClearColor_t *src ) { m_ClearColor.Write( src ); }
		FORCEINLINE void	WriteClearDepth( GLClearDepth_t *src ) { m_ClearDepth.Write( src ); }
		FORCEINLINE void	WriteClearStencil( GLClearStencil_t *src ) { m_ClearStencil.Write( src ); }

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

		FORCEINLINE void SetMaxUsedVertexShaderConstantsHint( uint nMaxConstants );

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
		friend class CGLMBufferSpanManager;
		friend class GLMTester;			// tester class needs access back into GLMContext
		
		friend struct IDirect3D9;
		friend struct IDirect3DDevice9;
		friend struct IDirect3DQuery9;
		
		// methods------------------------------------------
		
				// old GLMContext( GLint displayMask, GLint rendererID, PseudoNSGLContextPtr nsglShareCtx );
		GLMContext( IDirect3DDevice9 *pDevice, GLMDisplayParams *params );
		~GLMContext();

		FORCEINLINE GLuint FindSamplerObject( const GLMTexSamplingParams &desiredParams );
								
		FORCEINLINE void SetBufAndVertexAttribPointer( uint nIndex, GLuint nGLName, GLuint stride, GLuint datatype, GLboolean normalized, GLuint nCompCount, const void *pBuf, uint nRevision )
		{
			VertexAttribs_t &curAttribs = m_boundVertexAttribs[nIndex];
			if ( nGLName != m_nBoundGLBuffer[kGLMVertexBuffer] )
			{
				m_nBoundGLBuffer[kGLMVertexBuffer] = nGLName;
				gGL->glBindBufferARB( GL_ARRAY_BUFFER_ARB, nGLName );
			}
			else if ( ( curAttribs.m_pPtr == pBuf ) && 
				( curAttribs.m_revision == nRevision ) &&
				( curAttribs.m_stride == stride ) &&
				( curAttribs.m_datatype == datatype ) &&
				( curAttribs.m_normalized == normalized ) &&
				( curAttribs.m_nCompCount == nCompCount ) )
			{
				return;
			}

			curAttribs.m_nCompCount = nCompCount;
			curAttribs.m_datatype = datatype;
			curAttribs.m_normalized = normalized;
			curAttribs.m_stride = stride;
			curAttribs.m_pPtr = pBuf;
			curAttribs.m_revision = nRevision;

			gGL->glVertexAttribPointer( nIndex, nCompCount, datatype, normalized, stride, pBuf );
		}

		struct CurAttribs_t
		{
			uint m_nTotalBufferRevision;
			IDirect3DVertexDeclaration9	*m_pVertDecl;
			D3DStreamDesc m_streams[ D3D_MAX_STREAMS ];
			uint64 m_vtxAttribMap[2];
		};

		CurAttribs_t m_CurAttribs;
		
		FORCEINLINE void ClearCurAttribs() 
		{ 
			m_CurAttribs.m_nTotalBufferRevision = 0;
			m_CurAttribs.m_pVertDecl = NULL;
			memset( m_CurAttribs.m_streams, 0, sizeof( m_CurAttribs.m_streams ) );
			m_CurAttribs.m_vtxAttribMap[0] = 0xBBBBBBBBBBBBBBBBULL;
			m_CurAttribs.m_vtxAttribMap[1] = 0xBBBBBBBBBBBBBBBBULL;
		}

		FORCEINLINE void ReleasedShader() {	NullProgram(); }
		
		// textures
		FORCEINLINE void SelectTMU( int tmu )
		{
			if ( tmu != m_activeTexture )
			{
				gGL->glActiveTexture( GL_TEXTURE0 + tmu );
				m_activeTexture = tmu;
			}
		}

		void BindTexToTMU( CGLMTex *tex, int tmu );
				
		// render targets / FBO's
		void BindFBOToCtx( CGLMFBO *fbo, GLenum bindPoint = GL_FRAMEBUFFER_EXT );				// you can also choose GL_READ_FRAMEBUFFER_EXT / GL_DRAW_FRAMEBUFFER_EXT
						
		// buffers
		FORCEINLINE void BindGLBufferToCtx( GLenum nGLBufType, GLuint nGLName, bool bForce = false )
		{
			Assert( ( nGLBufType == GL_ARRAY_BUFFER_ARB ) || ( nGLBufType == GL_ELEMENT_ARRAY_BUFFER_ARB ) );
						
			const uint nIndex = ( nGLBufType == GL_ARRAY_BUFFER_ARB ) ? kGLMVertexBuffer : kGLMIndexBuffer;
			if ( ( bForce ) || ( m_nBoundGLBuffer[nIndex] != nGLName ) )
			{
				m_nBoundGLBuffer[nIndex] = nGLName;
				gGL->glBindBufferARB( nGLBufType, nGLName );
			}
		}
				
		void BindBufferToCtx( EGLMBufferType type, CGLMBuffer *buff, bool force = false );		// does not twiddle any enables.

		FORCEINLINE void BindIndexBufferToCtx( CGLMBuffer *buff );
		FORCEINLINE void BindVertexBufferToCtx( CGLMBuffer *buff );
						
		CPinnedMemoryBuffer *GetCurPinnedMemoryBuffer( ) { return &m_PinnedMemoryBuffers[m_nCurPinnedMemoryBuffer]; }
						
		// members------------------------------------------
		
		// context
		DWORD							m_nCurOwnerThreadId;
		uint							m_nThreadOwnershipReleaseCounter;

		bool							m_bUseSamplerObjects;
		bool							m_bPreferMapBufferRange;

		IDirect3DDevice9				*m_pDevice;
		GLMRendererInfoFields			m_caps;
					
		bool							m_displayParamsValid;		// is there a param block copied in yet
		GLMDisplayParams				m_displayParams;			// last known display config, either via constructor, or by SetDisplayParams...
		
#ifdef OSX
		CGLPixelFormatAttribute			m_pixelFormatAttribs[100];	// more than enough
		PseudoNSGLContextPtr			m_nsctx;
		CGLContextObj					m_ctx;
#elif defined( USE_SDL )
		int								m_pixelFormatAttribs[100];	// more than enough
		PseudoNSGLContextPtr			m_nsctx;
		void *							m_ctx;
#endif
		bool							m_oneCtxEnable;			// true if we use the window's context directly instead of making a second one shared against it

		bool							m_bUseBoneUniformBuffers; // if true, we use two uniform buffers for vertex shader constants vs. one

		// texture form table
		CGLMTexLayoutTable				*m_texLayoutTable;

		// context state mirrors
		
		GLState<GLAlphaTestEnable_t>	m_AlphaTestEnable;
		
		GLState<GLAlphaTestFunc_t>		m_AlphaTestFunc;
		
		GLState<GLCullFaceEnable_t>		m_CullFaceEnable;			
		GLState<GLCullFrontFace_t>		m_CullFrontFace;	
		GLState<GLPolygonMode_t>		m_PolygonMode;
		
		GLState<GLDepthBias_t>			m_DepthBias;		
		
		GLStateArray<GLClipPlaneEnable_t,kGLMUserClipPlanes> m_ClipPlaneEnable;
		GLStateArray<GLClipPlaneEquation_t,kGLMUserClipPlanes> m_ClipPlaneEquation;	// dxabstract puts them directly into param slot 253(0) and 254(1)
		
		GLState<GLScissorEnable_t>		m_ScissorEnable;	
		GLState<GLScissorBox_t>			m_ScissorBox;

		GLState<GLAlphaToCoverageEnable_t> m_AlphaToCoverageEnable;	
		
		GLState<GLViewportBox_t>		m_ViewportBox;		
		GLState<GLViewportDepthRange_t>	m_ViewportDepthRange;
		
		GLState<GLColorMaskSingle_t>	m_ColorMaskSingle;	
		GLStateArray<GLColorMaskMultiple_t,8>	m_ColorMaskMultiple;	// need an official constant for the color buffers limit
		
		GLState<GLBlendEnable_t>		m_BlendEnable;		
		GLState<GLBlendFactor_t>		m_BlendFactor;		
		GLState<GLBlendEquation_t> m_BlendEquation;	
		GLState<GLBlendColor_t>	m_BlendColor;		
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
		
		uint8							m_nDirtySamplerFlags[GLM_SAMPLER_COUNT];	// 0 if the sampler is dirty, 1 if not
		uint32							m_nNumDirtySamplers;						// # of unique dirty sampler indices in m_nDirtySamplers
		uint8							m_nDirtySamplers[GLM_SAMPLER_COUNT + 1];	// dirty sampler indices

		void MarkAllSamplersDirty();
						
		struct SamplerHashEntry
		{
			GLuint m_samplerObject;
			GLMTexSamplingParams m_params;
		};

		enum 
		{ 
			cSamplerObjectHashBits = 9, cSamplerObjectHashSize = 1 << cSamplerObjectHashBits 
		};
		SamplerHashEntry				m_samplerObjectHash[cSamplerObjectHashSize];
		uint							m_nSamplerObjectHashNumEntries;
					
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
		bool							m_bDirtyPrograms;
						
		GLMProgramParamsF				m_programParamsF[ kGLMNumProgramTypes ];
		GLMProgramParamsB				m_programParamsB[ kGLMNumProgramTypes ];	// two banks, but only the vertex one is used
		GLMProgramParamsI				m_programParamsI[ kGLMNumProgramTypes ];	// two banks, but only the vertex one is used
		EGLMParamWriteMode				m_paramWriteMode;
		
		CGLMProgram						*m_pNullFragmentProgram;		// write opaque black.  Activate when caller asks for null FP

		CGLMProgram						*m_preloadTexVertexProgram;			// programs to help preload textures (dummies)
		CGLMProgram						*m_preload2DTexFragmentProgram;
		CGLMProgram						*m_preload3DTexFragmentProgram;
		CGLMProgram						*m_preloadCubeTexFragmentProgram;
				
		CGLMShaderPairCache				*m_pairCache;				// GLSL only
		CGLMShaderPair					*m_pBoundPair;				// GLSL only

		FORCEINLINE void NewLinkedProgram() { ClearCurAttribs(); }

		//uint							m_boundPairRevision;		// GLSL only
		//GLhandleARB						m_boundPairProgram;			// GLSL only

		// buffer bindings
		GLuint							m_nBoundGLBuffer[kGLMNumBufferTypes];
		
		struct VertexAttribs_t
		{
			GLuint m_nCompCount;
			GLenum m_datatype;
			GLboolean m_normalized;
			GLuint m_stride;
			const void *m_pPtr;
			uint m_revision;
		};

		VertexAttribs_t					m_boundVertexAttribs[ kGLMVertexAttributeIndexMax ];	// tracked per attrib for dupe-set-absorb
		uint							m_lastKnownVertexAttribMask;								// tracked for dupe-enable-absorb
		int								m_nNumSetVertexAttributes;
						
		// FIXME: Remove this, it's no longer used
		GLMVertexSetup					m_drawVertexSetup;

		EGLMAttribWriteMode				m_attribWriteMode;
		
		bool							m_slowCheckEnable;		// turn this on or no native checking is done ("-glmassertslow" or "-glmsspewslow")
		bool							m_slowAssertEnable;		// turn this on to assert on a non-native batch	"-glmassertslow"
		bool							m_slowSpewEnable;	// turn this on to log non-native batches to stdout "-glmspewslow"
		bool							m_checkglErrorsAfterEveryBatch;	// turn this on to check for GL errors after each batch (slow) ("-glcheckerrors")
		
		// debug font texture
		CGLMTex							*m_debugFontTex;		// might be NULL unless you call GenDebugFontTex
		CGLMBuffer						*m_debugFontIndices;	// up to 1024 indices (256 chars times 4)
		CGLMBuffer						*m_debugFontVertices;	// up to 1024 verts

		// batch/frame debugging support
		int								m_debugFrameIndex;			// init to -1. Increment at BeginFrame
														
		int							    m_nMaxUsedVertexProgramConstantsHint;
		
		uint32							m_dwRenderThreadId;
		volatile bool					m_bIsThreading;
				
		uint m_nCurFrame;
		uint m_nBatchCounter;

		enum { cNumPinnedMemoryBuffers = 4 };
		CPinnedMemoryBuffer m_PinnedMemoryBuffers[cNumPinnedMemoryBuffers];
		uint m_nCurPinnedMemoryBuffer;
		
		void SaveColorMaskAndSetToDefault();
		void RestoreSavedColorMask();
		GLColorMaskSingle_t				m_SavedColorMask;
										
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

#if GL_BATCH_PERF_ANALYSIS
		uint m_nTotalVSUniformCalls;
		uint m_nTotalVSUniformBoneCalls;
		uint m_nTotalVSUniformsSet;
		uint m_nTotalVSUniformsBoneSet;
		uint m_nTotalPSUniformCalls;
		uint m_nTotalPSUniformsSet;
		
		CFlushDrawStatesStats m_FlushStats;
#endif
};

FORCEINLINE void GLMContext::DrawRangeElements(	GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices, uint baseVertex, CGLMBuffer *pIndexBuf )
{
#if GL_ENABLE_INDEX_VERIFICATION
	DrawRangeElementsNonInline( mode, start, end, count, type, indices, baseVertex, pIndexBuf );
#else

#if GLMDEBUG
	GLM_FUNC;
#else
	//tmZone( TELEMETRY_LEVEL0, TMZF_NONE, "%s %d-%d count:%d mode:%d type:%d", __FUNCTION__, start, end, count, mode, type );
#endif
		
	++m_nBatchCounter;

	SetIndexBuffer( pIndexBuf );

	void *indicesActual = (void*)indices;
	
	if ( pIndexBuf->m_bPseudo )
	{
		// you have to pass actual address, not offset
		indicesActual = (void*)( (int)indicesActual + (int)pIndexBuf->m_pPseudoBuf );
	}

#if GLMDEBUG
	bool	hasVP = m_drawingProgram[ kGLMVertexProgram ] != NULL;
	bool	hasFP = m_drawingProgram[ kGLMFragmentProgram ] != NULL;

	// init debug hook information
	GLMDebugHookInfo info;
	memset( &info, 0, sizeof(info) );
	info.m_caller = eDrawElements;

	// relay parameters we're operating under
	info.m_drawMode = mode;
	info.m_drawStart = start;
	info.m_drawEnd = end;
	info.m_drawCount = count;
	info.m_drawType = type;
	info.m_drawIndices = indices;
		
	do
	{
		// obey global options re pre-draw clear
		if ( m_autoClearColor || m_autoClearDepth || m_autoClearStencil )
		{
			GLMPRINTF(("-- DrawRangeElements auto clear" ));
			this->DebugClear();
		}

		// always sync with editable shader text prior to draw
#if GLMDEBUG
		//FIXME disengage this path if context is in GLSL mode..
		// it will need fixes to get the shader pair re-linked etc if edits happen anyway.

		if (m_drawingProgram[ kGLMVertexProgram ])
		{
			m_drawingProgram[ kGLMVertexProgram ]->SyncWithEditable();
		}
		else
		{
			AssertOnce(!"drawing with no vertex program bound");
		}


		if (m_drawingProgram[ kGLMFragmentProgram ])
		{
			m_drawingProgram[ kGLMFragmentProgram ]->SyncWithEditable();
		}
		else
		{
			AssertOnce(!"drawing with no fragment program bound");
		}
#endif
		// do the drawing
		if (hasVP && hasFP)
		{
			gGL->glDrawRangeElementsBaseVertex( mode, start, end, count, type, indicesActual, baseVertex );

			if ( m_slowCheckEnable )
			{
				CheckNative();
			}
		}
		this->DebugHook( &info );

	} while ( info.m_loop );
#else
	Assert( m_drawingLang == kGLMGLSL );

	if ( m_pBoundPair )
	{
		gGL->glDrawRangeElementsBaseVertex( mode, start, end, count, type, indicesActual, baseVertex );

#if GLMDEBUG
		if ( m_slowCheckEnable )
		{
			CheckNative();
		}
#endif
	}
#endif

#endif // GL_ENABLE_INDEX_VERIFICATION
}

FORCEINLINE void GLMContext::SetVertexProgram( CGLMProgram *pProg )
{
	m_drawingProgram[kGLMVertexProgram] = pProg;
	m_bDirtyPrograms = true;
}

FORCEINLINE void GLMContext::SetFragmentProgram( CGLMProgram *pProg )
{
	m_drawingProgram[kGLMFragmentProgram] = pProg ? pProg : m_pNullFragmentProgram;
	m_bDirtyPrograms = true;
}

// "slot" means a vec4-sized thing
// these write into .env parameter space
FORCEINLINE void GLMContext::SetProgramParametersF( EGLMProgramType type, uint baseSlot, float *slotData, uint slotCount )
{
#if GLMDEBUG
	GLM_FUNC;
#endif

	Assert( baseSlot < kGLMProgramParamFloat4Limit );
	Assert( baseSlot+slotCount <= kGLMProgramParamFloat4Limit );

#if GLMDEBUG
	GLMPRINTF(("-S-GLMContext::SetProgramParametersF %s slots %d - %d: ", (type==kGLMVertexProgram) ? "VS" : "FS", baseSlot, baseSlot + slotCount - 1 ));
	for( uint i=0; i<slotCount; i++ )
	{
		GLMPRINTF((		"-S-    %03d: [ %7.4f %7.4f %7.4f %7.4f ]",
			baseSlot+i,
			slotData[i*4], slotData[i*4+1], slotData[i*4+2], slotData[i*4+3]
		));
	}
#endif

	memcpy( &m_programParamsF[type].m_values[baseSlot][0], slotData, (4 * sizeof(float)) * slotCount );

	if ( ( type == kGLMVertexProgram ) && ( m_bUseBoneUniformBuffers ) )
	{
		if ( ( baseSlot + slotCount ) > DXABSTRACT_VS_FIRST_BONE_SLOT )
		{
			if ( baseSlot < DXABSTRACT_VS_FIRST_BONE_SLOT ) 
			{
				// The register set crosses between the constant buffers - should only happen at startup during init.
				m_programParamsF[kGLMVertexProgram].m_firstDirtySlotNonBone = MIN( m_programParamsF[kGLMVertexProgram].m_firstDirtySlotNonBone, (int)baseSlot );
				m_programParamsF[kGLMVertexProgram].m_dirtySlotHighWaterNonBone = MAX( m_programParamsF[kGLMVertexProgram].m_dirtySlotHighWaterNonBone, (int)MIN( baseSlot + slotCount, DXABSTRACT_VS_FIRST_BONE_SLOT ) );
				baseSlot = DXABSTRACT_VS_FIRST_BONE_SLOT;
			}

			int nNumActualBones = ( baseSlot + slotCount ) - DXABSTRACT_VS_FIRST_BONE_SLOT;
			m_programParamsF[kGLMVertexProgram].m_dirtySlotHighWaterBone = MAX( m_programParamsF[kGLMVertexProgram].m_dirtySlotHighWaterBone, nNumActualBones );
		}
		else
		{
			m_programParamsF[kGLMVertexProgram].m_firstDirtySlotNonBone = MIN( m_programParamsF[kGLMVertexProgram].m_firstDirtySlotNonBone, (int)baseSlot );
			m_programParamsF[kGLMVertexProgram].m_dirtySlotHighWaterNonBone = MAX( m_programParamsF[kGLMVertexProgram].m_dirtySlotHighWaterNonBone, (int)(baseSlot + slotCount) );
		}
	}
	else
	{
		m_programParamsF[type].m_dirtySlotHighWaterNonBone = MAX( m_programParamsF[type].m_dirtySlotHighWaterNonBone, (int)(baseSlot + slotCount) );
		m_programParamsF[type].m_firstDirtySlotNonBone = MIN( m_programParamsF[type].m_firstDirtySlotNonBone, (int)baseSlot );
	}
}

FORCEINLINE void GLMContext::SetProgramParametersB( EGLMProgramType type, uint baseSlot, int *slotData, uint boolCount )
{
#if GLMDEBUG
	GLM_FUNC;
#endif

	Assert( m_drawingLang == kGLMGLSL );
	Assert( type==kGLMVertexProgram );

	Assert( baseSlot < kGLMProgramParamBoolLimit );
	Assert( baseSlot+boolCount <= kGLMProgramParamBoolLimit );

#if GLMDEBUG
	GLMPRINTF(("-S-GLMContext::SetProgramParametersB %s bools %d - %d: ", (type==kGLMVertexProgram) ? "VS" : "FS", baseSlot, baseSlot + boolCount - 1 ));
	for( uint i=0; i<boolCount; i++ )
	{
		GLMPRINTF((		"-S-    %03d: %d (bool)",
			baseSlot+i,
			slotData[i]
		));
	}
#endif

	memcpy( &m_programParamsB[type].m_values[baseSlot], slotData, sizeof(int) * boolCount );
	
	if ( (baseSlot+boolCount) > m_programParamsB[type].m_dirtySlotCount)
		m_programParamsB[type].m_dirtySlotCount = baseSlot+boolCount;
}

FORCEINLINE void GLMContext::SetProgramParametersI( EGLMProgramType type, uint baseSlot, int *slotData, uint slotCount )	// groups of 4 ints...
{
#if GLMDEBUG
	GLM_FUNC;
#endif

	Assert( m_drawingLang == kGLMGLSL );
	Assert( type==kGLMVertexProgram );

	Assert( baseSlot < kGLMProgramParamInt4Limit );
	Assert( baseSlot+slotCount <= kGLMProgramParamInt4Limit );

#if GLMDEBUG
	GLMPRINTF(("-S-GLMContext::SetProgramParametersI %s slots %d - %d: ", (type==kGLMVertexProgram) ? "VS" : "FS", baseSlot, baseSlot + slotCount - 1 ));
	for( uint i=0; i<slotCount; i++ )
	{
		GLMPRINTF((		"-S-    %03d: %d %d %d %d (int4)",
			baseSlot+i,
			slotData[i*4],slotData[i*4+1],slotData[i*4+2],slotData[i*4+3]
		));
	}
#endif

	memcpy( &m_programParamsI[type].m_values[baseSlot][0], slotData, (4*sizeof(int)) * slotCount );
	
	if ( (baseSlot + slotCount) > m_programParamsI[type].m_dirtySlotCount)
	{
		m_programParamsI[type].m_dirtySlotCount = baseSlot + slotCount;
	}
}

FORCEINLINE void GLMContext::SetSamplerDirty( int sampler )
{
	Assert( sampler < GLM_SAMPLER_COUNT );
	m_nDirtySamplers[m_nNumDirtySamplers] = sampler;
	m_nNumDirtySamplers += m_nDirtySamplerFlags[sampler];
	m_nDirtySamplerFlags[sampler] = 0;
}

FORCEINLINE void GLMContext::SetSamplerTex( int sampler, CGLMTex *tex ) 
{ 
	Assert( sampler < GLM_SAMPLER_COUNT );
	m_samplers[sampler].m_pBoundTex = tex;
	if ( tex )
	{
		if ( !gGL->m_bHave_GL_EXT_direct_state_access )
		{
			if ( sampler != m_activeTexture )
			{
				gGL->glActiveTexture( GL_TEXTURE0 + sampler );
				m_activeTexture = sampler;
			}

			gGL->glBindTexture( tex->m_texGLTarget, tex->m_texName );
		}
		else
		{
			gGL->glBindMultiTextureEXT( GL_TEXTURE0 + sampler, tex->m_texGLTarget, tex->m_texName );
		}
	}
	
	if ( !m_bUseSamplerObjects )
	{
		SetSamplerDirty( sampler );
	}
}

FORCEINLINE void GLMContext::SetSamplerMinFilter( int sampler, GLenum Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MIN_FILTER_BITS ) );
	m_samplers[sampler].m_samp.m_packed.m_minFilter = Value;
}

FORCEINLINE void GLMContext::SetSamplerMagFilter( int sampler, GLenum Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MAG_FILTER_BITS ) );
	m_samplers[sampler].m_samp.m_packed.m_magFilter = Value;
}

FORCEINLINE void GLMContext::SetSamplerMipFilter( int sampler, GLenum Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MIP_FILTER_BITS ) );
	m_samplers[sampler].m_samp.m_packed.m_mipFilter = Value;
}

FORCEINLINE void GLMContext::SetSamplerAddressU( int sampler, GLenum Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS) );
	m_samplers[sampler].m_samp.m_packed.m_addressU = Value;
}

FORCEINLINE void GLMContext::SetSamplerAddressV( int sampler, GLenum Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS) );
	m_samplers[sampler].m_samp.m_packed.m_addressV = Value;
}

FORCEINLINE void GLMContext::SetSamplerAddressW( int sampler, GLenum Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS) );
	m_samplers[sampler].m_samp.m_packed.m_addressW = Value;
}

FORCEINLINE void GLMContext::SetSamplerStates( int sampler, GLenum AddressU, GLenum AddressV, GLenum AddressW, GLenum minFilter, GLenum magFilter, GLenum mipFilter )
{
	Assert( AddressU < ( 1 << GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS) );
	Assert( AddressV < ( 1 << GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS) );
	Assert( AddressW < ( 1 << GLM_PACKED_SAMPLER_PARAMS_ADDRESS_BITS) );
	Assert( minFilter < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MIN_FILTER_BITS ) );
	Assert( magFilter < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MAG_FILTER_BITS ) );
	Assert( mipFilter < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MIP_FILTER_BITS ) );

	GLMTexSamplingParams &params = m_samplers[sampler].m_samp;
	params.m_packed.m_addressU = AddressU;
	params.m_packed.m_addressV = AddressV;
	params.m_packed.m_addressW = AddressW;
	params.m_packed.m_minFilter = minFilter;
	params.m_packed.m_magFilter = magFilter;
	params.m_packed.m_mipFilter = mipFilter;
}

FORCEINLINE void GLMContext::SetSamplerBorderColor( int sampler, DWORD Value )
{
	m_samplers[sampler].m_samp.m_borderColor = Value;
}

FORCEINLINE void GLMContext::SetSamplerMipMapLODBias( int sampler, DWORD Value )
{
	// not currently supported
}

FORCEINLINE void GLMContext::SetSamplerMaxMipLevel( int sampler, DWORD Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MIN_LOD_BITS ) );
	m_samplers[sampler].m_samp.m_packed.m_minLOD = Value;
}

FORCEINLINE void GLMContext::SetSamplerMaxAnisotropy( int sampler, DWORD Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_MAX_ANISO_BITS ) );
	m_samplers[sampler].m_samp.m_packed.m_maxAniso = Value;
}

FORCEINLINE void GLMContext::SetSamplerSRGBTexture( int sampler, DWORD Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_SRGB_BITS ) );
	m_samplers[sampler].m_samp.m_packed.m_srgb = Value;
}

FORCEINLINE void GLMContext::SetShadowFilter( int sampler, DWORD Value )
{
	Assert( Value < ( 1 << GLM_PACKED_SAMPLER_PARAMS_COMPARE_MODE_BITS ) );
	m_samplers[sampler].m_samp.m_packed.m_compareMode = Value;
}

FORCEINLINE void GLMContext::BindIndexBufferToCtx( CGLMBuffer *buff )
{
	GLMPRINTF(( "--- GLMContext::BindIndexBufferToCtx buff %p, GL name %d", buff, (buff) ? buff->m_nHandle : -1 ));
		
	Assert( !buff || ( buff->m_buffGLTarget == GL_ELEMENT_ARRAY_BUFFER_ARB ) );
		
	GLuint nGLName = buff ? buff->m_nHandle : 0;

	if ( m_nBoundGLBuffer[ kGLMIndexBuffer] == nGLName )
		return;

	m_nBoundGLBuffer[ kGLMIndexBuffer] = nGLName;
	gGL->glBindBufferARB( GL_ELEMENT_ARRAY_BUFFER_ARB, nGLName );
}

FORCEINLINE void GLMContext::BindVertexBufferToCtx( CGLMBuffer *buff )
{
	GLMPRINTF(( "--- GLMContext::BindVertexBufferToCtx buff %p, GL name %d", buff, (buff) ? buff->m_nHandle : -1 ));

	Assert( !buff || ( buff->m_buffGLTarget == GL_ARRAY_BUFFER_ARB ) );
		
	GLuint nGLName = buff ? buff->m_nHandle : 0;

	if ( m_nBoundGLBuffer[ kGLMVertexBuffer] == nGLName )
		return;

	m_nBoundGLBuffer[ kGLMVertexBuffer] = nGLName;
	gGL->glBindBufferARB( GL_ARRAY_BUFFER_ARB, nGLName );
}

FORCEINLINE void GLMContext::SetMaxUsedVertexShaderConstantsHint( uint nMaxConstants )
{
	static bool bUseMaxVertexShadeConstantHints = !CommandLine()->CheckParm("-disablemaxvertexshaderconstanthints");
	if ( bUseMaxVertexShadeConstantHints )
	{
		m_nMaxUsedVertexProgramConstantsHint = nMaxConstants;
	}
}

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

#endif // GLMGR_H

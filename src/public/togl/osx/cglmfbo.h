//========= Copyright Valve Corporation, All rights reserved. ============//
//
// cglmfbo.h
//	GLMgr FBO's (render targets)
//
//===============================================================================

#ifndef CGLMFBO_H
#define	CGLMFBO_H

#pragma once

#include "togl/rendermechanism.h"

// good FBO references / recaps
// http://www.songho.ca/opengl/gl_fbo.html
// http://www.gamedev.net/reference/articles/article2331.asp

// ext links

// http://www.opengl.org/registry/specs/EXT/framebuffer_object.txt
// http://www.opengl.org/registry/specs/EXT/framebuffer_multisample.txt

//===============================================================================

// tokens not in the SDK headers

#ifndef	GL_DEPTH_STENCIL_ATTACHMENT_EXT
	#define GL_DEPTH_STENCIL_ATTACHMENT_EXT 0x84F9
#endif

//===============================================================================

// forward declarations

class	GLMContext;

// implicitly 16 maximum color attachments possible
enum EGLMFBOAttachment {
	kAttColor0,		kAttColor1,		kAttColor2,		kAttColor3,
	kAttColor4,		kAttColor5,		kAttColor6,		kAttColor7,
	kAttColor8,		kAttColor9,		kAttColor10,	kAttColor11,
	kAttColor12,	kAttColor13,	kAttColor14,	kAttColor15,
	kAttDepth,		kAttStencil,	kAttDepthStencil,
	kAttCount
};

struct GLMFBOTexAttachParams
{	
	CGLMTex				*m_tex;
	int					m_face;		// keep zero if not cube map
	int					m_mip;		// keep zero if notmip mapped
	int					m_zslice;	// keep zero if not a 3D tex
};

class CGLMFBO
{

public:

protected:
	friend class GLMContext;			// only GLMContext can make CGLMFBO objects
	friend class GLMTester;
	friend class CGLMTex;
	
	friend class IDirect3D9;
	friend class IDirect3DDevice9;
		
			CGLMFBO( GLMContext *ctx );
			~CGLMFBO( );	

	void	TexAttach( GLMFBOTexAttachParams *params, EGLMFBOAttachment attachIndex, GLenum fboBindPoint = GL_FRAMEBUFFER_EXT );
	void	TexDetach( EGLMFBOAttachment attachIndex, GLenum fboBindPoint = GL_FRAMEBUFFER_EXT );
		// you can also pass GL_READ_FRAMEBUFFER_EXT or GL_DRAW_FRAMEBUFFER_EXT to selectively bind the receiving FBO to one or the other.

	void	TexScrub( CGLMTex *tex );
		// search and destroy any attachment for the named texture
	
	bool	IsReady( void );				// aka FBO completeness check - ready to draw
	
	GLMContext				*m_ctx;			// link back to parent context

	GLuint					m_name;			// name of this FBO in the context
	
	GLMFBOTexAttachParams	m_attach[ kAttCount ];	// indexed by EGLMFBOAttachment
	
	int						m_sizeX,m_sizeY;
};	


#endif

//========= Copyright Valve Corporation, All rights reserved. ============//
//
// cglmprogram.h
//	GLMgr buffers (index / vertex)
//	... maybe add PBO later as well
//===============================================================================

#ifndef CGLMBUFFER_H
#define	CGLMBUFFER_H

#pragma once

// ext links

// http://www.opengl.org/registry/specs/ARB/vertex_buffer_object.txt

//===============================================================================

// tokens not in the SDK headers

//#ifndef	GL_DEPTH_STENCIL_ATTACHMENT_EXT
//	#define GL_DEPTH_STENCIL_ATTACHMENT_EXT 0x84F9
//#endif

//===============================================================================

// forward declarations

class GLMContext;

enum EGLMBufferType
{
	kGLMVertexBuffer,
	kGLMIndexBuffer,
	kGLMUniformBuffer,	// for bindable uniform
	kGLMPixelBuffer,	// for PBO
	
	kGLMNumBufferTypes
};

	// pass this in "options" to constructor to make a dynamic buffer
#define	GLMBufferOptionDynamic	0x00000001

struct GLMBuffLockParams
{
	uint	m_offset;
	uint	m_size;
	bool	m_nonblocking;
	bool	m_discard;
};

class CGLMBuffer
{

public:
	void	Lock( GLMBuffLockParams *params, char **addressOut );
	void	Unlock( void );

//protected:
	friend class GLMContext;			// only GLMContext can make CGLMBuffer objects
	friend class GLMTester;	
	friend class IDirect3D9;
	friend class IDirect3DDevice9;
		
	CGLMBuffer					( GLMContext *ctx, EGLMBufferType type, uint size, uint options );
	~CGLMBuffer					( );	
	
	void	SetModes			( bool asyncMap, bool explicitFlush, bool force = false );
	void	FlushRange			( uint offset, uint size );
	
	GLMContext				*m_ctx;					// link back to parent context
	EGLMBufferType			m_type;
	uint					m_size;
	GLenum					m_buffGLTarget;			// GL_ARRAY_BUFFER_ARB / GL_ELEMENT_BUFFER_ARB	
	GLuint					m_name;					// name of this program in the context	
	uint					m_revision;				// bump anytime the size changes or buffer is orphaned
	bool					m_enableAsyncMap;		// mirror of the buffer state
	bool					m_enableExplicitFlush;	// mirror of the buffer state

	bool					m_bound;				// true if bound to context		
	bool					m_mapped;				// is it currently mapped
	uint					m_dirtyMinOffset;		// when equal, range is empty
	uint					m_dirtyMaxOffset;
	
	float					*m_lastMappedAddress;
	
	// --------------------- pseudo-VBO support below here (explicitly for dynamic index buffers)
	bool					m_pseudo;				// true if the m_name is 0, and the backing is plain RAM
	
	// in pseudo mode, there is just one RAM buffer that acts as the backing.
	// expectation is that this mode would only be used for dynamic indices.
	// since indices have to be consumed (copied to command stream) prior to return from a drawing call,
	// there's no need to do any fencing or multibuffering.  orphaning in particular becomes a no-op.
	
	char					*m_pseudoBuf;			// storage for pseudo buffer
};	


#endif
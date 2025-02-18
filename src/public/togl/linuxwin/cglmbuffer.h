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
// cglmprogram.h
//	GLMgr buffers (index / vertex)
//	... maybe add PBO later as well
//===============================================================================

#ifndef CGLMBUFFER_H
#define	CGLMBUFFER_H

#pragma once

//===============================================================================

extern bool g_bUsePseudoBufs;

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
	uint m_nOffset;
	uint m_nSize;
	bool m_bNoOverwrite;		
	bool m_bDiscard;			
};

#define GL_STATIC_BUFFER_SIZE	( 2048 * 1024 )
#define GL_MAX_STATIC_BUFFERS	2

extern void glBufferSubDataMaxSize( GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data, uint nMaxSizePerCall = 128 * 1024 );

//===========================================================================//

// Creates an immutable storage for a buffer object
// https://www.opengl.org/registry/specs/ARB/buffer_storage.txt
class CPersistentBuffer
{
public:

	CPersistentBuffer();
	~CPersistentBuffer();

	void Init( EGLMBufferType type,uint nSize );
	void Deinit();

	void InsertFence();
	void BlockUntilNotBusy();

	void Append( uint nSize );

	inline uint GetBytesRemaining() const { return m_nSize - m_nOffset; }
	inline uint GetOffset() const { return m_nOffset; }
	inline void *GetPtr() const { return m_pImmutablePersistentBuf; }
	inline GLuint GetHandle() const { return m_nHandle; }

private:

	CPersistentBuffer( const CPersistentBuffer & );
	CPersistentBuffer & operator= (const CPersistentBuffer &);

	uint			m_nSize;

	EGLMBufferType	m_type;
	GLenum			m_buffGLTarget;			// GL_ARRAY_BUFFER_ARB / GL_ELEMENT_BUFFER_ARB	
	GLuint			m_nHandle;				// handle of this program in the GL context

	// Holds a pointer to the persistently mapped buffer
	void*			m_pImmutablePersistentBuf;

	uint			m_nOffset;

#ifdef HAVE_GL_ARB_SYNC
	GLsync			m_nSyncObj;
#endif
};

//===============================================================================

#if GL_ENABLE_INDEX_VERIFICATION

struct GLDynamicBuf_t
{
	GLenum m_nGLType;
	uint m_nHandle;
	uint m_nActualBufSize;
	uint m_nSize;

	uint m_nLockOffset;
	uint m_nLockSize;
};

class CGLMBufferSpanManager
{
	CGLMBufferSpanManager( const CGLMBufferSpanManager& );
	CGLMBufferSpanManager& operator= ( const CGLMBufferSpanManager& );

public:
	CGLMBufferSpanManager();
	~CGLMBufferSpanManager();

	void Init( GLMContext *pContext, EGLMBufferType nBufType, uint nInitialCapacity, uint nBufSize, bool bDynamic );
	void Deinit();

	inline GLMContext *GetContext() const { return m_pCtx; }

	inline GLenum GetGLBufType() const { return ( m_nBufType == kGLMVertexBuffer ) ? GL_ARRAY_BUFFER_ARB : GL_ELEMENT_ARRAY_BUFFER_ARB; }

	struct ActiveSpan_t
	{
		uint m_nStart;
		uint m_nEnd;

		GLDynamicBuf_t m_buf;
		bool m_bOriginalAlloc;

		inline ActiveSpan_t() { }
		inline ActiveSpan_t( uint nStart, uint nEnd, GLDynamicBuf_t &buf, bool bOriginalAlloc ) : m_nStart( nStart ), m_nEnd( nEnd ), m_buf( buf ), m_bOriginalAlloc( bOriginalAlloc ) { Assert( nStart <= nEnd ); }
	};

	ActiveSpan_t *AddSpan( uint nOffset, uint nMaxSize, uint nActualSize, bool bDiscard, bool bNoOverwrite );
	
	void DiscardAllSpans();
		
	bool IsValid( uint nOffset, uint nSize ) const;

private:
	bool AllocDynamicBuf( uint nSize, GLDynamicBuf_t &buf );
	void ReleaseDynamicBuf( GLDynamicBuf_t &buf );

	GLMContext *m_pCtx;

	EGLMBufferType m_nBufType;

	uint m_nBufSize;
	bool m_bDynamic;

	CUtlVector<ActiveSpan_t> m_ActiveSpans;
	CUtlVector<ActiveSpan_t> m_DeletedSpans;

	int m_nSpanEndMax;

	int m_nNumAllocatedBufs;
	int m_nTotalBytesAllocated;
};

#endif // GL_ENABLE_INDEX_VERIFICATION

class CGLMBuffer
{
public:
	void Lock( GLMBuffLockParams *pParams, char **pAddressOut );
	void Unlock( int nActualSize = -1, const void *pActualData = NULL );

	GLuint GetHandle() const;
		
	friend class GLMContext;			// only GLMContext can make CGLMBuffer objects
	friend class GLMTester;	
	friend struct IDirect3D9;
	friend struct IDirect3DDevice9;
		
	CGLMBuffer( GLMContext *pCtx, EGLMBufferType type, uint size, uint options );
	~CGLMBuffer();
	
	void SetModes( bool bAsyncMap, bool bExplicitFlush, bool bForce = false );
	void FlushRange( uint offset, uint size );

#if GL_ENABLE_INDEX_VERIFICATION
	bool IsSpanValid( uint nOffset, uint nSize ) const;
#endif
	
	GLMContext				*m_pCtx;					// link back to parent context
	EGLMBufferType			m_type;
	uint					m_nSize;
	uint					m_nActualSize;
	
	bool					m_bDynamic;
	
	GLenum					m_buffGLTarget;			// GL_ARRAY_BUFFER_ARB / GL_ELEMENT_BUFFER_ARB	
	GLuint					m_nHandle;					// name of this program in the context	

	uint					m_nRevision;				// bump anytime the size changes or buffer is orphaned

	bool					m_bEnableAsyncMap;		// mirror of the buffer state
	bool					m_bEnableExplicitFlush;	// mirror of the buffer state
		
	bool					m_bMapped;				// is it currently mapped

	uint					m_dirtyMinOffset;		// when equal, range is empty
	uint					m_dirtyMaxOffset;
	
	float					*m_pLastMappedAddress;

	int						m_nPinnedMemoryOfs;

	uint					m_nPersistentBufferStartOffset;
	bool					m_bUsingPersistentBuffer;
	
	bool					m_bPseudo;				// true if the m_name is 0, and the backing is plain RAM
			
	// in pseudo mode, there is just one RAM buffer that acts as the backing.
	// expectation is that this mode would only be used for dynamic indices.
	// since indices have to be consumed (copied to command stream) prior to return from a drawing call,
	// there's no need to do any fencing or multibuffering.  orphaning in particular becomes a no-op.
	
	char					*m_pActualPseudoBuf;			// storage for pseudo buffer
	char					*m_pPseudoBuf;			// storage for pseudo buffer
	char					*m_pStaticBuffer;
	
	GLMBuffLockParams		m_LockParams;
											
	static char				ALIGN16 m_StaticBuffers[ GL_MAX_STATIC_BUFFERS ][ GL_STATIC_BUFFER_SIZE ] ALIGN16_POST;
	static bool				m_bStaticBufferUsed[ GL_MAX_STATIC_BUFFERS ];

#if GL_ENABLE_INDEX_VERIFICATION
	CGLMBufferSpanManager	m_BufferSpanManager;
#endif

#if GL_ENABLE_UNLOCK_BUFFER_OVERWRITE_DETECTION
	uint					m_nDirtyRangeStart;
	uint					m_nDirtyRangeEnd;
#endif
};	

#endif // CGLMBUFFER_H


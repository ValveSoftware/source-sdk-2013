//========= Copyright Valve Corporation, All rights reserved. ============//
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


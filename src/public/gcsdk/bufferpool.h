//========= Copyright ©, Valve LLC, All rights reserved. ======================
//
// Purpose: Defines a buffer pool used to group small allocations
//
//=============================================================================

#ifndef BUFFERPOOL_H
#define BUFFERPOOL_H
#ifdef _WIN32
#pragma once
#endif

#include "tier1/utlbuffer.h"
#include "tier1/utlvector.h"

namespace GCSDK
{

//----------------------------------------------------------------------------
// Purpose: Defines buffers that can be used to group lots of small allocs
//	together to improve performance. The buffers will naturally grow over time
//	to accommodate the largest consumers of the feature.
//----------------------------------------------------------------------------
class CBufferPool
{
public:
	CBufferPool( const char *pchName, const GCConVar &cvMaxSizeMB, const GCConVar &cvInitBufferSize, int nFlags = 0 );
	~CBufferPool();

	CUtlBuffer *GetBuffer();
	void ReturnBuffer( CUtlBuffer *pBuffer );

	static void DumpPools();

private:
	static CUtlVector<CBufferPool *> sm_vecBufferPools;

	const GCConVar &m_cvMaxSizeMB;
	const GCConVar &m_cvInitBufferSize;
	int		m_nFlags;

	int32	m_nBuffersInUse;
	int32	m_nHighWatermark;
	int32	m_nBuffersTotal;
	size_t	m_cubFree;
	CUtlVector<CUtlBuffer *> m_vecFreeBuffers;
	CUtlConstString m_sName;
};

//thread safe version of the above which synchronizes access at the buffer allocate/release
class CBufferPoolMT
{
public:
	CBufferPoolMT( const char *pchName, const GCConVar &cvMaxSizeMB, const GCConVar &cvInitBufferSize, int nFlags = 0 ) :
		m_BufferPool( pchName, cvMaxSizeMB, cvInitBufferSize, nFlags )
	{}

	CUtlBuffer *GetBuffer()						{ AUTO_LOCK( m_mutex ); return m_BufferPool.GetBuffer(); }
	void ReturnBuffer( CUtlBuffer *pBuffer )	{ AUTO_LOCK( m_mutex ); m_BufferPool.ReturnBuffer( pBuffer ); }
private:
	CBufferPool			m_BufferPool;
	CThreadFastMutex	m_mutex;
};

} // namespace GCSDK

#endif // BUFFERPOOL_H

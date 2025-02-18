//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#if defined( _WIN32 ) && !defined( _X360 )
#define WIN_32_LEAN_AND_MEAN
#include <windows.h>
#define VA_COMMIT_FLAGS MEM_COMMIT
#define VA_RESERVE_FLAGS MEM_RESERVE
#elif defined( _X360 )
#define VA_COMMIT_FLAGS (MEM_COMMIT|MEM_NOZERO|MEM_LARGE_PAGES)
#define VA_RESERVE_FLAGS (MEM_RESERVE|MEM_LARGE_PAGES)
#endif

#include "tier0/dbg.h"
#include "memstack.h"
#include "utlmap.h"
#include "tier0/memdbgon.h"

#ifdef _WIN32
#pragma warning(disable:4073)
#pragma init_seg(lib)
#endif

//-----------------------------------------------------------------------------

MEMALLOC_DEFINE_EXTERNAL_TRACKING(CMemoryStack);

//-----------------------------------------------------------------------------

CMemoryStack::CMemoryStack()
 : 	m_pBase( NULL ),
	m_pNextAlloc( NULL ),
	m_pAllocLimit( NULL ),
	m_pCommitLimit( NULL ),
	m_alignment( 16 ),
#if defined(_WIN32)
 	m_commitSize( 0 ),
	m_minCommit( 0 ),
#endif
 	m_maxSize( 0 )
{
}
	
//-------------------------------------

CMemoryStack::~CMemoryStack()
{
	if ( m_pBase )
		Term();
}

//-------------------------------------

bool CMemoryStack::Init( unsigned maxSize, unsigned commitSize, unsigned initialCommit, unsigned alignment )
{
	Assert( !m_pBase );

#ifdef _X360
	m_bPhysical = false;
#endif

	m_maxSize = maxSize;
	m_alignment = AlignValue( alignment, 4 );

	Assert( m_alignment == alignment );
	Assert( m_maxSize > 0 );

#if defined(_WIN32)
	if ( commitSize != 0 )
	{
		m_commitSize = commitSize;
	}

	unsigned pageSize;

#ifndef _X360
	SYSTEM_INFO sysInfo;
	GetSystemInfo( &sysInfo );
	Assert( !( sysInfo.dwPageSize & (sysInfo.dwPageSize-1)) );
	pageSize = sysInfo.dwPageSize;
#else
	pageSize = 64*1024;
#endif

	if ( m_commitSize == 0 )
	{
		m_commitSize = pageSize;
	}
	else
	{
		m_commitSize = AlignValue( m_commitSize, pageSize );
	}

	m_maxSize = AlignValue( m_maxSize, m_commitSize );
	
	Assert( m_maxSize % pageSize == 0 && m_commitSize % pageSize == 0 && m_commitSize <= m_maxSize );

	m_pBase = (unsigned char *)VirtualAlloc( NULL, m_maxSize, VA_RESERVE_FLAGS, PAGE_NOACCESS );
	Assert( m_pBase );
	m_pCommitLimit = m_pNextAlloc = m_pBase;

	if ( initialCommit )
	{
		initialCommit = AlignValue( initialCommit, m_commitSize );
		Assert( initialCommit < m_maxSize );
		if ( !VirtualAlloc( m_pCommitLimit, initialCommit, VA_COMMIT_FLAGS, PAGE_READWRITE ) )
			return false;
		m_minCommit = initialCommit;
		m_pCommitLimit += initialCommit;
		MemAlloc_RegisterExternalAllocation( CMemoryStack, GetBase(), GetSize() );
	}

#else
	m_pBase = (byte *)MemAlloc_AllocAligned( m_maxSize, alignment ? alignment : 1 );
	m_pNextAlloc = m_pBase;
	m_pCommitLimit = m_pBase + m_maxSize;
#endif

	m_pAllocLimit = m_pBase + m_maxSize;

	return ( m_pBase != NULL );
}

//-------------------------------------

#ifdef _X360
bool CMemoryStack::InitPhysical( unsigned size, unsigned alignment )
{
	m_bPhysical = true;

	m_maxSize = m_commitSize = size;
	m_alignment = AlignValue( alignment, 4 );

	int flags = PAGE_READWRITE;
	if ( size >= 16*1024*1024 )
	{
		flags |= MEM_16MB_PAGES;
	}
	else
	{
		flags |= MEM_LARGE_PAGES;
	}
	m_pBase = (unsigned char *)XPhysicalAlloc( m_maxSize, MAXULONG_PTR, 4096, flags );
	Assert( m_pBase );
	m_pNextAlloc = m_pBase;
	m_pCommitLimit = m_pBase + m_maxSize;
	m_pAllocLimit = m_pBase + m_maxSize;

	MemAlloc_RegisterExternalAllocation( CMemoryStack, GetBase(), GetSize() );
	return ( m_pBase != NULL );
}
#endif

//-------------------------------------

void CMemoryStack::Term()
{
	FreeAll();
	if ( m_pBase )
	{
#if defined(_WIN32)
		VirtualFree( m_pBase, 0, MEM_RELEASE );
#else
		MemAlloc_FreeAligned( m_pBase );
#endif
		m_pBase = NULL;
	}
}

//-------------------------------------

int CMemoryStack::GetSize()
{ 
#ifdef _WIN32
	return m_pCommitLimit - m_pBase; 
#else
	return m_maxSize;
#endif
}


//-------------------------------------

bool CMemoryStack::CommitTo( byte *pNextAlloc ) RESTRICT
{
#ifdef _X360
	if ( m_bPhysical )
	{
		return NULL;
	}
#endif
#if defined(_WIN32)
	unsigned char *	pNewCommitLimit = AlignValue( pNextAlloc, m_commitSize );
	unsigned 		commitSize 		= pNewCommitLimit - m_pCommitLimit;
	
	if ( GetSize() )
		MemAlloc_RegisterExternalDeallocation( CMemoryStack, GetBase(), GetSize() );

	if( m_pCommitLimit + commitSize > m_pAllocLimit )
	{
		return false;
	}

	if ( !VirtualAlloc( m_pCommitLimit, commitSize, VA_COMMIT_FLAGS, PAGE_READWRITE ) )
	{
		Assert( 0 );
		return false;
	}
	m_pCommitLimit = pNewCommitLimit;

	if ( GetSize() )
		MemAlloc_RegisterExternalAllocation( CMemoryStack, GetBase(), GetSize() );
	return true;
#else
	Assert( 0 );
	return false;
#endif
}

//-------------------------------------

void CMemoryStack::FreeToAllocPoint( MemoryStackMark_t mark, bool bDecommit )
{
	void *pAllocPoint = m_pBase + mark;
	Assert( pAllocPoint >= m_pBase && pAllocPoint <= m_pNextAlloc );
	
	if ( pAllocPoint >= m_pBase && pAllocPoint < m_pNextAlloc )
	{
		if ( bDecommit )
		{
#if defined(_WIN32)
			unsigned char *pDecommitPoint = AlignValue( (unsigned char *)pAllocPoint, m_commitSize );

			if ( pDecommitPoint < m_pBase + m_minCommit )
			{
				pDecommitPoint = m_pBase + m_minCommit;
			}

			unsigned decommitSize = m_pCommitLimit - pDecommitPoint;

			if ( decommitSize > 0 )
			{
				MemAlloc_RegisterExternalDeallocation( CMemoryStack, GetBase(), GetSize() );

				VirtualFree( pDecommitPoint, decommitSize, MEM_DECOMMIT );
				m_pCommitLimit = pDecommitPoint;

				if ( mark > 0 )
				{
					MemAlloc_RegisterExternalAllocation( CMemoryStack, GetBase(), GetSize() );
				}
			}
#endif
		}
		m_pNextAlloc = (unsigned char *)pAllocPoint;
	}
}

//-------------------------------------

void CMemoryStack::FreeAll( bool bDecommit )
{
	if ( m_pBase && m_pCommitLimit - m_pBase > 0 )
	{
		if ( bDecommit )
		{
#if defined(_WIN32)
			MemAlloc_RegisterExternalDeallocation( CMemoryStack, GetBase(), GetSize() );

			VirtualFree( m_pBase, m_pCommitLimit - m_pBase, MEM_DECOMMIT );
			m_pCommitLimit = m_pBase;
#endif
		}
		m_pNextAlloc = m_pBase;
	}
}

//-------------------------------------

void CMemoryStack::Access( void **ppRegion, unsigned *pBytes )
{
	*ppRegion = m_pBase;
	*pBytes = ( m_pNextAlloc - m_pBase);
}

//-------------------------------------

void CMemoryStack::PrintContents()
{
	Msg( "Total used memory:      %d\n", GetUsed() );
	Msg( "Total committed memory: %d\n", GetSize() );
}

//-----------------------------------------------------------------------------

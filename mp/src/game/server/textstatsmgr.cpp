//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "textstatsmgr.h"
#include "tier0/dbg.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CTextStatsMgr g_TextStatsMgr;	// The default text stats manager.


// ------------------------------------------------------------------------------------------ //
// CTextStatsMgr implementation.
// ------------------------------------------------------------------------------------------ //
CTextStatsMgr::CTextStatsMgr( void )
{
	m_szStatFilename[0] = 0;
}

bool CTextStatsMgr::WriteFile( IFileSystem *pFileSys, const char *pFilename )
{
	// If no filename was specified, use out preset one
	if ( !pFilename )
	{
		pFilename = m_szStatFilename;
	}

	FileHandle_t hFile = pFileSys->Open( pFilename, "wt", "LOGDIR" );
	if ( hFile == FILESYSTEM_INVALID_HANDLE )
		return false;
	
	CTextStat *pHead = CTextStat::GetTextStatsList();
	for ( CTextStat *pCur=pHead->m_pNext; pCur != pHead; pCur=pCur->m_pNext )
	{
		if ( pCur->m_pMgr == this )
			pCur->m_PrintFn( pFileSys, hFile, pCur->m_pUserData );
	}

	pFileSys->Close( hFile );

	// Call each CTextStatFile..
	for( CTextStatFile *pCurFile=CTextStatFile::s_pHead; pCurFile; pCurFile=pCurFile->m_pNext )
	{
		pCurFile->m_pFn();
	}

	return true;
}

char *CTextStatsMgr::GetStatsFilename( void )
{
	return m_szStatFilename;
}

void CTextStatsMgr::SetStatsFilename( char *sFilename )
{
	Assert( sFilename && sFilename[0] );

	Q_strncpy( m_szStatFilename, sFilename, sizeof(m_szStatFilename) );
}

// ------------------------------------------------------------------------------------------ //
// CTextStat implementation.
// ------------------------------------------------------------------------------------------ //

CTextStat::CTextStat()
{
	m_pPrev = m_pNext = this;
	m_pMgr = NULL;
}


CTextStat::CTextStat( TextStatPrintFn printFn, void *pUserData, CTextStatsMgr *pMgr )
{
	m_pPrev = m_pNext = this;
	Init( printFn, pUserData, pMgr );
}


CTextStat::~CTextStat()
{
	Term();		
}


void CTextStat::Init( TextStatPrintFn printFn, void *pUserData, CTextStatsMgr *pMgr )
{
	Term();

	m_pPrev = GetTextStatsList();
	m_pNext = GetTextStatsList()->m_pNext;
	m_pPrev->m_pNext = m_pNext->m_pPrev = this;
	
	m_PrintFn = printFn;
	m_pUserData = pUserData;
	m_pMgr = pMgr;
}


void CTextStat::Term()
{
	// Remove from the global list.
	m_pPrev->m_pNext = m_pNext;
	m_pNext->m_pPrev = m_pPrev;
	m_pPrev = m_pNext = this;
	m_pMgr = NULL;
}


CTextStat::CTextStat( bool bGlobalListHead )
{
	Assert( bGlobalListHead );
	m_pPrev = m_pNext = this;
}


CTextStat* CTextStat::GetTextStatsList()
{
	static CTextStat theList( true );
	return &theList;
}


void CTextStat::RemoveFn( void *pUserData )
{
	CTextStat *pReg = (CTextStat*)pUserData;
	pReg->Term();
}


// ------------------------------------------------------------------------------------------ //
// CTextStatInt implementation.
// ------------------------------------------------------------------------------------------ //
				
CTextStatInt::CTextStatInt( const char *pName, int initialValue, CTextStatsMgr *pMgr )
{
	m_pName = pName;
	m_Value = initialValue;
	m_Reg.Init( &CTextStatInt::PrintFn, this, pMgr );
}


void CTextStatInt::PrintFn( IFileSystem *pFileSys, FileHandle_t hFile, void *pUserData )
{
	CTextStatInt *pStat = (CTextStatInt*)pUserData;
	pFileSys->FPrintf( hFile, "%s %d\n", pStat->m_pName, pStat->m_Value );
}



// ------------------------------------------------------------------------------------------ //
// CTextStatFile functions.
// ------------------------------------------------------------------------------------------ //

CTextStatFile *CTextStatFile::s_pHead = NULL;


CTextStatFile::CTextStatFile( TextStatFileFn fn )
{
	m_pFn = fn;
	m_pNext = CTextStatFile::s_pHead;
	CTextStatFile::s_pHead = this;
}






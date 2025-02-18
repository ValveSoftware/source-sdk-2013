//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
// File extracted from MFC due to symbol conflicts

// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"

#ifdef AFX_CORE1_SEG
#pragma code_seg(AFX_CORE1_SEG)
#endif


/////////////////////////////////////////////////////////////////////////////
// Debug memory globals and implementation helpers

#ifdef _DEBUG       // most of this file is for debugging

void* __cdecl operator new(size_t nSize, int nType, LPCSTR lpszFileName, int nLine);
#if _MSC_VER >= 1210
void* __cdecl operator new[](size_t nSize, int nType, LPCSTR lpszFileName, int nLine);
#endif

/////////////////////////////////////////////////////////////////////////////
// test allocation routines

void* PASCAL CObject::operator new(size_t nSize)
{
#ifdef _AFX_NO_DEBUG_CRT
	return ::operator new(nSize);
#else
	return ::operator new(nSize, _AFX_CLIENT_BLOCK, NULL, 0);
#endif // _AFX_NO_DEBUG_CRT
}

void PASCAL CObject::operator delete(void* p)
{
#ifdef _AFX_NO_DEBUG_CRT
	free(p);
#else
	_free_dbg(p, _AFX_CLIENT_BLOCK);
#endif
}

#if _MSC_VER >= 1200
void PASCAL CObject::operator delete(void* p, void*)
{
#ifdef _AFX_NO_DEBUG_CRT
	free(p);
#else
	_free_dbg(p, _AFX_CLIENT_BLOCK);
#endif
}
#endif

#ifndef _AFX_NO_DEBUG_CRT

void* __cdecl operator new(size_t nSize, LPCSTR lpszFileName, int nLine)
{
	return ::operator new(nSize, _NORMAL_BLOCK, lpszFileName, nLine);
}

#if _MSC_VER >= 1210
void* __cdecl operator new[](size_t nSize, LPCSTR lpszFileName, int nLine)
{
	return ::operator new[](nSize, _NORMAL_BLOCK, lpszFileName, nLine);
}
#endif

#if _MSC_VER >= 1200
void __cdecl operator delete(void* pData, LPCSTR /* lpszFileName */,
	int /* nLine */)
{
	::operator delete(pData);
}
#endif

#if _MSC_VER >= 1210
void __cdecl operator delete[](void* pData, LPCSTR /* lpszFileName */,
	int /* nLine */)
{
	::operator delete(pData);
}
#endif

void* PASCAL
CObject::operator new(size_t nSize, LPCSTR lpszFileName, int nLine)
{
	return ::operator new(nSize, _AFX_CLIENT_BLOCK, lpszFileName, nLine);
}

#if _MSC_VER >= 1200
void PASCAL
CObject::operator delete(void *pObject, LPCSTR /* lpszFileName */,
	int /* nLine */)
{
#ifdef _AFX_NO_DEBUG_CRT
	free(pObject);
#else
	_free_dbg(pObject, _AFX_CLIENT_BLOCK);
#endif
}
#endif

void* AFXAPI AfxAllocMemoryDebug(size_t nSize, BOOL bIsObject,  LPCSTR lpszFileName, int nLine)
{
	return _malloc_dbg(nSize, bIsObject ? _AFX_CLIENT_BLOCK : _NORMAL_BLOCK,
		lpszFileName, nLine);
}

void AFXAPI AfxFreeMemoryDebug(void* pbData, BOOL bIsObject)
{
	_free_dbg(pbData, bIsObject ? _AFX_CLIENT_BLOCK : _NORMAL_BLOCK);
}

/////////////////////////////////////////////////////////////////////////////
// allocation failure hook, tracking turn on

BOOL AFXAPI _AfxDefaultAllocHook(size_t, BOOL, LONG)
	{ return TRUE; }

AFX_STATIC_DATA AFX_ALLOC_HOOK pfnAllocHook = _AfxDefaultAllocHook;

AFX_STATIC_DATA _CRT_ALLOC_HOOK pfnCrtAllocHook = NULL;
#if _MSC_VER >= 1200
int __cdecl _AfxAllocHookProxy(int nAllocType, void * pvData, size_t nSize,
	int nBlockUse, long lRequest, const unsigned char * szFilename, int nLine)
#else
int __cdecl _AfxAllocHookProxy(int nAllocType, void * pvData, size_t nSize,
	int nBlockUse, long lRequest, const char * szFilename, int nLine)
#endif
{
#if _MSC_VER >= 1200
	if (nAllocType != _HOOK_ALLOC)
		return (pfnCrtAllocHook)(nAllocType, pvData, nSize,
			nBlockUse, lRequest, (const unsigned char*) szFilename, nLine);
	if ((pfnAllocHook)(nSize, _BLOCK_TYPE(nBlockUse) == _AFX_CLIENT_BLOCK, lRequest))
		return (pfnCrtAllocHook)(nAllocType, pvData, nSize,
			nBlockUse, lRequest, (const unsigned char*) szFilename, nLine);
#else
	if (nAllocType != _HOOK_ALLOC)
		return (pfnCrtAllocHook)(nAllocType, pvData, nSize,
			nBlockUse, lRequest, szFilename, nLine);
	if ((pfnAllocHook)(nSize, _BLOCK_TYPE(nBlockUse) == _AFX_CLIENT_BLOCK, lRequest))
		return (pfnCrtAllocHook)(nAllocType, pvData, nSize,
			nBlockUse, lRequest, szFilename, nLine);
#endif
	return FALSE;
}

AFX_ALLOC_HOOK AFXAPI AfxSetAllocHook(AFX_ALLOC_HOOK pfnNewHook)
{
	if (pfnCrtAllocHook == NULL)
		pfnCrtAllocHook = _CrtSetAllocHook(_AfxAllocHookProxy);

	AFX_ALLOC_HOOK pfnOldHook = pfnAllocHook;
	pfnAllocHook = pfnNewHook;
	return pfnOldHook;
}

// This can be set to TRUE to override all AfxEnableMemoryTracking calls,
// allowing all allocations, even MFC internal allocations to be tracked.
BOOL _afxMemoryLeakOverride = FALSE;

BOOL AFXAPI AfxEnableMemoryTracking(BOOL bTrack)
{
	if (_afxMemoryLeakOverride)
		return TRUE;

	int nOldState = _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG);
	if (bTrack)
		_CrtSetDbgFlag(nOldState | _CRTDBG_ALLOC_MEM_DF);
	else
		_CrtSetDbgFlag(nOldState & ~_CRTDBG_ALLOC_MEM_DF);
	return nOldState & _CRTDBG_ALLOC_MEM_DF;
}

/////////////////////////////////////////////////////////////////////////////
// stop on a specific memory request

// Obsolete API
void AFXAPI AfxSetAllocStop(LONG lRequestNumber)
{
	_CrtSetBreakAlloc(lRequestNumber);
}

BOOL AFXAPI AfxCheckMemory()
  // check all of memory (look for memory tromps)
{
	return _CrtCheckMemory();
}

// -- true if block of exact size, allocated on the heap
// -- set *plRequestNumber to request number (or 0)
BOOL AFXAPI AfxIsMemoryBlock(const void* pData, UINT nBytes,
		LONG* plRequestNumber)
{
	return _CrtIsMemoryBlock(pData, nBytes, plRequestNumber, NULL, NULL);
}

/////////////////////////////////////////////////////////////////////////////
// CMemoryState

CMemoryState::CMemoryState()
{
	memset(this, 0, sizeof(*this));
}

void CMemoryState::UpdateData()
{
	for(int i = 0; i < nBlockUseMax; i++)
	{
		m_lCounts[i] = m_memState.lCounts[i];
		m_lSizes[i] = m_memState.lSizes[i];
	}
	m_lHighWaterCount = m_memState.lHighWaterCount;
	m_lTotalCount = m_memState.lTotalCount;
}

// fills 'this' with the difference, returns TRUE if significant
BOOL CMemoryState::Difference(const CMemoryState& oldState,
		const CMemoryState& newState)
{
	int nResult = _CrtMemDifference(&m_memState, &oldState.m_memState, &newState.m_memState);
	UpdateData();
	return nResult != 0;
}

void CMemoryState::DumpStatistics() const
{
	_CrtMemDumpStatistics(&m_memState);
}

// -- fill with current memory state
void CMemoryState::Checkpoint()
{
	_CrtMemCheckpoint(&m_memState);
	UpdateData();
}

// Dump objects created after this memory state was checkpointed
// Will dump all objects if this memory state wasn't checkpointed
// Dump all objects, report about non-objects also
// List request number in {}
void CMemoryState::DumpAllObjectsSince() const
{
	_CrtMemDumpAllObjectsSince(&m_memState);
}

/////////////////////////////////////////////////////////////////////////////
// Enumerate all objects allocated in the diagnostic memory heap

struct _AFX_ENUM_CONTEXT
{
	void (*m_pfn)(CObject*,void*);
	void* m_pContext;
};

AFX_STATIC void _AfxDoForAllObjectsProxy(void* pObject, void* pContext)
{
	_AFX_ENUM_CONTEXT* p = (_AFX_ENUM_CONTEXT*)pContext;
	(*p->m_pfn)((CObject*)pObject, p->m_pContext);
}

void AFXAPI
AfxDoForAllObjects(void (AFX_CDECL *pfn)(CObject*, void*), void* pContext)
{
	if (pfn == NULL)
	{
		AfxThrowInvalidArgException();
	}
	_AFX_ENUM_CONTEXT context;
	context.m_pfn = pfn;
	context.m_pContext = pContext;
	_CrtDoForAllClientObjects(_AfxDoForAllObjectsProxy, &context);
}

/////////////////////////////////////////////////////////////////////////////
// Automatic debug memory diagnostics

BOOL AFXAPI AfxDumpMemoryLeaks()
{
	return _CrtDumpMemoryLeaks();
}

#endif // _AFX_NO_DEBUG_CRT
#endif // _DEBUG

/////////////////////////////////////////////////////////////////////////////
// Non-diagnostic memory routines

int AFX_CDECL AfxNewHandler(size_t /* nSize */)
{
	AfxThrowMemoryException();
}

#pragma warning(disable: 4273)

#ifndef _AFXDLL
_PNH _afxNewHandler = &AfxNewHandler;
#endif

_PNH AFXAPI AfxGetNewHandler(void)
{
#ifdef _AFXDLL
	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
	return pState->m_pfnNewHandler;
#else
	return _afxNewHandler;
#endif
}

_PNH AFXAPI AfxSetNewHandler(_PNH pfnNewHandler)
{
#ifdef _AFXDLL
	AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
	_PNH pfnOldHandler = pState->m_pfnNewHandler;
	pState->m_pfnNewHandler = pfnNewHandler;
	return pfnOldHandler;
#else
	_PNH pfnOldHandler = _afxNewHandler;
	_afxNewHandler = pfnNewHandler;
	return pfnOldHandler;
#endif
}

AFX_STATIC_DATA const _PNH _pfnUninitialized = (_PNH)-1;

void* __cdecl operator new(size_t nSize)
{
	void* pResult;
#ifdef _AFXDLL
	_PNH pfnNewHandler = _pfnUninitialized;
#endif
	for (;;)
	{
#if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
		pResult = _malloc_dbg(nSize, _NORMAL_BLOCK, NULL, 0);
#else
		pResult = malloc(nSize);
#endif
		if (pResult != NULL)
			return pResult;

#ifdef _AFXDLL
		if (pfnNewHandler == _pfnUninitialized)
		{
			AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
			pfnNewHandler = pState->m_pfnNewHandler;
		}
		if (pfnNewHandler == NULL || (*pfnNewHandler)(nSize) == 0)
			break;
#else
		if (_afxNewHandler == NULL || (*_afxNewHandler)(nSize) == 0)
			break;
#endif
	}
	return pResult;
}

void __cdecl operator delete(void* p)
{
#if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
		_free_dbg(p, _NORMAL_BLOCK);
#else
		free(p);
#endif
}

#if _MSC_VER >= 1210
void* __cdecl operator new[](size_t nSize)
{
	return ::operator new(nSize);
}

void __cdecl operator delete[](void* p)
{
	::operator delete(p);
}
#endif

#ifdef _DEBUG

void* __cdecl operator new(size_t nSize, int nType, LPCSTR lpszFileName, int nLine)
{
#ifdef _AFX_NO_DEBUG_CRT
	UNUSED_ALWAYS(nType);
	UNUSED_ALWAYS(lpszFileName);
	UNUSED_ALWAYS(nLine);
	return ::operator new(nSize);
#else
	void* pResult;
#ifdef _AFXDLL
	_PNH pfnNewHandler = _pfnUninitialized;
#endif
	for (;;)
	{
		pResult = _malloc_dbg(nSize, nType, lpszFileName, nLine);
		if (pResult != NULL)
			return pResult;

#ifdef _AFXDLL
		if (pfnNewHandler == _pfnUninitialized)
		{
			AFX_MODULE_THREAD_STATE* pState = AfxGetModuleThreadState();
			pfnNewHandler = pState->m_pfnNewHandler;
		}
		if (pfnNewHandler == NULL || (*pfnNewHandler)(nSize) == 0)
			break;
#else
		if (_afxNewHandler == NULL || (*_afxNewHandler)(nSize) == 0)
			break;
#endif
	}
	return pResult;
#endif
}

#if _MSC_VER >= 1700
void __cdecl operator delete(void* p, int nType, LPCSTR /* lpszFileName */, int /* nLine */)
{
#if !defined(_AFX_NO_DEBUG_CRT) && defined(_DEBUG)
		_free_dbg(p, nType);
#else
		free(p);
#endif
}
#endif // _MSC_VER >= 1200

#if _MSC_VER >= 1700
void* __cdecl operator new[](size_t nSize, int nType, LPCSTR lpszFileName, int nLine)
{
	return ::operator new(nSize, nType, lpszFileName, nLine);
}
void __cdecl operator delete[](void* p, int nType, LPCSTR lpszFileName, int nLine)
{
	::operator delete(p, nType, lpszFileName, nLine);
}
#endif // _MSC_VER >= 1210

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////

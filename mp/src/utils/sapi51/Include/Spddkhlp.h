/*******************************************************************************
* SPDDKHLP.h *
*------------*
*   Description:
*       This is the header file for core helper functions implementation.
*
*   Copyright (c) Microsoft Corporation. All rights reserved.
*
*******************************************************************************/
#ifndef SPDDKHLP_h
#define SPDDKHLP_h

#ifndef SPHelper_h
#include <sphelper.h>
#endif

#include <sapiddk.h>

#ifndef SPError_h
#include <SPError.h>
#endif

#ifndef SPDebug_h
#include <SPDebug.h>
#endif

#ifndef _INC_LIMITS
#include <limits.h>
#endif

#ifndef _INC_CRTDBG
#include <crtdbg.h>
#endif

#ifndef _INC_MALLOC
#include <malloc.h>
#endif

#ifndef _INC_MMSYSTEM
#include <mmsystem.h>
#endif

#ifndef __comcat_h__
#include <comcat.h>
#endif

//=== Constants ==============================================================
#define sp_countof(x) ((sizeof(x) / sizeof(*(x))))

#define SP_IS_BAD_WRITE_PTR(p)     ( SPIsBadWritePtr( p, sizeof(*(p)) ))
#define SP_IS_BAD_READ_PTR(p)      ( SPIsBadReadPtr(  p, sizeof(*(p)) ))
#define SP_IS_BAD_CODE_PTR(p)      ( ::IsBadCodePtr((FARPROC)(p) )
#define SP_IS_BAD_INTERFACE_PTR(p) ( SPIsBadInterfacePtr( (p) )  )
#define SP_IS_BAD_VARIANT_PTR(p)   ( SPIsBadVARIANTPtr( (p) ) )
#define SP_IS_BAD_STRING_PTR(p)    ( SPIsBadStringPtr( (p) ) )

#define SP_IS_BAD_OPTIONAL_WRITE_PTR(p)     ((p) && SPIsBadWritePtr( p, sizeof(*(p)) ))
#define SP_IS_BAD_OPTIONAL_READ_PTR(p)      ((p) && SPIsBadReadPtr(  p, sizeof(*(p)) ))
#define SP_IS_BAD_OPTIONAL_INTERFACE_PTR(p) ((p) && SPIsBadInterfacePtr(p))
#define SP_IS_BAD_OPTIONAL_STRING_PTR(p)    ((p) && SPIsBadStringPtr(p))

//=== Class, Enum, Struct, Template, and Union Declarations ==================

//=== Inlines ================================================================

/*** Pointer validation functions
*/

// TODO:  Add decent debug output for bad parameters

inline BOOL SPIsBadStringPtr( const WCHAR * psz, ULONG cMaxChars = 0xFFFF )
{
    BOOL IsBad = false;
    __try
    {
        do
        {
            if( *psz++ == 0 ) return IsBad;
        }
        while( --cMaxChars );
    }
    __except( GetExceptionCode() == EXCEPTION_ACCESS_VIOLATION )
    {
        IsBad = true;
    }

    return IsBad;
}

inline BOOL SPIsBadReadPtr( const void* pMem, UINT Size )
{
#ifdef _DEBUG
    BOOL bIsBad = ::IsBadReadPtr( pMem, Size );
    SPDBG_ASSERT(!bIsBad);
    return bIsBad;
#else
    return ::IsBadReadPtr( pMem, Size );
#endif
}

inline BOOL SPIsBadWritePtr( void* pMem, UINT Size )
{
#ifdef _DEBUG
    BOOL bIsBad = ::IsBadWritePtr( pMem, Size );
    SPDBG_ASSERT(!bIsBad);
    return bIsBad;
#else
    return ::IsBadWritePtr( pMem, Size );
#endif
}

inline BOOL SPIsBadInterfacePtr( const IUnknown* pUnknown )
{
#ifdef _DEBUG
    BOOL bIsBad = ( ::IsBadReadPtr( pUnknown, sizeof( *pUnknown ) ) ||
                    ::IsBadCodePtr( (FARPROC)((void**)pUnknown)[0] ))?
                   (true):(false);
    SPDBG_ASSERT(!bIsBad);
    return bIsBad;
#else
    return ( ::IsBadReadPtr( pUnknown, sizeof( *pUnknown ) ) ||
             ::IsBadCodePtr( (FARPROC)((void**)pUnknown)[0] ))?
            (true):(false);
#endif
}

inline BOOL SPIsBadVARIANTPtr( const VARIANT* pVar )
{
#ifdef _DEBUG
    BOOL bIsBad = ::IsBadReadPtr( pVar, sizeof( *pVar ) );
    SPDBG_ASSERT(!bIsBad);
    return bIsBad;
#else
    return ::IsBadReadPtr( pVar, sizeof( *pVar ) );
#endif
}

#ifdef __ATLCOM_H__     //--- Only enable these if ATL is being used

//
//  Helper functions can be used to implement GetObjectToken/SetObjectToken for objects that 
//  support ISpObjectWithToken
//
inline HRESULT SpGenericSetObjectToken(ISpObjectToken * pCallersToken, CComPtr<ISpObjectToken> & cpObjToken)
{
    HRESULT hr = S_OK;
    if (SP_IS_BAD_INTERFACE_PTR(pCallersToken))
    {
        hr = E_INVALIDARG;
    }
    else
    {
        if (cpObjToken)
        {
            hr = SPERR_ALREADY_INITIALIZED;
        }
        else
        {
            cpObjToken = pCallersToken;
        }
    }
    return hr;
}


inline HRESULT SpGenericGetObjectToken(ISpObjectToken ** ppCallersToken, CComPtr<ISpObjectToken> & cpObjToken)
{
    HRESULT hr = S_OK;
    if (SP_IS_BAD_WRITE_PTR(ppCallersToken))
    {
        hr = E_POINTER;
    }
    else
    {
        *ppCallersToken = cpObjToken;
        if (*ppCallersToken)
        {
            (*ppCallersToken)->AddRef();
        }
        else
        {
            hr = S_FALSE;
        }
    }
    return hr;
}

#endif  // __ATLCOM_H__


//
//  Helper class for SPSTATEINFO sturcture automatically initializes and cleans up
//  the structure + provides a few helper functions.
//
class CSpStateInfo : public SPSTATEINFO
{
public:
    CSpStateInfo()
    {
        cAllocatedEntries = NULL;
        pTransitions = NULL;
    }
    ~CSpStateInfo()
    {
        ::CoTaskMemFree(pTransitions);
    }
    SPTRANSITIONENTRY * FirstEpsilon()
    {
        return pTransitions;
    }
    SPTRANSITIONENTRY * FirstRule()
    {
        return pTransitions + cEpsilons;
    }
    SPTRANSITIONENTRY * FirstWord()
    {
        return pTransitions + cEpsilons + cRules;
    }
    SPTRANSITIONENTRY * FirstSpecialTransition()
    {
        return pTransitions + cEpsilons + cRules + cWords;
    }
};


//
//  This basic queue implementation can be used to maintin linked lists of classes.  The class T
//  must contain the member m_pNext which is used by this template to point to the next element.
//  If the bPurgeWhenDeleted is TRUE then all of the elements in the queue will be deleted
//  when the queue is deleted, otherwise they will not.
//  If bMaintainCount is TRUE then a running count will be maintained, and GetCount() will be
//  efficent.  If it is FALSE then a running count will not be maintained, and GetCount() will
//  be an order N operation.  If you do not require a count, then 
//

template <class T, BOOL bPurgeWhenDeleted> class CSpBasicList;

template <class T, BOOL bPurgeWhenDeleted = TRUE, BOOL bMaintainCount = FALSE>
class CSpBasicQueue
{
public:
    T     * m_pHead;
    T     * m_pTail;
    ULONG   m_cElements;    // Warning!  Use GetCount() -- Not maintained if bMaintainCount is FALSE.

    CSpBasicQueue() 
    {
        m_pHead = NULL;
        if (bMaintainCount)
        {
            m_cElements = 0;
        }
    }

    ~CSpBasicQueue()
    {
        if (bPurgeWhenDeleted)
        {
            Purge();
        }
    }

    HRESULT CreateNode(T ** ppNode)
    {
        *ppNode = new T;
        if (*ppNode)
        {
            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    T * GetNext(const T * pNode)
    {
        return pNode->m_pNext;
    }


    T * Item(ULONG i)
    {
        T * pNode = m_pHead;
        while (pNode && i)
        {
            i--;
            pNode = pNode->m_pNext;
        }
        return pNode;
    }

    void InsertAfter(T * pPrev, T * pNewNode)
    {
        if (pPrev)
        {
            pNewNode->m_pNext = pPrev->m_pNext;
            pPrev->m_pNext = pNewNode;
            if (pNewNode->m_pNext == NULL)
            {
                m_pTail = pNewNode;
            }
            if (bMaintainCount) ++m_cElements;
        }
        else
        {
            InsertHead(pNewNode);
        }
    }

    void InsertTail(T * pNode)
    {
        pNode->m_pNext = NULL;
        if (m_pHead)
        {
            m_pTail->m_pNext = pNode;
        }
        else
        {
            m_pHead = pNode;
        }
        m_pTail = pNode;
        if (bMaintainCount) ++m_cElements;
    }

    void InsertHead(T * pNode)
    {
        pNode->m_pNext = m_pHead;
        if (m_pHead == NULL)
        {
            m_pTail = pNode;
        }
        m_pHead = pNode;
        if (bMaintainCount) ++m_cElements;
    }

    T * RemoveHead()
    {
        T * pNode = m_pHead;
        if (pNode)
        {
            m_pHead = pNode->m_pNext;
            if (bMaintainCount) --m_cElements;
        }
        return pNode;
    }

    T * RemoveTail()
    {
        T * pNode = m_pHead;
        if (pNode)
        {
            if (pNode == m_pTail)
            {
                m_pHead = NULL;
            }
            else
            {
                T * pPrev;
                do
                {
                    pPrev = pNode;
                    pNode = pNode->m_pNext;
                } while ( pNode != m_pTail );
                pPrev->m_pNext = NULL;
                m_pTail = pPrev;
            }
            if (bMaintainCount) --m_cElements;
        }
        return pNode;
    }

    void Purge()
    {
        while (m_pHead)
        {
            T * pDie = m_pHead;
            m_pHead = pDie->m_pNext;
            delete pDie;
        }
        if (bMaintainCount) m_cElements = 0;
    }

    void ExplicitPurge()
    {
        T * pDie;
        BYTE * pb;

        while (m_pHead)
        {
            pDie = m_pHead;
            m_pHead = pDie->m_pNext;

            pDie->~T();

            pb = reinterpret_cast<BYTE *>(pDie);
            delete [] pb;
        }
        if (bMaintainCount) m_cElements = 0;
    }


    T * GetTail() const
    {
        if (m_pHead)
        {
            return m_pTail;
        }
        return NULL;
    }

    T * GetHead() const
    {
        return m_pHead;
    }

    BOOL IsEmpty() const
    {
        return m_pHead == NULL; 
    }

    BOOL Remove(T * pNode)
    {
        if (m_pHead == pNode)
        {
            m_pHead = pNode->m_pNext;
            if (bMaintainCount) --m_cElements;
            return TRUE;
        }
        else
        {
            T * pCur = m_pHead;
            while (pCur)
            {
                T * pNext = pCur->m_pNext;
                if (pNext == pNode)
                {
                    if ((pCur->m_pNext = pNode->m_pNext) == NULL)
                    {
                        m_pTail = pCur;
                    }
                    if (bMaintainCount) --m_cElements;
                    return TRUE;
                }
                pCur = pNext;
            }
        }
        return FALSE;
    }

    void MoveAllToHeadOf(CSpBasicQueue & DestQueue)
    {
        if (m_pHead)
        {
            m_pTail->m_pNext = DestQueue.m_pHead;
            if (DestQueue.m_pHead == NULL)
            {
                DestQueue.m_pTail = m_pTail;
            }
            DestQueue.m_pHead = m_pHead;
            m_pHead = NULL;
            if (bMaintainCount)
            {
                DestQueue.m_cElements += m_cElements;
                m_cElements = 0;
            }
        }
    }

    void MoveAllToList(CSpBasicList<T, bPurgeWhenDeleted> & List)
    {
        if (m_pHead)
        {
            m_pTail->m_pNext = List.m_pFirst;
            List.m_pFirst = m_pHead;
            m_pHead = NULL;
        }
        if (bMaintainCount)
        {
            m_cElements = 0;
        }
    }

    BOOL MoveToList(T * pNode, CSpBasicList<T, bPurgeWhenDeleted> & List)
    {
        BOOL bFound = Remove(pNode);
        if (bFound)
        {
            List.AddNode(pNode);
        }
        return bFound;
    }

    ULONG GetCount() const
    {
        if (bMaintainCount)
        {
            return m_cElements;
        }
        else
        {
            ULONG c = 0;
            for (T * pNode = m_pHead;
                 pNode;
                 pNode = pNode->m_pNext, c++) {}
            return c;
        }
    }

    //
    //  The following functions require the class T to implement a static function:
    //
    //      LONG Compare(const T * pElem1, const T * pElem2)
    //
    //  which returns < 0 if pElem1 is less than pElem2, 0 if they are equal, and > 0 if
    //  pElem1 is greater than pElem2.
    //
    void InsertSorted(T * pNode)
    {
        if (m_pHead)
        {
            if (T::Compare(pNode, m_pTail) >= 0)
            {
                pNode->m_pNext = NULL;
                m_pTail->m_pNext = pNode;
                m_pTail = pNode;
            }
            else
            {
                //
                //  We don't have to worry about walking off of the end of the list here since
                //  we have already checked the tail.
                //
                T ** ppNext = &m_pHead;
                while (T::Compare(pNode, *ppNext) >= 0)
                {
                    ppNext = &((*ppNext)->m_pNext);
                }
                pNode->m_pNext = *ppNext;
                *ppNext = pNode;
            }
        }
        else
        {
            pNode->m_pNext = NULL;
            m_pHead = m_pTail = pNode;
        }
        if (bMaintainCount) ++m_cElements;
    }

    HRESULT InsertSortedUnique(T * pNode)
    {
        HRESULT hr = S_OK;
        if (m_pHead)
        {
            if (T::Compare(pNode, m_pTail) > 0)
            {
                pNode->m_pNext = NULL;
                m_pTail->m_pNext = pNode;
                m_pTail = pNode;
            }
            else
            {
                //
                //  We don't have to worry about walking off of the end of the list here since
                //  we have already checked the tail.
                //
                T ** ppNext = &m_pHead;
                while (T::Compare(pNode, *ppNext) > 0)
                {
                    ppNext = &((*ppNext)->m_pNext);
                }
                if (T::Compare(pNode, *ppNext) != 0)
                {
                    pNode->m_pNext = *ppNext;
                    *ppNext = pNode;
                }
                else
                {
                    delete pNode;
                    hr = S_FALSE;
                }
            }
        }
        else
        {
            pNode->m_pNext = NULL;
            m_pHead = m_pTail = pNode;
        }
        if (bMaintainCount) ++m_cElements;
        return hr;
    }

    //
    //  These functions must support the "==" operator for the TFIND type.
    //
    template <class TFIND> 
    T * Find(TFIND & FindVal) const 
    {
        for (T * pNode = m_pHead; pNode && (!(*pNode == FindVal)); pNode = pNode->m_pNext)
        {}
        return pNode;
    }

    template <class TFIND> 
    T * FindNext(const T * pCurNode, TFIND & FindVal) const 
    {
        for (T * pNode = pCurNode->m_pNext; pNode && (!(*pNode == FindVal)); pNode = pNode->m_pNext)
        {}
        return pNode;
    }

    //
    //  Searches for and removes a single list element
    //  
    template <class TFIND> 
    T * FindAndRemove(TFIND & FindVal)
    {
        T * pNode = m_pHead;
        if (pNode)
        {
            if (*pNode == FindVal)
            {
                m_pHead = pNode->m_pNext;
                if (bMaintainCount) --m_cElements;
            }
            else
            {
                T * pPrev = pNode;
                for (pNode = pNode->m_pNext;
                     pNode;
                     pPrev = pNode, pNode = pNode->m_pNext)
                {
                    if (*pNode == FindVal)
                    {
                        pPrev->m_pNext = pNode->m_pNext;
                        if (pNode->m_pNext == NULL)
                        {
                            m_pTail = pPrev;
                        }
                        if (bMaintainCount) --m_cElements;
                        break;
                    }
                }
            }
        }
        return pNode;
    }

    //
    //  Searches for and deletes all list elements that match
    //  
    template <class TFIND> 
    void FindAndDeleteAll(TFIND & FindVal)
    {
        T * pNode = m_pHead;
        while (pNode && *pNode == FindVal)
        {
            m_pHead = pNode->m_pNext;
            delete pNode;
            if (bMaintainCount) --m_cElements;
            pNode = m_pHead;
        }
        T * pPrev = pNode;
        while (pNode)
        {
            T * pNext = pNode->m_pNext;
            if (*pNode == FindVal)
            {
                pPrev->m_pNext = pNext;
                delete pNode;
                if (bMaintainCount) --m_cElements;
            }
            else
            {
                pPrev = pNode;
            }
            pNode = pNext;
        }
        m_pTail = pPrev;    // Just always set it in case we removed the tail.
    }


};

template <class T, BOOL bPurgeWhenDeleted = TRUE>
class CSpBasicList
{
public:
    T * m_pFirst;
    CSpBasicList() : m_pFirst(NULL) {}
    ~CSpBasicList()
    {
        if (bPurgeWhenDeleted)
        {
            Purge();
        }
    }

    void Purge()
    {
        while (m_pFirst)
        {
            T * pNext = m_pFirst->m_pNext;
            delete m_pFirst;
            m_pFirst = pNext;
        }
    }

    void ExplicitPurge()
    {
        T * pDie;
        BYTE * pb;

        while (m_pFirst)
        {
            pDie = m_pFirst;
            m_pFirst = pDie->m_pNext;

            pDie->~T();

            pb = reinterpret_cast<BYTE *>(pDie);
            delete [] pb;
        }
    }

    HRESULT RemoveFirstOrAllocateNew(T ** ppNode)
    {
        if (m_pFirst)
        {
            *ppNode = m_pFirst;
            m_pFirst = m_pFirst->m_pNext;
        }
        else
        {
            *ppNode = new T;
            if (*ppNode == NULL)
            {
                return E_OUTOFMEMORY;
            }
        }
        return S_OK;
    }

    void AddNode(T * pNode)
    {
        pNode->m_pNext = m_pFirst;
        m_pFirst = pNode;
    }
    T * GetFirst()
    {
        return m_pFirst;
    }
    T * RemoveFirst()
    {
        T * pNode = m_pFirst;
        if (pNode)
        {
            m_pFirst = pNode->m_pNext;
        }
        return pNode;
    }
};

#define STACK_ALLOC(TYPE, COUNT) (TYPE *)_alloca(sizeof(TYPE) * (COUNT))
#define STACK_ALLOC_AND_ZERO(TYPE, COUNT) (TYPE *)memset(_alloca(sizeof(TYPE) * (COUNT)), 0, (sizeof(TYPE) * (COUNT)))
#define STACK_ALLOC_AND_COPY(TYPE, COUNT, SOURCE) (TYPE *)memcpy(_alloca(sizeof(TYPE) * (COUNT)), (SOURCE), (sizeof(TYPE) * (COUNT)))

inline HRESULT SpGetSubTokenFromToken(
    ISpObjectToken * pToken,
    const WCHAR * pszSubKeyName,
    ISpObjectToken ** ppToken,
    BOOL fCreateIfNotExist = FALSE)
{
    SPDBG_FUNC("SpGetTokenFromDataKey");
    HRESULT hr = S_OK;

    if (SP_IS_BAD_INTERFACE_PTR(pToken) ||
        SP_IS_BAD_STRING_PTR(pszSubKeyName) ||
        SP_IS_BAD_WRITE_PTR(ppToken))
    {
        hr = E_POINTER;
    }

    // First, either create or open the datakey for the new token
    CComPtr<ISpDataKey> cpDataKeyForNewToken;
    if (SUCCEEDED(hr))
    {
        if (fCreateIfNotExist)
        {
            hr = pToken->CreateKey(pszSubKeyName, &cpDataKeyForNewToken);
        }
        else
        {
            hr = pToken->OpenKey(pszSubKeyName, &cpDataKeyForNewToken);
        }
    }

    // The sub token's category will be the token id of it's parent token
    CSpDynamicString dstrCategoryId;
    if (SUCCEEDED(hr))
    {
        hr = pToken->GetId(&dstrCategoryId);
    }

    // The sub token's token id will be it's category id + "\\" the key name
    CSpDynamicString dstrTokenId;
    if (SUCCEEDED(hr))
    {
        dstrTokenId = dstrCategoryId;
        dstrTokenId.Append2(L"\\", pszSubKeyName);
    }

    // Now create the token and initalize it
    CComPtr<ISpObjectTokenInit> cpTokenInit;
    if (SUCCEEDED(hr))
    {
        hr = cpTokenInit.CoCreateInstance(CLSID_SpObjectToken);
    }

    if (SUCCEEDED(hr))
    {
        hr = cpTokenInit->InitFromDataKey(dstrCategoryId, dstrTokenId, cpDataKeyForNewToken);
    }

    if (SUCCEEDED(hr))
    {
        *ppToken = cpTokenInit.Detach();
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

template<class T>
HRESULT SpCreateObjectFromSubToken(ISpObjectToken * pToken, const WCHAR * pszSubKeyName, T ** ppObject,
                       IUnknown * pUnkOuter = NULL, DWORD dwClsCtxt = CLSCTX_ALL)
{
    SPDBG_FUNC("SpCreateObjectFromSubToken");
    HRESULT hr;

    CComPtr<ISpObjectToken> cpSubToken;
    hr = SpGetSubTokenFromToken(pToken, pszSubKeyName, &cpSubToken);
    
    if (SUCCEEDED(hr))
    {
        hr = SpCreateObjectFromToken(cpSubToken, ppObject, pUnkOuter, dwClsCtxt);
    }

    SPDBG_REPORT_ON_FAIL(hr);
    return hr;
}

#endif /* This must be the last line in the file */

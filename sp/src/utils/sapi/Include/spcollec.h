/*****************************************************************************
* SPCollec.h *
*------------*
*       This header file contains the SAPI5 collection class templates. These
*   are a modified version of the MFC template classes without the dependencies.
*-----------------------------------------------------------------------------
*   Copyright (c) Microsoft Corporation. All rights reserved.
*****************************************************************************/
#ifndef SPCollec_h
#define SPCollec_h

#ifndef _INC_LIMITS
#include <limits.h>
#endif

#ifndef _INC_STRING
#include <string.h>
#endif

#ifndef _INC_STDLIB
#include <stdlib.h>
#endif

#ifndef _WIN32_WCE
#ifndef _INC_SEARCH
#include <search.h>
#endif
#endif

/////////////////////////////////////////////////////////////////////////////
#define SPASSERT_VALID( a )             // This doesn't do anything right now

typedef void* SPLISTPOS;
typedef DWORD SPLISTHANDLE;

#define SP_BEFORE_START_POSITION ((void*)-1L)

inline BOOL SPIsValidAddress(const void* lp, UINT nBytes, BOOL bReadWrite)
{
    // simple version using Win-32 APIs for pointer validation.
    return (lp != NULL && !IsBadReadPtr(lp, nBytes) &&
        (!bReadWrite || !IsBadWritePtr((LPVOID)lp, nBytes)));
}

/////////////////////////////////////////////////////////////////////////////
// global helpers (can be overridden)
template<class TYPE>
inline HRESULT SPConstructElements(TYPE* pElements, int nCount)
{
    HRESULT hr = S_OK;
    SPDBG_ASSERT( nCount == 0 ||
             SPIsValidAddress( pElements, nCount * sizeof(TYPE), TRUE ) );

    // default is bit-wise zero initialization
    memset((void*)pElements, 0, nCount * sizeof(TYPE));
    return hr;
}

template<class TYPE>
inline void SPDestructElements(TYPE* pElements, int nCount)
{
    SPDBG_ASSERT( ( nCount == 0 ||
               SPIsValidAddress( pElements, nCount * sizeof(TYPE), TRUE  ) ) );
    pElements;  // not used
    nCount; // not used

    // default does nothing
}

template<class TYPE>
inline HRESULT SPCopyElements(TYPE* pDest, const TYPE* pSrc, int nCount)
{
    HRESULT hr = S_OK;
    SPDBG_ASSERT( ( nCount == 0 ||
               SPIsValidAddress( pDest, nCount * sizeof(TYPE), TRUE  )) );
    SPDBG_ASSERT( ( nCount == 0 ||
               SPIsValidAddress( pSrc, nCount * sizeof(TYPE), FALSE  )) );

    // default is bit-wise copy
    memcpy(pDest, pSrc, nCount * sizeof(TYPE));
    return hr;
}

template<class TYPE, class ARG_TYPE>
BOOL SPCompareElements(const TYPE* pElement1, const ARG_TYPE* pElement2)
{
    SPDBG_ASSERT( SPIsValidAddress( pElement1, sizeof(TYPE), FALSE ) );
    SPDBG_ASSERT( SPIsValidAddress( pElement2, sizeof(ARG_TYPE), FALSE ) );
    return *pElement1 == *pElement2;
}

template<class ARG_KEY>
inline UINT SPHashKey(ARG_KEY key)
{
    // default identity hash - works for most primitive values
    return ((UINT)(void*)(DWORD)key) >> 4;
}

/////////////////////////////////////////////////////////////////////////////
// CSPPlex

struct CSPPlex    // warning variable length structure
{
    CSPPlex* pNext;
    UINT nMax;
    UINT nCur;
    /* BYTE data[maxNum*elementSize]; */
    void* data() { return this+1; }

    static CSPPlex* PASCAL Create( CSPPlex*& pHead, UINT nMax, UINT cbElement )
    {
        CSPPlex* p = (CSPPlex*) new BYTE[sizeof(CSPPlex) + nMax * cbElement];
        SPDBG_ASSERT(p);
        p->nMax = nMax;
        p->nCur = 0;
        p->pNext = pHead;
        pHead = p;  // change head (adds in reverse order for simplicity)
        return p;
    }

    void FreeDataChain()
    {
        CSPPlex* p = this;
        while (p != NULL)
        {
            BYTE* bytes = (BYTE*) p;
            CSPPlex* pNext = p->pNext;
            delete[] bytes;
            p = pNext;
        }
    }
};


/////////////////////////////////////////////////////////////////////////////
// CSPArray<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CSPArray
{
public:
// Construction
    CSPArray();

// Attributes
    int GetSize() const;
    int GetUpperBound() const;
    HRESULT SetSize(int nNewSize, int nGrowBy = -1);

// Operations
    // Clean up
    void FreeExtra();
    void RemoveAll();

    // Accessing elements
    TYPE GetAt(int nIndex) const;
    void SetAt(int nIndex, ARG_TYPE newElement);
    TYPE& ElementAt(int nIndex);

    // Direct Access to the element data (may return NULL)
    const TYPE* GetData() const;
    TYPE* GetData();

    // Potentially growing the array
    HRESULT SetAtGrow(int nIndex, ARG_TYPE newElement);
    int Add(ARG_TYPE newElement);
    int Append(const CSPArray& src);
    HRESULT Copy(const CSPArray& src);

    // overloaded operator helpers
    TYPE operator[](int nIndex) const;
    TYPE& operator[](int nIndex);

    // Operations that move elements around
    HRESULT InsertAt(int nIndex, ARG_TYPE newElement, int nCount = 1);
    void    RemoveAt(int nIndex, int nCount = 1);
    HRESULT InsertAt(int nStartIndex, CSPArray* pNewArray);
    void    Sort(int (__cdecl *compare )(const void *elem1, const void *elem2 ));

// Implementation
protected:
    TYPE* m_pData;   // the actual array of data
    int m_nSize;     // # of elements (upperBound - 1)
    int m_nMaxSize;  // max allocated
    int m_nGrowBy;   // grow amount

public:
    ~CSPArray();
#ifdef _DEBUG
//  void Dump(CDumpContext&) const;
    void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CSPArray<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
inline int CSPArray<TYPE, ARG_TYPE>::GetSize() const
    { return m_nSize; }
template<class TYPE, class ARG_TYPE>
inline int CSPArray<TYPE, ARG_TYPE>::GetUpperBound() const
    { return m_nSize-1; }
template<class TYPE, class ARG_TYPE>
inline void CSPArray<TYPE, ARG_TYPE>::RemoveAll()
    { SetSize(0, -1); }
template<class TYPE, class ARG_TYPE>
inline TYPE CSPArray<TYPE, ARG_TYPE>::GetAt(int nIndex) const
    { SPDBG_ASSERT( (nIndex >= 0 && nIndex < m_nSize) );
        return m_pData[nIndex]; }
template<class TYPE, class ARG_TYPE>
inline void CSPArray<TYPE, ARG_TYPE>::SetAt(int nIndex, ARG_TYPE newElement)
    { SPDBG_ASSERT( (nIndex >= 0 && nIndex < m_nSize) );
        m_pData[nIndex] = newElement; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CSPArray<TYPE, ARG_TYPE>::ElementAt(int nIndex)
    { SPDBG_ASSERT( (nIndex >= 0 && nIndex < m_nSize) );
        return m_pData[nIndex]; }
template<class TYPE, class ARG_TYPE>
inline const TYPE* CSPArray<TYPE, ARG_TYPE>::GetData() const
    { return (const TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
inline TYPE* CSPArray<TYPE, ARG_TYPE>::GetData()
    { return (TYPE*)m_pData; }
template<class TYPE, class ARG_TYPE>
inline int CSPArray<TYPE, ARG_TYPE>::Add(ARG_TYPE newElement)
    { int nIndex = m_nSize;
        SetAtGrow(nIndex, newElement);
        return nIndex; }
template<class TYPE, class ARG_TYPE>
inline TYPE CSPArray<TYPE, ARG_TYPE>::operator[](int nIndex) const
    { return GetAt(nIndex); }
template<class TYPE, class ARG_TYPE>
inline TYPE& CSPArray<TYPE, ARG_TYPE>::operator[](int nIndex)
    { return ElementAt(nIndex); }

/////////////////////////////////////////////////////////////////////////////
// CSPArray<TYPE, ARG_TYPE> out-of-line functions

template<class TYPE, class ARG_TYPE>
CSPArray<TYPE, ARG_TYPE>::CSPArray()
{
    m_pData = NULL;
    m_nSize = m_nMaxSize = m_nGrowBy = 0;
}

template<class TYPE, class ARG_TYPE>
CSPArray<TYPE, ARG_TYPE>::~CSPArray()
{
    SPASSERT_VALID( this );

    if (m_pData != NULL)
    {
        SPDestructElements(m_pData, m_nSize);
        delete[] (BYTE*)m_pData;
    }
}

template<class TYPE, class ARG_TYPE>
HRESULT CSPArray<TYPE, ARG_TYPE>::SetSize(int nNewSize, int nGrowBy)
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( nNewSize >= 0 );
    HRESULT hr = S_OK;

    if (nGrowBy != -1)
        m_nGrowBy = nGrowBy;  // set new size

    if (nNewSize == 0)
    {
        // shrink to nothing
        if (m_pData != NULL)
        {
            SPDestructElements(m_pData, m_nSize);
            delete[] (BYTE*)m_pData;
            m_pData = NULL;
        }
        m_nSize = m_nMaxSize = 0;
    }
    else if (m_pData == NULL)
    {
        // create one with exact size
#ifdef SIZE_T_MAX
        SPDBG_ASSERT( nNewSize <= SIZE_T_MAX/sizeof(TYPE) );    // no overflow
#endif
        m_pData = (TYPE*) new BYTE[nNewSize * sizeof(TYPE)];
        if( m_pData )
        {
            hr = SPConstructElements(m_pData, nNewSize);
            if( SUCCEEDED( hr ) )
            {
                m_nSize = m_nMaxSize = nNewSize;
            }
            else
            {
                delete[] (BYTE*)m_pData;
                m_pData = NULL;
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else if (nNewSize <= m_nMaxSize)
    {
        // it fits
        if (nNewSize > m_nSize)
        {
            // initialize the new elements
            hr = SPConstructElements(&m_pData[m_nSize], nNewSize-m_nSize);
        }
        else if (m_nSize > nNewSize)
        {
            // destroy the old elements
            SPDestructElements(&m_pData[nNewSize], m_nSize-nNewSize);
        }

        if( SUCCEEDED( hr ) )
        {
            m_nSize = nNewSize;
        }
    }
    else
    {
        // otherwise, grow array
        int nGrowBy = m_nGrowBy;
        if (nGrowBy == 0)
        {
            // heuristically determe growth when nGrowBy == 0
            //  (this avoids heap fragmentation in many situations)
            nGrowBy = min(1024, max(4, m_nSize / 8));
        }
        int nNewMax;
        if (nNewSize < m_nMaxSize + nGrowBy)
            nNewMax = m_nMaxSize + nGrowBy;  // granularity
        else
            nNewMax = nNewSize;  // no slush

        SPDBG_ASSERT( nNewMax >= m_nMaxSize );  // no wrap around
#ifdef SIZE_T_MAX
        SPDBG_ASSERT( nNewMax <= SIZE_T_MAX/sizeof(TYPE) ); // no overflow
#endif
        TYPE* pNewData = (TYPE*) new BYTE[nNewMax * sizeof(TYPE)];

        if( pNewData )
        {
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(TYPE));

            // construct remaining elements
            SPDBG_ASSERT( nNewSize > m_nSize );
            hr = SPConstructElements(&pNewData[m_nSize], nNewSize-m_nSize);

            // get rid of old stuff (note: no destructors called)
            delete[] (BYTE*)m_pData;
            m_pData = pNewData;
            m_nSize = nNewSize;
            m_nMaxSize = nNewMax;
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    return hr;
}

template<class TYPE, class ARG_TYPE>
int CSPArray<TYPE, ARG_TYPE>::Append(const CSPArray& src)
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( this != &src );   // cannot append to itself

    int nOldSize = m_nSize;
    HRESULT hr = SetSize(m_nSize + src.m_nSize);
    if( SUCCEEDED( hr ) )
    {
        hr = SPCopyElements(m_pData + nOldSize, src.m_pData, src.m_nSize);
    }
    return ( SUCCEEDED( hr ) )?(nOldSize):(-1);
}

template<class TYPE, class ARG_TYPE>
HRESULT CSPArray<TYPE, ARG_TYPE>::Copy(const CSPArray& src)
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( this != &src );   // cannot copy to itself

    HRESULT hr = SetSize(src.m_nSize);
    if( SUCCEEDED( hr ) )
    {
        hr = SPCopyElements(m_pData, src.m_pData, src.m_nSize);
    }
    return hr;
}

template<class TYPE, class ARG_TYPE>
void CSPArray<TYPE, ARG_TYPE>::FreeExtra()
{
    SPASSERT_VALID( this );

    if (m_nSize != m_nMaxSize)
    {
        // shrink to desired size
#ifdef SIZE_T_MAX
        SPDBG_ASSERT( m_nSize <= SIZE_T_MAX/sizeof(TYPE)); // no overflow
#endif
        TYPE* pNewData = NULL;
        if (m_nSize != 0)
        {
            pNewData = (TYPE*) new BYTE[m_nSize * sizeof(TYPE)];
            SPDBG_ASSERT(pNewData);
            // copy new data from old
            memcpy(pNewData, m_pData, m_nSize * sizeof(TYPE));
        }

        // get rid of old stuff (note: no destructors called)
        delete[] (BYTE*)m_pData;
        m_pData = pNewData;
        m_nMaxSize = m_nSize;
    }
}

template<class TYPE, class ARG_TYPE>
HRESULT CSPArray<TYPE, ARG_TYPE>::SetAtGrow(int nIndex, ARG_TYPE newElement)
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( nIndex >= 0 );
    HRESULT hr = S_OK;

    if (nIndex >= m_nSize)
    {
        hr = SetSize(nIndex+1, -1);
    }

    if( SUCCEEDED( hr ) )
    {
        m_pData[nIndex] = newElement;
    }
    return hr;
}

template<class TYPE, class ARG_TYPE>
HRESULT CSPArray<TYPE, ARG_TYPE>::InsertAt(int nIndex, ARG_TYPE newElement, int nCount /*=1*/)
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( nIndex >= 0 );    // will expand to meet need
    SPDBG_ASSERT( nCount > 0 );     // zero or negative size not allowed
    HRESULT hr = S_OK;

    if (nIndex >= m_nSize)
    {
        // adding after the end of the array
        hr = SetSize(nIndex + nCount, -1);   // grow so nIndex is valid
    }
    else
    {
        // inserting in the middle of the array
        int nOldSize = m_nSize;
        hr = SetSize(m_nSize + nCount, -1);  // grow it to new size
        if( SUCCEEDED( hr ) )
        {
            // shift old data up to fill gap
            memmove(&m_pData[nIndex+nCount], &m_pData[nIndex],
                (nOldSize-nIndex) * sizeof(TYPE));

            // re-init slots we copied from
            hr = SPConstructElements(&m_pData[nIndex], nCount);
        }
    }

    // insert new value in the gap
    if( SUCCEEDED( hr ) )
    {
        SPDBG_ASSERT( nIndex + nCount <= m_nSize );
        while (nCount--)
            m_pData[nIndex++] = newElement;
    }
    return hr;
}

template<class TYPE, class ARG_TYPE>
void CSPArray<TYPE, ARG_TYPE>::RemoveAt(int nIndex, int nCount)
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( nIndex >= 0 );
    SPDBG_ASSERT( nCount >= 0 );
    SPDBG_ASSERT( nIndex + nCount <= m_nSize );

    // just remove a range
    int nMoveCount = m_nSize - (nIndex + nCount);
    SPDestructElements(&m_pData[nIndex], nCount);
    if (nMoveCount)
        memcpy(&m_pData[nIndex], &m_pData[nIndex + nCount],
            nMoveCount * sizeof(TYPE));
    m_nSize -= nCount;
}

template<class TYPE, class ARG_TYPE>
HRESULT CSPArray<TYPE, ARG_TYPE>::InsertAt(int nStartIndex, CSPArray* pNewArray)
{
    SPASSERT_VALID( this );
    SPASSERT_VALID( pNewArray );
    SPDBG_ASSERT( nStartIndex >= 0 );
    HRESULT hr = S_OK;

    if (pNewArray->GetSize() > 0)
    {
        hr = InsertAt(nStartIndex, pNewArray->GetAt(0), pNewArray->GetSize());
        for (int i = 0; SUCCEEDED( hr )&& (i < pNewArray->GetSize()); i++)
        {
            SetAt(nStartIndex + i, pNewArray->GetAt(i));
        }
    }
    return hr;
}

template<class TYPE, class ARG_TYPE>
void CSPArray<TYPE, ARG_TYPE>::Sort(int (__cdecl *compare )(const void *elem1, const void *elem2 ))
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( m_pData != NULL );

    qsort( m_pData, m_nSize, sizeof(TYPE), compare );
}

#ifdef _DEBUG
template<class TYPE, class ARG_TYPE>
void CSPArray<TYPE, ARG_TYPE>::AssertValid() const
{
    if (m_pData == NULL)
    {
        SPDBG_ASSERT( m_nSize == 0 );
        SPDBG_ASSERT( m_nMaxSize == 0 );
    }
    else
    {
        SPDBG_ASSERT( m_nSize >= 0 );
        SPDBG_ASSERT( m_nMaxSize >= 0 );
        SPDBG_ASSERT( m_nSize <= m_nMaxSize );
        SPDBG_ASSERT( SPIsValidAddress(m_pData, m_nMaxSize * sizeof(TYPE), TRUE ) );
    }
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSPList<TYPE, ARG_TYPE>

template<class TYPE, class ARG_TYPE>
class CSPList
{
protected:
    struct CNode
    {
        CNode* pNext;
        CNode* pPrev;
        TYPE data;
    };
public:

// Construction
    CSPList(int nBlockSize = 10);

// Attributes (head and tail)
    // count of elements
    int GetCount() const;
    BOOL IsEmpty() const;

    // peek at head or tail
    TYPE& GetHead();
    TYPE GetHead() const;
    TYPE& GetTail();
    TYPE GetTail() const;

// Operations
    // get head or tail (and remove it) - don't call on empty list !
    TYPE RemoveHead();
    TYPE RemoveTail();

    // add before head or after tail
    SPLISTPOS AddHead(ARG_TYPE newElement);
    SPLISTPOS AddTail(ARG_TYPE newElement);

    // add another list of elements before head or after tail
    void AddHead(CSPList* pNewList);
    void AddTail(CSPList* pNewList);

    // remove all elements
    void RemoveAll();

    // iteration
    SPLISTPOS GetHeadPosition() const;
    SPLISTPOS GetTailPosition() const;
    TYPE& GetNext(SPLISTPOS& rPosition); // return *Position++
    TYPE GetNext(SPLISTPOS& rPosition) const; // return *Position++
    TYPE& GetPrev(SPLISTPOS& rPosition); // return *Position--
    TYPE GetPrev(SPLISTPOS& rPosition) const; // return *Position--

    // getting/modifying an element at a given position
    TYPE& GetAt(SPLISTPOS position);
    TYPE GetAt(SPLISTPOS position) const;
    void SetAt(SPLISTPOS pos, ARG_TYPE newElement);
    void RemoveAt(SPLISTPOS position);

    // inserting before or after a given position
    SPLISTPOS InsertBefore(SPLISTPOS position, ARG_TYPE newElement);
    SPLISTPOS InsertAfter(SPLISTPOS position, ARG_TYPE newElement);

    // helper functions (note: O(n) speed)
    SPLISTPOS Find(ARG_TYPE searchValue, SPLISTPOS startAfter = NULL) const;
        // defaults to starting at the HEAD, return NULL if not found
    SPLISTPOS FindIndex(int nIndex) const;
        // get the 'nIndex'th element (may return NULL)

// Implementation
protected:
    CNode* m_pNodeHead;
    CNode* m_pNodeTail;
    int m_nCount;
    CNode* m_pNodeFree;
    struct CSPPlex* m_pBlocks;
    int m_nBlockSize;

    CNode* NewNode(CNode*, CNode*);
    void FreeNode(CNode*);

public:
    ~CSPList();
#ifdef _DEBUG
    void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CSPList<TYPE, ARG_TYPE> inline functions

template<class TYPE, class ARG_TYPE>
inline int CSPList<TYPE, ARG_TYPE>::GetCount() const
    { return m_nCount; }
template<class TYPE, class ARG_TYPE>
inline BOOL CSPList<TYPE, ARG_TYPE>::IsEmpty() const
    { return m_nCount == 0; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CSPList<TYPE, ARG_TYPE>::GetHead()
    {   SPDBG_ASSERT( m_pNodeHead != NULL );
        return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CSPList<TYPE, ARG_TYPE>::GetHead() const
    {   SPDBG_ASSERT( m_pNodeHead != NULL );
        return m_pNodeHead->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CSPList<TYPE, ARG_TYPE>::GetTail()
    {   SPDBG_ASSERT( m_pNodeTail != NULL );
        return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CSPList<TYPE, ARG_TYPE>::GetTail() const
    {   SPDBG_ASSERT( m_pNodeTail != NULL );
        return m_pNodeTail->data; }
template<class TYPE, class ARG_TYPE>
inline SPLISTPOS CSPList<TYPE, ARG_TYPE>::GetHeadPosition() const
    { return (SPLISTPOS) m_pNodeHead; }
template<class TYPE, class ARG_TYPE>
inline SPLISTPOS CSPList<TYPE, ARG_TYPE>::GetTailPosition() const
    { return (SPLISTPOS) m_pNodeTail; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CSPList<TYPE, ARG_TYPE>::GetNext(SPLISTPOS& rPosition) // return *Position++
    {   CNode* pNode = (CNode*) rPosition;
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        rPosition = (SPLISTPOS) pNode->pNext;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CSPList<TYPE, ARG_TYPE>::GetNext(SPLISTPOS& rPosition) const // return *Position++
    {   CNode* pNode = (CNode*) rPosition;
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        rPosition = (SPLISTPOS) pNode->pNext;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CSPList<TYPE, ARG_TYPE>::GetPrev(SPLISTPOS& rPosition) // return *Position--
    {   CNode* pNode = (CNode*) rPosition;
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        rPosition = (SPLISTPOS) pNode->pPrev;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CSPList<TYPE, ARG_TYPE>::GetPrev(SPLISTPOS& rPosition) const // return *Position--
    {   CNode* pNode = (CNode*) rPosition;
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        rPosition = (SPLISTPOS) pNode->pPrev;
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE& CSPList<TYPE, ARG_TYPE>::GetAt(SPLISTPOS position)
    {   CNode* pNode = (CNode*) position;
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline TYPE CSPList<TYPE, ARG_TYPE>::GetAt(SPLISTPOS position) const
    {   CNode* pNode = (CNode*) position;
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        return pNode->data; }
template<class TYPE, class ARG_TYPE>
inline void CSPList<TYPE, ARG_TYPE>::SetAt(SPLISTPOS pos, ARG_TYPE newElement)
    {   CNode* pNode = (CNode*) pos;
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        pNode->data = newElement; }

/////////////////////////////////////////////////////////////////////////////
// CSPList<TYPE, ARG_TYPE> out-of-line functions

template<class TYPE, class ARG_TYPE>
CSPList<TYPE, ARG_TYPE>::CSPList( int nBlockSize )
{
    SPDBG_ASSERT( nBlockSize > 0 );

    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    m_pBlocks = NULL;
    m_nBlockSize = nBlockSize;
}

template<class TYPE, class ARG_TYPE>
void CSPList<TYPE, ARG_TYPE>::RemoveAll()
{
    SPASSERT_VALID( this );

    // destroy elements
    CNode* pNode;
    for (pNode = m_pNodeHead; pNode != NULL; pNode = pNode->pNext)
        SPDestructElements(&pNode->data, 1);

    m_nCount = 0;
    m_pNodeHead = m_pNodeTail = m_pNodeFree = NULL;
    m_pBlocks->FreeDataChain();
    m_pBlocks = NULL;
}

template<class TYPE, class ARG_TYPE>
CSPList<TYPE, ARG_TYPE>::~CSPList()
{
    RemoveAll();
    SPDBG_ASSERT( m_nCount == 0 );
}

/////////////////////////////////////////////////////////////////////////////
// Node helpers
//
// Implementation note: CNode's are stored in CSPPlex blocks and
//  chained together. Free blocks are maintained in a singly linked list
//  using the 'pNext' member of CNode with 'm_pNodeFree' as the head.
//  Used blocks are maintained in a doubly linked list using both 'pNext'
//  and 'pPrev' as links and 'm_pNodeHead' and 'm_pNodeTail'
//   as the head/tail.
//
// We never free a CSPPlex block unless the List is destroyed or RemoveAll()
//  is used - so the total number of CSPPlex blocks may grow large depending
//  on the maximum past size of the list.
//

template<class TYPE, class ARG_TYPE>
CSPList<TYPE, ARG_TYPE>::CNode*
CSPList<TYPE, ARG_TYPE>::NewNode(CSPList::CNode* pPrev, CSPList::CNode* pNext)
{
    if (m_pNodeFree == NULL)
    {
        // add another block
        CSPPlex* pNewBlock = CSPPlex::Create(m_pBlocks, m_nBlockSize,sizeof(CNode));

        // chain them into free list
        CNode* pNode = (CNode*) pNewBlock->data();
        // free in reverse order to make it easier to debug
        pNode += m_nBlockSize - 1;
        for (int i = m_nBlockSize-1; i >= 0; i--, pNode--)
        {
            pNode->pNext = m_pNodeFree;
            m_pNodeFree = pNode;
        }
    }

    CSPList::CNode* pNode = m_pNodeFree;
    if( pNode )
    {
        if( SUCCEEDED( SPConstructElements(&pNode->data, 1) ) )
        {
            m_pNodeFree  = m_pNodeFree->pNext;
            pNode->pPrev = pPrev;
            pNode->pNext = pNext;
            m_nCount++;
            SPDBG_ASSERT( m_nCount > 0 );  // make sure we don't overflow
        }
    }
    return pNode;
}

template<class TYPE, class ARG_TYPE>
void CSPList<TYPE, ARG_TYPE>::FreeNode(CSPList::CNode* pNode)
{
    SPDestructElements(&pNode->data, 1);
    pNode->pNext = m_pNodeFree;
    m_pNodeFree = pNode;
    m_nCount--;
    SPDBG_ASSERT( m_nCount >= 0 );  // make sure we don't underflow
}

template<class TYPE, class ARG_TYPE>
SPLISTPOS CSPList<TYPE, ARG_TYPE>::AddHead(ARG_TYPE newElement)
{
    SPASSERT_VALID( this );

    CNode* pNewNode = NewNode(NULL, m_pNodeHead);
    if( pNewNode )
    {
        pNewNode->data = newElement;
        if (m_pNodeHead != NULL)
            m_pNodeHead->pPrev = pNewNode;
        else
            m_pNodeTail = pNewNode;
        m_pNodeHead = pNewNode;
    }
    return (SPLISTPOS) pNewNode;
}

template<class TYPE, class ARG_TYPE>
SPLISTPOS CSPList<TYPE, ARG_TYPE>::AddTail(ARG_TYPE newElement)
{
    SPASSERT_VALID( this );

    CNode* pNewNode = NewNode(m_pNodeTail, NULL);
    if( pNewNode )
    {
        pNewNode->data = newElement;
        if (m_pNodeTail != NULL)
            m_pNodeTail->pNext = pNewNode;
        else
            m_pNodeHead = pNewNode;
        m_pNodeTail = pNewNode;
    }
    return (SPLISTPOS) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CSPList<TYPE, ARG_TYPE>::AddHead(CSPList* pNewList)
{
    SPASSERT_VALID( this );
    SPASSERT_VALID( pNewList );

    // add a list of same elements to head (maintain order)
    SPLISTPOS pos = pNewList->GetTailPosition();
    while (pos != NULL)
        AddHead(pNewList->GetPrev(pos));
}

template<class TYPE, class ARG_TYPE>
void CSPList<TYPE, ARG_TYPE>::AddTail(CSPList* pNewList)
{
    SPASSERT_VALID( this );
    SPASSERT_VALID( pNewList );

    // add a list of same elements
    SPLISTPOS pos = pNewList->GetHeadPosition();
    while (pos != NULL)
        AddTail(pNewList->GetNext(pos));
}

template<class TYPE, class ARG_TYPE>
TYPE CSPList<TYPE, ARG_TYPE>::RemoveHead()
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( m_pNodeHead != NULL );  // don't call on empty list !!!
    SPDBG_ASSERT( SPIsValidAddress(m_pNodeHead, sizeof(CNode), TRUE ) );

    CNode* pOldNode = m_pNodeHead;
    TYPE returnValue = pOldNode->data;

    m_pNodeHead = pOldNode->pNext;
    if (m_pNodeHead != NULL)
        m_pNodeHead->pPrev = NULL;
    else
        m_pNodeTail = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

template<class TYPE, class ARG_TYPE>
TYPE CSPList<TYPE, ARG_TYPE>::RemoveTail()
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( m_pNodeTail != NULL );  // don't call on empty list !!!
    SPDBG_ASSERT( SPIsValidAddress(m_pNodeTail, sizeof(CNode), TRUE ) );

    CNode* pOldNode = m_pNodeTail;
    TYPE returnValue = pOldNode->data;

    m_pNodeTail = pOldNode->pPrev;
    if (m_pNodeTail != NULL)
        m_pNodeTail->pNext = NULL;
    else
        m_pNodeHead = NULL;
    FreeNode(pOldNode);
    return returnValue;
}

template<class TYPE, class ARG_TYPE>
SPLISTPOS CSPList<TYPE, ARG_TYPE>::InsertBefore(SPLISTPOS position, ARG_TYPE newElement)
{
    SPASSERT_VALID( this );

    if (position == NULL)
        return AddHead(newElement); // insert before nothing -> head of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    CNode* pNewNode = NewNode(pOldNode->pPrev, pOldNode);
    if( pNewNode )
    {
        pNewNode->data = newElement;

        if (pOldNode->pPrev != NULL)
        {
            SPDBG_ASSERT( SPIsValidAddress(pOldNode->pPrev, sizeof(CNode), TRUE ) );
            pOldNode->pPrev->pNext = pNewNode;
        }
        else
        {
            SPDBG_ASSERT( pOldNode == m_pNodeHead );
            m_pNodeHead = pNewNode;
        }
        pOldNode->pPrev = pNewNode;
    }
    return (SPLISTPOS) pNewNode;
}

template<class TYPE, class ARG_TYPE>
SPLISTPOS CSPList<TYPE, ARG_TYPE>::InsertAfter(SPLISTPOS position, ARG_TYPE newElement)
{
    SPASSERT_VALID( this );

    if (position == NULL)
        return AddTail(newElement); // insert after nothing -> tail of the list

    // Insert it before position
    CNode* pOldNode = (CNode*) position;
    SPDBG_ASSERT( SPIsValidAddress(pOldNode, sizeof(CNode), TRUE ));
    CNode* pNewNode = NewNode(pOldNode, pOldNode->pNext);
    if( pNewNode )
    {
        pNewNode->data = newElement;

        if (pOldNode->pNext != NULL)
        {
            SPDBG_ASSERT( SPIsValidAddress(pOldNode->pNext, sizeof(CNode), TRUE ));
            pOldNode->pNext->pPrev = pNewNode;
        }
        else
        {
            SPDBG_ASSERT( pOldNode == m_pNodeTail );
            m_pNodeTail = pNewNode;
        }
        pOldNode->pNext = pNewNode;
    }
    return (SPLISTPOS) pNewNode;
}

template<class TYPE, class ARG_TYPE>
void CSPList<TYPE, ARG_TYPE>::RemoveAt(SPLISTPOS position)
{
    SPASSERT_VALID( this );

    CNode* pOldNode = (CNode*) position;
    SPDBG_ASSERT( SPIsValidAddress(pOldNode, sizeof(CNode), TRUE ) );

    // remove pOldNode from list
    if (pOldNode == m_pNodeHead)
    {
        m_pNodeHead = pOldNode->pNext;
    }
    else
    {
        SPDBG_ASSERT( SPIsValidAddress(pOldNode->pPrev, sizeof(CNode), TRUE ) );
        pOldNode->pPrev->pNext = pOldNode->pNext;
    }
    if (pOldNode == m_pNodeTail)
    {
        m_pNodeTail = pOldNode->pPrev;
    }
    else
    {
        SPDBG_ASSERT( SPIsValidAddress(pOldNode->pNext, sizeof(CNode), TRUE ) );
        pOldNode->pNext->pPrev = pOldNode->pPrev;
    }
    FreeNode(pOldNode);
}

template<class TYPE, class ARG_TYPE>
SPLISTPOS CSPList<TYPE, ARG_TYPE>::FindIndex(int nIndex) const
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( nIndex >= 0 );

    if (nIndex >= m_nCount)
        return NULL;  // went too far

    CNode* pNode = m_pNodeHead;
    while (nIndex--)
    {
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ));
        pNode = pNode->pNext;
    }
    return (SPLISTPOS) pNode;
}

template<class TYPE, class ARG_TYPE>
SPLISTPOS CSPList<TYPE, ARG_TYPE>::Find(ARG_TYPE searchValue, SPLISTPOS startAfter) const
{
    SPASSERT_VALID( this );

    CNode* pNode = (CNode*) startAfter;
    if (pNode == NULL)
    {
        pNode = m_pNodeHead;  // start at head
    }
    else
    {
        SPDBG_ASSERT( SPIsValidAddress(pNode, sizeof(CNode), TRUE ) );
        pNode = pNode->pNext;  // start after the one specified
    }

    for (; pNode != NULL; pNode = pNode->pNext)
        if (SPCompareElements(&pNode->data, &searchValue))
            return (SPLISTPOS)pNode;
    return NULL;
}

#ifdef _DEBUG
template<class TYPE, class ARG_TYPE>
void CSPList<TYPE, ARG_TYPE>::AssertValid() const
{
    if (m_nCount == 0)
    {
        // empty list
        SPDBG_ASSERT( m_pNodeHead == NULL );
        SPDBG_ASSERT( m_pNodeTail == NULL );
    }
    else
    {
        // non-empty list
        SPDBG_ASSERT( SPIsValidAddress(m_pNodeHead, sizeof(CNode), TRUE ));
        SPDBG_ASSERT( SPIsValidAddress(m_pNodeTail, sizeof(CNode), TRUE ));
    }
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
class CSPMap
{
protected:
    // Association
    struct CAssoc
    {
        CAssoc* pNext;
        UINT nHashValue;  // needed for efficient iteration
        KEY key;
        VALUE value;
    };
public:
// Construction
    CSPMap( int nBlockSize = 10 );

// Attributes
    // number of elements
    int GetCount() const;
    BOOL IsEmpty() const;

    // Lookup
    BOOL Lookup(ARG_KEY key, VALUE& rValue) const;

// Operations
    // Lookup and add if not there
    VALUE& operator[](ARG_KEY key);

    // add a new (key, value) pair
    void SetAt(ARG_KEY key, ARG_VALUE newValue);

    // removing existing (key, ?) pair
    BOOL RemoveKey(ARG_KEY key);
    void RemoveAll();

    // iterating all (key, value) pairs
    SPLISTPOS GetStartPosition() const;
    void GetNextAssoc(SPLISTPOS& rNextPosition, KEY& rKey, VALUE& rValue) const;

    // advanced features for derived classes
    UINT    GetHashTableSize() const;
    HRESULT InitHashTable(UINT hashSize, BOOL bAllocNow = TRUE);

// Implementation
protected:
    CAssoc** m_pHashTable;
    UINT m_nHashTableSize;
    int m_nCount;
    CAssoc* m_pFreeList;
    struct CSPPlex* m_pBlocks;
    int m_nBlockSize;

    CAssoc* NewAssoc();
    void FreeAssoc(CAssoc*);
    CAssoc* GetAssocAt(ARG_KEY, UINT&) const;

public:
    ~CSPMap();
#ifdef _DEBUG
//  void Dump(CDumpContext&) const;
    void AssertValid() const;
#endif
};

/////////////////////////////////////////////////////////////////////////////
// CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE> inline functions

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline int CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetCount() const
    { return m_nCount; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline BOOL CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::IsEmpty() const
    { return m_nCount == 0; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline void CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::SetAt(ARG_KEY key, ARG_VALUE newValue)
    { (*this)[key] = newValue; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline SPLISTPOS CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetStartPosition() const
    { return (m_nCount == 0) ? NULL : SP_BEFORE_START_POSITION; }
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
inline UINT CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetHashTableSize() const
    { return m_nHashTableSize; }

/////////////////////////////////////////////////////////////////////////////
// CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE> out-of-line functions

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CSPMap( int nBlockSize )
{
    SPDBG_ASSERT( nBlockSize > 0 );

    m_pHashTable = NULL;
    m_nHashTableSize = 17;  // default size
    m_nCount = 0;
    m_pFreeList = NULL;
    m_pBlocks = NULL;
    m_nBlockSize = nBlockSize;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
HRESULT CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::InitHashTable(
                UINT nHashSize, BOOL bAllocNow)
//
// Used to force allocation of a hash table or to override the default
//   hash table size of (which is fairly small)
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( m_nCount == 0 );
    SPDBG_ASSERT( nHashSize > 0 );
    HRESULT hr = S_OK;

    if (m_pHashTable != NULL)
    {
        // free hash table
        delete[] m_pHashTable;
        m_pHashTable = NULL;
    }

    if (bAllocNow)
    {
        m_pHashTable = new CAssoc* [nHashSize];
        if( m_pHashTable )
        {
            memset(m_pHashTable, 0, sizeof(CAssoc*) * nHashSize);
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    m_nHashTableSize = ( SUCCEEDED( hr ) )?(nHashSize):(0);
    return hr;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::RemoveAll()
{
    SPASSERT_VALID( this );

    if (m_pHashTable != NULL)
    {
        // destroy elements (values and keys)
        for (UINT nHash = 0; nHash < m_nHashTableSize; nHash++)
        {
            CAssoc* pAssoc;
            for( pAssoc = m_pHashTable[nHash]; pAssoc != NULL;
                 pAssoc = pAssoc->pNext)
            {
                SPDestructElements(&pAssoc->value, 1);
                SPDestructElements(&pAssoc->key, 1);
            }
        }
    }

    // free hash table
    delete[] m_pHashTable;
    m_pHashTable = NULL;

    m_nCount = 0;
    m_pFreeList = NULL;
    m_pBlocks->FreeDataChain();
    m_pBlocks = NULL;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::~CSPMap()
{
    RemoveAll();
    SPDBG_ASSERT( m_nCount == 0 );
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc*
CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::NewAssoc()
{
    if (m_pFreeList == NULL)
    {
        // add another block
        CSPPlex* newBlock = CSPPlex::Create(m_pBlocks, m_nBlockSize, sizeof(CSPMap::CAssoc));

        if( newBlock )
        {
            // chain them into free list
            CSPMap::CAssoc* pAssoc = (CSPMap::CAssoc*) newBlock->data();
            // free in reverse order to make it easier to debug
            pAssoc += m_nBlockSize - 1;
            for (int i = m_nBlockSize-1; i >= 0; i--, pAssoc--)
            {
                pAssoc->pNext = m_pFreeList;
                m_pFreeList = pAssoc;
            }
        }
    }

    CSPMap::CAssoc* pAssoc = m_pFreeList;
    if( pAssoc )
    {
        if( SUCCEEDED( SPConstructElements(&pAssoc->key, 1   ) ) )
        {
            if( SUCCEEDED( SPConstructElements(&pAssoc->value, 1 ) ) )
            {
                m_pFreeList = m_pFreeList->pNext;
                m_nCount++;
                SPDBG_ASSERT( m_nCount > 0 );  // make sure we don't overflow
            }
            else
            {
                SPDestructElements( &pAssoc->key, 1 );
            }
        }
        else
        {
            pAssoc = NULL;
        }
    }
    return pAssoc;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::FreeAssoc(CSPMap::CAssoc* pAssoc)
{
    SPDestructElements(&pAssoc->value, 1);
    SPDestructElements(&pAssoc->key, 1);
    pAssoc->pNext = m_pFreeList;
    m_pFreeList = pAssoc;
    m_nCount--;
    SPDBG_ASSERT( m_nCount >= 0 );  // make sure we don't underflow
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::CAssoc*
CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetAssocAt(ARG_KEY key, UINT& nHash) const
// find association (or return NULL)
{
    nHash = SPHashKey(key) % m_nHashTableSize;

    if (m_pHashTable == NULL)
        return NULL;

    // see if it exists
    CAssoc* pAssoc;
    for (pAssoc = m_pHashTable[nHash]; pAssoc != NULL; pAssoc = pAssoc->pNext)
    {
        if (SPCompareElements(&pAssoc->key, &key))
            return pAssoc;
    }
    return NULL;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::Lookup(ARG_KEY key, VALUE& rValue) const
{
    SPASSERT_VALID( this );

    UINT nHash;
    CAssoc* pAssoc = GetAssocAt(key, nHash);
    if (pAssoc == NULL)
        return FALSE;  // not in map

    rValue = pAssoc->value;
    return TRUE;
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
VALUE& CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::operator[](ARG_KEY key)
{
    SPASSERT_VALID( this );
    HRESULT hr = S_OK;
    static const CAssoc ErrAssoc = 0;

    UINT nHash;
    CAssoc* pAssoc;
    if ((pAssoc = GetAssocAt(key, nHash)) == NULL)
    {
        if( m_pHashTable == NULL )
        {
            hr = InitHashTable(m_nHashTableSize);
        }

        if( SUCCEEDED( hr ) )
        {
            // it doesn't exist, add a new Association
            pAssoc = NewAssoc();
            if( pAssoc )
            {
                pAssoc->nHashValue = nHash;
                pAssoc->key = key;
                // 'pAssoc->value' is a constructed object, nothing more

                // put into hash table
                pAssoc->pNext = m_pHashTable[nHash];
                m_pHashTable[nHash] = pAssoc;
            }
            else
            {
                pAssoc = &ErrAssoc;
            }
        }
    }
    return pAssoc->value;  // return new reference
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
BOOL CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::RemoveKey(ARG_KEY key)
// remove key - return TRUE if removed
{
    SPASSERT_VALID( this );

    if (m_pHashTable == NULL)
        return FALSE;  // nothing in the table

    CAssoc** ppAssocPrev;
    ppAssocPrev = &m_pHashTable[SPHashKey(key) % m_nHashTableSize];

    CAssoc* pAssoc;
    for (pAssoc = *ppAssocPrev; pAssoc != NULL; pAssoc = pAssoc->pNext)
    {
        if (SPCompareElements(&pAssoc->key, &key))
        {
            // remove it
            *ppAssocPrev = pAssoc->pNext;  // remove from list
            FreeAssoc(pAssoc);
            return TRUE;
        }
        ppAssocPrev = &pAssoc->pNext;
    }
    return FALSE;  // not found
}

template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::GetNextAssoc(SPLISTPOS& rNextPosition,
    KEY& rKey, VALUE& rValue) const
{
    SPASSERT_VALID( this );
    SPDBG_ASSERT( m_pHashTable != NULL );  // never call on empty map

    CAssoc* pAssocRet = (CAssoc*)rNextPosition;
    SPDBG_ASSERT( pAssocRet != NULL );

    if (pAssocRet == (CAssoc*) SP_BEFORE_START_POSITION)
    {
        // find the first association
        for (UINT nBucket = 0; nBucket < m_nHashTableSize; nBucket++)
            if ((pAssocRet = m_pHashTable[nBucket]) != NULL)
                break;
        SPDBG_ASSERT( pAssocRet != NULL );  // must find something
    }

    // find next association
    SPDBG_ASSERT( SPIsValidAddress(pAssocRet, sizeof(CAssoc), TRUE ));
    CAssoc* pAssocNext;
    if ((pAssocNext = pAssocRet->pNext) == NULL)
    {
        // go to next bucket
        for (UINT nBucket = pAssocRet->nHashValue + 1;
          nBucket < m_nHashTableSize; nBucket++)
            if ((pAssocNext = m_pHashTable[nBucket]) != NULL)
                break;
    }

    rNextPosition = (SPLISTPOS) pAssocNext;

    // fill in return data
    rKey = pAssocRet->key;
    rValue = pAssocRet->value;
}

#ifdef _DEBUG
template<class KEY, class ARG_KEY, class VALUE, class ARG_VALUE>
void CSPMap<KEY, ARG_KEY, VALUE, ARG_VALUE>::AssertValid() const
{
    SPDBG_ASSERT( m_nHashTableSize > 0 );
    SPDBG_ASSERT( (m_nCount == 0 || m_pHashTable != NULL) );
        // non-empty map should have hash table
}
#endif //_DEBUG

#endif //--- This must be the last line in the file

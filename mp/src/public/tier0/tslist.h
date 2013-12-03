//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// LIFO from disassembly of Windows API and http://perso.wanadoo.fr/gmem/evenements/jim2002/articles/L17_Fober.pdf
// FIFO from http://perso.wanadoo.fr/gmem/evenements/jim2002/articles/L17_Fober.pdf
//
//=============================================================================

#ifndef TSLIST_H
#define TSLIST_H

#if defined( _WIN32 )
#pragma once
// Suppress this spurious warning:
// warning C4700: uninitialized local variable 'oldHead' used
#pragma warning( push )
#pragma warning( disable : 4700 )
#endif

#if defined( USE_NATIVE_SLIST ) && !defined( _X360 )
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include "tier0/dbg.h"
#include "tier0/threadtools.h"
#include "tier0/memalloc.h"
#include "tier0/memdbgoff.h"

#if defined( _X360 )
#define USE_NATIVE_SLIST
#endif

//-----------------------------------------------------------------------------

#if defined( PLATFORM_64BITS )

#define TSLIST_HEAD_ALIGNMENT 16
#define TSLIST_NODE_ALIGNMENT 16
inline bool ThreadInterlockedAssignIf64x128( volatile int128 *pDest, const int128 &value, const int128 &comperand )
	{ return ThreadInterlockedAssignIf128( pDest, value, comperand ); }
#else
#define TSLIST_HEAD_ALIGNMENT 8
#define TSLIST_NODE_ALIGNMENT 8
inline bool ThreadInterlockedAssignIf64x128( volatile int64 *pDest, const int64 value, const int64 comperand )
	{ return ThreadInterlockedAssignIf64( pDest, value, comperand ); }
#endif

#ifdef _MSC_VER
#define TSLIST_HEAD_ALIGN DECL_ALIGN(TSLIST_HEAD_ALIGNMENT)
#define TSLIST_NODE_ALIGN DECL_ALIGN(TSLIST_NODE_ALIGNMENT)
#define TSLIST_HEAD_ALIGN_POST
#define TSLIST_NODE_ALIGN_POST
#elif defined( GNUC )
#define TSLIST_HEAD_ALIGN 
#define TSLIST_NODE_ALIGN 
#define TSLIST_HEAD_ALIGN_POST DECL_ALIGN(TSLIST_HEAD_ALIGNMENT)
#define TSLIST_NODE_ALIGN_POST DECL_ALIGN(TSLIST_NODE_ALIGNMENT)
#elif defined( _PS3 )
#define TSLIST_HEAD_ALIGNMENT 8
#define TSLIST_NODE_ALIGNMENT 8

#define TSLIST_HEAD_ALIGN ALIGN8
#define TSLIST_NODE_ALIGN ALIGN8
#define TSLIST_HEAD_ALIGN_POST ALIGN8_POST
#define TSLIST_NODE_ALIGN_POST ALIGN8_POST

#else
#error
#endif

//-----------------------------------------------------------------------------

PLATFORM_INTERFACE bool RunTSQueueTests( int nListSize = 10000, int nTests = 1 );
PLATFORM_INTERFACE bool RunTSListTests( int nListSize = 10000, int nTests = 1 );

//-----------------------------------------------------------------------------
// Lock free list.
//-----------------------------------------------------------------------------
//#define USE_NATIVE_SLIST

#ifdef USE_NATIVE_SLIST
typedef SLIST_ENTRY TSLNodeBase_t;
typedef SLIST_HEADER TSLHead_t;
#else
struct TSLIST_NODE_ALIGN TSLNodeBase_t
{
	TSLNodeBase_t *Next; // name to match Windows
} TSLIST_NODE_ALIGN_POST;

union TSLIST_HEAD_ALIGN TSLHead_t
{
	struct Value_t
	{
		TSLNodeBase_t *Next;
		// <sergiy> Depth must be in the least significant halfword when atomically loading into register,
		//          to avoid carrying digits from Sequence. Carrying digits from Depth to Sequence is ok,
		//          because Sequence can be pretty much random. We could operate on both of them separately,
		//          but it could perhaps (?) lead to problems with store forwarding. I don't know 'cause I didn't 
		//          performance-test or design original code, I'm just making it work on PowerPC.
		#ifdef VALVE_BIG_ENDIAN
		int16	Sequence;
		int16   Depth;
		#else
		int16   Depth;
		int16	Sequence;
		#endif
#ifdef PLATFORM_64BITS
		int32   Padding;
#endif
	} value;

	struct Value32_t
	{
		TSLNodeBase_t *Next_do_not_use_me;
		int32   DepthAndSequence;
	} value32;

#ifdef PLATFORM_64BITS
	int128 value64x128;
#else
	int64 value64x128;
#endif
} TSLIST_HEAD_ALIGN_POST;

#endif

//-------------------------------------
class CTSListBase
{
public:

	// override new/delete so we can guarantee 8-byte aligned allocs
	static void * operator new( size_t size )
	{
		CTSListBase *pNode = (CTSListBase *)MemAlloc_AllocAligned( size, TSLIST_HEAD_ALIGNMENT, __FILE__, __LINE__ );
		return pNode;
	}

	static void * operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )
	{
		CTSListBase *pNode = (CTSListBase *)MemAlloc_AllocAligned( size, TSLIST_HEAD_ALIGNMENT, pFileName, nLine );
		return pNode;
	}

	static void operator delete( void *p)
	{
		MemAlloc_FreeAligned( p );
	}

	static void operator delete( void *p, int nBlockUse, const char *pFileName, int nLine )
	{
		MemAlloc_FreeAligned( p );
	}

private:
	// These ain't gonna work
	static void * operator new[] ( size_t size );
	static void operator delete [] ( void *p);
	
public:

	CTSListBase()
	{
		if ( ((size_t)&m_Head) % TSLIST_HEAD_ALIGNMENT != 0 )
		{
			Error( "CTSListBase: Misaligned list\n" );
			DebuggerBreak();
		}

#ifdef USE_NATIVE_SLIST
		InitializeSListHead( &m_Head );
#elif defined(PLATFORM_64BITS)
		m_Head.value64x128 = int128_zero();
#else
		m_Head.value64x128 = (int64)0;
#endif
	}

	~CTSListBase()
	{
		Detach();
	}

	TSLNodeBase_t *Push( TSLNodeBase_t *pNode )
	{
#ifdef _DEBUG
		if ( (size_t)pNode % TSLIST_NODE_ALIGNMENT != 0 )
		{
			Error( "CTSListBase: Misaligned node\n" );
			DebuggerBreak();
		}
#endif

#ifdef USE_NATIVE_SLIST
#ifdef _X360
		// integrated write-release barrier
		return (TSLNodeBase_t *)InterlockedPushEntrySListRelease( &m_Head, pNode );
#else
		return (TSLNodeBase_t *)InterlockedPushEntrySList( &m_Head, pNode );
#endif
#else
		TSLHead_t oldHead;
		TSLHead_t newHead;

		#if defined( PLATFORM_PS3 ) || defined( PLATFORM_X360 )
		__lwsync(); // write-release barrier
		#endif

#ifdef PLATFORM_64BITS
		newHead.value.Padding = 0;
#endif
		for (;;)
		{
			oldHead.value64x128 = m_Head.value64x128;
			pNode->Next = oldHead.value.Next;
			newHead.value.Next = pNode;
			
			newHead.value32.DepthAndSequence = oldHead.value32.DepthAndSequence + 0x10001;
			
			
			if ( ThreadInterlockedAssignIf64x128( &m_Head.value64x128, newHead.value64x128, oldHead.value64x128 ) )
			{
				break;
			}
			ThreadPause();
		};

		return (TSLNodeBase_t *)oldHead.value.Next;
#endif
	}

	TSLNodeBase_t *Pop()
	{
#ifdef USE_NATIVE_SLIST
#ifdef _X360
		// integrated read-acquire barrier
		TSLNodeBase_t *pNode = (TSLNodeBase_t *)InterlockedPopEntrySListAcquire( &m_Head );
#else
		TSLNodeBase_t *pNode = (TSLNodeBase_t *)InterlockedPopEntrySList( &m_Head );
#endif
		return pNode;
#else
		TSLHead_t oldHead;
		TSLHead_t newHead;

#ifdef PLATFORM_64BITS
		newHead.value.Padding = 0;
#endif
		for (;;)
		{
			oldHead.value64x128 = m_Head.value64x128;
			if ( !oldHead.value.Next )
				return NULL;

			newHead.value.Next = oldHead.value.Next->Next;
			newHead.value32.DepthAndSequence = oldHead.value32.DepthAndSequence	- 1;


			if ( ThreadInterlockedAssignIf64x128( &m_Head.value64x128, newHead.value64x128, oldHead.value64x128 ) )
			{
				#if defined( PLATFORM_PS3 ) || defined( PLATFORM_X360 )
					__lwsync(); // read-acquire barrier
				#endif
				break;
			}
			ThreadPause();
		};

		return (TSLNodeBase_t *)oldHead.value.Next;
#endif
	}

	TSLNodeBase_t *Detach()
	{
#ifdef USE_NATIVE_SLIST
		TSLNodeBase_t *pBase = (TSLNodeBase_t *)InterlockedFlushSList( &m_Head );
#if defined( _X360 ) || defined( _PS3 )
		__lwsync(); // read-acquire barrier
#endif
		return pBase;
#else
		TSLHead_t oldHead;
		TSLHead_t newHead;

#ifdef PLATFORM_64BITS
		newHead.value.Padding = 0;
#endif
		do
		{
			ThreadPause();

			oldHead.value64x128 = m_Head.value64x128;
			if ( !oldHead.value.Next )
				return NULL;

			newHead.value.Next = NULL;
			// <sergiy> the reason for AND'ing it instead of poking a short into memory 
			//          is probably to avoid store forward issues, but I'm not sure because
			//          I didn't construct this code. In any case, leaving it as is on big-endian
			newHead.value32.DepthAndSequence = oldHead.value32.DepthAndSequence & 0xffff0000;

		} while( !ThreadInterlockedAssignIf64x128( &m_Head.value64x128, newHead.value64x128, oldHead.value64x128 ) );

		return (TSLNodeBase_t *)oldHead.value.Next;
#endif
	}

	TSLHead_t *AccessUnprotected()
	{
		return &m_Head;
	}

	int Count() const
	{
#ifdef USE_NATIVE_SLIST
		return QueryDepthSList( const_cast<TSLHead_t*>( &m_Head ) );
#else
		return m_Head.value.Depth;
#endif
	}

private:
	TSLHead_t m_Head;
} TSLIST_HEAD_ALIGN_POST;

//-------------------------------------

template <typename T>
class TSLIST_HEAD_ALIGN CTSSimpleList : public CTSListBase
{
public:
	void Push( T *pNode )
	{
		Assert( sizeof(T) >= sizeof(TSLNodeBase_t) );
		CTSListBase::Push( (TSLNodeBase_t *)pNode );
	}

	T *Pop()
	{
		return (T *)CTSListBase::Pop();
	}
} TSLIST_HEAD_ALIGN_POST;

//-------------------------------------
// this is a replacement for CTSList<> and CObjectPool<> that does not
// have a per-item, per-alloc new/delete overhead
// similar to CTSSimpleList except that it allocates it's own pool objects
// and frees them on destruct.  Also it does not overlay the TSNodeBase_t memory
// on T's memory
template< class T > 
class TSLIST_HEAD_ALIGN CTSPool : public CTSListBase
{
	// packs the node and the item (T) into a single struct and pools those
	struct TSLIST_NODE_ALIGN simpleTSPoolStruct_t : public TSLNodeBase_t
	{
		T elem;
	} TSLIST_NODE_ALIGN_POST;

public:

	~CTSPool()
	{
		Purge();
	}

	void Purge()
	{
		simpleTSPoolStruct_t *pNode = NULL;
		while ( 1 )
		{
			pNode = (simpleTSPoolStruct_t *)CTSListBase::Pop();
			if ( !pNode )
				break;
			delete pNode;
		}
	}

	void PutObject( T *pInfo )
	{
		char *pElem = (char *)pInfo;
		pElem -= offsetof(simpleTSPoolStruct_t,elem);
		simpleTSPoolStruct_t *pNode = (simpleTSPoolStruct_t *)pElem;

		CTSListBase::Push( pNode );
	}

	T *GetObject()
	{
		simpleTSPoolStruct_t *pNode = (simpleTSPoolStruct_t *)CTSListBase::Pop();
		if ( !pNode )
		{
			pNode = new simpleTSPoolStruct_t;
		}
		return &pNode->elem;
	}

	// omg windows sdk - why do you #define GetObject()?
	FORCEINLINE T *Get()
	{
		return GetObject();
	}
} TSLIST_HEAD_ALIGN_POST;
//-------------------------------------

template <typename T>
class TSLIST_HEAD_ALIGN CTSList : public CTSListBase
{
public:
	struct TSLIST_NODE_ALIGN Node_t : public TSLNodeBase_t
	{
		Node_t() {}
		Node_t( const T &init ) : elem( init ) {}
		T elem;

	    // override new/delete so we can guarantee 8-byte aligned allocs
	    static void * operator new( size_t size )
	    {
      		Node_t *pNode = (Node_t *)MemAlloc_AllocAligned( size, TSLIST_NODE_ALIGNMENT, __FILE__, __LINE__ );
			return pNode;
	    }

		// override new/delete so we can guarantee 8-byte aligned allocs
		static void * operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )
		{
			Node_t *pNode = (Node_t *)MemAlloc_AllocAligned( size, TSLIST_NODE_ALIGNMENT, pFileName, nLine );
			return pNode;
		}

	    static void operator delete( void *p)
	    {
			MemAlloc_FreeAligned( p );
	    }
		static void operator delete( void *p, int nBlockUse, const char *pFileName, int nLine )
		{
			MemAlloc_FreeAligned( p );
		}

	} TSLIST_NODE_ALIGN_POST;

	~CTSList()
	{
		Purge();
	}

	void Purge()
	{
		Node_t *pCurrent = Detach();
		Node_t *pNext;
		while ( pCurrent )
		{
			pNext = (Node_t *)pCurrent->Next;
			delete pCurrent;
			pCurrent = pNext;
		}
	}

	void RemoveAll()
	{
		Purge();
	}

	Node_t *Push( Node_t *pNode )
	{
		return (Node_t *)CTSListBase::Push( pNode );
	}

	Node_t *Pop()
	{
		return (Node_t *)CTSListBase::Pop();
	}

	void PushItem( const T &init )
	{
		Push( new Node_t( init ) );
	}

	bool PopItem( T *pResult)
	{
		Node_t *pNode = Pop();
		if ( !pNode )
			return false;
		*pResult = pNode->elem;
		delete pNode;
		return true;
	}

	Node_t *Detach()
	{
		return (Node_t *)CTSListBase::Detach();
	}

} TSLIST_HEAD_ALIGN_POST;

//-------------------------------------

template <typename T>
class TSLIST_HEAD_ALIGN CTSListWithFreeList : public CTSListBase
{
public:
	struct TSLIST_NODE_ALIGN Node_t : public TSLNodeBase_t
	{
		Node_t() {}
		Node_t( const T &init ) : elem( init ) {}

		T elem;
	} TSLIST_NODE_ALIGN_POST;

	~CTSListWithFreeList()
	{
		Purge();
	}

	void Purge()
	{
		Node_t *pCurrent = Detach();
		Node_t *pNext;
		while ( pCurrent )
		{
			pNext = (Node_t *)pCurrent->Next;
			delete pCurrent;
			pCurrent = pNext;
		}
		pCurrent = (Node_t *)m_FreeList.Detach();
		while ( pCurrent )
		{
			pNext = (Node_t *)pCurrent->Next;
			delete pCurrent;
			pCurrent = pNext;
		}
	}

	void RemoveAll()
	{
		Node_t *pCurrent = Detach();
		Node_t *pNext;
		while ( pCurrent )
		{
			pNext = (Node_t *)pCurrent->Next;
			m_FreeList.Push( pCurrent );
			pCurrent = pNext;
		}
	}

	Node_t *Push( Node_t *pNode )
	{
		return (Node_t *)CTSListBase::Push( pNode );
	}

	Node_t *Pop()
	{
		return (Node_t *)CTSListBase::Pop();
	}

	void PushItem( const T &init )
	{
		Node_t *pNode = (Node_t *)m_FreeList.Pop();
		if ( !pNode )
		{
			pNode = new Node_t;
		}
		pNode->elem = init;
		Push( pNode );
	}

	bool PopItem( T *pResult)
	{
		Node_t *pNode = Pop();
		if ( !pNode )
			return false;
		*pResult = pNode->elem;
		m_FreeList.Push( pNode );
		return true;
	}

	Node_t *Detach()
	{
		return (Node_t *)CTSListBase::Detach();
	}

	void FreeNode( Node_t *pNode )
	{
		m_FreeList.Push( pNode );
	}

private:
	CTSListBase m_FreeList;
} TSLIST_HEAD_ALIGN_POST;

//-----------------------------------------------------------------------------
// Lock free queue
//
// A special consideration: the element type should be simple. This code
// actually dereferences freed nodes as part of pop, but later detects
// that. If the item in the queue is a complex type, only bad things can
// come of that. Also, therefore, if you're using Push/Pop instead of
// push item, be aware that the node memory cannot be freed until
// all threads that might have been popping have completed the pop.
// The PushItem()/PopItem() for handles this by keeping a persistent
// free list. Dont mix Push/PushItem. Note also nodes will be freed at the end, 
// and are expected to have been allocated with operator new.
//-----------------------------------------------------------------------------

template <typename T, bool bTestOptimizer = false>
class TSLIST_HEAD_ALIGN CTSQueue
{
public:

	// override new/delete so we can guarantee 8-byte aligned allocs
	static void * operator new( size_t size )
	{
		CTSQueue *pNode = (CTSQueue *)MemAlloc_AllocAligned( size, TSLIST_HEAD_ALIGNMENT, __FILE__, __LINE__ );
		return pNode;
	}

	// override new/delete so we can guarantee 8-byte aligned allocs
	static void * operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )
	{
		CTSQueue *pNode = (CTSQueue *)MemAlloc_AllocAligned( size, TSLIST_HEAD_ALIGNMENT, pFileName, nLine );
		return pNode;
	}

	static void operator delete( void *p)
	{
		MemAlloc_FreeAligned( p );
	}

	static void operator delete( void *p, int nBlockUse, const char *pFileName, int nLine )
	{
		MemAlloc_FreeAligned( p );
	}

private:
	// These ain't gonna work
	static void * operator new[] ( size_t size ) throw()
	{
		return NULL;
	}

	static void operator delete [] ( void *p)
	{
	}

public:

	struct TSLIST_NODE_ALIGN Node_t
	{
		// override new/delete so we can guarantee 8-byte aligned allocs
		static void * operator new( size_t size )
		{
			Node_t *pNode = (Node_t *)MemAlloc_AllocAligned( size, TSLIST_HEAD_ALIGNMENT, __FILE__, __LINE__ );
			return pNode;
		}

		static void * operator new( size_t size, int nBlockUse, const char *pFileName, int nLine )
		{
			Node_t *pNode = (Node_t *)MemAlloc_AllocAligned( size, TSLIST_HEAD_ALIGNMENT, pFileName, nLine );
			return pNode;
		}

		static void operator delete( void *p)
		{
			MemAlloc_FreeAligned( p );
		}

		static void operator delete( void *p, int nBlockUse, const char *pFileName, int nLine )
		{
			MemAlloc_FreeAligned( p );
		}

		Node_t() {}
		Node_t( const T &init ) : elem( init ) {}

		Node_t *pNext;
		T elem;
	} TSLIST_NODE_ALIGN_POST;

	union TSLIST_HEAD_ALIGN NodeLink_t
	{
		// override new/delete so we can guarantee 8-byte aligned allocs
		static void * operator new( size_t size )
		{
			NodeLink_t *pNode = (NodeLink_t *)MemAlloc_AllocAligned( size, TSLIST_HEAD_ALIGNMENT, __FILE__, __LINE__ );
			return pNode;
		}

		static void operator delete( void *p)
		{
			MemAlloc_FreeAligned( p );
		}

		struct Value_t
		{
			Node_t *pNode;
			intp	sequence;
		} value;

#ifdef PLATFORM_64BITS
		int128 value64x128;
#else
		int64 value64x128;
#endif
	} TSLIST_HEAD_ALIGN_POST;

	CTSQueue()
	{
		COMPILE_TIME_ASSERT( sizeof(Node_t) >= sizeof(TSLNodeBase_t) );
		if ( ((size_t)&m_Head) % TSLIST_HEAD_ALIGNMENT != 0 )
		{
			Error( "CTSQueue: Misaligned queue\n" );
			DebuggerBreak();
		}
		if ( ((size_t)&m_Tail) % TSLIST_HEAD_ALIGNMENT != 0 )
		{
			Error( "CTSQueue: Misaligned queue\n" );
			DebuggerBreak();
		}
		m_Count = 0;
		m_Head.value.sequence = m_Tail.value.sequence = 0;
		m_Head.value.pNode = m_Tail.value.pNode = new Node_t; // list always contains a dummy node
		m_Head.value.pNode->pNext = End();
	}

	~CTSQueue()
	{
		Purge();
		Assert( m_Count == 0 );
		Assert( m_Head.value.pNode == m_Tail.value.pNode );
		Assert( m_Head.value.pNode->pNext == End() );
		delete m_Head.value.pNode;
	}

	// Note: Purge, RemoveAll, and Validate are *not* threadsafe
	void Purge()
	{
		if ( IsDebug() )
		{
			ValidateQueue();
		}

		Node_t *pNode;
		while ( ( pNode = Pop() ) != NULL )
		{
			delete pNode;
		}

		while ( ( pNode = (Node_t *)m_FreeNodes.Pop() ) != NULL )
		{
			delete pNode;
		}

		Assert( m_Count == 0 );
		Assert( m_Head.value.pNode == m_Tail.value.pNode );
		Assert( m_Head.value.pNode->pNext == End() );

		m_Head.value.sequence = m_Tail.value.sequence = 0;
	}

	void RemoveAll()
	{
		if ( IsDebug() )
		{
			ValidateQueue();
		}

		Node_t *pNode;
		while ( ( pNode = Pop() ) != NULL )
		{
			m_FreeNodes.Push( (TSLNodeBase_t *)pNode );
		}
	}

	bool ValidateQueue()
	{
		if ( IsDebug() )
		{
			bool bResult = true;
			int nNodes = 0;
			if ( m_Tail.value.pNode->pNext != End() )
			{
				DebuggerBreakIfDebugging();
				bResult = false;
			}

			if ( m_Count == 0 )
			{
				if ( m_Head.value.pNode != m_Tail.value.pNode )
				{
					DebuggerBreakIfDebugging();
					bResult = false;
				}
			}

			Node_t *pNode = m_Head.value.pNode;
			while ( pNode != End() )
			{
				nNodes++;
				pNode = pNode->pNext;
			}

			nNodes--;// skip dummy node

			if ( nNodes != m_Count )
			{
				DebuggerBreakIfDebugging();
				bResult = false;
			}

			if ( !bResult )
			{
				Msg( "Corrupt CTSQueueDetected" );
			}

			return bResult;
		}
		else
		{
			return true;
		}
	}

	void FinishPush( Node_t *pNode, const NodeLink_t &oldTail )
	{
		NodeLink_t newTail;

		newTail.value.pNode = pNode;
		newTail.value.sequence = oldTail.value.sequence + 1;

		ThreadMemoryBarrier();

		InterlockedCompareExchangeNodeLink( &m_Tail, newTail, oldTail );
	}

	Node_t *Push( Node_t *pNode )
	{
#ifdef _DEBUG
		if ( (size_t)pNode % TSLIST_NODE_ALIGNMENT != 0 )
		{
			Error( "CTSListBase: Misaligned node\n" );
			DebuggerBreak();
		}
#endif

		NodeLink_t oldTail;

		pNode->pNext = End();

		for (;;)
		{
			oldTail.value.sequence = m_Tail.value.sequence;
			oldTail.value.pNode = m_Tail.value.pNode;
			if ( InterlockedCompareExchangeNode( &(oldTail.value.pNode->pNext), pNode, End() ) == End() )
			{
				break;
			}
			else
			{
				// Another thread is trying to push, help it along
				FinishPush( oldTail.value.pNode->pNext, oldTail );
			}
		}

		FinishPush( pNode, oldTail ); // This can fail if another thread pushed between the sequence and node grabs above. Later pushes or pops corrects

		m_Count++;

		return oldTail.value.pNode;
	}

	Node_t *Pop()
	{
		#define TSQUEUE_BAD_NODE_LINK ( (Node_t *)INT_TO_POINTER( 0xdeadbeef ) )
		NodeLink_t * volatile		pHead = &m_Head;
		NodeLink_t * volatile		pTail = &m_Tail;
		Node_t * volatile *			pHeadNode = &m_Head.value.pNode;
		volatile intp * volatile	pHeadSequence = &m_Head.value.sequence;
		Node_t * volatile * 		pTailNode = &pTail->value.pNode;

		NodeLink_t head;
		NodeLink_t newHead;
		Node_t *pNext;
		intp tailSequence;
		T elem;

		for (;;)
		{
			head.value.sequence = *pHeadSequence; // must grab sequence first, which allows condition below to ensure pNext is valid
			ThreadMemoryBarrier(); // need a barrier to prevent reordering of these assignments
			head.value.pNode	= *pHeadNode;
			tailSequence		= pTail->value.sequence;
         	pNext				= head.value.pNode->pNext;

			// Checking pNext only to force optimizer to not reorder the assignment
			// to pNext and the compare of the sequence
			if ( !pNext || head.value.sequence != *pHeadSequence ) 
				continue;

			if ( bTestOptimizer )
			{
				if ( pNext == TSQUEUE_BAD_NODE_LINK )
				{
					Msg( "Bad node link detected\n" );
					continue;
				}
			}

			if ( head.value.pNode == *pTailNode )
			{
				if ( pNext == End() )
					return NULL;

				// Another thread is trying to push, help it along
				NodeLink_t &oldTail = head; // just reuse local memory for head to build old tail
				oldTail.value.sequence = tailSequence; // reuse head pNode
				FinishPush( pNext, oldTail );
				continue;
			}
			
			if ( pNext != End() )
			{
				elem = pNext->elem; // NOTE: next could be a freed node here, by design
				newHead.value.pNode = pNext;
				newHead.value.sequence = head.value.sequence + 1;
				if ( InterlockedCompareExchangeNodeLink( pHead, newHead, head ) )
				{
					ThreadMemoryBarrier();
					if ( bTestOptimizer )
					{
						head.value.pNode->pNext = TSQUEUE_BAD_NODE_LINK;
					}
					break;
				}
			}
		}

		m_Count--;
		head.value.pNode->elem = elem;
		return head.value.pNode;
	}

	void FreeNode( Node_t *pNode )
	{
		m_FreeNodes.Push( (TSLNodeBase_t *)pNode );
	}

	void PushItem( const T &init )
	{
		Node_t *pNode = (Node_t *)m_FreeNodes.Pop();
		if ( pNode )
		{
			pNode->elem = init;
		}
		else
		{
			pNode = new Node_t( init );
		}
		Push( pNode );
	}

	bool PopItem( T *pResult )
	{
		Node_t *pNode = Pop();
		if ( !pNode )
			return false;

		*pResult = pNode->elem;
		m_FreeNodes.Push( (TSLNodeBase_t *)pNode );
		return true;
	}

	int Count() const
	{
		return m_Count;
	}

private:
	Node_t *End() { return (Node_t *)this; } // just need a unique signifier

	Node_t *InterlockedCompareExchangeNode( Node_t * volatile *ppNode, Node_t *value, Node_t *comperand )
	{
		return (Node_t *)::ThreadInterlockedCompareExchangePointer( (void **)ppNode, value, comperand );
	}

	bool InterlockedCompareExchangeNodeLink( NodeLink_t volatile *pLink, const NodeLink_t &value, const NodeLink_t &comperand )
	{
		return ThreadInterlockedAssignIf64x128( &pLink->value64x128, value.value64x128, comperand.value64x128 );
	}

	NodeLink_t m_Head;
	NodeLink_t m_Tail;

	CInterlockedInt m_Count;
	
	CTSListBase m_FreeNodes;
} TSLIST_NODE_ALIGN_POST;

#if defined( _WIN32 )
// Suppress this spurious warning:
// warning C4700: uninitialized local variable 'oldHead' used
#pragma warning( pop )
#endif

#endif // TSLIST_H

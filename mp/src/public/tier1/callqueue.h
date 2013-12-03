//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================

#ifndef CALLQUEUE_H
#define CALLQUEUE_H

#include "tier0/tslist.h"
#include "functors.h"

#if defined( _WIN32 )
#pragma once
#endif

//-----------------------------------------------------
// Avert thy eyes! Imagine rather:
//
// void QueueCall( <function>, [args1, [arg2,]...]
// void QueueCall( <object>, <function>, [args1, [arg2,]...]
// void QueueRefCall( <object>, <<function>, [args1, [arg2,]...]
//-----------------------------------------------------

#define DEFINE_CALLQUEUE_NONMEMBER_QUEUE_CALL(N) \
	template <typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
	void QueueCall(FUNCTION_RETTYPE (*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
		QueueFunctorInternal( CreateFunctor( pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ) ); \
		}

//-------------------------------------

#define DEFINE_CALLQUEUE_MEMBER_QUEUE_CALL(N) \
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
	void QueueCall(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
		QueueFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ) ); \
		}

//-------------------------------------

#define DEFINE_CALLQUEUE_CONST_MEMBER_QUEUE_CALL(N) \
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
	void QueueCall(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
		QueueFunctorInternal( CreateFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ) ); \
		}

//-------------------------------------

#define DEFINE_CALLQUEUE_REF_COUNTING_MEMBER_QUEUE_CALL(N) \
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
	void QueueRefCall(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
		QueueFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ) ); \
		}

//-------------------------------------

#define DEFINE_CALLQUEUE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL(N) \
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N FUNC_TEMPLATE_ARG_PARAMS_##N> \
	void QueueRefCall(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const FUNC_ARG_FORMAL_PARAMS_##N ) \
		{ \
		QueueFunctorInternal( CreateRefCountingFunctor( pObject, pfnProxied FUNC_FUNCTOR_CALL_ARGS_##N ) ); \
		\
		}

#define FUNC_GENERATE_QUEUE_METHODS() \
	FUNC_GENERATE_ALL( DEFINE_CALLQUEUE_NONMEMBER_QUEUE_CALL ); \
	FUNC_GENERATE_ALL( DEFINE_CALLQUEUE_MEMBER_QUEUE_CALL ); \
	FUNC_GENERATE_ALL( DEFINE_CALLQUEUE_CONST_MEMBER_QUEUE_CALL );\
	FUNC_GENERATE_ALL( DEFINE_CALLQUEUE_REF_COUNTING_MEMBER_QUEUE_CALL ); \
	FUNC_GENERATE_ALL( DEFINE_CALLQUEUE_REF_COUNTING_CONST_MEMBER_QUEUE_CALL )

//-----------------------------------------------------

template <typename QUEUE_TYPE = CTSQueue<CFunctor *> >
class CCallQueueT
{
public:
	CCallQueueT()
		: m_bNoQueue( false )
	{
#ifdef _DEBUG
		m_nCurSerialNumber = 0;
		m_nBreakSerialNumber = (unsigned)-1;
#endif
	}

	void DisableQueue( bool bDisable )
	{
		if ( m_bNoQueue == bDisable )
		{
			return;
		}
		if ( !m_bNoQueue )
			CallQueued();

		m_bNoQueue = bDisable;
	}

	bool IsDisabled() const
	{
		return m_bNoQueue;
	}

	int Count()
	{
		return m_queue.Count();
	}

	void CallQueued()
	{
		if ( !m_queue.Count() )
		{
			return;
		}

		m_queue.PushItem( NULL );

		CFunctor *pFunctor;

		while ( m_queue.PopItem( &pFunctor ) && pFunctor != NULL )
		{
#ifdef _DEBUG
			if ( pFunctor->m_nUserID == m_nBreakSerialNumber)
			{
				m_nBreakSerialNumber = (unsigned)-1;
			}
#endif
			(*pFunctor)();
			pFunctor->Release();
		}

	}

	void QueueFunctor( CFunctor *pFunctor )
	{
		Assert( pFunctor );
		QueueFunctorInternal( RetAddRef( pFunctor ) );
	}

	void Flush()
	{
		m_queue.PushItem( NULL );

		CFunctor *pFunctor;

		while ( m_queue.PopItem( &pFunctor ) && pFunctor != NULL )
		{
			pFunctor->Release();
		}
	}

	FUNC_GENERATE_QUEUE_METHODS();

private:
	void QueueFunctorInternal( CFunctor *pFunctor )
	{
		if ( !m_bNoQueue )
		{
#ifdef _DEBUG
			pFunctor->m_nUserID = m_nCurSerialNumber++;
#endif
			m_queue.PushItem( pFunctor );
		}
		else
		{
			(*pFunctor)();
			pFunctor->Release();
		}
	}

	QUEUE_TYPE m_queue;
	bool m_bNoQueue;
	unsigned m_nCurSerialNumber;
	unsigned m_nBreakSerialNumber;
};

class CCallQueue : public CCallQueueT<>
{
};

//-----------------------------------------------------
// Optional interface that can be bound to concrete CCallQueue
//-----------------------------------------------------

class ICallQueue
{
public:
	void QueueFunctor( CFunctor *pFunctor )
	{
		QueueFunctorInternal( RetAddRef( pFunctor ) );
	}

	FUNC_GENERATE_QUEUE_METHODS();

private:
	virtual void QueueFunctorInternal( CFunctor *pFunctor ) = 0;
};

#endif // CALLQUEUE_H

//========== Copyright (c) 2008, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef VSCRIPT_TEMPLATES_H
#define VSCRIPT_TEMPLATES_H

#include "tier0/basetypes.h"

#if defined( _WIN32 )
#pragma once
#endif

#define	FUNC_APPEND_PARAMS_0	
#define	FUNC_APPEND_PARAMS_1	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 1 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) );
#define	FUNC_APPEND_PARAMS_2	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 2 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) );
#define	FUNC_APPEND_PARAMS_3	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 3 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );
#define	FUNC_APPEND_PARAMS_4	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 4 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) );
#define	FUNC_APPEND_PARAMS_5	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 5 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) );
#define	FUNC_APPEND_PARAMS_6	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 6 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) );
#define	FUNC_APPEND_PARAMS_7	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 7 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) );
#define	FUNC_APPEND_PARAMS_8	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 8 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_8 ) );
#define	FUNC_APPEND_PARAMS_9	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 9 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_8 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_9 ) );
#define	FUNC_APPEND_PARAMS_10	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 10 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_8 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_9 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_10 ) );
#define	FUNC_APPEND_PARAMS_11	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 11 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_8 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_9 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_10 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_11 ) );
#define	FUNC_APPEND_PARAMS_12	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 12 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_8 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_9 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_10 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_11 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_12 ) );
#define	FUNC_APPEND_PARAMS_13	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 13 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_8 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_9 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_10 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_11 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_12 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_13 ) );
#define	FUNC_APPEND_PARAMS_14	pDesc->m_Parameters.SetGrowSize( 1 ); pDesc->m_Parameters.EnsureCapacity( 14 ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_1 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_2 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_3 ) );	pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_4 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_5 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_6 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_7 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_8 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_9 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_10 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_11 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_12 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_13 ) ); pDesc->m_Parameters.AddToTail( ScriptDeduceType( FUNC_ARG_TYPE_14 ) );

#define DEFINE_NONMEMBER_FUNC_TYPE_DEDUCER(N) \
	template <typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	inline void ScriptDeduceFunctionSignature(ScriptFuncDescriptor_t *pDesc, FUNCTION_RETTYPE (*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) ) \
	{ \
		pDesc->m_ReturnType = ScriptDeduceType(FUNCTION_RETTYPE); \
		FUNC_APPEND_PARAMS_##N \
	}

FUNC_GENERATE_ALL( DEFINE_NONMEMBER_FUNC_TYPE_DEDUCER );

#define DEFINE_MEMBER_FUNC_TYPE_DEDUCER(N) \
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	inline void ScriptDeduceFunctionSignature(ScriptFuncDescriptor_t *pDesc, OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) ) \
	{ \
		pDesc->m_ReturnType = ScriptDeduceType(FUNCTION_RETTYPE); \
		FUNC_APPEND_PARAMS_##N \
	}

FUNC_GENERATE_ALL( DEFINE_MEMBER_FUNC_TYPE_DEDUCER );

//-------------------------------------

#define DEFINE_CONST_MEMBER_FUNC_TYPE_DEDUCER(N) \
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	inline void ScriptDeduceFunctionSignature(ScriptFuncDescriptor_t *pDesc, OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE ( FUNCTION_CLASS::*pfnProxied )( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const ) \
	{ \
		pDesc->m_ReturnType = ScriptDeduceType(FUNCTION_RETTYPE); \
		FUNC_APPEND_PARAMS_##N \
	}

FUNC_GENERATE_ALL( DEFINE_CONST_MEMBER_FUNC_TYPE_DEDUCER );

#define ScriptInitMemberFuncDescriptor_( pDesc, class, func, scriptName )	if ( 0 ) {} else { (pDesc)->m_pszScriptName = scriptName; (pDesc)->m_pszFunction = #func; ScriptDeduceFunctionSignature( pDesc, (class *)(0), &class::func ); }

#define ScriptInitFuncDescriptorNamed( pDesc, func, scriptName )						if ( 0 ) {} else { (pDesc)->m_pszScriptName = scriptName; (pDesc)->m_pszFunction = #func; ScriptDeduceFunctionSignature( pDesc, &func ); }
#define ScriptInitFuncDescriptor( pDesc, func )											ScriptInitFuncDescriptorNamed( pDesc, func, #func )
#define ScriptInitMemberFuncDescriptorNamed( pDesc, class, func, scriptName )			ScriptInitMemberFuncDescriptor_( pDesc, class, func, scriptName )
#define ScriptInitMemberFuncDescriptor( pDesc, class, func )							ScriptInitMemberFuncDescriptorNamed( pDesc, class, func, #func )

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

template <typename FUNCPTR_TYPE>
inline ScriptFunctionBindingStorageType_t ScriptConvertFreeFuncPtrToVoid( FUNCPTR_TYPE pFunc )
{
	ScriptFunctionBindingStorageType_t type = { };
	COMPILE_TIME_ASSERT( sizeof( type ) >= sizeof( pFunc ) );

	memcpy( &type, &pFunc, sizeof( pFunc ) );
	return type;
}

template <typename FUNCPTR_TYPE>
inline FUNCPTR_TYPE ScriptConvertFreeFuncPtrFromVoid( ScriptFunctionBindingStorageType_t p )
{
	FUNCPTR_TYPE func = { };
	COMPILE_TIME_ASSERT( sizeof( func ) <= sizeof( p ) );

	memcpy( &func, &p, sizeof( func ) );
	return func;
}

template <typename FUNCPTR_TYPE>
inline ScriptFunctionBindingStorageType_t ScriptConvertFuncPtrToVoid( FUNCPTR_TYPE pFunc )
{
	return ScriptConvertFreeFuncPtrToVoid<FUNCPTR_TYPE>( pFunc );
}

template <typename FUNCPTR_TYPE>
inline FUNCPTR_TYPE ScriptConvertFuncPtrFromVoid( ScriptFunctionBindingStorageType_t p )
{
	return ScriptConvertFreeFuncPtrFromVoid<FUNCPTR_TYPE>( p );
}

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_0
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_1 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_1
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_2 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_2
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_3 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_3
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_4 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_4
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_5 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_5
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_6 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_6
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_7 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_7
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_8 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_8
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_9 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_9
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_10 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_10
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_11 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_11
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_12 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_12
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_13 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_13
#define FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_14 , FUNC_BASE_TEMPLATE_FUNC_PARAMS_14

#define	SCRIPT_BINDING_ARGS_0
#define	SCRIPT_BINDING_ARGS_1 pArguments[0]
#define	SCRIPT_BINDING_ARGS_2 pArguments[0], pArguments[1]
#define	SCRIPT_BINDING_ARGS_3 pArguments[0], pArguments[1], pArguments[2]
#define	SCRIPT_BINDING_ARGS_4 pArguments[0], pArguments[1], pArguments[2], pArguments[3]
#define	SCRIPT_BINDING_ARGS_5 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4]
#define	SCRIPT_BINDING_ARGS_6 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5]
#define	SCRIPT_BINDING_ARGS_7 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6]
#define	SCRIPT_BINDING_ARGS_8 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6], pArguments[7]
#define	SCRIPT_BINDING_ARGS_9 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6], pArguments[7], pArguments[8]
#define	SCRIPT_BINDING_ARGS_10 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6], pArguments[7], pArguments[8], pArguments[9]
#define	SCRIPT_BINDING_ARGS_11 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6], pArguments[7], pArguments[8], pArguments[9], pArguments[10]
#define	SCRIPT_BINDING_ARGS_12 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6], pArguments[7], pArguments[8], pArguments[9], pArguments[10], pArguments[11]
#define	SCRIPT_BINDING_ARGS_13 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6], pArguments[7], pArguments[8], pArguments[9], pArguments[10], pArguments[11], pArguments[12]
#define	SCRIPT_BINDING_ARGS_14 pArguments[0], pArguments[1], pArguments[2], pArguments[3], pArguments[4], pArguments[5], pArguments[6], pArguments[7], pArguments[8], pArguments[9], pArguments[10], pArguments[11], pArguments[12], pArguments[13]


#define DEFINE_SCRIPT_BINDINGS(N) \
	template <typename FUNC_TYPE, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	class CNonMemberScriptBinding##N \
	{ \
	public: \
 		static bool Call( ScriptFunctionBindingStorageType_t pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
 		{ \
			Assert( nArguments == N ); \
			Assert( pReturn ); \
			Assert( !pContext ); \
			\
			if ( nArguments != N || !pReturn || pContext ) \
			{ \
				return false; \
			} \
			*pReturn = (ScriptConvertFreeFuncPtrFromVoid<FUNC_TYPE>(pFunction))( SCRIPT_BINDING_ARGS_##N ); \
			if ( pReturn->GetType() == FIELD_VECTOR2D || pReturn->GetType() == FIELD_VECTOR || /*pReturn->GetType() == FIELD_VECTOR4D ||*/ pReturn->GetType() == FIELD_QANGLE || pReturn->GetType() == FIELD_QUATERNION ) \
				pReturn->ConvertToCopiedData(); \
 			return true; \
 		} \
	}; \
	\
	template <typename FUNC_TYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	class CNonMemberScriptBinding##N<FUNC_TYPE, void FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_##N> \
	{ \
	public: \
		static bool Call( ScriptFunctionBindingStorageType_t pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
		{ \
			Assert( nArguments == N ); \
			Assert( !pReturn ); \
			Assert( !pContext ); \
			\
			if ( nArguments != N || pReturn || pContext ) \
			{ \
				return false; \
			} \
			(ScriptConvertFreeFuncPtrFromVoid<FUNC_TYPE>(pFunction))( SCRIPT_BINDING_ARGS_##N ); \
			return true; \
		} \
	}; \
	\
	template <class OBJECT_TYPE_PTR, typename FUNC_TYPE, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	class CMemberScriptBinding##N \
	{ \
	public: \
 		static bool Call( ScriptFunctionBindingStorageType_t pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
 		{ \
			Assert( nArguments == N ); \
			Assert( pReturn ); \
			Assert( pContext ); \
			\
			if ( nArguments != N || !pReturn || !pContext ) \
			{ \
				return false; \
			} \
			*pReturn = (((OBJECT_TYPE_PTR)(pContext))->*ScriptConvertFuncPtrFromVoid<FUNC_TYPE>(pFunction))( SCRIPT_BINDING_ARGS_##N ); \
			if ( pReturn->GetType() == FIELD_VECTOR2D || pReturn->GetType() == FIELD_VECTOR || /*pReturn->GetType() == FIELD_VECTOR4D ||*/ pReturn->GetType() == FIELD_QANGLE || pReturn->GetType() == FIELD_QUATERNION ) \
				pReturn->ConvertToCopiedData(); \
 			return true; \
 		} \
	}; \
	\
	template <class OBJECT_TYPE_PTR, typename FUNC_TYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	class CMemberScriptBinding##N<OBJECT_TYPE_PTR, FUNC_TYPE, void FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_##N> \
	{ \
	public: \
		static bool Call( ScriptFunctionBindingStorageType_t pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
		{ \
			Assert( nArguments == N ); \
			Assert( !pReturn ); \
			Assert( pContext ); \
			\
			if ( nArguments != N || pReturn || !pContext ) \
			{ \
				return false; \
			} \
			(((OBJECT_TYPE_PTR)(pContext))->*ScriptConvertFuncPtrFromVoid<FUNC_TYPE>(pFunction))( SCRIPT_BINDING_ARGS_##N ); \
			return true; \
		} \
	}; \
	\
	template <typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	inline ScriptBindingFunc_t ScriptCreateBinding(FUNCTION_RETTYPE (*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) ) \
	{ \
		typedef FUNCTION_RETTYPE (*Func_t)(FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N); \
		return &CNonMemberScriptBinding##N<Func_t, FUNCTION_RETTYPE FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_##N>::Call; \
	} \
	\
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
		inline ScriptBindingFunc_t ScriptCreateBinding(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE (FUNCTION_CLASS::*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) ) \
	{ \
		typedef FUNCTION_RETTYPE (FUNCTION_CLASS::*Func_t)(FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N); \
		return &CMemberScriptBinding##N<OBJECT_TYPE_PTR, Func_t, FUNCTION_RETTYPE FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_##N>::Call; \
	} \
	\
	template <typename OBJECT_TYPE_PTR, typename FUNCTION_CLASS, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
		inline ScriptBindingFunc_t ScriptCreateBinding(OBJECT_TYPE_PTR pObject, FUNCTION_RETTYPE (FUNCTION_CLASS::*pfnProxied)( FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N ) const ) \
	{ \
		typedef FUNCTION_RETTYPE (FUNCTION_CLASS::*Func_t)(FUNC_BASE_TEMPLATE_FUNC_PARAMS_##N); \
		return &CMemberScriptBinding##N<OBJECT_TYPE_PTR, Func_t, FUNCTION_RETTYPE FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_##N>::Call; \
	}

FUNC_GENERATE_ALL( DEFINE_SCRIPT_BINDINGS );

//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------

#endif // VSCRIPT_TEMPLATES_H

//========== Copyright © 2008, Valve Corporation, All rights reserved. ========
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
inline void *ScriptConvertFuncPtrToVoid( FUNCPTR_TYPE pFunc )
{
	if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) ) )
	{
		union FuncPtrConvert
		{
			void *p;
			FUNCPTR_TYPE pFunc;
		};

		FuncPtrConvert convert;
		convert.pFunc = pFunc;
		return convert.p;
	}
#if defined( _MSC_VER )
	else if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) + sizeof( int ) ) )
	{
		struct MicrosoftUnknownMFP
		{
			void *p;
			int m_delta;
		};
	
		union FuncPtrConvertMI
		{
			MicrosoftUnknownMFP mfp;
			FUNCPTR_TYPE pFunc;
		};

		FuncPtrConvertMI convert;
		convert.pFunc = pFunc;
		if ( convert.mfp.m_delta == 0 )
		{
			return convert.mfp.p;
		}
		AssertMsg( 0, "Function pointer must be from primary vtable" );
	}
	else if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) + ( sizeof( int ) * 3 ) ) )
	{
		struct MicrosoftUnknownMFP
		{
			void *p;
			int m_delta;
			int m_vtordisp;
			int m_vtable_index;
		};

		union FuncPtrConvertMI
		{
			MicrosoftUnknownMFP mfp;
			FUNCPTR_TYPE pFunc;
		};

		FuncPtrConvertMI convert;
		convert.pFunc = pFunc;
		if ( convert.mfp.m_delta == 0 )
		{
			return convert.mfp.p;
		}
		AssertMsg( 0, "Function pointer must be from primary vtable" );
	}
#elif defined( GNUC )
	else if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) + sizeof( int ) ) )
	{
		struct GnuMFP
		{
			union
			{
				void *funcadr;		// If vtable_index_2 is even, then this is the function pointer.
				int vtable_index_2;		// If vtable_index_2 is odd, then (vtable_index_2 - 1) * 2 is the index into the vtable.
			};
			int delta;	// Offset from this-ptr to vtable
		};

		GnuMFP *p = (GnuMFP*)&pFunc;
		if ( p->delta == 0 )
		{
			// No need to check whether this is a direct function pointer or not,
			// this gets converted back to a "proper" member-function pointer in
			// ScriptConvertFuncPtrFromVoid() to get invoked
			return p->funcadr;
		}
		AssertMsg( 0, "Function pointer must be from primary vtable" );
	}
#else
#error "Need to implement code to crack non-offset member function pointer case"
	// For gcc, see: http://www.codeproject.com/KB/cpp/FastDelegate.aspx
	//
	// Current versions of the GNU compiler use a strange and tricky 
	// optimization. It observes that, for virtual inheritance, you have to look 
	// up the vtable in order to get the voffset required to calculate the this 
	// pointer. While you're doing that, you might as well store the function 
	// pointer in the vtable. By doing this, they combine the m_func_address and 
	// m_vtable_index fields into one, and they distinguish between them by 
	// ensuring that function pointers always point to even addresses but vtable 
	// indices are always odd:
	// 
	// 	// GNU g++ uses a tricky space optimisation, also adopted by IBM's VisualAge and XLC.
	// 	struct GnuMFP {
	// 	   union {
	// 	     CODEPTR funcadr; // always even
	// 	     int vtable_index_2; //  = vindex*2+1, always odd
	// 	   };
	// 	   int delta;
	// 	};
	// 	adjustedthis = this + delta
	// 	if (funcadr & 1) CALL (* ( *delta + (vindex+1)/2) + 4)
	// 	else CALL funcadr
	// 
	// The G++ method is well documented, so it has been adopted by many other 
	// vendors, including IBM's VisualAge and XLC compilers, recent versions of 
	// Open64, Pathscale EKO, and Metrowerks' 64-bit compilers. A simpler scheme 
	// used by earlier versions of GCC is also very common. SGI's now 
	// discontinued MIPSPro and Pro64 compilers, and Apple's ancient MrCpp 
	// compiler used this method. (Note that the Pro64 compiler has become the 
	// open source Open64 compiler).

#endif
	else
		AssertMsg( 0, "Member function pointer not supported. Why on earth are you using virtual inheritance!?" );
	return NULL;
}

template <typename FUNCPTR_TYPE>
inline FUNCPTR_TYPE ScriptConvertFuncPtrFromVoid( void *p )
{
	if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) ) )
	{
		union FuncPtrConvert
		{
			void *p;
			FUNCPTR_TYPE pFunc;
		};

		FuncPtrConvert convert;
		convert.p = p;
		return convert.pFunc;
	}

#if defined( _MSC_VER )
	if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) + sizeof( int ) ) )
	{
		struct MicrosoftUnknownMFP
		{
			void *p;
			int m_delta;
		};

		union FuncPtrConvertMI
		{
			MicrosoftUnknownMFP mfp;
			FUNCPTR_TYPE pFunc;
		};

		FuncPtrConvertMI convert;
		convert.mfp.p = p;
		convert.mfp.m_delta = 0;
		return convert.pFunc;
	}
	if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) + ( sizeof( int ) * 3 ) ) )
	{
		struct MicrosoftUnknownMFP
		{
			void *p;
			int m_delta;
			int m_vtordisp;
			int m_vtable_index;
		};

		union FuncPtrConvertMI
		{
			MicrosoftUnknownMFP mfp;
			FUNCPTR_TYPE pFunc;
		};

		FuncPtrConvertMI convert;
		convert.mfp.p = p;
		convert.mfp.m_delta = 0;
		return convert.pFunc;
	}
#elif defined( GNUC )
	if ( ( sizeof( FUNCPTR_TYPE ) == sizeof( void * ) + sizeof( int ) ) )
	{
		struct GnuMFP
		{
			union
			{
				void *funcadr;		// If vtable_index_2 is even, then this is the function pointer.
				int vtable_index_2;		// If vtable_index_2 is odd, then (vtable_index_2 - 1) * 2 is the index into the vtable.
			};
			int delta;	// Offset from this-ptr to vtable
		};

		union FuncPtrConvertGnu
		{
			GnuMFP mfp;
			FUNCPTR_TYPE pFunc;
		};

		FuncPtrConvertGnu convert;
		convert.mfp.funcadr = p;
		convert.mfp.delta = 0;
		return convert.pFunc;
	}
#else
#error "Need to implement code to crack non-offset member function pointer case"
#endif
	Assert( 0 );
	return NULL;
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
 		static bool Call( void *pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
 		{ \
			Assert( nArguments == N ); \
			Assert( pReturn ); \
			Assert( !pContext ); \
			\
			if ( nArguments != N || !pReturn || pContext ) \
			{ \
				return false; \
			} \
			*pReturn = ((FUNC_TYPE)pFunction)( SCRIPT_BINDING_ARGS_##N ); \
			if ( pReturn->m_type == FIELD_VECTOR ) \
				pReturn->m_pVector = new Vector(*pReturn->m_pVector); \
 			return true; \
 		} \
	}; \
	\
	template <typename FUNC_TYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	class CNonMemberScriptBinding##N<FUNC_TYPE, void FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_##N> \
	{ \
	public: \
		static bool Call( void *pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
		{ \
			Assert( nArguments == N ); \
			Assert( !pReturn ); \
			Assert( !pContext ); \
			\
			if ( nArguments != N || pReturn || pContext ) \
			{ \
				return false; \
			} \
			((FUNC_TYPE)pFunction)( SCRIPT_BINDING_ARGS_##N ); \
			return true; \
		} \
	}; \
	\
	template <class OBJECT_TYPE_PTR, typename FUNC_TYPE, typename FUNCTION_RETTYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	class CMemberScriptBinding##N \
	{ \
	public: \
 		static bool Call( void *pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
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
			if ( pReturn->m_type == FIELD_VECTOR ) \
				pReturn->m_pVector = new Vector(*pReturn->m_pVector); \
 			return true; \
 		} \
	}; \
	\
	template <class OBJECT_TYPE_PTR, typename FUNC_TYPE FUNC_TEMPLATE_FUNC_PARAMS_##N> \
	class CMemberScriptBinding##N<OBJECT_TYPE_PTR, FUNC_TYPE, void FUNC_BASE_TEMPLATE_FUNC_PARAMS_PASSTHRU_##N> \
	{ \
	public: \
		static bool Call( void *pFunction, void *pContext, ScriptVariant_t *pArguments, int nArguments, ScriptVariant_t *pReturn ) \
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
